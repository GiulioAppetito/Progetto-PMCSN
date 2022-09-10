#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "DESsimulator.h"
#include "dataStructures.h"
#include "rvms.h"
#include "rvgs.h"
#include "parameters.h"

char *statsNames[NUM_STATS+1] = {"avgInterarrivalTime","avgArrivalRate","avgWait","avgDelay","avgServiceTime","avgNumNode","avgNumQueue","utilization","systemResponseTime"};
FILE *fpWait;

void printGreen(char *string){
    printf("\033[22;32m%s\n\033[0m",string);
}

void writeStatsOnCSV(double intervalMatrix[(NUM_CENTERS)*(NUM_STATS)][3], outputStats matrix[NUM_REPLICATIONS+1][NUM_CENTERS-1], char *FILEPATH){
	//---CREAZIONE DI UN FILE csv
    FILE * fp;
    fp = fopen(FILEPATH, "w");
     
    //---SCRITTURA DEL FILE
    for(int i=0; i<(NUM_CENTERS-1)*NUM_STATS; i++){
        int center = intervalMatrix[i][0];
        double mean = intervalMatrix[i][1];
        double w = intervalMatrix[i][2];
        fprintf(fp, "%s,%s,%.3f,%.3f\n", matrix[0][center].name, statsNames[i%8], mean, w);   
    }

    //---CHIUSURA DEL FILE   
    fclose(fp);

}

confidenceInterval estimate(double array[], int lenght, char *phrase){

  long   n    = 0;                   
  double sum  = 0.0;
  double mean = 0.0;
  double data;
  double stdev;
  double u, t, w;
  double diff;

  for(int i=0; i<lenght; i++) {                 /* use Welford's one-pass method */
    data = array[i];               /* to calculate the sample mean  */
    n++;                                 /* and standard deviation        */
    diff  = data - mean;
    sum  += diff * diff * (n - 1.0) / n;
    mean += diff / n;
  }
  stdev  = sqrt(sum / n);

  if (n > 1) {
    u = 1.0 - 0.5 * (1.0 - LOC);              /* interval parameter  */
    t = idfStudent(n - 1, u);                 /* critical value of t */
    w = t * stdev / sqrt(n - 1);              /* interval half width */
    printf(phrase);
    printf(" based upon %ld data points", n);
    printf(" and with %d%% confidence\n", (int) (100.0 * LOC + 0.5));
    printf("the expected value is in the interval ");
    printf("%.3f +/- %.3f\n", mean, w);
    confidenceInterval interval;
    interval.mean = mean;
    interval.w = w;
    return interval;
  }
  else printf("ERROR - insufficient data\n");
}

void estimateCinemaWait(double array[], int k, char *phrase, int b){

  long   n    = 0;                     /* counts data points */
  double sum  = 0.0;
  double mean = 0.0;
  double data;
  double stdev;
  double u, t, w;
  double diff;

  for(int i=0; i<k; i++) {                 /* use Welford's one-pass method */
    data = array[i];               /* to calculate the sample mean  */
    n++;                                 /* and standard deviation        */
    diff  = data - mean;
    sum  += diff * diff * (n - 1.0) / n;
    mean += diff / n;
  }
  stdev  = sqrt(sum / n);

  if (n > 1) {
    u = 1.0 - 0.5 * (1.0 - LOC);              /* interval parameter  */
    t = idfStudent(n - 1, u);                 /* critical value of t */
    w = t * stdev / sqrt(n - 1);              /* interval half width */
    printf(phrase);
    printf(" based upon %ld data points", n);
    printf(" and with %d%% confidence\n", (int) (100.0 * LOC + 0.5));
    printf("the expected value is in the interval ");
    printf("%f +/- %f\n", mean, w);
    //fprintf(fpWait,"%d,%d,%f,%f\n",b,k,mean,w);


  }
  else printf("ERROR - insufficient data\n");
}

void finiteHorizonSimulation(int fasciaOraria){
        outputStats matrix[NUM_REPLICATIONS+1][NUM_CENTERS-1];
        double percentagesPeopleForPubblicity[NUM_REPLICATIONS];
        double intervalMatrix[(NUM_CENTERS-1)*NUM_STATS][3];
        double resultingRoutingProbabilities[NUM_REPLICATIONS][4];
        double cinemaWait[NUM_REPLICATIONS];

        int i;
        for(i=0; i < NUM_REPLICATIONS; i++){
            printf("Replication %d\n", i);
            percentagesPeopleForPubblicity[i] = simulation(fasciaOraria, matrix[i], NULL,NULL, resultingRoutingProbabilities, 1, 0, 0, i);
        }

        estimate(percentagesPeopleForPubblicity, NUM_REPLICATIONS, "Percentage of people inside for pubblicity");
        

        long   n[NUM_STATS];                    /* counts data points */
        double sum[NUM_STATS];
        double mean[NUM_STATS];
        double data[NUM_STATS];
        double stdev[NUM_STATS];
        double u[NUM_STATS], t[NUM_STATS], w[NUM_STATS];
        double diff[NUM_STATS];

        int center;
        int replication;
        //system("cls");
        printf("\033[22;32m _______________________________ \n\033[0m");
        printf("\033[22;32m|                             | |\n\033[0m");
        printf("\033[22;32m|    GESTIONE DI UN CINEMA    | |\n\033[0m");
        printf("\033[22;32m|        Progetto PMCSN       | |\n\033[0m");
        printf("\033[22;32m|                             | |\n\033[0m");
        printf("\033[22;32m|      Brinati Anastasia      | |\n\033[0m");
        printf("\033[22;32m|       Appetito Giulio       | |\n\033[0m");
        printf("\033[22;32m|_____________________________| |\n\033[0m");
        printf("\033[22;32m|_______________________________|\n\n\033[0m");

        //simulation parameters
        printf("\033[22;32mSTATISTICHE TRANSIENTE (Finite Horizon Simulation)\n\033[0m");
        printf("\033[22;32mFascia oraria .. : \033[0m");
        printf("%d\n",fasciaOraria);
        printf("\033[22;32mReplicazioni ... : \033[0m");
        printf("%d\n",NUM_REPLICATIONS);
        printf("\033[22;32mLOC ............ : \033[0m");
        printf("%.2f\n\n",LOC);

        for(int i=0; i<NUM_STATS; i++){
            n[i] = 0;
            sum[i] = 0.0;
            mean[i] = 0.0;
        }

        
        for(center = 0; center < NUM_CENTERS-1; center++){
            printGreen(matrix[0][center].name);
            for(replication=0; replication < NUM_REPLICATIONS - 1; replication++) { /* use Welford's one-pass method */
                data[0] = matrix[replication][center].avgInterarrivalTime;          
                data[1] = matrix[replication][center].avgArrivalRate;           
                data[2] = matrix[replication][center].avgWait;        
                data[3] = matrix[replication][center].avgDelay; 
                data[4] = matrix[replication][center].avgServiceTime; 
                data[5] = matrix[replication][center].avgNumNode; 
                data[6] = matrix[replication][center].avgNumQueue; 
                data[7] = matrix[replication][center].utilization; 
                                    
                for(int stat = 0; stat<NUM_STATS; stat++){   
                    n[stat]++;                                 /* and standard deviation        */
                    diff[stat]  = data[stat] - mean[stat];
                    sum[stat]  += diff[stat] * diff[stat] * (n[stat] - 1.0) / n[stat];
                    mean[stat] += diff[stat] / n[stat];
                }
                
            }
            for(int stat=0; stat<NUM_STATS; stat++){
                stdev[stat]  = sqrt(sum[stat] / n[stat]);
                if (n[stat] > 1) {
                    u[stat] = 1.0 - 0.5 * (1.0 - LOC);                                /* interval parameter  */
                    t[stat] = idfStudent(n[stat] - 1, u[stat]);                       /* critical value of t */
                    w[stat] = t[stat] * stdev[stat] / sqrt(n[stat] - 1);              /* interval half width */
                    printf("%s based upon %ld data points",statsNames[stat], n[stat]);
                    printf(" and with %d%% confidence\n", (int) (100.0 * LOC + 0.5));
                    printf("the expected value is in the interval\n");
                    printf("%10.3f +/- %6.3f\n\n", mean[stat], w[stat]);

                    //salvo valori intervallo nella matrice usando un offset per ogni centro
                    intervalMatrix[center*NUM_STATS + stat][0] = center;
                    intervalMatrix[center*NUM_STATS + stat][1] = mean[stat];
                    intervalMatrix[center*NUM_STATS + stat][2] = w[stat];
                }   
            }
            //salvataggio intervalli di confidenza su file csv
            writeStatsOnCSV(intervalMatrix, matrix,"C:/Users/Giulio/Desktop/PMCSN/PROGETTO/Progetto-PMCSN.git/trunk/outputStatsFiniteHorizon.csv");

            //azzera le statistiche
            for(int i=0; i<NUM_STATS; i++){
                n[i] = 0;
                sum[i] = 0.0;
                mean[i] = 0.0;
            }
        }

        //cinema response time
        for(int replication=0; replication<NUM_REPLICATIONS; replication++){

            cinemaWait[replication] = (1-(resultingRoutingProbabilities[replication][0]))*(matrix[replication][INDEX_BIGLIETTERIA].avgWait)+
                                matrix[replication][INDEX_CONTROLLOBIGLIETTI].avgWait+
                                (resultingRoutingProbabilities[replication][1])*(matrix[replication][INDEX_CASSAFOODAREA].avgWait + matrix[replication][INDEX_FOODAREA].avgWait)+
                                (resultingRoutingProbabilities[replication][2])*(matrix[replication][INDEX_GADGETSAREA].avgWait)+ (resultingRoutingProbabilities[replication][3]*resultingRoutingProbabilities[replication][1])*matrix[replication][INDEX_GADGETSAREA].avgWait;
        }
        
        printf("\n\n");
        for(int replication = 0; replication < NUM_REPLICATIONS; replication++){
            printf("%f | ",cinemaWait[replication]);
        }printf("\n\n");
        estimateCinemaWait(cinemaWait, NUM_REPLICATIONS, "Cinema response time",NULL);
}

void infiniteHorizonSimulation(int fasciaOraria, int b, int k){
    outputStats matrix[k][NUM_CENTERS-1];                         //matrice per cui riga[i]==batch[i], colonna[j]==centro[j]
    double cinemaWait[k];
    double intervalMatrix[(NUM_CENTERS-1)*(NUM_STATS)][3];    
    double resultingRoutingProbabilities[k][4];                      /* p_online | p_food | p_gadgets | p_gadgetsAfterFood */    
    
    simulation(fasciaOraria, NULL, matrix, cinemaWait, &resultingRoutingProbabilities, 0, b, k, NULL);

    double p_food[k];
    double p_onlineTicket[k];
    double p_gadgets[k];
    double p_gadgetsAndFood[k];

    for(int i=0; i<k; i++){
        p_onlineTicket[i] = resultingRoutingProbabilities[i][0];
        p_food[i] = resultingRoutingProbabilities[i][1];
        p_gadgets[i] = resultingRoutingProbabilities[i][2];
        p_gadgetsAndFood[i] = resultingRoutingProbabilities[i][3];
    }

    printf("\n\p_gadgetsAndFood[] : \n");
    for(int i=0; i<k;i++){
        printf("%.3f | ",p_gadgetsAndFood[i]);
    }
    printf("\n\n");

    confidenceInterval onlineProbabilityInterval;
    confidenceInterval foodProbabilityInterval;
    confidenceInterval gadgetsProbabilityInterval;
    confidenceInterval gadgetsAndFoodProbabilityInterval;
    onlineProbabilityInterval = estimate(p_onlineTicket, k, "Online probability");
    foodProbabilityInterval = estimate(p_food, k, "Food probability");
    gadgetsProbabilityInterval = estimate(p_gadgets, k, "Gadgets probability");
    gadgetsAndFoodProbabilityInterval = estimate(p_gadgetsAndFood, k, "Gadgets and food probability");

    long   n[NUM_STATS];                              /* counts data points */
    double sum[NUM_STATS];
    double mean[NUM_STATS];
    double data[NUM_STATS];
    double stdev[NUM_STATS];
    double u[NUM_STATS], t[NUM_STATS], w[NUM_STATS];
    double diff[NUM_STATS];

    int center;
    int batch;
    int stat;

    for(int i=0; i<NUM_STATS; i++){
        n[i] = 0;
        sum[i] = 0.0;
        mean[i] = 0.0;
    }
    

    //simulation parameters
    printf("\033[22;32mSTATISTICHE STAZIONARIO (Infinite Horizon Simulation)\n\033[0m");
    printf("\033[22;32mFascia oraria ..... : \033[0m");
    printf("%d\n",fasciaOraria);
    printf("\033[22;32mBatch size (b) .... : \033[0m");
    printf("%d\n",b);
    printf("\033[22;32mBatch number (k) .. : \033[0m");
    printf("%d\n\n",k);


    for(center = 0; center < NUM_CENTERS-1; center++){
        printGreen(matrix[0][center].name);
        for(batch = 0; batch < k; batch++){
            data[0] = matrix[batch][center].avgInterarrivalTime;          
            data[1] = matrix[batch][center].avgArrivalRate;           
            data[2] = matrix[batch][center].avgWait;        
            data[3] = matrix[batch][center].avgDelay; 
            data[4] = matrix[batch][center].avgServiceTime; 
            data[5] = matrix[batch][center].avgNumNode; 
            data[6] = matrix[batch][center].avgNumQueue; 
            data[7] = matrix[batch][center].utilization; 
            for(stat = 0; stat < NUM_STATS; stat++){
                n[stat]++;
                diff[stat]  = data[stat] - mean[stat];
                sum[stat]  += diff[stat] * diff[stat] * (n[stat] - 1.0) / n[stat];
                mean[stat] += diff[stat] / n[stat];

            }
        }
        printf("Statistics based upon %ld data points.\nWith %d%% confidence,the expected values are in the intervals:\n\n",n[0],(int) (100.0 * LOC + 0.5));
        for(int stat=0; stat<NUM_STATS; stat++){
                stdev[stat]  = sqrt(sum[stat] / n[stat]);
                if (n[stat] > 1) {
                    u[stat] = 1.0 - 0.5 * (1.0 - LOC);                                /* interval parameter  */
                    t[stat] = idfStudent(n[stat] - 1, u[stat]);                       /* critical value of t */
                    w[stat] = t[stat] * stdev[stat] / sqrt(n[stat] - 1);              /* interval half width */
                    printf("%s\n", statsNames[stat]);
                    printf("%.3f +/- %.3f\n\n", mean[stat], w[stat]);

                    //salvo valori intervallo nella matrice usando un offset per ogni centro
                    intervalMatrix[center*NUM_STATS + stat][0] = center;
                    intervalMatrix[center*NUM_STATS + stat][1] = mean[stat];
                    intervalMatrix[center*NUM_STATS + stat][2] = w[stat];
                    
                    /*
                    printf("|  average interarrival time = %6.3f   |   %6.3f     |\n", obsTime / (center->index), theorical_interarrival);
                    printf("|  average arrival rate .... = %6.3f   |   %6.3f     |\n", center->index / obsTime, theorical_lambda);
                    //printf("   [con t.current] average arrival rate .... = %6.3f\n", center->index  / t.current);

                    // mean wait: E(w) = sum(wi)/n
                    printf("|  average wait ............ = %6.3f   |   %6.3f     |\n", center->node / center->index, theorical_Tq + theorical_meanServiceTime);
                    // mean delay: E(d) = sum(di)/n
                    printf("|  average delay ........... = %6.3f   |   %6.3f     |\n", center->queue / center->index, theorical_Tq);

                    /* consistency check: E(Ts) = E(Tq) + E(s) 

                    // mean service time : E(s) = sum(si)/n
                    printf("|  average service time .... = %6.3f   |   %6.3f     |\n", center->service / center->index, theorical_meanServiceTime);
                    printf("|  average # in the node ... = %6.3f   |   %6.3f     |\n", center->node / obsTime2, theorical_N);
                    printf("|  average # in the queue .. = %6.3f   |   %6.3f     |\n", center->queue / obsTime2, theorical_lambda * theorical_Tq);
                    printf("|  utilization ............. = %6.3f   |   %6.3f     |\n", center->service / obsTime2, theorical_ro);       
                    */

                }   
            }
        
        //salvataggio intervalli di confidenza su file csv
        writeStatsOnCSV(intervalMatrix, matrix, "C:/Users/Giulio/Desktop/PMCSN/PROGETTO/Progetto-PMCSN.git/trunk/outputStatsInfiniteHorizon.csv");

        //azzera le statistiche
        for(int i=0; i<NUM_STATS; i++){
            n[i] = 0;
            sum[i] = 0.0;
            mean[i] = 0.0;
        }
    }
    
    
    //cinema response time
    for(int batch=0; batch<k; batch++){
        cinemaWait[batch] = (1-(resultingRoutingProbabilities[batch][0]))*(matrix[batch][INDEX_BIGLIETTERIA].avgWait)+
                            matrix[batch][INDEX_CONTROLLOBIGLIETTI].avgWait+
                            (resultingRoutingProbabilities[batch][1])*(matrix[batch][INDEX_CASSAFOODAREA].avgWait + matrix[batch][INDEX_FOODAREA].avgWait)+
                            (resultingRoutingProbabilities[batch][2])*(matrix[batch][INDEX_GADGETSAREA].avgWait)+ (resultingRoutingProbabilities[batch][3]*resultingRoutingProbabilities[batch][1])*matrix[batch][INDEX_GADGETSAREA].avgWait;
    }
    estimateCinemaWait(cinemaWait, k, "Cinema response time",b);
    
    printf("\n");
}



int main(void){
    int fasciaOraria;
    int simulazione;
    
    system("cls");
    printf("\033[22;32m _______________________________ \n\033[0m");
    printf("\033[22;32m|                             | |\n\033[0m");
    printf("\033[22;32m|    GESTIONE DI UN CINEMA    | |\n\033[0m");
    printf("\033[22;32m|        Progetto PMCSN       | |\n\033[0m");
    printf("\033[22;32m|                             | |\n\033[0m");
    printf("\033[22;32m|      Brinati Anastasia      | |\n\033[0m");
    printf("\033[22;32m|       Appetito Giulio       | |\n\033[0m");
    printf("\033[22;32m|_____________________________| |\n\033[0m");
    printf("\033[22;32m|_______________________________|\n\n\033[0m");

    PlantSeeds(SEED);
    simSelection:
    printf("\nSelezionare tipo di simulazione :\n 1 -> Orizzone finito\n 2 -> Orizzonte infinito\nSelezione [1 / 2 ] : ");
    scanf("%d",&simulazione);
    printf("\nSelezionare fascia oraria da simulare :\n 1 -> Fascia 1 [15:00-16:00]\n 2 -> Fascia 2 [19:00-20:00]\n 3 -> Fascia 3 [22:00-23:00]\nSelezione [1 / 2 / 3] : ");
    scanf("%d",&fasciaOraria); 

    if(simulazione == 1){
        finiteHorizonSimulation(fasciaOraria);
    }else if(simulazione == 2){
    
        int b = BATCH_SIZE;
        int k = NUM_BATCHES;
        infiniteHorizonSimulation(fasciaOraria,b,k);
    }else{
        printf("Invalid choice.\n");
        goto simSelection;
    }



        
    
    return 0;
    
}