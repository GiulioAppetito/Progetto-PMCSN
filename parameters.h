#define SEED 123456789

#define START         0.0                /* initial time                   */
#define STOP_FINITE   60.0               /* terminal (close the door) time */
#define STOP_INFINITE 200000.0           /* terminal (close the door) time */
#define INFINITY      (100.0 * STOP)     /* must be much larger than STOP  */

/* ------------------------------
 * stream index for each one
 * ------------------------------*/
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

/* --------------------------------------------
 * routing probabilities for each fasciaOraria
 * --------------------------------------------*/
#define P_GADGETS_AREA_1       0.1376    
#define P_FOOD_AREA_1          0.8441
#define P_GADGETS_AFTER_FOOD_1 0.3000
#define P_GADGETS_AREA_2       0.1136
#define P_FOOD_AREA_2          0.8292
#define P_GADGETS_AFTER_FOOD_2 0.2000
#define P_GADGETS_AREA_3       0.0890
#define P_FOOD_AREA_3          0.7096
#define P_GADGETS_AFTER_FOOD_3 0.100
#define P_ONLINE               0.2

/* ------------------------------------
 * number of servers for each center
 * ------------------------------------*/
#define SERVERS_BIGLIETTERIA         1  
#define SERVERS_CONTROLLO_BIGLIETTI  2
#define SERVERS_CASSA_FOOD_AREA      1
#define SERVERS_FOOD_AREA            4
#define SERVERS_GADGETS_AREA         2

/* ------------------------------
 * mean arrival rates
 * ------------------------------*/
#define LAMBDA_1 7.166                   
#define LAMBDA_2 6.833
#define LAMBDA_3 5.166

#define NUMBER_OF_EVENTS 12     

/* ------------------------------
 * arrays index for each center
 * ------------------------------*/
#define INDEX_BIGLIETTERIA0 0            
#define INDEX_BIGLIETTERIA1 1
#define INDEX_CONTROLLOBIGLIETTI 2
#define INDEX_CASSAFOODAREA 3
#define INDEX_FOODAREA 4
#define INDEX_GADGETSAREA 5

/* ------------------------------
 * finite horizon simulation
 * ------------------------------*/
#define NUM_REPLICATIONS 512            
#define NUM_CENTERS      6
#define NUM_STATS        8
#define SEED             123456789
#define LOC              0.95 
#define OUTPUT_DIRECTORY "./outputStatistics"

/* ------------------------------
 * infinite horizon simulation
 * ------------------------------*/
#define BATCH_SIZE   256 /* b */
#define NUM_BATCHES  64  /* k */

/* ------------------------------
 * output file names
 * ------------------------------*/
#define FILENAME_OUTPUT_FINITEHORIZON   "C:/Users/Giulio/Desktop/PMCSN/PROGETTO/Progetto-PMCSN.git/trunk/outputStatsFiniteHorizon.csv"
#define FILENAME_OUTPUT_INFINITEHORIZON "C:/Users/Giulio/Desktop/PMCSN/PROGETTO/Progetto-PMCSN.git/trunk/outputStatsInfiniteHorizon.csv"