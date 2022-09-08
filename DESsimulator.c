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
#include "ourlib.h"
#include "DESsimulator.h"
#include "parameters.h"

//#define DEBUG 
//#define DEBUG2
        
double MU_BIGLIETTERIA       = 3.0;
double MU_CONTROLLOBIGLIETTI = 6.0;      /* da modificare */
double MU_CASSA_FOOD_AREA    = 7.0;
double MU_FOOD_AREA          = 2.0;
double MU_GADGETS_AREA       = 2.0;

double lambda;
double STOP;
double n;


double arrival = START;
int fasciaOraria = 0;

double batchesCounter = 0.0;

double TsBIGLIETTERIAUNICA;
double TsBiglietteria0;
double TsBiglietteria1;
double TsControlloBiglietti;
double TsCassaFoodArea;
double TsFoodArea;
double TsGadgetsArea;


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

void updateIntegralsBIGLIETTERIA(center *biglietteria, center *b1, center *b2){

  biglietteria->number = (b1->number + b2->number);
  if (biglietteria->number > 0)  {        
        /* update integrals  */
        biglietteria->node    += (t.next - t.current) * (biglietteria->number);

        if(t.next != t.current){
          if(biglietteria->number > 2){
            biglietteria->queue   += (t.next - t.current) * (biglietteria->number - 2);
            biglietteria->service += (t.next - t.current)*2;
          }
          if(biglietteria->number == 1){
            biglietteria->service += (t.next - t.current);
          }
          if(biglietteria->number == 2){
            biglietteria->service += (t.next - t.current)*2;
          }
        }              
  }
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

double theoricalLambda(center *center){
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
    if(strcmp(center->name, "biglietteria_0")==0){
      theorical_lambda = lambda*(1-p_online)*0.5;
    }
    if(strcmp(center->name, "biglietteria_1")==0){
      theorical_lambda = lambda*(1-p_online)*0.5;
    }
    if(strcmp(center->name, "cassaFoodArea")==0){
      theorical_lambda = lambda*p_foodArea;
    }
    if(strcmp(center->name, "biglietteria")==0){
      theorical_lambda = lambda*(1-p_online);
    } 
    return theorical_lambda;
}

double theoricalDelaySS(center *center){

  double theorical_lambda = theoricalLambda(center);
  
  double theorical_service = 1 / center->serviceParams->mean;
  /* ro = lambda * E(s) */
  double ro = theorical_lambda*theorical_service;
  // E(Tq) = Pq*E(s) / (1-ro)
  double theorical_delay = (ro*theorical_service)/(1-ro);

  return theorical_delay;

}

double theoricalDelayBIGLIETTERIAUNICA(center *center){

  double theorical_lambda = theoricalLambda(center);
  double theorical_service = 1 / center->serviceParams->mean;
  /* ro = lambda * E(s) */
  double ro = theorical_lambda*theorical_service*0.5;
  // E(Tq) = Pq*E(s) / (1-ro)
  double theorical_delay = (ro*theorical_service)/(1-ro);

  return theorical_delay;

}

double theoricalWaitBIGLIETTERIAUNICA(center *center){
  
  double theorical_Tq = theoricalDelayBIGLIETTERIAUNICA(center);
  double theorical_wait = theorical_Tq + 1/center->serviceParams->mean;
  return theorical_wait;

}

double theoricalWaitSS(center *center){
  
  double theorical_Tq = theoricalDelaySS(center);
  double theorical_wait = theorical_Tq + 1/center->serviceParams->mean;
  return theorical_wait;

}

void printingValues(char *name, outputStats *output, double theorical_lambda, double theorical_Ts, double theorical_Tq, double theorical_meanServiceTime, double theorical_ro){
    
    printf("\nSTATISTICHE\033[22;32m %s\033[0m[Fascia oraria %d] :\n", name, fasciaOraria);
    
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

double visualizeStatistics(outputStats *output,center *center){
    
    double theorical_lambda = theoricalLambda(center);
    double theorical_meanServiceTime = 1/center->serviceParams->mean;
    double theorical_ro = theorical_lambda * theorical_meanServiceTime;
    double theorical_Tq = theoricalDelaySS(center);
    double theorical_Ts = theoricalWaitSS(center);
    
    printingValues(center->name, output, theorical_lambda, theorical_Ts, theorical_Tq, theorical_meanServiceTime, theorical_ro);
    
    return theorical_ro;
}

double theoricalDelayMS(center *center){

  double theorical_lambda = theoricalLambda(center);
  
  double mu = center->serviceParams->mean;
  int m = center->servers;
  /* E(s) = 1 / mu * m */
  double theorical_service = 1 / (mu*m);
  /* ro = lambda * E(s) */
  double ro = theorical_lambda*theorical_service;
  double p_zero = 1 / (  ((pow((m*ro),m)) / ((1-ro)*(Factorial(m))) ) + SumUp(m, ro));
  double p_q = p_zero * ((pow((m*ro),m)) / (Factorial(m)*(1-ro)) );
  // E(Tq) = Pq*E(s) / (1-ro)
  double theorical_delay = (p_q * theorical_service) / (1-ro);

  return theorical_delay;

}

double theoricalWaitMS(center *center){
  
  double mu = center->serviceParams->mean;
  double theorical_delay = theoricalDelayMS(center);
  double theorical_wait = theorical_delay + 1/mu;

  return theorical_wait;

}

double visualizeStatisticsBiglietteria(outputStats *output, center *center){

  double theorical_lambda=theoricalLambda(center);

  double theorical_service = 1 / center->serviceParams->mean;
  /* ro = lambda * E(s) */
  double theorical_ro = theorical_lambda*theorical_service*0.5;
  // E(Tq) = Pq*E(s) / (1-ro)
  double theorical_Tq = theoricalDelayBIGLIETTERIAUNICA(center);
  // E(Ts) = E(si) + E(Tq)
  double theorical_Ts = theoricalWaitBIGLIETTERIAUNICA(center);
    
  printingValues(center->name, output, theorical_lambda, theorical_Ts, theorical_Tq, theorical_service, theorical_ro);

  return theorical_ro;

}

double visualizeStatisticsMultiservers(outputStats *output,center *center){

    double theorical_lambda=theoricalLambda(center);
    int m = center->servers;
    double mu = center->serviceParams->mean;
    /* E(s) = 1 / mu * m */
    double theorical_service = 1 / (mu*m);
    /* ro = lambda * E(s) */
    double theorical_ro = theorical_lambda*theorical_service;
    // E(Tq) = Pq*E(s) / (1-ro)
    double theorical_Tq = theoricalDelayMS(center);
    // E(Ts) = E(si) + E(Tq)
    double theorical_Ts = theoricalWaitMS(center);
    
    printingValues(center->name, output, theorical_lambda, theorical_Ts, theorical_Tq, theorical_service, theorical_ro);

    return theorical_ro;
}

void visualizeRunParameters(double tot, double lambda, double p_foodArea, double p_gadgetsArea, double p_gadgetsAfterFood){
    char a[5];
    strcpy(a, "%");
    printf("\n\033[22;30mSTOP (close the door) time ....... = %.2f\033[0m\n",STOP);
    printf("\033[22;30mNumero totale di arrivi al sistema = %.0f\033[0m\n",tot);
    printf("\033[22;30mLambda ........................... = %.3f job/min\033[0m\n",lambda);
    printf("\033[22;30mp_foodArea ....................... = %.2f %s\033[0m\n",p_foodArea*100,a);
    printf("\033[22;30mp_gadgetsArea .................... = %.2f %s\033[0m\n",p_gadgetsArea*100,a);
    printf("\033[22;30mp_gadgetsAfterFood ............... = %.2f %s\033[0m\n\n",p_gadgetsAfterFood*100,a);
    printf("\033[22;30mServer configuration\nbiglietteria[0] : %d\nbiglietteria[1] : %d\ncontrolloBiglietti : %d\ncassaFoodArea : %d\nfoodArea : %d\ngadgetsArea : %d\n\033[0m\n",SERVERS_BIGLIETTERIA,SERVERS_BIGLIETTERIA,SERVERS_CONTROLLO_BIGLIETTI,SERVERS_CASSA_FOOD_AREA,SERVERS_FOOD_AREA,SERVERS_GADGETS_AREA);

}

outputStats updateStatisticsMS(center *center){
  outputStats output;
  output.name = center->name;
  output.avgInterarrivalTime = ((center->lastArrival - center->firstArrival) / center->index);
  output.avgArrivalRate = center->index / (center->lastArrival - center->firstArrival);
  output.avgWait = (center->node) / center->index;
  output.avgDelay = center->queue / center->index;
  output.avgServiceTime = (center->service/center->servers) / center->index;
  output.avgNumNode = (center->node) / (center->lastService - center->firstArrival);
  output.avgNumQueue = center->queue / (center->lastService - center->firstArrival);
  output.utilization = center->service / (center->servers*(center->lastService - center->firstArrival));
  return output;
}

outputStats updateStatisticsBIGLIETTERIA(center *center){
  outputStats output;
  output.name = center->name;
  output.avgInterarrivalTime = ((center->lastArrival - center->firstArrival) / center->index);
  output.avgArrivalRate = center->index / (center->lastArrival - center->firstArrival);
  output.avgWait = (center->node) / center->index;
  output.avgDelay = (center->queue) / center->index;
  output.avgServiceTime = center->service / center->index;
  output.avgNumNode = (center->node) / (center->lastService - center->firstArrival);
  output.avgNumQueue = center->queue / (center->lastService - center->firstArrival);
  output.utilization = center->service / (center->servers*(center->lastService - center->firstArrival));
  return output;
}

outputStats updateStatisticsSS(center *center){
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

void saveBatchStatsSS(int centerIndex, center *center, outputStats matrix[NUM_BATCHES][NUM_CENTERS-1]){
  //printf("addr center: %p\n", center);
  outputStats output = updateStatisticsSS(center);
  batchesCounter++;
  matrix[batches[centerIndex]][centerIndex] = output; /* save statistics in the matrix cell */
  batches[centerIndex] = batches[centerIndex] + 1;
}
void saveBatchStatsMS(int centerIndex, center *center, outputStats matrix[NUM_BATCHES][NUM_CENTERS-1]){
  outputStats output = updateStatisticsMS(center);
  batchesCounter++;
  matrix[batches[centerIndex]][centerIndex] = output; /* save statistics in the matrix cell */
  batches[centerIndex] = batches[centerIndex] + 1;
 
}



double simulation(int fascia_oraria, outputStats row[], outputStats matrix[NUM_BATCHES][NUM_CENTERS-1], double cinemaWait[NUM_BATCHES], int finite, int b, int k){
  
  // NUM_CENTERS-1 PERCHé UNA SOLA BIGLIETTERIA

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

  //creazione dei centri
  center BIGLIETTERIAUNICA;

  center biglietteria[2];
  center controlloBiglietti;
  center cassaFoodArea;
  center foodArea;
  center gadgetsArea;
  center cinema;

  //inizializzazione dei centri
  resetCenterStats(&BIGLIETTERIAUNICA, SERVERS_BIGLIETTERIA*2, "biglietteria");
  BIGLIETTERIAUNICA.number = 0.0;

  resetCenterStats(&biglietteria[0], SERVERS_BIGLIETTERIA, "biglietteria_0");
  biglietteria[0].number = 0.0;
  resetCenterStats(&biglietteria[1], SERVERS_BIGLIETTERIA, "biglietteria_1");
  biglietteria[1].number = 0.0;
  resetCenterStats(&controlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI, "controlloBiglietti");
  controlloBiglietti.number = 0.0;
  resetCenterStats(&cassaFoodArea, SERVERS_CASSA_FOOD_AREA,"cassaFoodArea");
  cassaFoodArea.number = 0.0;
  resetCenterStats(&foodArea, SERVERS_FOOD_AREA,"foodArea");
  foodArea.number = 0.0;
  resetCenterStats(&gadgetsArea, SERVERS_GADGETS_AREA,"gadgetsArea");
  gadgetsArea.number = 0.0;
  resetCenterStats(&cinema, 0,"cinema");
  cinema.number = 0.0;

  //creazione dei serviceData per ogni centro
  serviceData BIGLIETTERIAUNICAservice;
  
  serviceData biglietteriaService0;
  serviceData biglietteriaService1;
  serviceData controlloBigliettiService;
  serviceData cassaFoodAreaService;
  serviceData foodAreaService;
  serviceData gadgetsAreaService;

  //inizializzazione e assegnamento dei serviceData per ogni centro
  initServiceData(&BIGLIETTERIAUNICAservice, MU_BIGLIETTERIA, STREAM_BIGLIETTERIA_UNICA);
  BIGLIETTERIAUNICA.serviceParams = &BIGLIETTERIAUNICAservice;

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

  //inizializzazione clock
  arrival = START;
  t.current    = START; /* set the clock */
  t.next = 0.0;
  t.last = 0.0;


  event[0].t = GetArrival();  /* genero e salvo il primo arrivo */
  event[0].x   = 1;
  cinema.firstArrival = event[0].t;
  double batchStart = event[0].t;

  int e;                      /* variabili per batch means */
  int i;

  while (
          ((finite) && ((event[0].t < STOP) || ((biglietteria[0].number + biglietteria[1].number + controlloBiglietti.number + cassaFoodArea.number + foodArea.number + gadgetsArea.number) > 0))) ||
          ((!finite) && ( batchesCounter < n))){
    
    
    e = NextEvent(event);                        /* next event index  */

    t.next = event[e].t;                         /* next event time   */
    for(int j=0; j<2; j++){
      updateIntegrals(&biglietteria[j], NULL);
    }

    updateIntegralsBIGLIETTERIA(&BIGLIETTERIAUNICA, &biglietteria[0], &biglietteria[1]);
    updateIntegrals(&controlloBiglietti,controlloBiglietti_MS);
    updateIntegrals(&cassaFoodArea, NULL);
    updateIntegrals(&foodArea, areaFood_MS);
    updateIntegrals(&gadgetsArea, areaGadgets_MS);
    updateIntegrals(&cinema, NULL);
    
    t.current       = t.next;                    /* advance the clock */

    int eventType = e;
    if(eventType == 0){
      if(routingBeforeBiglietteria() == ONLINE){
        cinema.number++;
        cinema.lastArrival = t.current;
        double time = event[0].t;
        event[0].t = GetArrival();    /* generate the next arrival event */
        if (event[0].t > STOP)  {
          t.last      = t.current;
          event[0].x  = 0;
        }
        event[4].t = time;
        event[4].x = 1;
        eventType = 4;
      }
    }
    switch(eventType){
      case 0:                         /* process an arrival to biglietteria */
        cinema.number++;
        cinema.lastArrival = t.current;
        BIGLIETTERIAUNICA.number++;
        BIGLIETTERIAUNICA.lastArrival = t.current;

        i = routingAfterBiglietteria();
        biglietteria[i].number++;
        biglietteria[i].lastArrival = t.current;

        if(t.current < biglietteria[i].firstArrival){
          biglietteria[i].firstArrival = t.current;
          BIGLIETTERIAUNICA.firstArrival = t.current;
        }
        event[0].t = GetArrival();    /* generate the next arrival event */
      
        if (event[0].t > STOP)  {
          t.last      = t.current;
          biglietteria[i].lastArrival = t.current;
          BIGLIETTERIAUNICA.lastArrival = t.current;
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
        biglietteria[0].number--;
        biglietteria[0].lastService = t.current;


        BIGLIETTERIAUNICA.index++;
        BIGLIETTERIAUNICA.number--;
        BIGLIETTERIAUNICA.lastService = t.current;

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
        biglietteria[1].number--;
        biglietteria[1].lastService = t.current;

        
        BIGLIETTERIAUNICA.index++;
        BIGLIETTERIAUNICA.number--;
        BIGLIETTERIAUNICA.lastService = t.current;

        
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
            food++;
            event[6].t = tempo;
            event[6].x = 1;
            break;
          case GADGETSAREA:
            gadgets++;
            event[10].t = tempo;
            event[10].x = 1;

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

        if(t.current < PUBBLICITY_START){
            peopleInsideBeforeStop++;
        }
        cinema.number--;
        cinema.index++;

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
        printf("Default: %d\n", eventType);
        break;
    }
    
    /* infinite simulation */
    if(!finite){
      /*
      if((biglietteria[0].index == b) && (batches[INDEX_BIGLIETTERIA0] < k)){   
        saveBatchStatsSS(INDEX_BIGLIETTERIA0, &biglietteria[0], matrix);
        resetCenterStats(&biglietteria[0], SERVERS_BIGLIETTERIA, "biglietteria_0");     
      }
      if((biglietteria[1].index == b) && (batches[INDEX_BIGLIETTERIA1] < k)){   
        saveBatchStatsSS(INDEX_BIGLIETTERIA1, &biglietteria[1], matrix);
        resetCenterStats(&biglietteria[1], SERVERS_BIGLIETTERIA, "biglietteria_1");      
      }
      */

      if((BIGLIETTERIAUNICA.index == b) && (batches[INDEX_BIGLIETTERIA] < k)){
        saveBatchStatsMS(INDEX_BIGLIETTERIA, &BIGLIETTERIAUNICA, matrix);
        resetCenterStats(&BIGLIETTERIAUNICA, SERVERS_BIGLIETTERIA*2, "biglietteria");
      }
      if((controlloBiglietti.index == b) && (batches[INDEX_CONTROLLOBIGLIETTI] < k)){   /* save statistics for batch */
        saveBatchStatsMS(INDEX_CONTROLLOBIGLIETTI, &controlloBiglietti, matrix);
        resetCenterStats(&controlloBiglietti, SERVERS_CONTROLLO_BIGLIETTI, "controlloBiglietti");      /* reset center statistics */
      }
      if((cassaFoodArea.index == b) && (batches[INDEX_CASSAFOODAREA] < k)){   /* save statistics for batch */
        saveBatchStatsSS(INDEX_CASSAFOODAREA, &cassaFoodArea, matrix);
        resetCenterStats(&cassaFoodArea, SERVERS_CASSA_FOOD_AREA, "cassaFoodArea");      /* reset center statistics */
      }
      if((foodArea.index == b) && (batches[INDEX_FOODAREA] < k)){   /* save statistics for batch */
        saveBatchStatsMS(INDEX_FOODAREA, &foodArea, matrix);
        resetCenterStats(&foodArea, SERVERS_FOOD_AREA, "foodArea");      /* reset center statistics */
      }
      if((gadgetsArea.index == b) && (batches[INDEX_GADGETSAREA] < k)){   /* save statistics for batch */
        saveBatchStatsMS(INDEX_GADGETSAREA, &gadgetsArea, matrix);
        resetCenterStats(&gadgetsArea, SERVERS_GADGETS_AREA, "gadgetsArea");      /* reset center statistics */
      }
      
      if((cinema.index == b) && (batches[INDEX_CINEMA] < k)){
        double obsTime = cinema.lastArrival - batchStart;
        batchStart = t.current;
        double cinemaAvgArrivalRate = cinema.index / obsTime;
        //double cinemaAvgWait = (1-p_online)*;
        //cinemaWait[batches[INDEX_CINEMA]] = cinemaAvgWait;
        
        batches[INDEX_CINEMA]++;
        resetCenterStats(&cinema, 0, "cinema");
        cinema.number = 0.0;
      }
    }
  }


  if(finite){
    visualizeRunParameters(cinema.index, lambda, p_foodArea, p_gadgetsArea, p_gadgetsAfterFood);
    //salvataggio delle statistiche di ogni centro nelle struct outputStats
    outputStats biglietteriaOutput = updateStatisticsBIGLIETTERIA(&BIGLIETTERIAUNICA);

   // outputStats biglietteria0Output = updateStatisticsSS(&biglietteria[0]);
   // outputStats biglietteria1Output = updateStatisticsSS(&biglietteria[1]);
    outputStats controlloBigliettiOutput = updateStatisticsMS(&controlloBiglietti);
    outputStats cassaFoodAreaOutput = updateStatisticsSS(&cassaFoodArea);
    outputStats foodAreaOutput = updateStatisticsMS(&foodArea);
    outputStats gadgetsAreaOutput = updateStatisticsMS(&gadgetsArea);
    
    //salvataggio delle outputStats nella riga della matrix relativa a questa run
    row[INDEX_BIGLIETTERIA] = biglietteriaOutput;

    //row[INDEX_BIGLIETTERIA0] = biglietteria0Output;
    //row[INDEX_BIGLIETTERIA1] = biglietteria1Output;
    row[INDEX_CONTROLLOBIGLIETTI] = controlloBigliettiOutput;
    row[INDEX_CASSAFOODAREA] = cassaFoodAreaOutput;
    row[INDEX_FOODAREA] = foodAreaOutput;
    row[INDEX_GADGETSAREA] = gadgetsAreaOutput;

   

    /*
    printf("\nwait_online_none ....... = %6.3f",wait_online_none);
    printf("\nwait_online_food ....... = %6.3f",wait_online_food);
    printf("\nwait_online_gadgets .... = %6.3f",wait_online_gadgets);
    printf("\nwait_online_all ........ = %6.3f",wait_online_all);
    printf("\nwait_physical_none ..... = %6.3f",wait_physical_none);
    printf("\nwait_physical_food ..... = %6.3f",wait_physical_food);
    printf("\nwait_physical_gadgets .. = %6.3f",wait_physical_gadgets);
    printf("\nwait_physical_all ...... = %6.3f\n",wait_physical_all);

    double avgWaitOnline = waitControlloBiglietti + p_foodArea*(waitCassaFoodArea + waitFoodArea + p_gadgetsAfterFood*waitGadgetsArea) + p_gadgetsArea*(waitGadgetsArea);
    printf("\nAverage wait online = %6.3f\n",avgWaitOnline);
    double avgWaitPhysical = waitBiglietteria + waitControlloBiglietti + p_foodArea*(waitCassaFoodArea + waitFoodArea + p_gadgetsAfterFood*waitGadgetsArea) + p_gadgetsArea*(waitGadgetsArea);
    printf("Average wait physical = %6.3f\n",avgWaitPhysical);
    */
  

    //visualizzazione a schermo delle statistiche dei centri
    visualizeStatisticsBiglietteria(&biglietteriaOutput,&BIGLIETTERIAUNICA);

    //visualizeStatistics(&biglietteria0Output,&biglietteria[0]);
    //visualizeStatistics(&biglietteria1Output,&biglietteria[1]);
    visualizeStatisticsMultiservers(&controlloBigliettiOutput,&controlloBiglietti);
    visualizeStatistics(&cassaFoodAreaOutput,&cassaFoodArea);
    visualizeStatisticsMultiservers(&foodAreaOutput,&foodArea);
    visualizeStatisticsMultiservers(&gadgetsAreaOutput,&gadgetsArea);


    printf("\npeople that got in : %f", peopleInsideBeforeStop);
    printf("\npeople in total: %f", cinema.index);
    
    double perc = (peopleInsideBeforeStop/cinema.index)*100;
    printGreen("\n% people inside before the pubblicity starts : ");
    printf("%.3f\n", perc);

    return perc;

}

    /* theorical wait calcolato con formula diversa */
    /*
    double L = 0.0;
    for(int i=0; i<6; i++){
      L += ro[i]/(1-ro[i]);
    }
    printf("\n\n\nE[L] = %f\n",L);
    printf("E[W] = %f\n\n",L/7.166);

    */
  if(!finite){
    TsBIGLIETTERIAUNICA = theoricalWaitBIGLIETTERIAUNICA(&BIGLIETTERIAUNICA);

    TsBiglietteria0 = theoricalWaitSS(&biglietteria[0]);
    TsBiglietteria1 = theoricalWaitSS(&biglietteria[1]);

    TsControlloBiglietti = theoricalWaitMS(&controlloBiglietti);
    TsCassaFoodArea = theoricalWaitSS(&cassaFoodArea);
    TsFoodArea = theoricalWaitMS(&foodArea);
    TsGadgetsArea = theoricalWaitMS(&gadgetsArea);

    double theorical_wait = (1-p_online)*(0.5*TsBiglietteria0 + 0.5*TsBiglietteria1)+
                            TsControlloBiglietti+
                            p_foodArea*(TsCassaFoodArea + TsFoodArea)+
                            p_gadgetsArea*TsGadgetsArea+ p_gadgetsAfterFood*p_foodArea*TsGadgetsArea;
    
    double theorical_wait_2 = (1-p_online)*(TsBIGLIETTERIAUNICA)+
                            TsControlloBiglietti+
                            p_foodArea*(TsCassaFoodArea + TsFoodArea)+
                            p_gadgetsArea*TsGadgetsArea+ p_gadgetsAfterFood*p_foodArea*TsGadgetsArea;
  
    printGreen("\nTheorical wait : ");
    printf("%.6f\n", theorical_wait);
    printGreen("\nTheorical wait 2: ");
    printf("%.6f\n", theorical_wait_2);
  }
  
 


  return 0.0;
}