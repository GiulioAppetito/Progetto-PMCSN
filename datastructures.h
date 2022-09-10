struct{
  double current;                 /* current time                        */
  double next;                    /* next (most imminent) event time     */
  double last;                    /* last arrival time                   */
} t;

typedef struct serviceData{
  double mean;
  int stream;
}serviceData;


typedef struct center{
  double node;                    /* time integrated number in the node  */
  double queue;                   /* time integrated number in the queue */
  double service;                 /* time integrated number in service   */
  double index;                     /* used to count departed jobs */
  double number;                    /* number in the node */
  double servers;
  char *name;
  double firstArrival;
  double lastArrival;
  double lastService;
  serviceData *serviceParams;
  
}center;

typedef struct event{    /* the next-event    */
  double t;                                  /*   next event time      */
  int    x;                                  /*   event status, 0 or 1 */
}event;                  /* [0 : a_biglietteria | 1 : c_biglietteria[0] | 2 : c_biglietteria[1] | 3 : a0_controlloBiglietti | 4 : a1_controlloBiglietti | 5 : c_controlloBiglietti | 6 : a_cassaFood | 7 : c_cassaFood | 8 : a_foodArea | 9 : c_foodArea | 10 : a_gadgetsArea | 11 : c_gadgetsArea] */

typedef struct multiserver{
    double service;
    int served;
    int occupied;
} multiserver;

typedef enum choice{
    FOODAREA,
    GADGETSAREA,
    NONE
} choice;

typedef enum ticketMode{
  ONLINE,
  PHYSICAL
} ticketMode;

typedef struct confidenceInterval{
  double mean;
  double w;
}confidenceInterval;

typedef struct outputStats{
  double avgInterarrivalTime;
  double avgArrivalRate;
  double avgWait;
  double avgDelay;
  double avgServiceTime;
  double avgNumNode;
  double avgNumQueue;
  double utilization;
  char *name;
  int jobs;
}outputStats;


