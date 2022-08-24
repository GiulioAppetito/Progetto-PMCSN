/* ------------------------------------------------------------------------- 
 * This program is a next-event simulation of a single-server FIFO service
 * node using Exponentially distributed interarrival times and Erlang 
 * distributed service times (i.e., a M/E/1 queue).  The service node is 
 * assumed to be initially idle, no arrivals are permitted after the 
 * terminal time STOP, and the node is then purged by processing any 
 * remaining jobs in the service node.
 *
 * Name            : ssq4.c  (Single Server Queue, version 4)
 * Author          : Steve Park & Dave Geyer
 * Language        : ANSI C
 * Latest Revision : 11-09-98
 * ------------------------------------------------------------------------- 
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "rngs.h"                      /* the multi-stream generator */
#include "rvgs.h"                      /* random variate generators  */

#define START         0.0              /* initial time                   */
#define STOP          1000.0          /* terminal (close the door) time */
#define INFINITY      (100.0 * STOP)      /* must be much larger than STOP  */
#define SEED          123456789

#define DEBUG

#define SERVERS_BIGLIETTERIA         1
#define SERVERS_CONTROLLO_BIGLIETTI  2
#define SERVERS_CASSA_FOOD_AREA      1
#define SERVERS_FOOD_AREA            3
#define SERVERS_GADGETS_AREA         1

#define NUMBER_OF_EVENTS 10

double lambda                = 7.166;
double MU_BIGLIETTERIA       = 4.0;
double MU_CONTROLLOBIGLIETTI = 7.0; /* da modificare */
double MU_CASSA_FOOD_AREA    = 6.0;
double MU_FOOD_AREA          = 5.0;


#define STREAM_ARRIVALS            0
#define STREAM_BIGLIETTERIA0       1
#define STREAM_BIGLIETTERIA1       2
#define STREAM_CONTROLLOBIGLIETTI  3
#define STREAM_CASSA_FOOD_AREA     4
#define STREAM_FOOD_AREA     5



#define P_FOOD_AREA    0.8441
#define P_GADGETS_AREA 0.1376


/* da cancellare */
int arrivalsControllo       = 0;
int departuresControllo     = 0;
int arrivalsBiglietteria    = 0;
int departuresBiglietteria0 = 0;
int departuresBiglietteria1 = 0;


struct{
  double current;                 /* current time                        */
  double next;                    /* next (most imminent) event time     */
  double last;                    /* last arrival time                   */
} t;

typedef struct{
  double node;                    /* time integrated number in the node  */
  double queue;                   /* time integrated number in the queue */
  double service;                 /* time integrated number in service   */
  double index;                     /* used to count departed jobs */
  double number;                    /* number in the node */
  double servers;
  char *name;
}center;

typedef struct serviceData{
  double mean;
  int stream;
}serviceData;

typedef struct event{                   /* the next-event    */
  double t;                                  /*   next event time      */
  int    x;                                  /*   event status, 0 or 1 */
}event;              /* [a_biglietteria | c_biglietteria[0] | c_biglietteria[1] | a0_controlloBiglietti | a1_controlloBiglietti | c_controlloBiglietti | a_cassaFood | c_cassaFood | a_foodArea | c_foodArea] */


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

double Min(double a, double c)
/* ------------------------------
 * return the smaller of a, b
 * ------------------------------
 */
{ 
  if (a < c)
    return (a);
  else
    return (c);
} 

double GetArrival()
/* ---------------------------------------------
 * generate the next arrival time, with rate 1/2
 * ---------------------------------------------
 */ 
{
  static double arrival = START;

  SelectStream(STREAM_ARRIVALS); 
  double mean = 1/lambda;
  arrival += Exponential(mean);
  return (arrival);
} 

double GetService(serviceData *center)
/* --------------------------------------------
 * generate the next service time with rate 2/3
 * --------------------------------------------
 */ 
{
  SelectStream(center->stream);
  double mean = 1 / center->mean;
  return (Exponential(mean));
}

int NextEvent(event events[])
/* ---------------------------------------
 * return the index of the next event type
 * ---------------------------------------
 */
{
  int e;                    /* event index */
  double current = INFINITY; /* current min */

  for(int i=0; i<NUMBER_OF_EVENTS; i++){
    if(events[i].x == 1){             /* check if the event is active */
      if(events[i].t <= current){     /* check if it came sooner then the current min */
        current = events[i].t;
        e = i;
      }
    }
  }
  return e;
}

int FindIdleServer(multiserver multiserver[], int servers){
  /* -----------------------------------------------------
 * return the index of the available server idle longest
 * -----------------------------------------------------
 */
  int s;
  double current = INFINITY; /* current min */

  for(int i=0; i<servers; i++){
    if(multiserver[i].occupied == 0){              /* check if the multiserver is available == 0 */
      if(multiserver[i].service <= current){             /* find which                                 */ 
        current = multiserver[i].service;          /* has been idle longest                      */
        s = i;
      }
    }
  }
  return (s);

}

void initCenterStats(center *center, int servers, char *name){
  center->node=0.0;
  center->queue=0.0;
  center->service=0.0;
  center->index=0;
  center->number = 0.0;
  center->servers = servers;
  center->name = name;
}

int routingBiglietteria(){
  int i = 0;
  double p = Uniform(0, 1);    /* routing probability p */
  if(p < 0.5){
    i=0;
  }else{
    i=1;
  }
  return i;
}

choice routingControlloBiglietti(){
  int i = 0;
  double p = Uniform(0, 1);    /* routing probability p */
  if(p < P_FOOD_AREA){
    return FOODAREA;
  }else if ( p < (P_FOOD_AREA + P_GADGETS_AREA)){
    return GADGETSAREA;
  }else{
    return NONE;
  }
}

void initServiceData(serviceData *center, double mu, int stream){
  center->mean = mu;
  center->stream = stream;
}

void visualizeStatistics(center *center){
    printf("\n******* STATISTICHE %s for %.2f jobs *******\n\n", center->name, center->index);
    printf("   average interarrival time = %6.2f\n", t.last / center->index);
    printf("   average wait ............ = %6.2f\n", center->node / center->index);
    printf("   average delay ........... = %6.2f\n", center->queue / center->index);
    printf("   average service time .... = %6.2f\n", center->service / center->index);
    printf("   average # in the node ... = %6.2f\n", center->node / t.current);
    printf("   average # in the queue .. = %6.2f\n", center->queue / t.current);
    printf("   utilization ............. = %6.2f\n", center->service / t.current);
  }

int main(void){
  system("cls");
  printf(" ____________________________\n");
  printf("|    GESTIONE DI UN CINEMA   |\n");
  printf("|      Brinati Anastasia     |\n");
  printf("|       Appetito Giulio      |\n");
  printf("|____________________________|\n");

  double food=0;
        double gadgets=0;
        double none=0;
        double defaults=0;

  center biglietteria[2];
  center controlloBiglietti;
  multiserver controlloBiglietti_MS[SERVERS_CONTROLLO_BIGLIETTI];
  for(int i=0; i<SERVERS_CONTROLLO_BIGLIETTI; i++){
    controlloBiglietti_MS[i].served = 0;
    controlloBiglietti_MS[i].occupied = 0;
    controlloBiglietti_MS[i].service = 0.0;
  }  
  center cassaFoodArea;
  center foodArea;
  center gadgetsArea;

  initCenterStats(&biglietteria[0], SERVERS_BIGLIETTERIA, "biglietteria_0");
  initCenterStats(&biglietteria[1], SERVERS_BIGLIETTERIA, "biglietteria_1");
  initCenterStats(&controlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI, "controlloBiglietti");
  initCenterStats(&cassaFoodArea, SERVERS_CASSA_FOOD_AREA,"cassaFoodArea");
  initCenterStats(&foodArea, SERVERS_FOOD_AREA,"foodArea");
  initCenterStats(&gadgetsArea, SERVERS_GADGETS_AREA,"gadgetsArea");

  serviceData biglietteriaService0;
  serviceData biglietteriaService1;
  serviceData controlloBigliettiService;
  serviceData cassaFoodAreaService;
  serviceData foodAreaService;
  initServiceData(&biglietteriaService0, MU_BIGLIETTERIA, STREAM_BIGLIETTERIA0);
  initServiceData(&biglietteriaService1, MU_BIGLIETTERIA, STREAM_BIGLIETTERIA1);
  initServiceData(&controlloBigliettiService, MU_CONTROLLOBIGLIETTI, STREAM_CONTROLLOBIGLIETTI);
  initServiceData(&cassaFoodAreaService, MU_CASSA_FOOD_AREA, STREAM_CASSA_FOOD_AREA);
  initServiceData(&foodAreaService, MU_FOOD_AREA, STREAM_FOOD_AREA);

  event departuresControlloBiglietti[SERVERS_CONTROLLO_BIGLIETTI];
   for(int i=0; i<SERVERS_CONTROLLO_BIGLIETTI; i++){
    departuresControlloBiglietti[i].x = 0;
    departuresControlloBiglietti[i].t = 0.0;
  }
  event event[NUMBER_OF_EVENTS]; /*arrivo0 servizio0 servizio1 arrivo1 departure1*/
  for(int i=0; i<NUMBER_OF_EVENTS; i++){
    event[i].x = 0;
    event[i].t = 0.0;
  }

  PlantSeeds(SEED);
  t.current    = START; /* set the clock */
  
  event[0].t = GetArrival(); /* genero e salvo il primo arrivo */
  event[0].x   = 1;

  int e;
  int i;

  while ((event[0].t < STOP) || ((biglietteria[0].number + biglietteria[1].number + controlloBiglietti.number + cassaFoodArea.number + foodArea.number) > 0)){
    e = NextEvent(event);                        /* next event index  */
    t.next = event[e].t;                         /* next event time   */
    for(int j=0; j<2; j++){
      if (biglietteria[j].number > 0)  {         /* update integrals  */
        biglietteria[j].node    += (t.next - t.current) * biglietteria[j].number;
        biglietteria[j].queue   += (t.next - t.current) * (biglietteria[j].number - 1);
        biglietteria[j].service += (t.next - t.current);
      }
    }
    if (controlloBiglietti.number > 0)  {        /* update integrals  */
        controlloBiglietti.node    += (t.next - t.current) * controlloBiglietti.number;
        controlloBiglietti.queue   += (t.next - t.current) * (controlloBiglietti.number - 1);
        controlloBiglietti.service += (t.next - t.current);
    }
    if (cassaFoodArea.number > 0)  {        /* update integrals  */
        cassaFoodArea.node    += (t.next - t.current) * cassaFoodArea.number;
        cassaFoodArea.queue   += (t.next - t.current) * (cassaFoodArea.number - 1);
        cassaFoodArea.service += (t.next - t.current);
    }
    if (foodArea.number > 0)  {        /* update integrals  */
        foodArea.node    += (t.next - t.current) * foodArea.number;
        foodArea.queue   += (t.next - t.current) * (foodArea.number - 1);
        foodArea.service += (t.next - t.current);
    }
    t.current       = t.next;                    /* advance the clock */

    int x = e;
    switch(x){
      case 0:                         /* process an arrival to biglietteria */
        arrivalsBiglietteria++;

        i = routingBiglietteria();

        biglietteria[i].number++;
        event[0].t = GetArrival();    /* generate the next arrival event */
      
        if (event[0].t > STOP)  {
          t.last      = t.current;
          event[0].x  = 0;
        }
        if (biglietteria[i].number == 1){
          serviceData serviceData;
          if(i == 0){
            serviceData = biglietteriaService0;
          }else{
            serviceData = biglietteriaService1;
          }
          event[i+1].t = t.current + GetService(&serviceData);     /* event i+1 because completions: biglietteria[0]-event[1], biglietteria[1]-event[2] */
          event[i+1].x = 1;
        }
        break;
      
      case 1:                            /* process a departure from biglietteria 0*/
        departuresBiglietteria0++;
      
        biglietteria[0].index++;
        biglietteria[0].number--;

        /* arrival from 0 to controlloBiglietti queue*/
        event[3].t = event[e].t;
        event[3].x = 1;
        if (biglietteria[0].number > 0){
          event[e].t = t.current + GetService(&biglietteriaService0);
          event[e].x = 1;
        }else{
          event[e].x = 0;
        }
        break;

      case 2:                           /* process a departure from biglietteria 1*/
        departuresBiglietteria1++;
        biglietteria[1].index++;
        biglietteria[1].number--;

        
        /* arrivals from 1 to controlloBiglietti queue*/
        event[4].t = event[e].t;
        event[4].x = 1;
        
        if (biglietteria[1].number > 0){
          event[e].t = t.current + GetService(&biglietteriaService1);
          event[e].x = 1;
        }else{
          event[e].x = 0;
        }

        break;
      
      case 3:                            /* process an arrival to controllo biglietti */
        arrivalsControllo++;
        controlloBiglietti.number++;

        event[3].x = 0;                  /* se ci sono arrivi entro i server disponibili, continuo a riempirli */
        if(controlloBiglietti.number <= SERVERS_CONTROLLO_BIGLIETTI){

          double service = GetService(&controlloBigliettiService);
          /* vado a cercare l'indice di quale servente è libero */
          int idleServer_controlloBiglietti = FindIdleServer(&controlloBiglietti_MS, SERVERS_CONTROLLO_BIGLIETTI);
          
          /* segnalo che è occupied */
          controlloBiglietti_MS[idleServer_controlloBiglietti].occupied = 1;
          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          controlloBiglietti_MS[idleServer_controlloBiglietti].service += service;
          /* incremento il numero di job serviti */
          controlloBiglietti_MS[idleServer_controlloBiglietti].served++;

          departuresControlloBiglietti[idleServer_controlloBiglietti].x = 1;
          departuresControlloBiglietti[idleServer_controlloBiglietti].t = t.current+service;

          /*segnalo disponibilità dell'evento di completion da controllo biglietti*/
          event[5].x = 1;
          double current_cb1 = INFINITY;
          for(int k=0; k<SERVERS_CONTROLLO_BIGLIETTI+1; k++){
            if(departuresControlloBiglietti[k].x == 1 && departuresControlloBiglietti[k].t<current_cb1){
                current_cb1 = departuresControlloBiglietti[k].t;
            }
          }
          event[5].t = current_cb1;
        }
        break;

      case 4:                            /* process an arrival to controllo biglietti */
        arrivalsControllo++;
        controlloBiglietti.number++;
        event[4].x = 0;
        if(controlloBiglietti.number <= SERVERS_CONTROLLO_BIGLIETTI){

          double service = GetService(&controlloBigliettiService);
          /* vado a cercare l'indice di quale servente è libero */
          int idleServer_controlloBiglietti = FindIdleServer(&controlloBiglietti_MS, SERVERS_CONTROLLO_BIGLIETTI);
          
          /* segnalo che è occupied */
          controlloBiglietti_MS[idleServer_controlloBiglietti].occupied = 1;
          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          controlloBiglietti_MS[idleServer_controlloBiglietti].service += service;
          /* incremento il numero di job serviti */
          controlloBiglietti_MS[idleServer_controlloBiglietti].served++;

          departuresControlloBiglietti[idleServer_controlloBiglietti].x = 1;
          departuresControlloBiglietti[idleServer_controlloBiglietti].t = t.current+service;

          /*segnalo disponibilità dell'evento di completion da controllo biglietti*/
          event[5].x = 1;
          double current_cb2 = INFINITY;
          for(int k=0; k<SERVERS_CONTROLLO_BIGLIETTI+1; k++){
            if(departuresControlloBiglietti[k].x == 1 && departuresControlloBiglietti[k].t<current_cb2){
                current_cb2 = departuresControlloBiglietti[k].t;
            }
          }
          event[5].t = current_cb2;
        }
        break;

      case 5:                            /* process a departure from controllo biglietti */
        departuresControllo++;
        controlloBiglietti.index++;
        controlloBiglietti.number--;

        int e_cb = 0;
          
        /* recupero l'indiced del server */
        for(int k=0; k<SERVERS_CONTROLLO_BIGLIETTI+1; k++){
            if(departuresControlloBiglietti[k].t == event[5].t){
                e_cb = k;
            }
        }
        
        /* si è appena liberato un server, ma ho almeno qualcuno in coda*/
        if(controlloBiglietti.number >= SERVERS_CONTROLLO_BIGLIETTI){
          
          double service = GetService(&controlloBigliettiService);
          /* segnalo che è occupied */
          /* anche se credo sia superfluo: controlloBiglietti_MS[e_cb].occupied = 1; */
          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          controlloBiglietti_MS[e_cb].service += service;
          /* incremento il numero di job serviti */
          controlloBiglietti_MS[e_cb].served++;
          /* credo anche questo sia superfluo: departuresControlloBiglietti[e_cb].x = 1; */
          departuresControlloBiglietti[e_cb].t = t.current+service;


        }else{
          /* server idle */
          controlloBiglietti_MS[e_cb].occupied = 0;
          /* no departure event available */
          departuresControlloBiglietti[e_cb].x = 0;
          departuresControlloBiglietti[e_cb].t = INFINITY;

          double current_cb3 = INFINITY;
          for(int k=0; k<SERVERS_CONTROLLO_BIGLIETTI+1; k++){
            if(departuresControlloBiglietti[k].x == 1 && departuresControlloBiglietti[k].t<current_cb3){
                current_cb3 = departuresControlloBiglietti[k].t;
            }
          }
          event[5].t = current_cb3;

          if(controlloBiglietti.number==0){
            event[5].x = 0;
            event[5].t = INFINITY;  /* per sicurezza ;) */
          }
        }

        /* from controlloBiglietti to cassaFoodArea */
        choice choice = routingControlloBiglietti();
        switch(choice){
          case FOODAREA:
            #ifdef DEBUG
              printf("++++++++++++++++++++++++++++++++++++ FOODAREA\n");
              food++;
            #endif
            event[6].t = event[e].t;
            event[6].x = 1;
            break;
          case GADGETSAREA:
            #ifdef DEBUG
              printf("++++++++++++++++++++++++++++++++++++ GADGETSAREA\n");
              gadgets++;
            #endif
            event[8].t = event[e].t;
            event[8].x = 1;
            break;
          case NONE:
            #ifdef DEBUG
              printf("++++++++++++++++++++++++++++++++++++ NONE\n");
            #endif
            none++;
            break;
          default:
            printf("++++++++++++++++++++++++++++++++++++ default\n");
            defaults++;
            break;
        }
        break;

      case 6:             /* process an arrival to cassaFoodArea */
        #ifdef DEBUG
          printf("arrival to cassaFoodArea\n");
        #endif
        cassaFoodArea.number++;
        event[6].x = 0;
        if(cassaFoodArea.number == 1){
          event[7].t = t.current + GetService(&cassaFoodAreaService);
          event[7].x = 1;
        }
        break;

      case 7:             /* process a departure from cassaFoodArea */
        #ifdef DEBUG
          printf("departure from cassaFoodArea\n");
        #endif
        cassaFoodArea.index++;
        cassaFoodArea.number--;
        if (cassaFoodArea.number > 0){
          event[e].t = t.current + GetService(&cassaFoodAreaService);
          event[e].x = 1;
        }else{
          event[e].x = 0;
        }
        break;

      case 8:             /* process an arrival to areaFood */
        #ifdef DEBUG
          printf("arrival to foodArea\n");
        #endif
        foodArea.number++;
        event[8].x = 0;
        if(foodArea.number == 1){
          event[9].t = t.current + GetService(&foodAreaService);
          event[9].x = 1;
        }
        break;

      case 9:             /* process a departure from areaFood */
        #ifdef DEBUG
          printf("departure from foodArea\n");
        #endif
        foodArea.index++;
        foodArea.number--;
        if (foodArea.number > 0){
          event[e].t = t.current + GetService(&foodAreaService);
          event[e].x = 1;
        }else{
          event[e].x = 0;
        }
        break;

      default:
        printf("Default: %d\n", x);
        return;
    }
  }

  visualizeStatistics(&biglietteria[0]);
  visualizeStatistics(&biglietteria[1]);
  visualizeStatistics(&controlloBiglietti);
  visualizeStatistics(&cassaFoodArea);
  visualizeStatistics(&foodArea);
  visualizeStatistics(&gadgetsArea);


  double tot = food + gadgets + none + defaults;
  printf("\nfood : %f, gadgets : %f, none : %f, defaults : %f",food/tot,gadgets/tot,none/tot,defaults/tot);
  return (0);
}
