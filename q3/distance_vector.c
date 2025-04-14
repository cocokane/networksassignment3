#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dvr.h"

#define LINKCHANGES 1    /* Set to 1 if you want the link changes in extra credit */
                          
struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct RoutePacket *rtpktptr; /* ptr to packet (if any) for this event */
   struct event *prev;
   struct event *next;
};

struct event *evlist = NULL;   /* the event list */
float clocktime = 0.000;       /* current time */
int TRACE = 1;                 /* trace level */

void insertevent(struct event *p);
void init();
void rtinit0(), rtinit1(), rtinit2(), rtinit3();
void rtupdate0(struct RoutePacket *), rtupdate1(struct RoutePacket *), 
     rtupdate2(struct RoutePacket *), rtupdate3(struct RoutePacket *);
void linkhandler0(int, int), linkhandler1(int, int);
void printevlist(void);

int main(int argc, char *argv[]) {
    struct event *eventptr;
    struct RoutePacket *rtpktptr;
    int i;
   
    init();
   
    /* Set trace level based on command argument */
    if (argc > 1)
        TRACE = atoi(argv[1]);
   
    printf("Starting simulation...\n");
   
    while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr == NULL)
            break;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        clocktime = eventptr->evtime;   /* update time to next event time */
      
        if (TRACE >= 2) {
            printf("\nEVENT time: %f,", clocktime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", node: %d", eventptr->eventity);
            printf(" \n");
        }
      
        switch (eventptr->evtype) {
            case 0:     /* initialize a node */
                if (eventptr->eventity == 0)
                    rtinit0();
                else if (eventptr->eventity == 1)
                    rtinit1();
                else if (eventptr->eventity == 2)
                    rtinit2();
                else if (eventptr->eventity == 3)
                    rtinit3();
                else { printf("Panic: unknown event entity\n"); exit(0); }
                break;
            case 1:     /* packet from layer 2 */
                rtpktptr = eventptr->rtpktptr;
                if (eventptr->eventity == 0)
                    rtupdate0(rtpktptr);
                else if (eventptr->eventity == 1)
                    rtupdate1(rtpktptr);
                else if (eventptr->eventity == 2)
                    rtupdate2(rtpktptr);
                else if (eventptr->eventity == 3)
                    rtupdate3(rtpktptr);
                else { printf("Panic: unknown event entity\n"); exit(0); }
                break;
            case 2:     /* change link cost */
                if (LINKCHANGES == 1) {
                    if (eventptr->eventity == 0)
                        linkhandler0(1, eventptr->evtime < 15.0 ? 20 : 1);
                    else
                        linkhandler1(0, eventptr->evtime < 15.0 ? 20 : 1);
                }
                break;
        }
        if (eventptr->evtype == 1)
            free(eventptr->rtpktptr);
        free(eventptr);
    }
    printf("\nSimulation complete.\n");
    return 0;
}

void init() {
    /* Initialize by scheduling initialization events for all nodes */
    float t = 0.0;
    struct event *evptr;
    
    for (int i = 0; i < NUM_NODES; i++) {
        evptr = (struct event *)malloc(sizeof(struct event));
        evptr->evtime = t;
        evptr->evtype = 0;    /* Initialize node */
        evptr->eventity = i;
        insertevent(evptr);
    }
    
    /* Schedule link cost change events if enabled */
    if (LINKCHANGES) {
        evptr = (struct event *)malloc(sizeof(struct event));
        evptr->evtype = 2;
        evptr->evtime = 10000.0;
        evptr->eventity = 0;
        insertevent(evptr);
        
        evptr = (struct event *)malloc(sizeof(struct event));
        evptr->evtype = 2;
        evptr->evtime = 20000.0;
        evptr->eventity = 0;
        insertevent(evptr);
    }
}

/* Insert event into the event list */
void insertevent(struct event *p) {
    struct event *q, *qold;
    
    if (TRACE > 3) {
        printf("            INSERTEVENT: time is %lf\n", clocktime);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    
    q = evlist;
    if (q == NULL) {   /* list is empty */
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL) {   /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist) { /* front of list */
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else {     /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

/* Send a routing packet to layer 2 */
void tolayer2(struct RoutePacket pkt) {
    struct RoutePacket *mypktptr;
    struct event *evptr, *q;
    float lastime;
    int i;
    
    /* Simulate losses */
    if (TRACE > 1)
        printf("    TOLAYER2: source: %d, dest: %d, contents: %d %d %d %d\n", 
              pkt.sourceid, pkt.destid, pkt.mincost[0], pkt.mincost[1], pkt.mincost[2], pkt.mincost[3]);
    
    /* Create future event for this packet */
    mypktptr = (struct RoutePacket *)malloc(sizeof(struct RoutePacket));
    mypktptr->sourceid = pkt.sourceid;
    mypktptr->destid = pkt.destid;
    for (i = 0; i < NUM_NODES; i++)
        mypktptr->mincost[i] = pkt.mincost[i];
    
    /* Add delay - simulation time units */
    lastime = clocktime;
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = lastime + 1 + 0.01 * (float)rand() / RAND_MAX;
    evptr->evtype = 1;
    evptr->eventity = pkt.destid;
    evptr->rtpktptr = mypktptr;
    
    /* Insert this event */
    insertevent(evptr);
}
