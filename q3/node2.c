// FILE: node2.c (Modified)
#include <stdio.h>
#include <string.h> // For memcpy
#include <stdlib.h> // For exit, potentially
#include "dvr.h"    // Includes struct definitions and global externs

// --- Node 2 Specific Data ---
#define NODE_ID 2

// Distance Table for Node 2: routing_table2[dest_node][via_neighbor]
static struct DistanceTable routing_table2;

// Direct link costs from Node 2 to other nodes
static int direct_link_costs2[NUM_NODES] = {3, 1, 0, 2}; // Node 2 connected to 0, 1, 3

// Node 2's current best estimate of minimum costs to other nodes (Distance Vector)
static int my_min_costs2[NUM_NODES];

// --- Helper Functions ---

// Prepare a routing packet
static void prepare_routing_packet(struct RoutePacket *packet, int src, int dest, int costs[]) {
    packet->sourceid = src;
    packet->destid = dest;
    memcpy(packet->mincost, costs, NUM_NODES * sizeof(int));
}

// Send Node 2's current distance vector to all direct neighbors
static void send_dv_to_neighbors() {
    struct RoutePacket outgoing_pkt;
    int neighbor_idx;

    for (neighbor_idx = 0; neighbor_idx < NUM_NODES; neighbor_idx++) {
        // Send only to directly connected neighbors (cost < INFINITY) and not to self
        if (neighbor_idx != NODE_ID && direct_link_costs2[neighbor_idx] < INFINITY) {
             if (TRACE >= 1) {
                 printf("   Node %d @ %.3f: Preparing to send DV to neighbor %d\n",
                        NODE_ID, clocktime, neighbor_idx);
            }
            prepare_routing_packet(&outgoing_pkt, NODE_ID, neighbor_idx, my_min_costs2);
            tolayer2(outgoing_pkt); // Hand off packet to simulator
        }
    }
}


// --- Main Node 2 Functions ---

// Initialization routine for Node 2
void rtinit2() {
    int dest_idx, neighbor_idx;

    printf("Node %d @ %.3f: Initializing routing table.\n", NODE_ID, clocktime);

    // Initialize routing table: Set all costs to INFINITY initially
    for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
        for (neighbor_idx = 0; neighbor_idx < NUM_NODES; neighbor_idx++) {
            routing_table2.costs[dest_idx][neighbor_idx] = INFINITY;
        }
    }

    // Set costs for directly connected links and self-cost
    for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
         routing_table2.costs[dest_idx][dest_idx] = direct_link_costs2[dest_idx];
         my_min_costs2[dest_idx] = direct_link_costs2[dest_idx];
    }
     my_min_costs2[NODE_ID] = 0; // Cost to self is always 0

    printf("Node %d @ %.3f: Initialization complete. Initial DV = { %d %d %d %d }\n",
           NODE_ID, clocktime, my_min_costs2[0], my_min_costs2[1], my_min_costs2[2], my_min_costs2[3]);

    // Print the initial distance table
    printdt2(&routing_table2);

    // Send initial distance vector to all neighbors (0, 1, 3)
    send_dv_to_neighbors();
}


// Update routine for Node 2, called when a packet arrives
void rtupdate2(struct RoutePacket *incoming_pkt) {
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
     if (direct_link_costs2[sender_id] >= INFINITY) {
         printf("   Node %d @ %.3f: WARNING - Received packet from non-neighbor %d? Ignoring.\n", NODE_ID, clocktime, sender_id);
         return;
    }

    for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
        calculated_cost = direct_link_costs2[sender_id] + incoming_pkt->mincost[dest_idx];
         if (direct_link_costs2[sender_id] >= INFINITY || incoming_pkt->mincost[dest_idx] >= INFINITY) {
             calculated_cost = INFINITY;
        }
        if (calculated_cost > INFINITY) calculated_cost = INFINITY;


        if (calculated_cost < routing_table2.costs[dest_idx][sender_id]) {
             if (TRACE >= 1) {
                printf("   Node %d @ %.3f: Updating cost to %d via %d. Old: %d, New: %d\n",
                       NODE_ID, clocktime, dest_idx, sender_id,
                       routing_table2.costs[dest_idx][sender_id], calculated_cost);
            }
            routing_table2.costs[dest_idx][sender_id] = calculated_cost;
            dist_table_changed = 1;
        }
    }

    // --- Step 2: If the table changed, recalculate the minimum cost vector ---
    if (dist_table_changed) {
        printf("   Node %d @ %.3f: Distance table updated. Recalculating minimum costs...\n", NODE_ID, clocktime);
        int previous_min_costs[NUM_NODES];
        memcpy(previous_min_costs, my_min_costs2, NUM_NODES * sizeof(int));

        for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
            int current_min_cost = INFINITY;
            for (neighbor_idx = 0; neighbor_idx < NUM_NODES; neighbor_idx++) {
                 if (routing_table2.costs[dest_idx][neighbor_idx] < current_min_cost) {
                     current_min_cost = routing_table2.costs[dest_idx][neighbor_idx];
                 }
            }
            my_min_costs2[dest_idx] = current_min_cost;
        }
         my_min_costs2[NODE_ID] = 0; // Ensure cost to self is 0

        for(dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
            if (my_min_costs2[dest_idx] != previous_min_costs[dest_idx]) {
                min_costs_changed = 1;
                break;
            }
        }

        // --- Step 3: If the minimum costs changed, notify neighbors ---
        if (min_costs_changed) {
            printf("   Node %d @ %.3f: Minimum cost vector CHANGED. New DV = { %d %d %d %d }. Notifying neighbors.\n",
                   NODE_ID, clocktime, my_min_costs2[0], my_min_costs2[1], my_min_costs2[2], my_min_costs2[3]);
            printdt2(&routing_table2);
            send_dv_to_neighbors();
        } else {
             printf("   Node %d @ %.3f: Minimum cost vector UNCHANGED. No update sent.\n", NODE_ID, clocktime);
             if (TRACE >=2) printdt2(&routing_table2);
        }

    } else {
         printf("   Node %d @ %.3f: Received packet caused no change to distance table or min costs.\n", NODE_ID, clocktime);
    }
}


// Pretty print Node 2's distance table
void printdt2(struct DistanceTable *table_ptr) {
    int i, j;
    printf("\n");
    printf("                    Distance Table for Node %d (via neighbor) @ time %.3f\n", NODE_ID, clocktime);
    printf("   D%d |", NODE_ID);
    // Neighbors of Node 2 are 0, 1, 3
    printf("    0   ");
    printf("    1   ");
    printf("    3   ");
    printf("\n");
    printf("------|-------------------"); // Adjust dashes
    printf("\n");
    for (i = 0; i < NUM_NODES; i++) {
         if (i == NODE_ID) continue;
        printf("dest %d|", i);

        // Column for neighbor 0
        if (direct_link_costs2[0] < INFINITY) {
             if (table_ptr->costs[i][0] >= INFINITY) printf("   %3s ", "-");
             else printf("   %3d ", table_ptr->costs[i][0]);
        } else { printf("       "); }

        // Column for neighbor 1
        if (direct_link_costs2[1] < INFINITY) {
             if (table_ptr->costs[i][1] >= INFINITY) printf("   %3s ", "-");
             else printf("   %3d ", table_ptr->costs[i][1]);
        } else { printf("       "); }

        // Column for neighbor 3
        if (direct_link_costs2[3] < INFINITY) {
             if (table_ptr->costs[i][3] >= INFINITY) printf("   %3s ", "-");
             else printf("   %3d ", table_ptr->costs[i][3]);
        } else { printf("       "); }

        printf("\n");
    }
    printf("\n");
}

// rtlinkhandler2 needs to be implemented if link changes involving node 2 are needed
// void rtlinkhandler2(int linkid, int newcost) { ... }
// void linkhandler2(int linkid, int newcost) { rtlinkhandler2(linkid, newcost); }