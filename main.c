/* ------------------------------------------------------------------------- 
  Created by Appetito Giulio, Brinati Anastasia
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "rngs.h"                      /* the multi-stream generator */
#include "rvgs.h"                      /* random variate generators  */

//#define DEBUG 
//#define DEBUG2      //areaFood

#define START         0.0              /* initial time                   */
#define STOP          60.0           /* terminal (close the door) time */
#define INFINITY      (100.0 * STOP)   /* must be much larger than STOP  */
#define SEED          123456789

#define SERVERS_BIGLIETTERIA         1   /* number of servers for each center */
#define SERVERS_CONTROLLO_BIGLIETTI  2
#define SERVERS_CASSA_FOOD_AREA      1
#define SERVERS_FOOD_AREA            3
#define SERVERS_GADGETS_AREA         2

#define LAMBDA_1 7.166
#define LAMBDA_2 6.833
#define LAMBDA_3 5.166

#define NUMBER_OF_EVENTS 12             

double MU_BIGLIETTERIA       = 3.0;
double MU_CONTROLLOBIGLIETTI = 5.0;      /* da modificare */
double MU_CASSA_FOOD_AREA    = 6.0;
double MU_FOOD_AREA          = 5.0;
double MU_GADGETS_AREA       = 3.0;

double lambda;


#define STREAM_ARRIVALS            0
#define STREAM_BIGLIETTERIA0       1
#define STREAM_BIGLIETTERIA1       2
#define STREAM_CONTROLLOBIGLIETTI  3
#define STREAM_CASSA_FOOD_AREA     4
#define STREAM_FOOD_AREA           5
#define STREAM_GADGETS_AREA        6
#define STREAM_ROUTING_TICKETMODE  7
#define STREAM_ROUTING_CONTROLLO   8
#define STREAM_ROUTING_GADGETS     9

#define P_GADGETS_AREA_1       0.1376        /*routing probabilities for each fasciaOraria */
#define P_FOOD_AREA_1          0.8441
#define P_GADGETS_AFTER_FOOD_1 0.3000
#define P_GADGETS_AREA_2       0.1136
#define P_FOOD_AREA_2          0.8292
#define P_GADGETS_AFTER_FOOD_2 0.2000
#define P_GADGETS_AREA_3       0.0890
#define P_FOOD_AREA_3          0.7096
#define P_GADGETS_AFTER_FOOD_3 0.100
#define P_ONLINE               0.2000

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

double p_foodArea;
double p_gadgetsArea;
double p_gadgetsAfterFood;
double p_online;
int fasciaOraria = 0;
int routingControlloTimes = 0;

#ifdef DEBUG
  int case6Iterations = 0;
#endif

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
  #ifdef DEBUG
    printf("[ ");
  #endif
  for(int i=0; i<NUMBER_OF_EVENTS; i++){
    if(events[i].x == 1 ){             /* check if the event is active */
      #ifdef DEBUG
        printf("%d: %f |", i, events[i].t);
      #endif
      if(events[i].t <= current){     /* check if it came sooner then the current min */
        current = events[i].t;
        e = i;
      }
    }
  }
  #ifdef DEBUG
    printf(" ]\n");
    printf("\nDentro next event, evento: %d tempo: %f\n", e, current);
  #endif
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

void printRed(char *string){
  printf("\033[22;31m%s\n\033[0m",string);
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

int routingAfterBiglietteria(){
  int i = 0;
  SelectStream(STREAM_ROUTING_CONTROLLO);
  double p = Uniform(0, 1);    /* routing probability p */
  if(p < 0.5){
    i=0;
  }else{
    i=1;
  }
  return i;
}

choice routingAfterFoodArea(){
  SelectStream(STREAM_ROUTING_GADGETS);
  double p = Uniform(0, 1);
  if (p < p_gadgetsAfterFood){
    return GADGETSAREA;
  }
  return NONE;
}

choice routingAfterControlloBiglietti(){
  SelectStream(STREAM_ROUTING_CONTROLLO);
  double p = Uniform(0, 1);    /* routing probability p */
  routingControlloTimes++;
  if(p < p_foodArea){
    return FOODAREA;
  }else if ( p < (p_foodArea + p_gadgetsArea)){
    return GADGETSAREA;
  }else{
    return NONE;
  }
}

void initServiceData(serviceData *center, double mu, int stream){
  center->mean = mu;
  center->stream = stream;
}

ticketMode routingBeforeBiglietteria(){
  double p = Uniform(0, 1);
  if( p < p_online){
    return ONLINE;
  }
  return PHYSICAL;
}

void updateIntegrals(center *center, multiserver multiserver[]){
  if (center->number > 0)  {        
        /* update integrals  */
        center->node    += (t.next - t.current) * center->number;
        if(center->number > center->servers){
          center->queue   += (t.next - t.current) * (center->number - center->servers);
        }
        if(center->servers == 1){
          center->service += (t.next - t.current);
        }else{
          for(int i=0; i<center->servers; i++){
            center->service += multiserver[i].service;
          }
        }
    }
}

void visualizeStatistics(center *center){
    printf("\nSTATISTICHE\033[22;32m %s\033[0m per %.0f jobs [Fascia oraria %d] :\n\n", center->name, center->index, fasciaOraria);
    printf("   average interarrival time = %6.3f\n", t.last / center->index);
    printf("   average wait ............ = %6.3f\n", center->node / center->index);
    printf("   average delay ........... = %6.3f\n", center->queue / center->index);
    printf("   average service time .... = %6.3f\n", center->service / center->index);
    printf("   average # in the node ... = %6.3f\n", center->node / t.current);
    printf("   average # in the queue .. = %6.3f\n", center->queue / t.current);
    printf("   utilization ............. = %6.3f\n", center->service / t.current);
}

void visualizeStatisticsMultiservers(center *center, multiserver multiserver[], int numberOfServers){
    printf("\nSTATISTICHE\033[22;32m %s\033[0m per %.0f jobs [Fascia oraria %d] :\n\n", center->name, center->index, fasciaOraria);
    printf("   average interarrival time ...... = %6.3f\n\n", t.last / center->index);
    printf("   average wait ................... = %6.3f\n", center->node / center->index);
    printf("   average delay .................. = %6.3f\n", center->queue / center->index);
    for(int i=0; i<numberOfServers; i++){
      printf("   [server %d] average service time = %6.3f\n", i, multiserver[i].service / multiserver[i].served);
      printf("   [server %d] service time ....... = %6.3f\n", i, multiserver[i].service);
      printf("   [server %d] utilization ........ = %6.3f\n\n", i, multiserver[i].service / t.current);
    }
  /*
    printf("   average wait ............ = %6.2f\n", center->node / center->index);
    printf("   average delay ........... = %6.2f\n", center->queue / center->index);
*/
/*
    //printf("   average # in the node ... = %6.2f\n", center->node / t.current);    
    printf("   average # in the queue .. = %6.2f\n", center->queue / t.current);

    printf("   utilization ............. = %6.2f\n", ? );
*/
}

void visualizeRunParameters(double tot, double lambda, double p_foodArea, double p_gadgetsArea, double p_gadgetsAfterFood){
    char a[5];
    strcpy(a, "%");
    printf("This is a's value: %s\n", a);
    printf("\n\033[22;32mNumero totale di arrivi al sistema = \033[0m%.0f\n",tot);
    printf("\033[22;32mLambda ........................... = \033[0m%.3f job/min\n",lambda);
    printf("\033[22;32mp_foodArea ....................... = \033[0m%.2f %s\n",p_foodArea*100,a);
    printf("\033[22;32mp_gadgetsArea .................... = \033[0m%.2f %s\n",p_gadgetsArea*100,a);
    printf("\033[22;32mp_gadgetsAfterFood ............... = \033[0m%.2f %s\n",p_gadgetsAfterFood*100,a);
  }

#ifdef DEBUG2
  void printFoodStats(center  *foodArea){
    printf("\033[22;31m\nAttributi foodArea ................. t.current = %f\n\n\033[0m",t.current);
    printf("   foodArea->index = %6.2f\n", foodArea->index);
    printf("   foodArea->node ............ = %6.2f\n", foodArea->node );
    printf("   foodArea->queue ........... = %6.2f\n", foodArea->queue);
    printf("   foodArea->service .... = %6.2f\n", foodArea->service);   
  }
#endif


int main(void){
  
  system("cls");
  printf("\033[22;32m _______________________________ \n\033[0m");
  printf("\033[22;32m|                             | |\n\033[0m");
  printf("\033[22;32m|    GESTIONE DI UN CINEMA    | |\n\033[0m");
  printf("\033[22;32m|        Progetto PMCSN       | |\n\033[0m");
  printf("\033[22;32m|                             | |\n\033[0m");
  printf("\033[22;32m|      Brinati Anastasia      | |\n\033[0m");
  printf("\033[22;32m|       Appetito Giulio       | |\n\033[0m");
  printf("\033[22;32m|_____________________________| |\n\033[0m");
  printf("\033[22;32m|_______________________________|\n\033[0m");

  selection:
  printf("\nSelezionare fascia oraria da simulare (STOP = %.2f):\n 1 -> Fascia 1 [15:00-16:00]\n 2 -> Fascia 2 [19:00-20:00]\n 3 -> Fascia 3 [22:00-23:00]\nSelezione [1 / 2 / 3] : ",STOP);
  scanf("%d",&fasciaOraria);
  switch(fasciaOraria){
    case 1:
      lambda = LAMBDA_1;
      p_foodArea = P_FOOD_AREA_1;
      p_gadgetsArea = P_GADGETS_AREA_1;
      p_gadgetsAfterFood = P_GADGETS_AFTER_FOOD_1;
      break;
    case 2:
      lambda = LAMBDA_2;
      p_foodArea = P_FOOD_AREA_2;
      p_gadgetsArea = P_GADGETS_AREA_2;
      p_gadgetsAfterFood = P_GADGETS_AFTER_FOOD_2;
      break;
    case 3:
      lambda = LAMBDA_3;
      p_foodArea = P_FOOD_AREA_3;
      p_gadgetsArea = P_GADGETS_AREA_3;
      p_gadgetsAfterFood = P_GADGETS_AFTER_FOOD_3;
      break;
    default:
      printRed("Selezione non valida.");
      goto selection;
      break;

  }
  p_online = P_ONLINE;

  double food=0;
  double gadgets=0;
  double none=0;
  double defaults=0;

  multiserver controlloBiglietti_MS[SERVERS_CONTROLLO_BIGLIETTI];
  for(int i=0; i<SERVERS_CONTROLLO_BIGLIETTI; i++){
    controlloBiglietti_MS[i].served = 0;
    controlloBiglietti_MS[i].occupied = 0;
    controlloBiglietti_MS[i].service = 0.0;
  }
  multiserver areaFood_MS[SERVERS_FOOD_AREA];
  for(int i=0; i<SERVERS_FOOD_AREA; i++){
    areaFood_MS[i].served = 0;
    areaFood_MS[i].occupied = 0;
    areaFood_MS[i].service = 0.0;
  }
  multiserver areaGadgets_MS[SERVERS_GADGETS_AREA];
  for(int i=0; i<SERVERS_GADGETS_AREA; i++){
    areaGadgets_MS[i].served = 0;
    areaGadgets_MS[i].occupied = 0;
    areaGadgets_MS[i].service = 0.0;
  } 
  center biglietteria[2];
  center controlloBiglietti;
  center cassaFoodArea;
  center foodArea;
  center gadgetsArea;
  center cinema;
  initCenterStats(&biglietteria[0], SERVERS_BIGLIETTERIA, "biglietteria_0");
  initCenterStats(&biglietteria[1], SERVERS_BIGLIETTERIA, "biglietteria_1");
  initCenterStats(&controlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI, "controlloBiglietti");
  initCenterStats(&cassaFoodArea, SERVERS_CASSA_FOOD_AREA,"cassaFoodArea");
  initCenterStats(&foodArea, SERVERS_FOOD_AREA,"foodArea");
  initCenterStats(&gadgetsArea, SERVERS_GADGETS_AREA,"gadgetsArea");
  initCenterStats(&cinema, SERVERS_BIGLIETTERIA*2 + SERVERS_CONTROLLO_BIGLIETTI + SERVERS_CASSA_FOOD_AREA + SERVERS_FOOD_AREA + SERVERS_GADGETS_AREA,"cinema");

  serviceData biglietteriaService0;
  serviceData biglietteriaService1;
  serviceData controlloBigliettiService;
  serviceData cassaFoodAreaService;
  serviceData foodAreaService;
  serviceData gadgetsAreaService;
  initServiceData(&biglietteriaService0, MU_BIGLIETTERIA, STREAM_BIGLIETTERIA0);
  initServiceData(&biglietteriaService1, MU_BIGLIETTERIA, STREAM_BIGLIETTERIA1);
  initServiceData(&controlloBigliettiService, MU_CONTROLLOBIGLIETTI, STREAM_CONTROLLOBIGLIETTI);
  initServiceData(&cassaFoodAreaService, MU_CASSA_FOOD_AREA, STREAM_CASSA_FOOD_AREA);
  initServiceData(&foodAreaService, MU_FOOD_AREA, STREAM_FOOD_AREA);
  initServiceData(&gadgetsAreaService, MU_GADGETS_AREA, STREAM_GADGETS_AREA);

  event departuresControlloBiglietti[SERVERS_CONTROLLO_BIGLIETTI];
   for(int i=0; i<SERVERS_CONTROLLO_BIGLIETTI; i++){
    departuresControlloBiglietti[i].x = 0;
    departuresControlloBiglietti[i].t = 0.0;
  }
  event departuresAreaFood[SERVERS_FOOD_AREA];
   for(int i=0; i<SERVERS_FOOD_AREA; i++){
    departuresAreaFood[i].x = 0;
    departuresAreaFood[i].t = 0.0;
  }
  event departuresGadgetsArea[SERVERS_GADGETS_AREA];
   for(int i=0; i<SERVERS_GADGETS_AREA; i++){
    departuresGadgetsArea[i].x = 0;
    departuresGadgetsArea[i].t = 0.0;
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

  int counter = 0;

  while ((event[0].t < STOP) || ((biglietteria[0].number + biglietteria[1].number + controlloBiglietti.number + cassaFoodArea.number + foodArea.number + gadgetsArea.number) > 0)){
    counter++;

    e = NextEvent(event);                        /* next event index  */
    t.next = event[e].t;                         /* next event time   */
    for(int j=0; j<2; j++){
      updateIntegrals(&biglietteria[j], NULL);
    }

    #ifdef DEBUG2
      printFoodStats(&foodArea);
    #endif

    updateIntegrals(&controlloBiglietti,controlloBiglietti_MS);
    updateIntegrals(&cassaFoodArea, NULL);
    updateIntegrals(&foodArea, areaFood_MS);
    updateIntegrals(&gadgetsArea, areaGadgets_MS);
    updateIntegrals(&cinema, NULL);

    t.current       = t.next;                    /* advance the clock */
    
    int x = e;
    if(x == 0){
      cinema.index++;
      if(routingBeforeBiglietteria() == ONLINE){
      #ifdef DEBUG
        printf("\033[22;35mArrival with online ticket\n\033[0m");
      #endif
      event[3].t = event[e].t;
      event[3].x = 1;
      x = 3;
      }else{
        #ifdef DEBUG
          printf("\033[22;34mArrival with physical ticket\n\033[0m");
        #endif
      }
    }
    switch(x){
      case 0:                         /* process an arrival to biglietteria */

        arrivalsBiglietteria++;
        i = routingAfterBiglietteria();
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

        event[3].x = 0;                  
        /* se ci sono arrivi entro i server disponibili, continuo a riempirli */
        if(controlloBiglietti.number <= SERVERS_CONTROLLO_BIGLIETTI){

          double service = GetService(&controlloBigliettiService);
          /* vado a cercare l'indice di quale servente è libero */
          int idleServer_controlloBiglietti = FindIdleServer(controlloBiglietti_MS, SERVERS_CONTROLLO_BIGLIETTI);
          
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
          double current_cb3 = INFINITY;
          for(int k=0; k<SERVERS_CONTROLLO_BIGLIETTI+1; k++){
            if(departuresControlloBiglietti[k].x == 1 && departuresControlloBiglietti[k].t<current_cb3){
                current_cb3 = departuresControlloBiglietti[k].t;
            }
          }
          event[5].t = current_cb3;
        }
        break;

      case 4:                            /* process an arrival to controllo biglietti */
        arrivalsControllo++;
        controlloBiglietti.number++;
        
        event[4].x = 0;
        if(controlloBiglietti.number <= SERVERS_CONTROLLO_BIGLIETTI){

          double service = GetService(&controlloBigliettiService);
          /* vado a cercare l'indice di quale servente è libero */
          int idleServer_controlloBiglietti = FindIdleServer(controlloBiglietti_MS, SERVERS_CONTROLLO_BIGLIETTI);
          
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
          double current_cb4 = INFINITY;
          for(int k=0; k<SERVERS_CONTROLLO_BIGLIETTI+1; k++){
            if(departuresControlloBiglietti[k].x == 1 && departuresControlloBiglietti[k].t<current_cb4){
                current_cb4 = departuresControlloBiglietti[k].t;
            }
          }
          event[5].t = current_cb4;
        }
        break;

      case 5:                            /* process a departure from controllo biglietti */
        departuresControllo++;
        controlloBiglietti.index++;
        controlloBiglietti.number--;

        int e_cb = 0;
        double tempo = event[5].t;
        /* recupero l'indiced del server */
        for(int k=0; k<SERVERS_CONTROLLO_BIGLIETTI+1; k++){
            if(departuresControlloBiglietti[k].t == tempo){
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

          double current_cb5 = INFINITY;
          for(int k=0; k<SERVERS_CONTROLLO_BIGLIETTI+1; k++){
            if(departuresControlloBiglietti[k].x == 1 && departuresControlloBiglietti[k].t<current_cb5){
                current_cb5 = departuresControlloBiglietti[k].t;
            }
          }
          event[5].t = current_cb5;

          if(controlloBiglietti.number==0){
            event[5].x = 0;
            event[5].t = INFINITY;  /* per sicurezza ;) */
          }
        }

        /* from controlloBiglietti to cassaFoodArea */
        choice choice = routingAfterControlloBiglietti();
        switch(choice){
          case FOODAREA:
            #ifdef DEBUG
              printf("routing to FOODAREA  ");
              printf("at time: %f\n", tempo);
            #endif
            food++;
            event[6].t = tempo;
            event[6].x = 1;
            break;
          case GADGETSAREA:
            #ifdef DEBUG
              printf("routing to GADGETSAREA\n");
            #endif
            gadgets++;
            event[10].t = tempo;
            event[10].x = 1;
            break;
          case NONE:
            #ifdef DEBUG
              printf("routing to NONE\n");
            #endif
            none++;
            cinema.number--;
            break;
          default:
            printf("default\n");
            defaults++;
            break;
        }
        break;

      case 6:             /* process an arrival to cassaFoodArea */
        #ifdef DEBUG
          printf("arrival to cassaFoodArea\n");
          case6Iterations++;
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
        tempo = event[7].t;
        cassaFoodArea.index++;
        cassaFoodArea.number--;
        if (cassaFoodArea.number > 0){
          event[7].t = t.current + GetService(&cassaFoodAreaService);
          event[7].x = 1;
        }else{
          event[7].x = 0;
        }
        event[8].t = tempo;
        event[8].x = 1;

        break;

      case 8:             /* process an arrival to areaFood */
        #ifdef DEBUG
          printf("arrival to foodArea\n");
        #endif
        foodArea.number++;

        event[8].x = 0;                  /* se ci sono arrivi entro i server disponibili, continuo a riempirli */
        if(foodArea.number <= SERVERS_FOOD_AREA){

          double service = GetService(&foodAreaService);
          /* vado a cercare l'indice di quale servente è libero */
          int idleServer_foodArea = FindIdleServer(areaFood_MS, SERVERS_FOOD_AREA);
          
          /* segnalo che è occupied */
          areaFood_MS[idleServer_foodArea].occupied = 1;
          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          areaFood_MS[idleServer_foodArea].service += service;
          /* incremento il numero di job serviti */
          areaFood_MS[idleServer_foodArea].served++;

          departuresAreaFood[idleServer_foodArea].x = 1;
          departuresAreaFood[idleServer_foodArea].t = t.current+service;

          /*segnalo disponibilità dell'evento di completion da controllo biglietti*/
          event[9].x = 1;
          double current_cb8 = INFINITY;
          for(int k=0; k<SERVERS_FOOD_AREA+1; k++){
            if(departuresAreaFood[k].x == 1 && departuresAreaFood[k].t<current_cb8){
                current_cb8 = departuresAreaFood[k].t;
                #ifdef DEBUG
                printf("\033[22;31m ***************** next departures at time: %f\n\n\033[0m", current_cb8);
                #endif
            }
          }
          event[9].t = current_cb8;
          #ifdef DEBUG
            printf("\033[22;31mpreparing event 9 at time : %f\n\n\033[0m", current_cb8);
          #endif
        }
        break;

      case 9:             /* process a departure from areaFood */
        #ifdef DEBUG
          printf("departure from foodArea\n");
        #endif
        foodArea.index++;
        foodArea.number--;

        int e_cb_9 = 0;
        tempo = event[9].t;
        /* recupero l'indiced del server */
        for(int k=0; k<SERVERS_FOOD_AREA+1; k++){
            if(departuresAreaFood[k].t == tempo){
                e_cb_9 = k;
            }
        }
        
        /* si è appena liberato un server, ma ho almeno qualcuno in coda*/
        if(foodArea.number >= SERVERS_FOOD_AREA){
          
          double service = GetService(&foodAreaService);
          /* segnalo che è occupied */
          /* anche se credo sia superfluo: areaFood_MS[e_cb_9].occupied = 1; */
          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          areaFood_MS[e_cb_9].service += service;
          /* incremento il numero di job serviti */
          areaFood_MS[e_cb_9].served++;
          /* credo anche questo sia superfluo: departuresAreaFood[e_cb_9].x = 1; */
          departuresAreaFood[e_cb_9].t = t.current+service;


        }else{
          /* server idle */
          areaFood_MS[e_cb_9].occupied = 0;
          /* no departure event available */
          departuresAreaFood[e_cb_9].x = 0;
          departuresAreaFood[e_cb_9].t = INFINITY;

          double current_cb9 = INFINITY;
          for(int k=0; k<SERVERS_FOOD_AREA+1; k++){
            if(departuresAreaFood[k].x == 1 && departuresAreaFood[k].t<current_cb9){
                current_cb9 = departuresAreaFood[k].t;
            }
          }
          
          event[9].t = current_cb9;

          if(foodArea.number==0){
            event[9].x = 0;
            event[9].t = INFINITY;  /* per sicurezza ;) */
          }
        }


        switch(routingAfterFoodArea()){
          case GADGETSAREA:
            #ifdef DEBUG
              printf("routing to gadget area after food\n");
            #endif
            event[10].t = tempo;
            event[10].x = 1;
            break;
          case NONE:
            #ifdef DEBUG
              printf("time after none: %f\n", tempo);
            #endif
            cinema.number--;
            break;
          default:
            #ifdef DEBUG
              printf("default after food\n");
            #endif
            break;
        }
        break;

      case 10:             /* process an arrival to gadgetsArea */
        #ifdef DEBUG
          printf("arrival to gadgetsArea\n");
        #endif
        gadgetsArea.number++;

        event[10].x = 0;                  /* se ci sono arrivi entro i server disponibili, continuo a riempirli */
        if(gadgetsArea.number <= SERVERS_GADGETS_AREA){

          double service = GetService(&gadgetsAreaService);
          /* vado a cercare l'indice di quale servente è libero */
          int idleServer_gadgetsArea = FindIdleServer(areaGadgets_MS, SERVERS_GADGETS_AREA);
          
          /* segnalo che è occupied */
          areaGadgets_MS[idleServer_gadgetsArea].occupied = 1;
          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          areaGadgets_MS[idleServer_gadgetsArea].service += service;
          /* incremento il numero di job serviti */
          areaGadgets_MS[idleServer_gadgetsArea].served++;

          departuresGadgetsArea[idleServer_gadgetsArea].x = 1;
          departuresGadgetsArea[idleServer_gadgetsArea].t = t.current+service;

          /*segnalo disponibilità dell'evento di completion da area gadgets*/
          event[11].x = 1;
          double current_cb10 = INFINITY;
          for(int k=0; k<SERVERS_GADGETS_AREA+1; k++){
            if(departuresGadgetsArea[k].x == 1 && departuresGadgetsArea[k].t<current_cb10){
                current_cb10 = departuresGadgetsArea[k].t;
            }
          }
          event[11].t = current_cb10;
        }
        break;

      case 11:             /* process a departure from gadgetsArea */
        #ifdef DEBUG
          printf("departure from gadgetsArea\n");
        #endif
        gadgetsArea.index++;
        gadgetsArea.number--;
        cinema.number--;

        int e_cb_11 = 0;
        tempo = event[11].t;
        /* recupero l'indiced del server */
        for(int k=0; k<SERVERS_GADGETS_AREA+1; k++){
            if(departuresGadgetsArea[k].t == tempo){
                e_cb_11 = k;
            }
        }
        
        /* si è appena liberato un server, ma ho almeno qualcuno in coda*/
        if(gadgetsArea.number >= SERVERS_GADGETS_AREA){
          
          double service = GetService(&gadgetsAreaService);
          /* segnalo che è occupied */
          /* anche se credo sia superfluo: areaFood_MS[e_cb_11].occupied = 1; */
          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          areaGadgets_MS[e_cb_11].service += service;
          /* incremento il numero di job serviti */
          areaGadgets_MS[e_cb_11].served++;
          /* credo anche questo sia superfluo: departuresGadgetsArea[e_cb_11].x = 1; */
          departuresGadgetsArea[e_cb_11].t = t.current+service;


        }else{
          /* server idle */
          areaGadgets_MS[e_cb_11].occupied = 0;
          /* no departure event available */
          departuresGadgetsArea[e_cb_11].x = 0;
          departuresGadgetsArea[e_cb_11].t = INFINITY;

          double current_cb11 = INFINITY;
          for(int k=0; k<SERVERS_GADGETS_AREA+1; k++){
            if(departuresGadgetsArea[k].x == 1 && departuresGadgetsArea[k].t<current_cb11){
                current_cb11 = departuresGadgetsArea[k].t;
            }
          }
          event[11].t = current_cb11;

          if(gadgetsArea.number==0){
            event[11].x = 0;
            event[11].t = INFINITY;  /* per sicurezza ;) */
          }
        }

      
        break;

      default:
        #ifdef DEBUG
          printf("Default e: %d\n", x);
          printf("t: %f\n", event[x].t);
          return;
        #endif
        printf("Default: %d\n", x);
        break;
    }
  }
  
  visualizeRunParameters(cinema.index, lambda, p_foodArea, p_gadgetsArea, p_gadgetsAfterFood);

  visualizeStatistics(&biglietteria[0]);
  visualizeStatistics(&biglietteria[1]);
  visualizeStatisticsMultiservers(&controlloBiglietti, controlloBiglietti_MS, SERVERS_CONTROLLO_BIGLIETTI);
  visualizeStatistics(&cassaFoodArea);
  visualizeStatisticsMultiservers(&foodArea, areaFood_MS, SERVERS_FOOD_AREA);
  visualizeStatisticsMultiservers(&gadgetsArea, areaGadgets_MS, SERVERS_GADGETS_AREA);

  return (0);
}
