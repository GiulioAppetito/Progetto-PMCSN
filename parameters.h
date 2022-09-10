#define SEED 123456789

#define START         0.0                /* initial time                   */
#define STOP_FINITE   60.0               /* terminal (close the door) time */
#define STOP_INFINITE 5000000.0      
#define PUBBLICITY_START (STOP_FINITE-15.0)    
#define INFINITY      (100.0 * STOP_INFINITE)     /* must be much larger than STOP  */

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
#define STREAM_BIGLIETTERIA_UNICA  10 

/* --------------------------------------------
 * routing probabilities for each fasciaOraria
 * --------------------------------------------*/

#define P_ONLINE_1             0.2755
#define P_ONLINE_2             0.3333
#define P_ONLINE_3             0.3064

#define P_GADGETS_AREA_1       0.1376
#define P_GADGETS_AREA_2       0.1138
#define P_GADGETS_AREA_3       0.0890

#define P_FOOD_AREA_1          0.7465
#define P_FOOD_AREA_2          0.7512
#define P_FOOD_AREA_3          0.6354

#define P_GADGETS_AFTER_FOOD_1 0.1879
#define P_GADGETS_AFTER_FOOD_2 0.1525
#define P_GADGETS_AFTER_FOOD_3 0.1341

/* ------------------------------------
 * number of servers for each center
 * ------------------------------------*/
#define SERVERS_BIGLIETTERIA         1  //*2
#define SERVERS_CONTROLLO_BIGLIETTI  2
#define SERVERS_CASSA_FOOD_AREA      1
#define SERVERS_FOOD_AREA            3
#define SERVERS_GADGETS_AREA         2

/* ------------------------------
 * mean arrival rates
 * ------------------------------*/
#define LAMBDA_1 7.166                   
#define LAMBDA_2 6.500
#define LAMBDA_3 3.166

#define NUMBER_OF_EVENTS 12     

/* ------------------------------
 * arrays index for each center
 * ------------------------------*/
#define INDEX_BIGLIETTERIA       0
#define INDEX_CONTROLLOBIGLIETTI 1
#define INDEX_CASSAFOODAREA      2
#define INDEX_FOODAREA           3
#define INDEX_GADGETSAREA        4
#define INDEX_CINEMA             5         

/* ------------------------------
 * finite horizon simulation
 * ------------------------------*/
#define NUM_REPLICATIONS 256            
#define NUM_CENTERS      6
#define NUM_STATS        8
#define SEED             123456789
#define LOC              0.95 
#define OUTPUT_DIRECTORY "./outputStatistics"

/* ------------------------------
 * infinite horizon simulation
 * ------------------------------*/
#define BATCH_SIZE   8192/* b */
#define NUM_BATCHES  256  /* k */


double p_foodArea;
double p_gadgetsArea;
double p_gadgetsAfterFood;
double p_online;
