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
} dt2;

int connectcosts2[4] = {3, 1, 0, 2};
static int mincosts2[4];

/* Function prototypes */
void printdt2(struct distance_table *dtptr);
extern void tolayer2(struct rtpkt packet);
extern void creatertpkt(struct rtpkt *initrtpkt, int srcid, int destid, int mincosts[]);

void rtinit2() {
  int i, j;
  
  // Initialize distance table to INFINITY (999)
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      dt2.costs[i][j] = 999;
      
  // Set direct costs
  for (i = 0; i < 4; i++) {
    dt2.costs[i][i] = connectcosts2[i];
    mincosts2[i] = connectcosts2[i];
  }
  
  printf("rtinit2: Initialized\n");
  printdt2(&dt2);
  
  // Send initial routing packets to neighbors
  struct rtpkt packet;
  for (i = 0; i < 4; i++) {
    if (i != 2 && connectcosts2[i] < 999) {
      creatertpkt(&packet, 2, i, mincosts2);
      tolayer2(packet);
    }
  }
}

void rtupdate2(struct rtpkt *rcvdpkt) {
  int i, j;
  int sourceid = rcvdpkt->sourceid;
  int changed = 0;
  
  printf("rtupdate2: Received packet from node %d\n", sourceid);
  
  // Update distance table based on received costs
  for (i = 0; i < 4; i++) {
    int newcost = connectcosts2[sourceid] + rcvdpkt->mincost[i];
    if (newcost < dt2.costs[i][sourceid]) {
      dt2.costs[i][sourceid] = newcost;
      changed = 1;
    }
  }
  
  // If costs changed, recalculate minimum costs
  if (changed) {
    int oldmincosts[4];
    for (i = 0; i < 4; i++)
      oldmincosts[i] = mincosts2[i];
      
    // Find new minimum costs
    for (i = 0; i < 4; i++) {
      int min = 999;
      for (j = 0; j < 4; j++) {
        if (dt2.costs[i][j] < min)
          min = dt2.costs[i][j];
      }
      mincosts2[i] = min;
    }
    
    // Check if minimum costs changed
    int dvchanged = 0;
    for (i = 0; i < 4; i++) {
      if (mincosts2[i] != oldmincosts[i]) {
        dvchanged = 1;
        break;
      }
    }
    
    // If minimum costs changed, notify neighbors
    if (dvchanged) {
      printdt2(&dt2);
      struct rtpkt packet;
      for (i = 0; i < 4; i++) {
        if (i != 2 && connectcosts2[i] < 999) {
          creatertpkt(&packet, 2, i, mincosts2);
          tolayer2(packet);
        }
      }
    }
  }
}

void printdt2(struct distance_table *dtptr) {
  printf("                via     \n");
  printf("   D2 |    0     1    3 \n");
  printf("  ----|-----------------\n");
  printf("     0|  %3d   %3d   %3d\n", dtptr->costs[0][0],
         dtptr->costs[0][1], dtptr->costs[0][3]);
  printf("dest 1|  %3d   %3d   %3d\n", dtptr->costs[1][0],
         dtptr->costs[1][1], dtptr->costs[1][3]);
  printf("     3|  %3d   %3d   %3d\n", dtptr->costs[3][0],
         dtptr->costs[3][1], dtptr->costs[3][3]);
}
