#ifndef DVR_H
#define DVR_H

#define NUM_NODES 4
#define INFINITY 999

// Declare clocktime as extern so node files can access it
extern float clocktime;

struct RoutePacket {
    int sourceid;
    int destid;
    int mincost[NUM_NODES];
};

// Function prototypes for simulation
void tolayer2(struct RoutePacket packet);

// Prototypes for node routines
void rtinit0();
void rtupdate0(struct RoutePacket *rcvdpkt);

void rtinit1();
void rtupdate1(struct RoutePacket *rcvdpkt);

void rtinit2();
void rtupdate2(struct RoutePacket *rcvdpkt);

void rtinit3();
void rtupdate3(struct RoutePacket *rcvdpkt);

// Extra credit link handler prototypes
void rtlinkhandler0(int linkid, int newcost);
void rtlinkhandler1(int linkid, int newcost);

#endif
