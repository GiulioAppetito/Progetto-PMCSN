/* corredal functions */

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


void printRed(char *string){
  printf("\033[22;31m%s\n\033[0m",string);
}

int Factorial(int m){
  if(m==0 || m == 1){
    return 1;
  }
  int n = m*Factorial(m-1);
  return n;
}

double SumUp(int m, double ro){
  double sum = 0;
  for(int i = 0; i < m; i++){
    sum += (pow(m*ro,i))/Factorial(i);
  }
  return sum;
}




/* servers functions */

int FindIdleServer(multiserver multiserver[], int servers){
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

double FindNextDeparture(event ms[], int n_servers){
  double departure = INFINITY;

  for(int k=0; k<n_servers; k++){
    if(ms[k].x == 1 && ms[k].t < departure){
      departure = ms[k].t;
    }
  }

  return departure;
}


void resetCenterStats(center *center, int servers, char *name){
  printf("\nreset center stats %s\n", name);
  center->node=0.0;
  center->queue=0.0;
  center->service=0.0;
  center->index=0.0;
  center->servers = servers;
  center->name = name;
  center->firstArrival = INFINITY;
  center->lastArrival = 0.0;
  center->lastService = 0.0;

}

void initServiceData(serviceData *center, double mu, int stream){
  center->mean = mu;
  center->stream = stream;
}

void initEvents(event departures[], int num_servers){
    for(int i=0; i<num_servers; i++){
      departures[i].x = 0;
      departures[i].t = INFINITY;
    }
}