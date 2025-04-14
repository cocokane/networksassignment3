#include <stdio.h>
#include <string.h>

struct rtpkt {
  int sourceid;
  int destid;
  int mincost[4];
};

// Make sure the function signature matches exactly what the node files expect
void creatertpkt(struct rtpkt *initrtpkt, int srcid, int destid, int mincosts[]) {
  initrtpkt->sourceid = srcid;
  initrtpkt->destid = destid;
  for (int i = 0; i < 4; i++)
    initrtpkt->mincost[i] = mincosts[i];
}
