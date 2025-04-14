// FILE: node1.c (Modified)
#include <stdio.h>
#include <string.h> // For memcpy
#include <stdlib.h> // For exit, potentially
#include "dvr.h"    // Includes struct definitions and global externs

// --- Node 1 Specific Data ---
#define NODE_ID 1

// Distance Table for Node 1: routing_table1[dest_node][via_neighbor]
static struct DistanceTable routing_table1;

// Direct link costs from Node 1 to other nodes
static int direct_link_costs1[NUM_NODES] = {1, 0, 1, INFINITY}; // Node 1 connected to 0 and 2

// Node 1's current best estimate of minimum costs to other nodes (Distance Vector)
static int my_min_costs1[NUM_NODES];

// --- Helper Functions ---

// Prepare a routing packet
static void prepare_routing_packet(struct RoutePacket *packet, int src, int dest, int costs[]) {
    packet->sourceid = src;
    packet->destid = dest;
    memcpy(packet->mincost, costs, NUM_NODES * sizeof(int));
}

// Send Node 1's current distance vector to all direct neighbors
static void send_dv_to_neighbors() {
    struct RoutePacket outgoing_pkt;
    int neighbor_idx;

    for (neighbor_idx = 0; neighbor_idx < NUM_NODES; neighbor_idx++) {
        // Send only to directly connected neighbors (cost < INFINITY) and not to self
        if (neighbor_idx != NODE_ID && direct_link_costs1[neighbor_idx] < INFINITY) {
            if (TRACE >= 1) {
                 printf("   Node %d @ %.3f: Preparing to send DV to neighbor %d\n",
                        NODE_ID, clocktime, neighbor_idx);
            }
            prepare_routing_packet(&outgoing_pkt, NODE_ID, neighbor_idx, my_min_costs1);
            tolayer2(outgoing_pkt); // Hand off packet to simulator
        }
    }
}


// --- Main Node 1 Functions ---

// Initialization routine for Node 1
void rtinit1() {
    int dest_idx, neighbor_idx;

    printf("Node %d @ %.3f: Initializing routing table.\n", NODE_ID, clocktime);

    // Initialize routing table: Set all costs to INFINITY initially
    for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
        for (neighbor_idx = 0; neighbor_idx < NUM_NODES; neighbor_idx++) {
            routing_table1.costs[dest_idx][neighbor_idx] = INFINITY;
        }
    }

    // Set costs for directly connected links and self-cost
    for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
         routing_table1.costs[dest_idx][dest_idx] = direct_link_costs1[dest_idx];
         my_min_costs1[dest_idx] = direct_link_costs1[dest_idx];
    }
     my_min_costs1[NODE_ID] = 0; // Cost to self is always 0

    printf("Node %d @ %.3f: Initialization complete. Initial DV = { %d %d %d %d }\n",
           NODE_ID, clocktime, my_min_costs1[0], my_min_costs1[1], my_min_costs1[2], my_min_costs1[3]);

    // Print the initial distance table
    printdt1(&routing_table1);

    // Send initial distance vector to all neighbors (0 and 2)
    send_dv_to_neighbors();
}


// Update routine for Node 1, called when a packet arrives
void rtupdate1(struct RoutePacket *incoming_pkt) {
    int sender_id = incoming_pkt->sourceid;
    int dest_idx, neighbor_idx;
    int calculated_cost;
    int dist_table_changed = 0;
    int min_costs_changed = 0;

    printf("Node %d @ %.3f: Received routing update from Node %d. DV = { %d %d %d %d }\n",
           NODE_ID, clocktime, sender_id,
           incoming_pkt->mincost[0], incoming_pkt->mincost[1],
           incoming_pkt->mincost[2], incoming_pkt->mincost[3]);

    // --- Step 1: Update the distance table using the received DV ---
     if (direct_link_costs1[sender_id] >= INFINITY) {
         printf("   Node %d @ %.3f: WARNING - Received packet from non-neighbor %d? Ignoring.\n", NODE_ID, clocktime, sender_id);
         return;
    }

    for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
        calculated_cost = direct_link_costs1[sender_id] + incoming_pkt->mincost[dest_idx];
        if (direct_link_costs1[sender_id] >= INFINITY || incoming_pkt->mincost[dest_idx] >= INFINITY) {
             calculated_cost = INFINITY;
        }
         if (calculated_cost > INFINITY) calculated_cost = INFINITY;

        if (calculated_cost < routing_table1.costs[dest_idx][sender_id]) {
             if (TRACE >= 1) {
                printf("   Node %d @ %.3f: Updating cost to %d via %d. Old: %d, New: %d\n",
                       NODE_ID, clocktime, dest_idx, sender_id,
                       routing_table1.costs[dest_idx][sender_id], calculated_cost);
            }
            routing_table1.costs[dest_idx][sender_id] = calculated_cost;
            dist_table_changed = 1;
        }
    }

    // --- Step 2: If the table changed, recalculate the minimum cost vector ---
    if (dist_table_changed) {
        printf("   Node %d @ %.3f: Distance table updated. Recalculating minimum costs...\n", NODE_ID, clocktime);
        int previous_min_costs[NUM_NODES];
        memcpy(previous_min_costs, my_min_costs1, NUM_NODES * sizeof(int));

        for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
            int current_min_cost = INFINITY;
            for (neighbor_idx = 0; neighbor_idx < NUM_NODES; neighbor_idx++) {
                 if (routing_table1.costs[dest_idx][neighbor_idx] < current_min_cost) {
                     current_min_cost = routing_table1.costs[dest_idx][neighbor_idx];
                 }
            }
            my_min_costs1[dest_idx] = current_min_cost;
        }
         my_min_costs1[NODE_ID] = 0; // Ensure cost to self is 0

        for(dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
            if (my_min_costs1[dest_idx] != previous_min_costs[dest_idx]) {
                min_costs_changed = 1;
                break;
            }
        }

        // --- Step 3: If the minimum costs changed, notify neighbors ---
        if (min_costs_changed) {
            printf("   Node %d @ %.3f: Minimum cost vector CHANGED. New DV = { %d %d %d %d }. Notifying neighbors.\n",
                   NODE_ID, clocktime, my_min_costs1[0], my_min_costs1[1], my_min_costs1[2], my_min_costs1[3]);
            printdt1(&routing_table1);
            send_dv_to_neighbors();
        } else {
             printf("   Node %d @ %.3f: Minimum cost vector UNCHANGED. No update sent.\n", NODE_ID, clocktime);
             if (TRACE >=2) printdt1(&routing_table1);
        }

    } else {
         printf("   Node %d @ %.3f: Received packet caused no change to distance table or min costs.\n", NODE_ID, clocktime);
    }
}


// Pretty print Node 1's distance table
void printdt1(struct DistanceTable *table_ptr) {
    int i, j;
    printf("\n");
    printf("              Distance Table for Node %d (via neighbor) @ time %.3f\n", NODE_ID, clocktime);
    printf("   D%d |", NODE_ID);
    // Neighbors of Node 1 are 0 and 2
    printf("    0   ");
    printf("    2   ");
    printf("\n");
    printf("------|-----------"); // Adjust dashes based on number of neighbors
    printf("\n");
    for (i = 0; i < NUM_NODES; i++) {
         if (i == NODE_ID) continue; // Don't print row for self
        printf("dest %d|", i);

        // Column for neighbor 0
        if (direct_link_costs1[0] < INFINITY) {
             if (table_ptr->costs[i][0] >= INFINITY) printf("   %3s ", "-");
             else printf("   %3d ", table_ptr->costs[i][0]);
        } else { printf("       "); } // Placeholder if not neighbor

        // Column for neighbor 2
        if (direct_link_costs1[2] < INFINITY) {
             if (table_ptr->costs[i][2] >= INFINITY) printf("   %3s ", "-");
             else printf("   %3d ", table_ptr->costs[i][2]);
        } else { printf("       "); } // Placeholder if not neighbor

        printf("\n");
    }
    printf("\n");
}

// Handler for link cost changes (Extra Credit) - Adjust if needed for Node 1 specific links
void rtlinkhandler1(int linkid, int newcost) {
   int dest_idx, neighbor_idx;
   int min_costs_changed = 0;

   printf("Node %d @ %.3f: Link handler: Link to %d cost changed to %d\n", NODE_ID, clocktime, linkid, newcost);

   int previous_min_costs[NUM_NODES];
   memcpy(previous_min_costs, my_min_costs1, NUM_NODES * sizeof(int));

   direct_link_costs1[linkid] = newcost;
   routing_table1.costs[linkid][linkid] = newcost;

   printf("   Node %d @ %.3f: Recalculating all minimum costs due to link change...\n", NODE_ID, clocktime);

    for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
       int current_min_cost = INFINITY;
       for (neighbor_idx = 0; neighbor_idx < NUM_NODES; neighbor_idx++) {
            // Update direct cost entry if it's the destination itself
            if(dest_idx == neighbor_idx) {
                 routing_table1.costs[dest_idx][dest_idx] = direct_link_costs1[dest_idx];
            }
           if (routing_table1.costs[dest_idx][neighbor_idx] < current_min_cost) {
               current_min_cost = routing_table1.costs[dest_idx][neighbor_idx];
           }
       }
        my_min_costs1[dest_idx] = current_min_cost;
   }
    my_min_costs1[NODE_ID] = 0; // Cost to self is always 0


   for(dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
       if (my_min_costs1[dest_idx] != previous_min_costs[dest_idx]) {
           min_costs_changed = 1;
           break;
       }
   }

   if (min_costs_changed) {
       printf("   Node %d @ %.3f: Minimum cost vector CHANGED after link update. New DV = { %d %d %d %d }. Notifying neighbors.\n",
              NODE_ID, clocktime, my_min_costs1[0], my_min_costs1[1], my_min_costs1[2], my_min_costs1[3]);
       printdt1(&routing_table1);
       send_dv_to_neighbors();
   } else {
       printf("   Node %d @ %.3f: Minimum cost vector UNCHANGED after link update.\n", NODE_ID, clocktime);
        if (TRACE >=2) printdt1(&routing_table1);
   }
}

// Alias function
void linkhandler1(int linkid, int newcost) {
  rtlinkhandler1(linkid, newcost);
}