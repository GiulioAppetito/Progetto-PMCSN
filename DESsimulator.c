/* ------------------------------------------------------------------------- 
  Created by Appetito Giulio, Brinati Anastasia
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "rngs.h"                      /* the multi-stream generator */
#include "rvgs.h"                      /* random variate generators  */
#include "datastructures.h"
#include "ourlib.h"

//#define DEBUG 
//#define DEBUG2

#define START         0.0              /* initial time                   */
#define STOP          60.0             /* terminal (close the door) time */
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
double MU_CONTROLLOBIGLIETTI = 4.0;      /* da modificare */
double MU_CASSA_FOOD_AREA    = 7.0;
double MU_FOOD_AREA          = 3.0;
double MU_GADGETS_AREA       = 2.0;

double lambda;
center cinema;

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
#define P_ONLINE               0.2

double arrival = START;
double p_foodArea;
double p_gadgetsArea;
double p_gadgetsAfterFood;
double p_online;
int fasciaOraria = 0;

double GetArrival()
/* ---------------------------------------------
 * generate the next arrival time
 * ---------------------------------------------
 */ 
{
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
  int e = 0;                    /* event index */
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
  if(p < p_foodArea){
    return FOODAREA;
  }else if ( p < (p_foodArea + p_gadgetsArea)){
    return GADGETSAREA;
  }else{
    return NONE;
  }
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
        if(t.next != t.current){
          if(center->number > center->servers){
            center->queue   += (t.next - t.current) * (center->number - center->servers);
          }
          if(center->servers == 1){
            center->service += (t.next - t.current);
          }else{
            for(int j=0; j< (center->servers); j++){
              if(multiserver[j].occupied==1){
                center->service += (t.next - t.current);
              }
            }
          }
        }              
  }
}

void visualizeStatistics(center *center){
    printf("\nSTATISTICHE\033[22;32m %s\033[0m per %.0f jobs [Fascia oraria %d] :\n", center->name, center->index, fasciaOraria);
    double obsTime = center->lastArrival;
    double obsTime2 = center->lastService - center->firstArrival;

    double theorical_lambda;
    if(strcmp(center->name, "biglietteria_0")==0){
      theorical_lambda = lambda*(1-p_online)*0.5;
    }
    if(strcmp(center->name, "biglietteria_1")==0){
      theorical_lambda = lambda*(1-p_online)*0.5;
    }
    if(strcmp(center->name, "cassaFoodArea")==0){
      theorical_lambda = lambda*p_foodArea;
    }
   
    double theorical_interarrival = 1/theorical_lambda;
    double theorical_mu = center->serviceParams->mean;
    double theorical_meanServiceTime = 1/theorical_mu;
    double theorical_ro = theorical_lambda / theorical_mu;
    double theorical_Tq = (theorical_ro*theorical_meanServiceTime)/(1-theorical_ro);
    double theorical_N = (theorical_meanServiceTime + theorical_Tq)*theorical_lambda;
    

    // mean interrarrivaltime: an/n
    printf(" ______________________________________________________\n");
    printf("|   SIMULATION                          |   THEORICAL  |\n");
    printf("|_______________________________________|______________|\n");
    printf("|  average interarrival time = %6.3f   |   %6.3f     |\n", obsTime / (center->index), theorical_interarrival);
    printf("|  average arrival rate .... = %6.3f   |   %6.3f     |\n", center->index / obsTime, theorical_lambda);
    //printf("   [con t.current] average arrival rate .... = %6.3f\n", center->index  / t.current);

    // mean wait: E(w) = sum(wi)/n
    printf("|  average wait ............ = %6.3f   |   %6.3f     |\n", center->node / center->index, theorical_Tq + theorical_meanServiceTime);
    // mean delay: E(d) = sum(di)/n
    printf("|  average delay ........... = %6.3f   |   %6.3f     |\n", center->queue / center->index, theorical_Tq);

    /* consistency check: E(w) = E(d) + E(s) */

    // mean service time : E(s) = sum(si)/n
    printf("|  average service time .... = %6.3f   |   %6.3f     |\n", center->service / center->index, theorical_meanServiceTime);
    printf("|  average # in the node ... = %6.3f   |   %6.3f     |\n", center->node / obsTime2, theorical_N);
    printf("|  average # in the queue .. = %6.3f   |   %6.3f     |\n", center->queue / obsTime2, theorical_lambda * theorical_Tq);
    printf("|  utilization ............. = %6.3f   |   %6.3f     |\n", center->service / obsTime2, theorical_ro);
    printf("|_______________________________________|______________|\n");
}

void visualizeStatisticsMultiservers(center *center){

    double theorical_lambda;
    if(strcmp(center->name, "controlloBiglietti")==0){
      theorical_lambda = lambda;
    } 
    if(strcmp(center->name, "foodArea")==0){
      theorical_lambda = lambda*p_foodArea;
    }
    if(strcmp(center->name, "gadgetsArea")==0){
      theorical_lambda = lambda*p_gadgetsArea + lambda*p_foodArea*p_gadgetsAfterFood;
    }  
    double theorical_interarrival = 1/theorical_lambda;


    int m = center->servers;
    double mu = center->serviceParams->mean;
    //printf("MU: %f\n", mu);
    /* E(s) = 1 / mu * m */
    double theorical_service = 1 / (mu*m);
    /* ro = lambda * E(s) */
    double ro = theorical_lambda*theorical_service;
    //printf("RO: %f\n", ro);

    double p_zero = 1 / (  ((pow((m*ro),m)) / ((1-ro)*(Factorial(m))) ) + SumUp(m, ro));
    double p_q = p_zero * ((pow((m*ro),m)) / (Factorial(m)*(1-ro)) );



    // E(Tq) = Pq*E(s) / (1-ro)
    double theorical_delay = (p_q * theorical_service) / (1-ro);
    // E(Ts) = E(si) + E(Tq)
    double theorical_wait = theorical_delay + 1/mu;

    printf("\nSTATISTICHE\033[22;32m %s\033[0m per %.0f jobs [Fascia oraria %d] :\n\n", center->name, center->index, fasciaOraria);
    
    printf("Pq = %f\n", p_q);
    printf("P(o) = %f\n", p_zero);
    printf("average delay: %f\n", theorical_delay);
    printf(" ______________________________________________________\n");
    printf("|   SIMULATION                          |   THEORICAL  |\n");
    printf("|_______________________________________|______________|\n");
    printf("|  average interarrival time = %6.3f   |   %6.3f     |\n", ((center->lastArrival - center->firstArrival) / center->index), theorical_interarrival);  //ok
    printf("|  average arrival rate .... = %6.3f   |   %6.3f     |\n", center->index / (center->lastArrival - center->firstArrival), theorical_lambda);                                   //ok

    // mean wait: E(w) = sum(wi)/n                 
    printf("|  average wait ............ = %6.3f   |   %6.3f     |\n", (center->node) / center->index, theorical_wait);                                            //ok
    // mean delay: E(d) = sum(di)/n               
    printf("|  average delay ........... = %6.3f   |   %6.3f     |\n", center->queue / center->index, theorical_delay);                                          //ok

    /* consistency check: E(w) = E(d) + E(s) */

    /* mean service time : E(s) = sum(si)/n */
    printf("|  average service time .... = %6.3f   |   %6.3f     |\n", (center->service/m) / center->index, theorical_service);                                      //ok
    /* E(N) = E(Ts) * lamba */
    printf("|  average # in the node ... = %6.3f   |   %6.3f     |\n", (center->node) / (center->lastService - center->firstArrival), theorical_lambda * theorical_wait);
    /* E(N) = E(Ts) * lamba */
    printf("|  average # in the queue .. = %6.3f   |   %6.3f     |\n", center->queue / (center->lastService - center->firstArrival), theorical_lambda * theorical_delay);
    printf("|  utilization ............. = %6.3f   |   %6.3f     |\n", center->service / (center->servers*(center->lastService - center->firstArrival)), ro);                     //ok
    printf("|_______________________________________|______________|\n");
}

void visualizeRunParameters(double tot, double lambda, double p_foodArea, double p_gadgetsArea, double p_gadgetsAfterFood){
    char a[5];
    strcpy(a, "%");
    printf("\n\033[22;30mSTOP (close the door) time ....... = %.2f\033[0m\n",STOP);
    printf("\033[22;30mNumero totale di arrivi al sistema = %.0f\033[0m\n",tot);
    printf("\033[22;30mLambda ........................... = %.3f job/min\033[0m\n",lambda);
    printf("\033[22;30mp_foodArea ....................... = %.2f %s\033[0m\n",p_foodArea*100,a);
    printf("\033[22;30mp_gadgetsArea .................... = %.2f %s\033[0m\n",p_gadgetsArea*100,a);
    printf("\033[22;30mp_gadgetsAfterFood ............... = %.2f %s\033[0m\n",p_gadgetsAfterFood*100,a);
}

int simulation(int fascia_oraria){
  
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

  fasciaOraria = fascia_oraria;
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
      return 0;

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
  biglietteria[0].serviceParams = &biglietteriaService0;
  initServiceData(&biglietteriaService1, MU_BIGLIETTERIA, STREAM_BIGLIETTERIA1);
  biglietteria[1].serviceParams = &biglietteriaService1;
  initServiceData(&controlloBigliettiService, MU_CONTROLLOBIGLIETTI, STREAM_CONTROLLOBIGLIETTI);
  controlloBiglietti.serviceParams = &controlloBigliettiService;
  initServiceData(&cassaFoodAreaService, MU_CASSA_FOOD_AREA, STREAM_CASSA_FOOD_AREA);
  cassaFoodArea.serviceParams = &cassaFoodAreaService;
  initServiceData(&foodAreaService, MU_FOOD_AREA, STREAM_FOOD_AREA);
  foodArea.serviceParams = &foodAreaService;
  initServiceData(&gadgetsAreaService, MU_GADGETS_AREA, STREAM_GADGETS_AREA);
  gadgetsArea.serviceParams = &gadgetsAreaService;

  event departuresControlloBiglietti[SERVERS_CONTROLLO_BIGLIETTI];
  initEvents(departuresControlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI);
  event departuresAreaFood[SERVERS_FOOD_AREA];
  initEvents(departuresAreaFood, SERVERS_FOOD_AREA);
  event departuresGadgetsArea[SERVERS_GADGETS_AREA];
  initEvents(departuresGadgetsArea, SERVERS_GADGETS_AREA);
  event event[NUMBER_OF_EVENTS];
  initEvents(event, NUMBER_OF_EVENTS);

  PlantSeeds(SEED);
  arrival = START;
  t.current    = START; /* set the clock */
  t.next = 0.0;
  t.last = 0.0;

  event[0].t = GetArrival(); /* genero e salvo il primo arrivo */
  event[0].x   = 1;

  int e;
  int i;

  while ((event[0].t < STOP) || ((biglietteria[0].number + biglietteria[1].number + controlloBiglietti.number + cassaFoodArea.number + foodArea.number + gadgetsArea.number) > 0)){

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
        printRed("Arrival with online ticket \n");
      #endif
      double time = event[0].t;
      event[0].t = GetArrival();    /* generate the next arrival event */
      if (event[0].t > STOP)  {
          t.last      = t.current;
          event[0].x  = 0;
        }
      event[4].t = time;
      event[4].x = 1;
      x = 4;
      }else{
        #ifdef DEBUG
          printfRed("Arrival with physical ticket \n");
        #endif
      }
    }
    switch(x){
      case 0:                         /* process an arrival to biglietteria */

        i = routingAfterBiglietteria();
        biglietteria[i].number++;
        biglietteria[i].lastArrival = t.current;

        if(t.current < biglietteria[i].firstArrival){
          biglietteria[i].firstArrival = t.current;
        }
        event[0].t = GetArrival();    /* generate the next arrival event */
      
        if (event[0].t > STOP)  {
          t.last      = t.current;
          biglietteria[i].lastArrival = t.current;
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
        biglietteria[0].index++;
        #ifdef DEBUG  
          printf("@@@@@@@@@@@@@@@@@@@@@ ho incrementato biglietteria[0]\n");
        #endif
        biglietteria[0].number--;
        biglietteria[0].lastService = t.current;

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
        biglietteria[1].index++;
        #ifdef DEBUG  
          printf("@@@@@@@@@@@@@@@@@@@@@ ho incrementato biglietteria[1]\n");
        #endif
        biglietteria[1].number--;
        biglietteria[1].lastService = t.current;

        
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
        #ifdef DEBUG
          printf("Process an arrival to c b 3\n");
        #endif
        controlloBiglietti.number++;
        controlloBiglietti.lastArrival = t.current;
        if(t.current < controlloBiglietti.firstArrival){
          controlloBiglietti.firstArrival = t.current;
        }

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
          event[5].t = FindNextDeparture(departuresControlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI);
        }
        break;

      case 4:                            /* process an arrival to controllo biglietti */
        #ifdef DEBUG
          printf("Process an arrival to c b 4\n");
        #endif
        controlloBiglietti.number++;
        controlloBiglietti.lastArrival = t.current;
        if(t.current < controlloBiglietti.firstArrival){
          controlloBiglietti.firstArrival = t.current;
        }
        
        event[4].x = 0;
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
          departuresControlloBiglietti[idleServer_controlloBiglietti].t = t.current + service;

          /*segnalo disponibilità dell'evento di completion da controllo biglietti*/
          event[5].x = 1;
          event[5].t = FindNextDeparture(departuresControlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI);
        }
        break;

      case 5:                            /* process a departure from controllo biglietti */
        #ifdef DEBUG
          printf("Process a departure from c b\n");
        #endif
        controlloBiglietti.index++;
        controlloBiglietti.number--;
        controlloBiglietti.lastService = t.current;

        int e_cb = 0;
        double tempo = event[5].t;
        /* recupero l'indice del server */
        for(int k=0; k<SERVERS_CONTROLLO_BIGLIETTI; k++){
            if(departuresControlloBiglietti[k].t == tempo){
                e_cb = k;
            }
        }
        
        /* si è appena liberato un server, ma ho almeno qualcuno in coda*/
        if(controlloBiglietti.number >= SERVERS_CONTROLLO_BIGLIETTI){
          
          double service = GetService(&controlloBigliettiService);

          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          controlloBiglietti_MS[e_cb].service += service;
          /* incremento il numero di job serviti */
          controlloBiglietti_MS[e_cb].served++;
          departuresControlloBiglietti[e_cb].t = t.current + service;

        }else{
          /* server idle */
          controlloBiglietti_MS[e_cb].occupied = 0;
          /* no departure event available */
          departuresControlloBiglietti[e_cb].x = 0;
          departuresControlloBiglietti[e_cb].t = INFINITY;

          if(controlloBiglietti.number==0){
            event[5].x = 0;
            event[5].t = INFINITY;  /* per sicurezza ;) */
          }
        }
        event[5].t = FindNextDeparture(departuresControlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI);


        switch(routingAfterControlloBiglietti()){
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
        cassaFoodArea.lastArrival = t.current;
        if(t.current < cassaFoodArea.firstArrival){
          cassaFoodArea.firstArrival = t.current;
        }

        event[6].x = 0;
        if(cassaFoodArea.number == 1){
          event[7].t = t.current + GetService(&cassaFoodAreaService);
          event[7].x = 1;
        }
        break;

      case 7:             /* process a departure from cassaFoodArea */
        tempo = event[7].t;
        cassaFoodArea.index++;
        cassaFoodArea.number--;
        cassaFoodArea.lastService = t.current;
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
          printf("Process an arrival to area food\n");
        #endif
        foodArea.number++;
        foodArea.lastArrival = t.current;
        if(t.current < foodArea.firstArrival){
          foodArea.firstArrival = t.current;
        }

        event[8].x = 0;                  
        /* se ci sono arrivi entro i server disponibili, continuo a riempirli */
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
          departuresAreaFood[idleServer_foodArea].t = t.current + service;

          /*segnalo disponibilità dell'evento di completion da area food*/
          event[9].x = 1;
          event[9].t = FindNextDeparture(departuresAreaFood, SERVERS_FOOD_AREA);
        }
        break;

      case 9:             /* process a departure from areaFood */
        #ifdef DEBUG
          printf("Process a departure from area food\n");
        #endif
        foodArea.index++;
        foodArea.number--;
        foodArea.lastService = t.current;

        int e_cb_9 = 0;
        tempo = event[9].t;
        /* recupero l'indice del server */
        for(int k=0; k<SERVERS_FOOD_AREA; k++){
            if(departuresAreaFood[k].t == tempo){
                e_cb_9 = k;
            }
        }
        
        /* si è appena liberato un server, ma ho almeno qualcuno in coda*/
        if(foodArea.number >= SERVERS_FOOD_AREA){
          
          double service = GetService(&foodAreaService);
          
          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          areaFood_MS[e_cb_9].service += service;
          /* incremento il numero di job serviti */
          areaFood_MS[e_cb_9].served++;
          departuresAreaFood[e_cb_9].t = t.current + service;

        }else{

          /* server idle */
          areaFood_MS[e_cb_9].occupied = 0;
          /* no departure event available */
          departuresAreaFood[e_cb_9].x = 0;
          departuresAreaFood[e_cb_9].t = INFINITY;

          if(foodArea.number==0){
            event[9].x = 0;
            event[9].t = INFINITY;  /* per sicurezza ;) */
            break;
          }
        }
        event[9].t = FindNextDeparture(departuresAreaFood, SERVERS_FOOD_AREA);


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
        gadgetsArea.number++;
        gadgetsArea.lastArrival = t.current;
        if(t.current < gadgetsArea.firstArrival){
          gadgetsArea.firstArrival = t.current;
        }

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
          departuresGadgetsArea[idleServer_gadgetsArea].t = t.current + service;

          /*segnalo disponibilità dell'evento di completion da area gadgets*/
          event[11].x = 1;
          event[11].t = FindNextDeparture(departuresGadgetsArea, SERVERS_GADGETS_AREA);
          
        }
        break;

      case 11:             /* process a departure from gadgetsArea */
        gadgetsArea.index++;
        gadgetsArea.number--;
        gadgetsArea.lastService = t.current;
        cinema.number--;

        int e_cb_11 = 0;
        tempo = event[11].t;
        /* recupero l'indiced del server */
        for(int k=0; k<SERVERS_GADGETS_AREA; k++){
            if(departuresGadgetsArea[k].t == tempo){
                e_cb_11 = k;
            }
        }
        
        /* si è appena liberato un server, ma ho almeno qualcuno in coda*/
        if(gadgetsArea.number >= SERVERS_GADGETS_AREA){

          double service = GetService(&gadgetsAreaService);
          
          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          areaGadgets_MS[e_cb_11].service += service;
          /* incremento il numero di job serviti */
          areaGadgets_MS[e_cb_11].served++;
          departuresGadgetsArea[e_cb_11].t = t.current + service;

        }else{
          
          /* server idle */
          areaGadgets_MS[e_cb_11].occupied = 0;
          /* no departure event available */
          departuresGadgetsArea[e_cb_11].x = 0;
          departuresGadgetsArea[e_cb_11].t = INFINITY;

          if(gadgetsArea.number==0){
            event[11].x = 0;
            event[11].t = INFINITY;  /* per sicurezza ;) */
            break;
          }
        }
        event[11].t = FindNextDeparture(departuresGadgetsArea, SERVERS_GADGETS_AREA);
      
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
  visualizeStatisticsMultiservers(&controlloBiglietti);
  visualizeStatistics(&cassaFoodArea);
  visualizeStatisticsMultiservers(&foodArea);
  visualizeStatisticsMultiservers(&gadgetsArea);
  
  return 0;
}