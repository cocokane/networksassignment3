// FILE: dvr.h (Modified)
#ifndef DVR_H
#define DVR_H

// --- Constants ---
#define NUM_NODES 4     // Total number of nodes in the network
#define INFINITY 999    // Value representing infinite cost (no direct path)

// --- Global Variables (Declared in distance_vector.c) ---
extern float clocktime; // Current simulation time
extern int TRACE;       // Simulation trace level (0: off, 1: basic, 2: detailed)

// --- Data Structures ---

// Structure for routing packets exchanged between nodes
struct RoutePacket {
    int sourceid;           // ID of the node sending this packet (0-3)
    int destid;             // ID of the intended recipient node (must be a neighbor)
    int mincost[NUM_NODES]; // Sender's current distance vector (minimum cost to nodes 0-3)
};

// Structure for the distance table maintained by each node
// (Note: Each nodeX.c file will define its own instance of this)
struct DistanceTable {
    int costs[NUM_NODES][NUM_NODES]; // costs[dest][via_neighbor]
};


// --- Function Prototypes for Simulation Engine (distance_vector.c) ---

// Function to send a routing packet to the network layer (Layer 2)
void tolayer2(struct RoutePacket packet);


// --- Function Prototypes for Node Logic (nodeX.c files) ---
// These are the functions you need to implement for each node.

// Node 0 routines
void rtinit0();
void rtupdate0(struct RoutePacket *rcvdpkt);
void printdt0(struct DistanceTable *dtptr); // Provided pretty-print function
void linkhandler0(int linkid, int newcost); // For extra credit link changes

// Node 1 routines
void rtinit1();
void rtupdate1(struct RoutePacket *rcvdpkt);
void printdt1(struct DistanceTable *dtptr); // Provided pretty-print function
void linkhandler1(int linkid, int newcost); // For extra credit link changes

// Node 2 routines
void rtinit2();
void rtupdate2(struct RoutePacket *rcvdpkt);
void printdt2(struct DistanceTable *dtptr); // Provided pretty-print function
// void linkhandler2(...) // If needed for extra credit

// Node 3 routines
void rtinit3();
void rtupdate3(struct RoutePacket *rcvdpkt);
void printdt3(struct DistanceTable *dtptr); // Provided pretty-print function
// void linkhandler3(...) // If needed for extra credit


#endif // DVR_H