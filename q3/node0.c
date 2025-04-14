// FILE: node0.c (Modified)
#include <stdio.h>
#include <string.h> // For memcpy
#include <stdlib.h> // For exit, potentially
#include "dvr.h"    // Includes struct definitions and global externs

// --- Node 0 Specific Data ---
#define NODE_ID 0

// Distance Table for Node 0: routing_table0[dest_node][via_neighbor]
static struct DistanceTable routing_table0;

// Direct link costs from Node 0 to other nodes
static int direct_link_costs0[NUM_NODES] = {0, 1, 3, 7};

// Node 0's current best estimate of minimum costs to other nodes (Distance Vector)
static int my_min_costs0[NUM_NODES];

// --- Helper Functions ---

// Prepare a routing packet
static void prepare_routing_packet(struct RoutePacket *packet, int src, int dest, int costs[]) {
    packet->sourceid = src;
    packet->destid = dest;
    memcpy(packet->mincost, costs, NUM_NODES * sizeof(int));
}

// Send Node 0's current distance vector to all direct neighbors
static void send_dv_to_neighbors() {
    struct RoutePacket outgoing_pkt;
    int neighbor_idx;

    for (neighbor_idx = 0; neighbor_idx < NUM_NODES; neighbor_idx++) {
        // Send only to directly connected neighbors (cost < INFINITY) and not to self
        if (neighbor_idx != NODE_ID && direct_link_costs0[neighbor_idx] < INFINITY) {
            if (TRACE >= 1) {
                 printf("   Node %d @ %.3f: Preparing to send DV to neighbor %d\n",
                        NODE_ID, clocktime, neighbor_idx);
            }
            prepare_routing_packet(&outgoing_pkt, NODE_ID, neighbor_idx, my_min_costs0);
            tolayer2(outgoing_pkt); // Hand off packet to simulator
        }
    }
}


// --- Main Node 0 Functions ---

// Initialization routine for Node 0
void rtinit0() {
    int dest_idx, neighbor_idx;

    printf("Node %d @ %.3f: Initializing routing table.\n", NODE_ID, clocktime);

    // Initialize routing table: Set all costs to INFINITY initially
    for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
        for (neighbor_idx = 0; neighbor_idx < NUM_NODES; neighbor_idx++) {
            routing_table0.costs[dest_idx][neighbor_idx] = INFINITY;
        }
    }

    // Set costs for directly connected links and self-cost
    // routing_table0[dest][dest] stores the direct cost if dest is a neighbor or self
    for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
         routing_table0.costs[dest_idx][dest_idx] = direct_link_costs0[dest_idx];
         // Initialize minimum cost vector with direct costs
         my_min_costs0[dest_idx] = direct_link_costs0[dest_idx];
    }
     my_min_costs0[NODE_ID] = 0; // Cost to self is always 0

    printf("Node %d @ %.3f: Initialization complete. Initial DV = { %d %d %d %d }\n",
           NODE_ID, clocktime, my_min_costs0[0], my_min_costs0[1], my_min_costs0[2], my_min_costs0[3]);

    // Print the initial distance table
    printdt0(&routing_table0);

    // Send initial distance vector to all neighbors
    send_dv_to_neighbors();
}


// Update routine for Node 0, called when a packet arrives
void rtupdate0(struct RoutePacket *incoming_pkt) {
    int sender_id = incoming_pkt->sourceid;
    int dest_idx, neighbor_idx;
    int calculated_cost;
    int dist_table_changed = 0; // Flag: Did any entry in the distance table change?
    int min_costs_changed = 0; // Flag: Did the node's minimum cost vector change?

    printf("Node %d @ %.3f: Received routing update from Node %d. DV = { %d %d %d %d }\n",
           NODE_ID, clocktime, sender_id,
           incoming_pkt->mincost[0], incoming_pkt->mincost[1],
           incoming_pkt->mincost[2], incoming_pkt->mincost[3]);

    // --- Step 1: Update the distance table using the received DV ---
    // Consider paths to all destinations 'dest_idx' going via 'sender_id'
    if (direct_link_costs0[sender_id] >= INFINITY) {
         printf("   Node %d @ %.3f: WARNING - Received packet from non-neighbor %d? Ignoring.\n", NODE_ID, clocktime, sender_id);
         return; // Should not happen in this simulation setup
    }

    for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
        // Cost to reach 'dest_idx' via 'sender_id' = (cost from 0 to sender_id) + (sender's cost to dest_idx)
        calculated_cost = direct_link_costs0[sender_id] + incoming_pkt->mincost[dest_idx];

        // Clamp cost to INFINITY if overflow happens or components are INFINITY
        if (direct_link_costs0[sender_id] >= INFINITY || incoming_pkt->mincost[dest_idx] >= INFINITY) {
             calculated_cost = INFINITY;
        }
        // Prevent overflow if costs are large but not quite INFINITY
        if (calculated_cost > INFINITY) calculated_cost = INFINITY;


        // If this new path is cheaper than the current path via 'sender_id'
        if (calculated_cost < routing_table0.costs[dest_idx][sender_id]) {
            if (TRACE >= 1) {
                printf("   Node %d @ %.3f: Updating cost to %d via %d. Old: %d, New: %d\n",
                       NODE_ID, clocktime, dest_idx, sender_id,
                       routing_table0.costs[dest_idx][sender_id], calculated_cost);
            }
            routing_table0.costs[dest_idx][sender_id] = calculated_cost;
            dist_table_changed = 1; // Mark that the table was modified
        }
    }

    // --- Step 2: If the table changed, recalculate the minimum cost vector ---
    if (dist_table_changed) {
        printf("   Node %d @ %.3f: Distance table updated. Recalculating minimum costs...\n", NODE_ID, clocktime);
        // Store previous min costs to detect changes
        int previous_min_costs[NUM_NODES];
        memcpy(previous_min_costs, my_min_costs0, NUM_NODES * sizeof(int));

        // Recalculate min cost for each destination
        for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
            int current_min_cost = INFINITY;
            // Find the minimum cost across all possible next hops (neighbors)
            for (neighbor_idx = 0; neighbor_idx < NUM_NODES; neighbor_idx++) {
                 if (routing_table0.costs[dest_idx][neighbor_idx] < current_min_cost) {
                     current_min_cost = routing_table0.costs[dest_idx][neighbor_idx];
                 }
            }
             // Update the node's minimum cost vector if a new minimum is found
             // Note: This comparison with previous_min_costs happens *after* recalculating all destinations
            my_min_costs0[dest_idx] = current_min_cost;
        }

        // Check if the overall minimum cost vector has changed
        for(dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
            if (my_min_costs0[dest_idx] != previous_min_costs[dest_idx]) {
                min_costs_changed = 1;
                break;
            }
        }

        // --- Step 3: If the minimum costs changed, notify neighbors ---
        if (min_costs_changed) {
            printf("   Node %d @ %.3f: Minimum cost vector CHANGED. New DV = { %d %d %d %d }. Notifying neighbors.\n",
                   NODE_ID, clocktime, my_min_costs0[0], my_min_costs0[1], my_min_costs0[2], my_min_costs0[3]);
            printdt0(&routing_table0); // Print updated table
            send_dv_to_neighbors();    // Send the new DV
        } else {
             printf("   Node %d @ %.3f: Minimum cost vector UNCHANGED. No update sent.\n", NODE_ID, clocktime);
              if (TRACE >=2) printdt0(&routing_table0); // Optionally print table even if DV didn't change
        }

    } else {
         printf("   Node %d @ %.3f: Received packet caused no change to distance table or min costs.\n", NODE_ID, clocktime);
    }
}


// Pretty print Node 0's distance table
void printdt0(struct DistanceTable *table_ptr) {
    int i, j;
    printf("\n");
    printf("                    Distance Table for Node %d (via neighbor) @ time %.3f\n", NODE_ID, clocktime);
    printf("   D%d |", NODE_ID);
    for (i = 0; i < NUM_NODES; i++) {
        if (i != NODE_ID) { // Only show columns for potential neighbors
             if (direct_link_costs0[i] < INFINITY || i == NODE_ID ) // Show neighbors + self column if needed
                 printf("    %d   ", i);
        }
    }
    printf("\n");
    printf("------|----------------------------------\n");
    for (i = 0; i < NUM_NODES; i++) {
         if (i == NODE_ID) continue; // Don't print row for self
        printf("dest %d|", i);
        for (j = 0; j < NUM_NODES; j++) {
             if (j != NODE_ID) { // Only print entries for neighbors
                if (direct_link_costs0[j] < INFINITY || j == NODE_ID ) {
                     if (table_ptr->costs[i][j] >= INFINITY)
                        printf("   %3s ", "-"); // Use '-' for infinity for cleaner look
                     else
                        printf("   %3d ", table_ptr->costs[i][j]);
                }
            }
        }
        printf("\n");
    }
    printf("\n");
}

// Handler for link cost changes (Extra Credit)
void rtlinkhandler0(int linkid, int newcost) {
  int dest_idx, neighbor_idx;
  int min_costs_changed = 0;

  printf("Node %d @ %.3f: Link handler: Link to %d cost changed to %d\n", NODE_ID, clocktime, linkid, newcost);

  // Store previous min costs to detect changes
  int previous_min_costs[NUM_NODES];
  memcpy(previous_min_costs, my_min_costs0, NUM_NODES * sizeof(int));

  // Update the direct cost in our static array
  direct_link_costs0[linkid] = newcost;

  // Update the cost in the distance table (cost to reach 'linkid' via 'linkid')
  routing_table0.costs[linkid][linkid] = newcost;

  // IMPORTANT: We potentially need to recalculate costs for *all* destinations
  // that might have used the link *via* linkid, AND costs that might have
  // used the link *directly* if the direct cost changed.
  // Simplest approach for DV: Recalculate the *entire* DV.

  printf("   Node %d @ %.3f: Recalculating all minimum costs due to link change...\n", NODE_ID, clocktime);

  // Recalculate min cost for each destination
   for (dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
      int current_min_cost = INFINITY;
      // We may need to re-evaluate costs previously learned *from* linkid as well,
      // if the cost *to* linkid changed significantly.
      // Re-running the Bellman-Ford update implicitly via recalculation handles this.

      // Find the minimum cost across all possible next hops (neighbors) using updated table/link costs
      for (neighbor_idx = 0; neighbor_idx < NUM_NODES; neighbor_idx++) {
           // Recompute cost via neighbor: cost(0,neighbor) + previously_learned_cost(neighbor, dest)
           // This isn't quite right for link changes. DV recalculation is simpler:
           // Just find the minimum value currently in each row of routing_table0.
           // The rtupdate mechanism will fix entries learned *from* neighbors later.

           // If the direct link cost to 'dest_idx' itself changed, use that.
           if (dest_idx == neighbor_idx) {
                 routing_table0.costs[dest_idx][dest_idx] = direct_link_costs0[dest_idx];
           }


           if (routing_table0.costs[dest_idx][neighbor_idx] < current_min_cost) {
               current_min_cost = routing_table0.costs[dest_idx][neighbor_idx];
           }
      }
       my_min_costs0[dest_idx] = current_min_cost;
       if (dest_idx == NODE_ID) my_min_costs0[dest_idx] = 0; // Ensure cost to self is 0
   }


  // Check if the overall minimum cost vector has changed
  for(dest_idx = 0; dest_idx < NUM_NODES; dest_idx++) {
      if (my_min_costs0[dest_idx] != previous_min_costs[dest_idx]) {
          min_costs_changed = 1;
          break;
      }
  }

   // If the minimum costs changed, notify neighbors
   if (min_costs_changed) {
       printf("   Node %d @ %.3f: Minimum cost vector CHANGED after link update. New DV = { %d %d %d %d }. Notifying neighbors.\n",
              NODE_ID, clocktime, my_min_costs0[0], my_min_costs0[1], my_min_costs0[2], my_min_costs0[3]);
       printdt0(&routing_table0); // Print updated table
       send_dv_to_neighbors();    // Send the new DV
   } else {
       printf("   Node %d @ %.3f: Minimum cost vector UNCHANGED after link update.\n", NODE_ID, clocktime);
       if (TRACE >=2) printdt0(&routing_table0);
   }
}

// Alias function for compatibility with distance_vector.c if needed
void linkhandler0(int linkid, int newcost) {
  rtlinkhandler0(linkid, newcost);
}