#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "DESsimulator.h"

int main(void){
    int fasciaOraria;
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

    while(1){
        printf("\nSelezionare fascia oraria da simulare :\n 1 -> Fascia 1 [15:00-16:00]\n 2 -> Fascia 2 [19:00-20:00]\n 3 -> Fascia 3 [22:00-23:00]\nSelezione [1 / 2 / 3] : ");
        scanf("%d",&fasciaOraria);
        simulation(fasciaOraria);
        printf("\nend of simulation\n");
    }
    return 0;
    
}