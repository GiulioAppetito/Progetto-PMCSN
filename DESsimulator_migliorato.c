/* ------------------------------------------------------------------------- 
  Created by Appetito Giulio, Brinati Anastasia
* --------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "rngs.h"                      /* the multi-stream generator */
#include "rvgs.h"                      /* random variate generators  */
#include "datastructures.h"
//#include "ourlib.h"
#include "DESsimulator.h"
#include "parameters.h"

#define NUMBER_OF_EVENTS 10  

//#define DEBUG 
//#define DEBUG2
        
double MU_BIGLIETTERIA2       = 3.428;
double MU_CONTROLLOBIGLIETTI2 = 6.667;      
double MU_CASSA_FOOD_AREA2    = 6.0;
double MU_FOOD_AREA2          = 2.0;
double MU_GADGETS_AREA2       = 1.333;

double lambda;
double STOP;
double n;


int Factorial2(int m){
  if(m==0 || m == 1){
    return 1;
  }
  int n = m*Factorial(m-1);
  return n;
}

double SumUp2(int m, double ro){
  double sum = 0;
  for(int i = 0; i < m; i++){
    sum += (pow(m*ro,i))/Factorial(i);
  }
  return sum;
}


/* servers functions */

int FindIdleServer2(multiserver multiserver[], int servers){
  /* -----------------------------------------------------
 * return the index of the available server idle longest
 * -----------------------------------------------------
 */
  int s;
  double current = INFINITY; /* current min */

  for(int i=0; i<servers; i++){
    if(multiserver[i].occupied == 0){              /* check if the multiserver is available == 0 */
      if(multiserver[i].service <= current){       /* find which                                 */ 
        current = multiserver[i].service;          /* has been idle longest                      */
        s = i;
      }
    }
  }
  return (s);
/*
 * -----------------------------------------------------
 * return in order the index of the first available server
 * -----------------------------------------------------
 
  int s = 0;
  double current = INFINITY;

  for(int i=0; i<servers; i++){
    
    if(multiserver[i].occupied == 0){ 
      s = i;
      return s;
    }

  }
  return s;
  */
}

double FindNextDeparture2(event ms[], int n_servers){
  double departure = INFINITY;

  for(int k=0; k<n_servers; k++){
    if(ms[k].x == 1 && ms[k].t < departure){
      departure = ms[k].t;
    }
  }

  return departure;
}


void resetCenterStats2(center *center, int servers, char *name){
  center->node=0.0;
  center->queue=0.0;
  center->service=0.0;
  center->index=0.0;
  center->servers = servers;
  center->name = name;
  center->firstArrival = center->lastArrival;
  center->lastArrival = 0.0;
  center->lastService = 0.0;

}

void initServiceData2(serviceData *center, double mu, int stream){
  center->mean = mu;
  center->stream = stream;
}

void initEvents2(event departures[], int num_servers){
    for(int i=0; i<num_servers; i++){
      departures[i].x = 0;
      departures[i].t = INFINITY;
    }
}


double arrival2 = START;
int fasciaOraria2 = 0;

double batchesCounter2 = 0.0;

double TsBiglietteria;
double TsControlloBiglietti;
double TsCassaFoodArea;
double TsFoodArea;
double TsGadgetsArea;

double gadgets2 = 0.0;
double gadgetsAfterFood2 = 0.0;
double food2 = 0.0;
double online2 = 0.0;

double GetArrival2()
/* ---------------------------------------------
 * generate the next arrival time
 * ---------------------------------------------
 */ 
{
  SelectStream(STREAM_ARRIVALS);
  double mean = 1/lambda;
  arrival2 += Exponential(mean);
  return (arrival2);
}

double GetService2(serviceData *center)
/* --------------------------------------------
 * generate the next service time with rate 2/3
 * --------------------------------------------
 */ 
{
  SelectStream(center->stream);
  double mean = 1 / center->mean;
  return (Exponential(mean));
}

int NextEvent2(event events[])
/* ---------------------------------------
 * return the index of the next event type
 * ---------------------------------------
 */
{
  int e = 0;                    /* event index */
  double current = INFINITY; /* current min */
  for(int i=0; i<NUMBER_OF_EVENTS; i++){
    if(events[i].x == 1 ){             /* check if the event is active */
      if(events[i].t <= current){     /* check if it came sooner then the current min */
        current = events[i].t;
        e = i;
      }
    }
  }
  return e;
}

choice routingAfterFoodArea2(){
  SelectStream(STREAM_ROUTING_GADGETS);
  double p = Uniform(0,1);
  if (p < p_gadgetsAfterFood){
    gadgetsAfterFood2++;
    return GADGETSAREA;
  }
  return NONE;
}

choice routingAfterControlloBiglietti2(){
  SelectStream(STREAM_ROUTING_CONTROLLO);
  double p = Uniform(0,1);    /* routing probability p */
  if(p < p_foodArea){
    food2++;
    return FOODAREA;
  }else if ( p < (p_foodArea + p_gadgetsArea)){
    gadgets2++;
    return GADGETSAREA;
  }else{
    return NONE;
  }
}

ticketMode routingBeforeBiglietteria2(){
  double p = Uniform(0,1);
  if( p < p_online){
    online2++;
    return ONLINE;
  }
  return PHYSICAL;
}


void updateIntegrals2(center *center, multiserver multiserver[]){

  if (center->number > 0)  {        
        /* update integrals  */
        center->node    += (t.next - t.current) * center->number;

        if(t.next != t.current){
          if(center->number > center->servers){
            center->queue   += (t.next - t.current) * (center->number - center->servers);
          }
          if(center->servers == 1){
            center->service += (t.next - t.current);
          }else if(center->servers > 1){
            for(int j=0; j< (center->servers); j++){
              if(multiserver[j].occupied==1){
                center->service += (t.next - t.current);
              }
            }
          }
        }              
  }
}

double theoricalLambda2(center *center){
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
    if(strcmp(center->name, "cassaFoodArea")==0){
      theorical_lambda = lambda*p_foodArea;
    }
    if(strcmp(center->name, "biglietteria")==0){
      theorical_lambda = lambda*(1-p_online);
    } 
    return theorical_lambda;
}

double theoricalDelaySS2(center *center){

  double theorical_lambda = theoricalLambda2(center);
  
  double theorical_service = 1 / center->serviceParams->mean;
  /* ro = lambda * E(s) */
  double ro = theorical_lambda*theorical_service;
  // E(Tq) = Pq*E(s) / (1-ro)
  double theorical_delay = (ro*theorical_service)/(1-ro);

  return theorical_delay;

}

double theoricalWaitSS2(center *center){
  
  double theorical_Tq = theoricalDelaySS2(center);
  double theorical_wait = theorical_Tq + 1/center->serviceParams->mean;
  return theorical_wait;

}

void printingValues2(char *name, outputStats *output, double theorical_lambda, double theorical_Ts, double theorical_Tq, double theorical_meanServiceTime, double theorical_ro){
    
    printf("\nSTATISTICHE\033[22;32m %s\033[0m[Fascia oraria %d] :\n", name, fasciaOraria2);
    
    printf(" ______________________________________________________\n");
    printf("|   SIMULATION                          |   THEORICAL  |\n");
    printf("|_______________________________________|______________|\n");
    printf("|  average interarrival time = %6.3f   |   %6.3f     |\n", output->avgInterarrivalTime, 1/theorical_lambda);
    printf("|  average arrival rate .... = %6.3f   |   %6.3f     |\n", output->avgArrivalRate, theorical_lambda);
      //printf("   [con t.current] average arrival rate .... = %6.3f\n", center->index  / t.current);

      // mean wait: E(w) = sum(wi)/n
    printf("|  average wait ............ = %6.3f   |   %6.3f     |\n", output->avgWait, theorical_Ts);
      // mean delay: E(d) = sum(di)/n
    printf("|  average delay ........... = %6.3f   |   %6.3f     |\n", output->avgDelay, theorical_Tq);

      /* consistency check: E(Ts) = E(Tq) + E(s) */

      // mean service time : E(s) = sum(si)/n
    printf("|  average service time .... = %6.3f   |   %6.3f     |\n", output->avgServiceTime, theorical_meanServiceTime);
    printf("|  average # in the node ... = %6.3f   |   %6.3f     |\n", output->avgNumNode, (theorical_meanServiceTime + theorical_Tq)*theorical_lambda);
    printf("|  average # in the queue .. = %6.3f   |   %6.3f     |\n", output->avgNumQueue, theorical_lambda * theorical_Tq);
    printf("|  utilization ............. = %6.3f   |   %6.3f     |\n", output->utilization, theorical_ro);
    printf("|_______________________________________|______________|\n");
}

double visualizeStatistics2(outputStats *output,center *center){
    
    double theorical_lambda = theoricalLambda2(center);
    double theorical_meanServiceTime = 1/center->serviceParams->mean;
    double theorical_ro = theorical_lambda * theorical_meanServiceTime;
    double theorical_Tq = theoricalDelaySS2(center);
    double theorical_Ts = theoricalWaitSS2(center);
    
    printingValues2(center->name, output, theorical_lambda, theorical_Ts, theorical_Tq, theorical_meanServiceTime, theorical_ro);
    
    return theorical_ro;
}

double theoricalDelayMS2(center *center){

  double theorical_lambda = theoricalLambda2(center);
  
  double mu = center->serviceParams->mean;
  int m = center->servers;
  /* E(s) = 1 / mu * m */
  double theorical_service = 1 / (mu*m);
  /* ro = lambda * E(s) */
  double ro = theorical_lambda*theorical_service;
  double p_zero = 1 / (  ((pow((m*ro),m)) / ((1-ro)*(Factorial2(m))) ) + SumUp2(m, ro));
  double p_q = p_zero * ((pow((m*ro),m)) / (Factorial2(m)*(1-ro)) );
/*
  printf("*********************************************** ro %s : %f\n", center->name, ro);
  printf("*********************************************** p_zero %s : %f\n", center->name, p_zero);
  printf("*********************************************** p_q %s : %f\n", center->name, p_q);
*/
  // E(Tq) = Pq*E(s) / (1-ro)
  double theorical_delay = (p_q * theorical_service) / (1-ro);
/*
  printf("*********************************************** th_s %s : %f\n", center->name, theorical_service);

  printf("*********************************************** th_d %s : %f\n", center->name, theorical_delay);
*/
  return theorical_delay;

}

double theoricalWaitMS2(center *center){
  
  double mu = center->serviceParams->mean;
  double theorical_delay = theoricalDelayMS2(center);
  double theorical_wait = theorical_delay + 1/mu;
  return theorical_wait;

}

double visualizeStatisticsMultiservers2(outputStats *output,center *center){

    double theorical_lambda=theoricalLambda2(center);
    int m = center->servers;
    double mu = center->serviceParams->mean;
    /* E(s) = 1 / mu * m */
    double theorical_service = 1 / (mu*m);
    //printf("********************%s theorical_service : %f\n", center->name, theorical_service);
    /* ro = lambda * E(s) */
    double theorical_ro = theorical_lambda*theorical_service;
   // printf("********************%s theorical_ro : %f\n", center->name, theorical_ro);
    // E(Tq) = Pq*E(s) / (1-ro)
    double theorical_Tq = theoricalDelayMS2(center);
   // printf("********************%s theorical_tq : %f\n", center->name, theorical_Tq);
    // E(Ts) = E(si) + E(Tq)
    double theorical_Ts = theoricalWaitMS2(center);
  //  printf("********************%s theorical_ts : %f\n", center->name, theorical_Ts);
    
    printingValues2(center->name, output, theorical_lambda, theorical_Ts, theorical_Tq, theorical_service, theorical_ro);

    return theorical_ro;
}

void visualizeRunParameters2(double tot, double lambda, double p_foodArea, double p_gadgetsArea, double p_gadgetsAfterFood){
    char a[5];
    strcpy(a, "%");
    printf("\n\033[22;30mSTOP (close the door) time ....... = %.2f\033[0m\n",STOP);
    printf("\033[22;30mNumero totale di arrivi al sistema = %.0f\033[0m\n",tot);
    printf("\033[22;30mLambda ........................... = %.3f job/min\033[0m\n",lambda);
    printf("\033[22;30mp_foodArea ....................... = %.2f %s\033[0m\n",p_foodArea*100,a);
    printf("\033[22;30mp_gadgetsArea .................... = %.2f %s\033[0m\n",p_gadgetsArea*100,a);
    printf("\033[22;30mp_gadgetsAfterFood ............... = %.2f %s\033[0m\n\n",p_gadgetsAfterFood*100,a);
    printf("\033[22;30mServer configuration \nbiglietteria : %d\ncontrolloBiglietti : %d\ncassaFoodArea : %d\nfoodArea : %d\ngadgetsArea : %d\n\033[0m\n",SERVERS_BIGLIETTERIA,SERVERS_CONTROLLO_BIGLIETTI,SERVERS_CASSA_FOOD_AREA,SERVERS_FOOD_AREA,SERVERS_GADGETS_AREA);

}

outputStats updateStatisticsMS2(center *center){
  outputStats output;
  output.name = center->name;
  output.avgInterarrivalTime = ((center->lastArrival - center->firstArrival) / center->index);
  printf("\n ************ %s .index %f \n", center->name, center->index);
  printf("\n %s (center->lastArrival - center->firstArrival): %f\n", center->name, (center->lastArrival - center->firstArrival));
  output.avgArrivalRate = center->index / (center->lastArrival - center->firstArrival);
  output.avgWait = (center->node) / center->index;
  output.avgDelay = center->queue / center->index;
  output.avgServiceTime = (center->service/center->servers) / center->index;
  output.avgNumNode = (center->node) / (center->lastService - center->firstArrival);
  output.avgNumQueue = center->queue / (center->lastService - center->firstArrival);
  output.utilization = center->service / (center->servers*(center->lastService - center->firstArrival));
  return output;
}

outputStats updateStatisticsSS2(center *center){
  outputStats output;
  output.name = center->name;
  output.avgInterarrivalTime =((center->lastArrival-center->firstArrival) / center->index);
  output.avgArrivalRate = center->index / (center->lastArrival-center->firstArrival);
  output.avgWait = center->node / center->index;
  output.avgDelay = center->queue / center->index;
  output.avgServiceTime = center->service / center->index;
  output.avgNumNode = center->node / (center->lastService - center->firstArrival);
  output.avgNumQueue = center->queue / (center->lastService - center->firstArrival);
  output.utilization = center->service / (center->lastService - center->firstArrival);
  return output;
}

/* -1 perchè una biglietteria, +1 per il cinema */
int batches[NUM_CENTERS]; 
int b;

void saveBatchStatsSS2(int centerIndex, center *center, outputStats matrix[NUM_BATCHES][NUM_CENTERS-1]){
  //printf("addr center: %p\n", center);
  outputStats output = updateStatisticsSS2(center);
  batchesCounter2++;
  matrix[batches[centerIndex]][centerIndex] = output; /* save statistics in the matrix cell */
  batches[centerIndex] = batches[centerIndex] + 1;
}
void saveBatchStatsMS2(int centerIndex, center *center, outputStats matrix[NUM_BATCHES][NUM_CENTERS-1]){
  outputStats output = updateStatisticsMS2(center);
  batchesCounter2++;
  matrix[batches[centerIndex]][centerIndex] = output; /* save statistics in the matrix cell */
  batches[centerIndex] = batches[centerIndex] + 1;
 
}


double betterSimulation(int fascia_oraria, outputStats row[], outputStats matrix[NUM_BATCHES][NUM_CENTERS-1], double totalArrivals[NUM_BATCHES], double probabilities[NUM_BATCHES][4],int finite, int b, int k, int replication){
    
  printf("\n ********** \n");
  if(finite == 0){
    STOP = STOP_INFINITE;             /* infinite horizon */
    n = k * (NUM_CENTERS-1);
    printf("\nn: %f\n", n);
  }else{
    STOP = STOP_FINITE;               /* finite horizon */
  }

  /* batch indexes data structure for batch means */
  // qui invece +1 perchè c'è il cinema
  for(int i=0; i<NUM_CENTERS; i++){
    batches[i] = 0;
  }

  double peopleInsideBeforeStop = 0.0;

  fasciaOraria2 = fascia_oraria;
  switch(fasciaOraria2){
    case 1:                                           /* fascia oraria 1 */
      lambda = LAMBDA_1;
      p_foodArea = P_FOOD_AREA_1;
      p_gadgetsArea = P_GADGETS_AREA_1;
      p_gadgetsAfterFood = P_GADGETS_AFTER_FOOD_1;
      p_online = P_ONLINE_1;
      break;
    case 2:                                           /* fascia oraria 2 */
      lambda = LAMBDA_2;
      p_foodArea = P_FOOD_AREA_2;
      p_gadgetsArea = P_GADGETS_AREA_2;
      p_gadgetsAfterFood = P_GADGETS_AFTER_FOOD_2;
      p_online = P_ONLINE_2;
      break;
    case 3:                                           /* fascia oraria 3 */
      lambda = LAMBDA_3;
      p_foodArea = P_FOOD_AREA_3;
      p_gadgetsArea = P_GADGETS_AREA_3;
      p_gadgetsAfterFood = P_GADGETS_AFTER_FOOD_3;
      p_online = P_ONLINE_3;
      break;
    default:
      printRed("Selezione non valida.");
      return 0;

  }

 //inizializzazione multiserver
  multiserver biglietteria_MS[SERVERS_BIGLIETTERIA];
  for(int i=0; i<SERVERS_BIGLIETTERIA; i++){
    biglietteria_MS[i].served = 0;
    biglietteria_MS[i].occupied = 0;
    biglietteria_MS[i].service = 0.0;
  } 
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

  //creazione dei centri
  center biglietteria;
  center controlloBiglietti;
  center cassaFoodArea;
  center foodArea;
  center gadgetsArea;
  center cinema;

  //inizializzazione dei centri
  resetCenterStats2(&biglietteria, SERVERS_BIGLIETTERIA, "biglietteria");
  biglietteria.number = 0.0;
  resetCenterStats2(&controlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI, "controlloBiglietti");
  controlloBiglietti.number = 0.0;
  resetCenterStats2(&cassaFoodArea, SERVERS_CASSA_FOOD_AREA,"cassaFoodArea");
  cassaFoodArea.number = 0.0;
  resetCenterStats2(&foodArea, SERVERS_FOOD_AREA,"foodArea");
  foodArea.number = 0.0;
  resetCenterStats2(&gadgetsArea, SERVERS_GADGETS_AREA,"gadgetsArea");
  gadgetsArea.number = 0.0;
  resetCenterStats2(&cinema, 0,"cinema");
  cinema.number = 0.0;


  //creazione dei serviceData per ogni centro
  serviceData biglietteriaService;
  serviceData controlloBigliettiService;
  serviceData cassaFoodAreaService;
  serviceData foodAreaService;
  serviceData gadgetsAreaService;

  //inizializzazione e assegnamento dei serviceData per ogni centro
  initServiceData2(&biglietteriaService, MU_BIGLIETTERIA2, STREAM_BIGLIETTERIA_UNICA);
  biglietteria.serviceParams = &biglietteriaService;
  initServiceData2(&controlloBigliettiService, MU_CONTROLLOBIGLIETTI2, STREAM_CONTROLLOBIGLIETTI);
  controlloBiglietti.serviceParams = &controlloBigliettiService;
  initServiceData2(&cassaFoodAreaService, MU_CASSA_FOOD_AREA2, STREAM_CASSA_FOOD_AREA);
  cassaFoodArea.serviceParams = &cassaFoodAreaService;
  initServiceData2(&foodAreaService, MU_FOOD_AREA2, STREAM_FOOD_AREA);
  foodArea.serviceParams = &foodAreaService;
  initServiceData2(&gadgetsAreaService, MU_GADGETS_AREA2, STREAM_GADGETS_AREA);
  gadgetsArea.serviceParams = &gadgetsAreaService;

  event departuresBiglietteria[SERVERS_BIGLIETTERIA];
  event departuresControlloBiglietti[SERVERS_CONTROLLO_BIGLIETTI];
  event departuresAreaFood[SERVERS_FOOD_AREA];
  event departuresGadgetsArea[SERVERS_GADGETS_AREA];
  event event[NUMBER_OF_EVENTS];

  initEvents2(departuresBiglietteria, SERVERS_BIGLIETTERIA);
  initEvents2(departuresControlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI);
  initEvents2(departuresAreaFood, SERVERS_FOOD_AREA); 
  initEvents2(departuresGadgetsArea, SERVERS_GADGETS_AREA);
  initEvents2(event, NUMBER_OF_EVENTS);

  //inizializzazione clock
  arrival2 = START;
  t.current    = START; /* set the clock */
  t.next = 0.0;
  t.last = 0.0;


  event[0].t = GetArrival2();  /* genero e salvo il primo arrivo */
  event[0].x   = 1;
  cinema.firstArrival = event[0].t;
  double batchStart = event[0].t;

  int e;                      /* variabili per batch means */

  while (((finite) && ((event[0].t < STOP) || ((biglietteria.number + controlloBiglietti.number + cassaFoodArea.number + foodArea.number + gadgetsArea.number) > 0))) ||
        ((!finite) && ( batchesCounter2 < n))){
    e = NextEvent2(event);                        /* next event index  */
    t.next = event[e].t;                         /* next event time   */
    updateIntegrals2(&biglietteria, biglietteria_MS);
    updateIntegrals2(&controlloBiglietti,controlloBiglietti_MS);
    updateIntegrals2(&cassaFoodArea, NULL);
    updateIntegrals2(&foodArea, areaFood_MS);
    updateIntegrals2(&gadgetsArea, areaGadgets_MS);
    updateIntegrals2(&cinema, NULL);
    
    t.current       = t.next;                    /* advance the clock */
    
    printf(" EVENT E : %d\n", e);
    int eventType = e;
    if(eventType == 0){
      if(routingBeforeBiglietteria2() == ONLINE){
        cinema.number++;
        cinema.lastArrival = t.current;
        double time = event[0].t;
        event[0].t = GetArrival2();    /* generate the next arrival event */
        if (event[0].t > STOP)  {
          t.last      = t.current;
          event[0].x  = 0;
        }
        event[2].t = time;
        event[2].x = 1;
        eventType = 2;
      }
    }
    switch(eventType){
           
      case 0:                            /* process an arrival to biglietteria */
        biglietteria.number++;
        biglietteria.lastArrival = t.current;
        if(t.current < biglietteria.firstArrival){
          biglietteria.firstArrival = t.current;
        }

        /* arrival from 0 to controlloBiglietti queue*/
        event[0].x = 0;
        
        if(biglietteria.number <= SERVERS_BIGLIETTERIA){

          double service = GetService2(&biglietteriaService);
          /* vado a cercare l'indice di quale servente è libero */
          int idleServer_biglietteria = FindIdleServer2(biglietteria_MS, SERVERS_BIGLIETTERIA);
          
          /* segnalo che è occupied */
          biglietteria_MS[idleServer_biglietteria].occupied = 1;

          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          biglietteria_MS[idleServer_biglietteria].service += service;
          /* incremento il numero di job serviti */
          biglietteria_MS[idleServer_biglietteria].served++;
          departuresBiglietteria[idleServer_biglietteria].x = 1;
          departuresBiglietteria[idleServer_biglietteria].t = t.current+service;

          /*segnalo disponibilità dell'evento di completion da biglietteria*/
          event[1].x = 1;
          event[1].t = FindNextDeparture2(departuresBiglietteria, SERVERS_BIGLIETTERIA);
        }
        break;

      case 1:                           /* process a departure from biglietteria */
       
        biglietteria.index++;
          printf("BIGLIETTERIA.INDEX : %f \n\n", biglietteria.index);
        biglietteria.number--;
        biglietteria.lastService = t.current;

        int e_cb = 0;
        double tempo = event[1].t;
        /* recupero l'indice del server */
        for(int k=0; k<SERVERS_BIGLIETTERIA; k++){
            if(departuresBiglietteria[k].t == tempo){
                e_cb = k;
            }
        }
        
        /* si è appena liberato un server, ma ho almeno qualcuno in coda*/
        if(biglietteria.number >= SERVERS_BIGLIETTERIA){
          
          double service = GetService2(&biglietteriaService);

          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          biglietteria_MS[e_cb].service += service;
          /* incremento il numero di job serviti */
          biglietteria_MS[e_cb].served++;
          departuresBiglietteria[e_cb].t = t.current + service;

        }else{
          /* server idle */
          biglietteria_MS[e_cb].occupied = 0;
          /* no departure event available */
          departuresBiglietteria[e_cb].x = 0;
          departuresBiglietteria[e_cb].t = INFINITY;

          if(biglietteria.number==0){
            event[1].x = 0;
            event[1].t = INFINITY;  /* per sicurezza ;) */
          }
        }
        event[1].t = FindNextDeparture2(departuresBiglietteria, SERVERS_BIGLIETTERIA);

        /* routing */
        event[2].t = tempo;
        event[2].x = 1;
        
        break;
      
      case 2:                            /* process an arrival to controllo biglietti */
        controlloBiglietti.number++;
        controlloBiglietti.lastArrival = t.current;
        if(t.current < controlloBiglietti.firstArrival){
          controlloBiglietti.firstArrival = t.current;
        }

        event[2].x = 0;                  
        /* se ci sono arrivi entro i server disponibili, continuo a riempirli */
        if(controlloBiglietti.number <= SERVERS_CONTROLLO_BIGLIETTI){

          double service = GetService2(&controlloBigliettiService);
          /* vado a cercare l'indice di quale servente è libero */
          int idleServer_controlloBiglietti = FindIdleServer2(controlloBiglietti_MS, SERVERS_CONTROLLO_BIGLIETTI);
          
          /* segnalo che è occupied */
          controlloBiglietti_MS[idleServer_controlloBiglietti].occupied = 1;

          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          controlloBiglietti_MS[idleServer_controlloBiglietti].service += service;
          /* incremento il numero di job serviti */
          controlloBiglietti_MS[idleServer_controlloBiglietti].served++;
          departuresControlloBiglietti[idleServer_controlloBiglietti].x = 1;
          departuresControlloBiglietti[idleServer_controlloBiglietti].t = t.current+service;

          /*segnalo disponibilità dell'evento di completion da controllo biglietti*/
          event[3].x = 1;
          event[3].t = FindNextDeparture2(departuresControlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI);
        }
        break;

      case 3:                            /* process a departure from controllo biglietti */
        controlloBiglietti.index++;
        controlloBiglietti.number--;
        controlloBiglietti.lastService = t.current;

        int e_cb2 = 0;
        tempo = event[3].t;
        /* recupero l'indice del server */
        for(int k=0; k<SERVERS_CONTROLLO_BIGLIETTI; k++){
            if(departuresControlloBiglietti[k].t == tempo){
                e_cb2 = k;
            }
        }
        
        /* si è appena liberato un server, ma ho almeno qualcuno in coda*/
        if(controlloBiglietti.number >= SERVERS_CONTROLLO_BIGLIETTI){
          
          double service = GetService2(&controlloBigliettiService);

          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          controlloBiglietti_MS[e_cb2].service += service;
          /* incremento il numero di job serviti */
          controlloBiglietti_MS[e_cb2].served++;
          departuresControlloBiglietti[e_cb2].t = t.current + service;

        }else{
          /* server idle */
          controlloBiglietti_MS[e_cb2].occupied = 0;
          /* no departure event available */
          departuresControlloBiglietti[e_cb2].x = 0;
          departuresControlloBiglietti[e_cb2].t = INFINITY;

          if(controlloBiglietti.number==0){
            event[3].x = 0;
            event[3].t = INFINITY;  /* per sicurezza ;) */
          }
        }
        event[3].t = FindNextDeparture2(departuresControlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI);


        switch(routingAfterControlloBiglietti2()){
          case FOODAREA:
            event[4].t = tempo;
            event[4].x = 1;
            break;
          case GADGETSAREA:
            event[8].t = tempo;
            event[8].x = 1;

            break;
          case NONE:

            if(t.current < PUBBLICITY_START){
              peopleInsideBeforeStop++;
            }
    
            cinema.number--;
            cinema.index++;
            break;
          default:
            printf("default\n");
            break;
        }
        break;

      case 4:             /* process an arrival to cassaFoodArea */
        #ifdef DEBUG
          printf("arrival to cassaFoodArea\n");
          case6Iterations++;
        #endif
        cassaFoodArea.number++;
        cassaFoodArea.lastArrival = t.current;
        if(t.current < cassaFoodArea.firstArrival){
          cassaFoodArea.firstArrival = t.current;
        }

        event[4].x = 0;
        if(cassaFoodArea.number == 1){
          event[5].t = t.current + GetService2(&cassaFoodAreaService);
          event[5].x = 1;
        }
        break;

      case 5:             /* process a departure from cassaFoodArea */
        tempo = event[5].t;
        cassaFoodArea.index++;
        cassaFoodArea.number--;
        cassaFoodArea.lastService = t.current;
        if (cassaFoodArea.number > 0){
          event[5].t = t.current + GetService2(&cassaFoodAreaService);
          event[5].x = 1;
        }else{
          event[5].x = 0;
        }
        event[6].t = tempo;
        event[6].x = 1;

        break;

      case 6:             /* process an arrival to areaFood */
        #ifdef DEBUG
          printf("Process an arrival to area food\n");
        #endif
        foodArea.number++;
        foodArea.lastArrival = t.current;
        if(t.current < foodArea.firstArrival){
          foodArea.firstArrival = t.current;
        }

        event[6].x = 0;                  
        /* se ci sono arrivi entro i server disponibili, continuo a riempirli */
        if(foodArea.number <= SERVERS_FOOD_AREA){

          double service = GetService2(&foodAreaService);
          /* vado a cercare l'indice di quale servente è libero */
          int idleServer_foodArea = FindIdleServer2(areaFood_MS, SERVERS_FOOD_AREA);   

          /* segnalo che è occupied */
          areaFood_MS[idleServer_foodArea].occupied = 1;

          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          areaFood_MS[idleServer_foodArea].service += service;
          /* incremento il numero di job serviti */
          areaFood_MS[idleServer_foodArea].served++;
          departuresAreaFood[idleServer_foodArea].x = 1;
          departuresAreaFood[idleServer_foodArea].t = t.current + service;

          /*segnalo disponibilità dell'evento di completion da area food*/
          event[7].x = 1;
          event[7].t = FindNextDeparture2(departuresAreaFood, SERVERS_FOOD_AREA);
        }
        break;

      case 7:             /* process a departure from areaFood */
        #ifdef DEBUG
          printf("Process a departure from area food\n");
        #endif
        foodArea.index++;
        foodArea.number--;
        foodArea.lastService = t.current;

        int e_cb_9 = 0;
        tempo = event[7].t;
        /* recupero l'indice del server */
        for(int k=0; k<SERVERS_FOOD_AREA; k++){
            if(departuresAreaFood[k].t == tempo){
                e_cb_9 = k;
            }
        }
        
        /* si è appena liberato un server, ma ho almeno qualcuno in coda*/
        if(foodArea.number >= SERVERS_FOOD_AREA){
          
          double service = GetService2(&foodAreaService);
          
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
            event[7].x = 0;
            event[7].t = INFINITY;  /* per sicurezza ;) */
            break;
          }
        }
        event[7].t = FindNextDeparture2(departuresAreaFood, SERVERS_FOOD_AREA);


        switch(routingAfterFoodArea2()){
          case GADGETSAREA:
            #ifdef DEBUG
              printf("routing to gadget area after food\n");
            #endif
            event[8].t = tempo;
            event[8].x = 1;
            break;
          case NONE:

            if(t.current < PUBBLICITY_START){
              peopleInsideBeforeStop++;
            }

            cinema.number--;
            cinema.index++;
            break;
          default:
            #ifdef DEBUG
              printf("default after food\n");
            #endif
            break;
        }
        break;

      case 8:             /* process an arrival to gadgetsArea */
        gadgetsArea.number++;
        gadgetsArea.lastArrival = t.current;
        if(t.current < gadgetsArea.firstArrival){
          gadgetsArea.firstArrival = t.current;
        }

        event[8].x = 0;                  /* se ci sono arrivi entro i server disponibili, continuo a riempirli */
        if(gadgetsArea.number <= SERVERS_GADGETS_AREA){

          double service = GetService2(&gadgetsAreaService);
          /* vado a cercare l'indice di quale servente è libero */
          int idleServer_gadgetsArea = FindIdleServer2(areaGadgets_MS, SERVERS_GADGETS_AREA);
          
          /* segnalo che è occupied */
          areaGadgets_MS[idleServer_gadgetsArea].occupied = 1;
          /* registro l'aumento di tempo di attività aggiungendo il tempo di servizio del nuovo job */
          areaGadgets_MS[idleServer_gadgetsArea].service += service;
          /* incremento il numero di job serviti */
          areaGadgets_MS[idleServer_gadgetsArea].served++;

          departuresGadgetsArea[idleServer_gadgetsArea].x = 1;
          departuresGadgetsArea[idleServer_gadgetsArea].t = t.current + service;

          /*segnalo disponibilità dell'evento di completion da area gadgets*/
          event[9].x = 1;
          event[9].t = FindNextDeparture2(departuresGadgetsArea, SERVERS_GADGETS_AREA);
          
        }
        break;

      case 9:             /* process a departure from gadgetsArea */
        gadgetsArea.index++;
        gadgetsArea.number--;
        gadgetsArea.lastService = t.current;

        if(t.current < PUBBLICITY_START){
            peopleInsideBeforeStop++;
        }
        cinema.number--;
        cinema.index++;

        int e_cb_11 = 0;
        tempo = event[9].t;
        /* recupero l'indiced del server */
        for(int k=0; k<SERVERS_GADGETS_AREA; k++){
            if(departuresGadgetsArea[k].t == tempo){
                e_cb_11 = k;
            }
        }
        
        /* si è appena liberato un server, ma ho almeno qualcuno in coda*/
        if(gadgetsArea.number >= SERVERS_GADGETS_AREA){

          double service = GetService2(&gadgetsAreaService);
          
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
            event[9].x = 0;
            event[9].t = INFINITY;  /* per sicurezza ;) */
            break;
          }
        }
        event[9].t = FindNextDeparture2(departuresGadgetsArea, SERVERS_GADGETS_AREA);
      
        break;

      default:
        printf("Default: %d\n", eventType);
        break;
    }
    
    /* infinite simulation */
    if(!finite){

      if((biglietteria.index == b) && (batches[INDEX_BIGLIETTERIA] < k)){
        saveBatchStatsMS2(INDEX_BIGLIETTERIA, &biglietteria, matrix);
        resetCenterStats2(&biglietteria, SERVERS_BIGLIETTERIA, "biglietteria");
      }
      if((controlloBiglietti.index == b) && (batches[INDEX_CONTROLLOBIGLIETTI] < k)){   /* save statistics for batch */
        saveBatchStatsMS2(INDEX_CONTROLLOBIGLIETTI, &controlloBiglietti, matrix);
        resetCenterStats2(&controlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI, "controlloBiglietti");      /* reset center statistics */
      }
      if((cassaFoodArea.index == b) && (batches[INDEX_CASSAFOODAREA] < k)){   /* save statistics for batch */
        saveBatchStatsSS2(INDEX_CASSAFOODAREA, &cassaFoodArea, matrix);
        resetCenterStats2(&cassaFoodArea, SERVERS_CASSA_FOOD_AREA, "cassaFoodArea");      /* reset center statistics */
      }
      if((foodArea.index == b) && (batches[INDEX_FOODAREA] < k)){   /* save statistics for batch */
        saveBatchStatsMS2(INDEX_FOODAREA, &foodArea, matrix);
        resetCenterStats2(&foodArea, SERVERS_FOOD_AREA, "foodArea");      /* reset center statistics */
      }
      if((gadgetsArea.index == b) && (batches[INDEX_GADGETSAREA] < k)){   /* save statistics for batch */
        saveBatchStatsMS2(INDEX_GADGETSAREA, &gadgetsArea, matrix);
        resetCenterStats2(&gadgetsArea, SERVERS_GADGETS_AREA, "gadgetsArea");      /* reset center statistics */
      }
      
      if((cinema.index == b) && (batches[INDEX_CINEMA] < k)){
        
        batchStart = t.current;

        int batch = batches[INDEX_CINEMA];

        probabilities[batch][0] = online2 / BATCH_SIZE;
        //printf("\n probabilities[replication][0]: %f\n", probabilities[replication][0]);

        probabilities[batch][1] = food2 / BATCH_SIZE;
       //printf("\n probabilities[replication][1]: %f\n", probabilities[replication][1]);

        probabilities[batch][2] = gadgets2 / BATCH_SIZE;
        //printf("\n probabilities[replication][2]: %f\n", probabilities[replication][2]);

        probabilities[batch][3] = gadgetsAfterFood2 / BATCH_SIZE;
        //printf("\n probabilities[replication][3]: %f\n", probabilities[replication][3]);

        gadgets2 = 0.0;
        gadgetsAfterFood2 = 0.0;
        food2 = 0.0;
        online2 = 0.0;

        batches[INDEX_CINEMA]++;
        resetCenterStats2(&cinema, 0, "cinema");
        cinema.number = 0.0;
      }
    }
  }


  if(finite){
    visualizeRunParameters2(cinema.index, lambda, p_foodArea, p_gadgetsArea, p_gadgetsAfterFood);
    
    //salvataggio delle statistiche di ogni centro nelle struct outputStats
    outputStats biglietteriaOutput = updateStatisticsMS2(&biglietteria);
    outputStats controlloBigliettiOutput = updateStatisticsMS2(&controlloBiglietti);
    outputStats cassaFoodAreaOutput = updateStatisticsSS2(&cassaFoodArea);
    outputStats foodAreaOutput = updateStatisticsMS2(&foodArea);
    outputStats gadgetsAreaOutput = updateStatisticsMS2(&gadgetsArea);
    
    //salvataggio delle outputStats nella riga della matrix relativa a questa run
    row[INDEX_BIGLIETTERIA] = biglietteriaOutput;
    row[INDEX_CONTROLLOBIGLIETTI] = controlloBigliettiOutput;
    row[INDEX_CASSAFOODAREA] = cassaFoodAreaOutput;
    row[INDEX_FOODAREA] = foodAreaOutput;
    row[INDEX_GADGETSAREA] = gadgetsAreaOutput;

    probabilities[replication][0] = online2 / cinema.index;
    probabilities[replication][1] = food2 / cinema.index;
    probabilities[replication][2] = gadgets2 / cinema.index;
    probabilities[replication][3] = gadgetsAfterFood2 / cinema.index;

    online2 = 0.0;
    food2 = 0.0;
    gadgets2 = 0.0;
    gadgetsAfterFood2 = 0.0;

    //visualizzazione a schermo delle statistiche dei centri
    visualizeStatisticsMultiservers2(&biglietteriaOutput,&biglietteria);
    visualizeStatisticsMultiservers2(&controlloBigliettiOutput,&controlloBiglietti);
    visualizeStatistics2(&cassaFoodAreaOutput,&cassaFoodArea);
    visualizeStatisticsMultiservers2(&foodAreaOutput,&foodArea);
    visualizeStatisticsMultiservers2(&gadgetsAreaOutput,&gadgetsArea);

    printf("\npeople that got in : %f", peopleInsideBeforeStop);
    printf("\npeople in total: %f", cinema.index);
    totalArrivals[replication] = cinema.index;
    
    double perc = (peopleInsideBeforeStop/cinema.index)*100;
    printGreen("\n% people inside before the pubblicity starts : ");
    printf("%.3f\n", perc);

    return perc;

}

  if(!finite){

    TsBiglietteria = theoricalWaitMS2(&biglietteria);
    TsControlloBiglietti = theoricalWaitMS2(&controlloBiglietti);
    TsCassaFoodArea = theoricalWaitSS2(&cassaFoodArea);
    TsFoodArea = theoricalWaitMS2(&foodArea);
    TsGadgetsArea = theoricalWaitMS2(&gadgetsArea);

    printf("\nts Biglietteria: %f\n", TsBiglietteria);
    printf("\nts ControlloBiglietti: %f\n", TsControlloBiglietti);
    printf("\nts CassaFoodArea: %f\n", TsCassaFoodArea);
    printf("\nts FoodArea: %f\n", TsFoodArea);
    printf("\nts GadgetsArea: %f\n", TsGadgetsArea);

    double TqBiglietteria = theoricalDelayMS2(&biglietteria);
    double TqControlloBiglietti = theoricalDelayMS2(&controlloBiglietti);
    double TqCassaFoodArea = theoricalDelaySS2(&cassaFoodArea);
    double TqFoodArea = theoricalDelayMS2(&foodArea);
    double TqGadgetsArea = theoricalDelayMS2(&gadgetsArea);

    printf("\n*******************************\n");
    printf("\ntq Biglietteria: %f\n", TqBiglietteria);
    printf("\ntq ControlloBiglietti: %f\n", TqControlloBiglietti);
    printf("\ntq CassaFoodArea: %f\n", TqCassaFoodArea);
    printf("\ntq FoodArea: %f\n", TqFoodArea);
    printf("\ntq GadgetsArea: %f\n", TqGadgetsArea);

    
    double theorical_wait = (1-p_online)*(TsBiglietteria)+
                            TsControlloBiglietti+
                            p_foodArea*(TsCassaFoodArea + TsFoodArea)+
                            p_gadgetsArea*TsGadgetsArea+ p_gadgetsAfterFood*p_foodArea*TsGadgetsArea;
  
    printGreen("\nTheorical wait : ");
    printf("%.6f\n", theorical_wait);
    
  }
  
  return 0.0;
}