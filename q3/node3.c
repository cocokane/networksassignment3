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
} dt3;

int connectcosts3[4] = {7, 999, 2, 0};
static int mincosts3[4];

/* Function prototypes */
void printdt3(struct distance_table *dtptr);
extern void tolayer2(struct rtpkt packet);
extern void creatertpkt(struct rtpkt *initrtpkt, int srcid, int destid, int mincosts[]);

void rtinit3() {
  int i, j;
  
  // Initialize distance table to INFINITY (999)
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      dt3.costs[i][j] = 999;
      
  // Set direct costs
  for (i = 0; i < 4; i++) {
    dt3.costs[i][i] = connectcosts3[i];
    mincosts3[i] = connectcosts3[i];
  }
  
  printf("rtinit3: Initialized\n");
  printdt3(&dt3);
  
  // Send initial routing packets to neighbors
  struct rtpkt packet;
  for (i = 0; i < 4; i++) {
    if (i != 3 && connectcosts3[i] < 999) {
      creatertpkt(&packet, 3, i, mincosts3);
      tolayer2(packet);
    }
  }
}

void rtupdate3(struct rtpkt *rcvdpkt) {
  int i, j;
  int sourceid = rcvdpkt->sourceid;
  int changed = 0;
  
  printf("rtupdate3: Received packet from node %d\n", sourceid);
  
  // Update distance table based on received costs
  for (i = 0; i < 4; i++) {
    int newcost = connectcosts3[sourceid] + rcvdpkt->mincost[i];
    if (newcost < dt3.costs[i][sourceid]) {
      dt3.costs[i][sourceid] = newcost;
      changed = 1;
    }
  }
  
  // If costs changed, recalculate minimum costs
  if (changed) {
    int oldmincosts[4];
    for (i = 0; i < 4; i++)
      oldmincosts[i] = mincosts3[i];
      
    // Find new minimum costs
    for (i = 0; i < 4; i++) {
      int min = 999;
      for (j = 0; j < 4; j++) {
        if (dt3.costs[i][j] < min)
          min = dt3.costs[i][j];
      }
      mincosts3[i] = min;
    }
    
    // Check if minimum costs changed
    int dvchanged = 0;
    for (i = 0; i < 4; i++) {
      if (mincosts3[i] != oldmincosts[i]) {
        dvchanged = 1;
        break;
      }
    }
    
    // If minimum costs changed, notify neighbors
    if (dvchanged) {
      printdt3(&dt3);
      struct rtpkt packet;
      for (i = 0; i < 4; i++) {
        if (i != 3 && connectcosts3[i] < 999) {
          creatertpkt(&packet, 3, i, mincosts3);
          tolayer2(packet);
        }
      }
    }
  }
}

void printdt3(struct distance_table *dtptr) {
  printf("             via     \n");
  printf("   D3 |    0     2 \n");
  printf("  ----|-----------\n");
  printf("     0|  %3d   %3d\n", dtptr->costs[0][0], dtptr->costs[0][2]);
  printf("dest 1|  %3d   %3d\n", dtptr->costs[1][0], dtptr->costs[1][2]);
  printf("     2|  %3d   %3d\n", dtptr->costs[2][0], dtptr->costs[2][2]);
}
