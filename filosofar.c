#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <sys/ipc.h>    
#include <sys/shm.h>    
#include <sys/sem.h>    
#include <signal.h>     
#include <unistd.h>     
#include <errno.h>      
#include "filosofar.h"  
#define CLAVE 41211294392005

void manejador_señales(int);

void manejador_señales(int mem_comp){
	if((shmctl(mem_comp,IPC_RMID,NULL))==-1){
		printf("Error al eliminar la memoria compartida");
	}
	
	kill(getppid(),SIGINT);
}

int main (int argc,char *argv[]){
	int semaforo,mem_comp,ret;
	int num_semaforos,tamano_mem_comp;
	struct DatosSimulaciOn *datos;

	if(argc!=4){
		printf("Debes usar <numFilosofos> <numVueltas> <lentitud>\n");
	}
	
	signal(SIGINT,manejador_señales);
	
	num_semaforos = FI_getNSemAforos();
	tamano_mem_comp = FI_getTamaNoMemoriaCompartida();
	
	if((semaforo = semget(IPC_PRIVATE,num_semaforos,IPC_CREAT | 0600))==-1){
		printf("Error al crear looos semaforos\n");
	}
	if((mem_comp=shmget(IPC_PRIVATE,tamano_mem_comp,IPC_CREAT | 0600))==-1){
		printf("Error al crear la memoria compartida\n");
	}
	
	ret=atoi(argv[1]);
	datos->maxFilOsofosEnPuente=9;
	datos->maxUnaDirecciOnPuente=2;
	datos->sitiosTemplo=15;
	datos->nTenedores=4;
	
	FI_inicio(ret, CLAVE,
              datos, semaforo, mem_comp,
              NULL);
	//el ultimo parametro está a NULL para probar, pero habrá que darle un valor para saber la dirección de los filosofos
	
}
