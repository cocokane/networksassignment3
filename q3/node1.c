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
} dt1;

int connectcosts1[4] = {1, 0, 1, 999};
static int mincosts1[4];

/* Function prototypes */
void printdt1(struct distance_table *dtptr);
extern void tolayer2(struct rtpkt packet);
extern void creatertpkt(struct rtpkt *initrtpkt, int srcid, int destid, int mincosts[]);

void rtinit1() {
  int i, j;
  
  // Initialize distance table to INFINITY (999)
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      dt1.costs[i][j] = 999;
      
  // Set direct costs
  for (i = 0; i < 4; i++) {
    dt1.costs[i][i] = connectcosts1[i];
    mincosts1[i] = connectcosts1[i];
  }
  
  printf("rtinit1: Initialized\n");
  printdt1(&dt1);
  
  // Send initial routing packets to neighbors
  struct rtpkt packet;
  for (i = 0; i < 4; i++) {
    if (i != 1 && connectcosts1[i] < 999) {
      creatertpkt(&packet, 1, i, mincosts1);
      tolayer2(packet);
    }
  }
}

void rtupdate1(struct rtpkt *rcvdpkt) {
  int i, j;
  int sourceid = rcvdpkt->sourceid;
  int changed = 0;
  
  printf("rtupdate1: Received packet from node %d\n", sourceid);
  
  // Update distance table based on received costs
  for (i = 0; i < 4; i++) {
    int newcost = connectcosts1[sourceid] + rcvdpkt->mincost[i];
    if (newcost < dt1.costs[i][sourceid]) {
      dt1.costs[i][sourceid] = newcost;
      changed = 1;
    }
  }
  
  // If costs changed, recalculate minimum costs
  if (changed) {
    int oldmincosts[4];
    for (i = 0; i < 4; i++)
      oldmincosts[i] = mincosts1[i];
      
    // Find new minimum costs
    for (i = 0; i < 4; i++) {
      int min = 999;
      for (j = 0; j < 4; j++) {
        if (dt1.costs[i][j] < min)
          min = dt1.costs[i][j];
      }
      mincosts1[i] = min;
    }
    
    // Check if minimum costs changed
    int dvchanged = 0;
    for (i = 0; i < 4; i++) {
      if (mincosts1[i] != oldmincosts[i]) {
        dvchanged = 1;
        break;
      }
    }
    
    // If minimum costs changed, notify neighbors
    if (dvchanged) {
      printdt1(&dt1);
      struct rtpkt packet;
      for (i = 0; i < 4; i++) {
        if (i != 1 && connectcosts1[i] < 999) {
          creatertpkt(&packet, 1, i, mincosts1);
          tolayer2(packet);
        }
      }
    }
  }
}

void printdt1(struct distance_table *dtptr) {
  printf("             via   \n");
  printf("   D1 |    0     2 \n");
  printf("  ----|-----------\n");
  printf("     0|  %3d   %3d\n", dtptr->costs[0][0], dtptr->costs[0][2]);
  printf("dest 2|  %3d   %3d\n", dtptr->costs[2][0], dtptr->costs[2][2]);
  printf("     3|  %3d   %3d\n", dtptr->costs[3][0], dtptr->costs[3][2]);
}

void rtlinkhandler1(int linkid, int newcost) {
  int i;
  int changed = 0;
  printf("linkhandler1: Link to %d now has cost %d\n", linkid, newcost);
  
  // Update direct cost to neighbor
  int oldcost = connectcosts1[linkid];
  connectcosts1[linkid] = newcost;
  
  // Update distance table for direct link
  dt1.costs[linkid][linkid] = newcost;
  
  // Recalculate minimum costs
  for (i = 0; i < 4; i++) {
    int min = 999;
    int j;
    for (j = 0; j < 4; j++) {
      if (dt1.costs[i][j] < min)
        min = dt1.costs[i][j];
    }
    if (min != mincosts1[i]) {
      mincosts1[i] = min;
      changed = 1;
    }
  }
  
  // If costs changed, notify neighbors
  if (changed) {
    printdt1(&dt1);
    struct rtpkt packet;
    for (i = 0; i < 4; i++) {
      if (i != 1 && connectcosts1[i] < 999) {
        creatertpkt(&packet, 1, i, mincosts1);
        tolayer2(packet);
      }
    }
  }
}

// Alias function to call rtlinkhandler1
void linkhandler1(int linkid, int newcost) {
  rtlinkhandler1(linkid, newcost);
}
