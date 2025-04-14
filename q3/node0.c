#include <stdio.h>

struct rtpkt {
  int sourceid;
  int destid;
  int mincost[4];
};

extern int TRACE;
extern int YES;
extern int NO;

struct distance_table {
  int costs[4][4];
} dt0;

static int connectcosts0[4] = {0, 1, 3, 7};
static int mincosts0[4];

/* Function prototypes */
void printdt0(struct distance_table *dtptr);
extern void tolayer2(struct rtpkt packet);
extern void creatertpkt(struct rtpkt *initrtpkt, int srcid, int destid, int mincosts[]);

void rtinit0() {
  int i, j;
  
  // Initialize distance table to INFINITY (999)
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      dt0.costs[i][j] = 999;
      
  // Set direct costs
  for (i = 0; i < 4; i++) {
    dt0.costs[i][i] = connectcosts0[i];
    mincosts0[i] = connectcosts0[i];
  }
  
  printf("rtinit0: Initialized\n");
  printdt0(&dt0);
  
  // Send initial routing packets to neighbors
  struct rtpkt packet;
  for (i = 1; i < 4; i++) {
    if (connectcosts0[i] < 999) {
      creatertpkt(&packet, 0, i, mincosts0);
      tolayer2(packet);
    }
  }
}

void rtupdate0(struct rtpkt *rcvdpkt) {
  int i, j;
  int sourceid = rcvdpkt->sourceid;
  int changed = 0;
  
  printf("rtupdate0: Received packet from node %d\n", sourceid);
  
  // Update distance table based on received costs
  for (i = 0; i < 4; i++) {
    int newcost = connectcosts0[sourceid] + rcvdpkt->mincost[i];
    if (newcost < dt0.costs[i][sourceid]) {
      dt0.costs[i][sourceid] = newcost;
      changed = 1;
    }
  }
  
  // If costs changed, recalculate minimum costs
  if (changed) {
    int oldmincosts[4];
    for (i = 0; i < 4; i++)
      oldmincosts[i] = mincosts0[i];
      
    // Find new minimum costs
    for (i = 0; i < 4; i++) {
      int min = 999;
      for (j = 0; j < 4; j++) {
        if (dt0.costs[i][j] < min)
          min = dt0.costs[i][j];
      }
      mincosts0[i] = min;
    }
    
    // Check if minimum costs changed
    int dvchanged = 0;
    for (i = 0; i < 4; i++) {
      if (mincosts0[i] != oldmincosts[i]) {
        dvchanged = 1;
        break;
      }
    }
    
    // If minimum costs changed, notify neighbors
    if (dvchanged) {
      printdt0(&dt0);
      struct rtpkt packet;
      for (i = 1; i < 4; i++) {
        if (connectcosts0[i] < 999) {
          creatertpkt(&packet, 0, i, mincosts0);
          tolayer2(packet);
        }
      }
    }
  }
}

void printdt0(struct distance_table *dtptr) {
  printf("                via     \n");
  printf("   D0 |    1     2    3 \n");
  printf("  ----|-----------------\n");
  printf("     1|  %3d   %3d   %3d\n", dtptr->costs[1][1],
         dtptr->costs[1][2], dtptr->costs[1][3]);
  printf("dest 2|  %3d   %3d   %3d\n", dtptr->costs[2][1],
         dtptr->costs[2][2], dtptr->costs[2][3]);
  printf("     3|  %3d   %3d   %3d\n", dtptr->costs[3][1],
         dtptr->costs[3][2], dtptr->costs[3][3]);
}

void rtlinkhandler0(int linkid, int newcost) {
  int i;
  int changed = 0;
  printf("rtlinkhandler0: Link to %d now has cost %d\n", linkid, newcost);
  
  // Update direct cost to neighbor
  int oldcost = connectcosts0[linkid];
  connectcosts0[linkid] = newcost;
  
  // Update distance table for direct link
  dt0.costs[linkid][linkid] = newcost;
  
  // Recalculate minimum costs
  for (i = 0; i < 4; i++) {
    int min = 999;
    int j;
    for (j = 0; j < 4; j++) {
      if (dt0.costs[i][j] < min)
        min = dt0.costs[i][j];
    }
    if (min != mincosts0[i]) {
      mincosts0[i] = min;
      changed = 1;
    }
  }
  
  // If costs changed, notify neighbors
  if (changed) {
    printdt0(&dt0);
    struct rtpkt packet;
    for (i = 1; i < 4; i++) {
      if (connectcosts0[i] < 999) {
        creatertpkt(&packet, 0, i, mincosts0);
        tolayer2(packet);
      }
    }
  }
}

// Alias function to call rtlinkhandler0 - keeps both signatures available
void linkhandler0(int linkid, int newcost) {
  rtlinkhandler0(linkid, newcost);
}
