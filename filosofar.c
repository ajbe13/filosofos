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

//funcion para registrar la señal SIGINT
void manejador_señales(int mem_comp){
	if((shmctl(mem_comp,IPC_RMID,NULL))==-1){
		perror("Error al eliminar la memoria compartida");
	}
	
	kill(getppid(),SIGINT);
}

int main (int argc,char *argv[]){
	int semaforo,mem_comp,ret,direccion[10]={0},debugInfo[4], idfilosofo;
	int num_semaforos,tamano_mem_comp;
	struct DatosSimulaciOn *datos;
	
	//comprobacion de argumentos que se le pasan al programa
	if(argc!=4){
		fprintf(stderr,"Debes usar <numFilosofos> <numVueltas> <lentitud>\n");
	}
	
	//registrar señal SIGINT
	signal(SIGINT,manejador_señales);
	
	//creacion de semaforos y memoria compartida
	num_semaforos = FI_getNSemAforos();
	tamano_mem_comp = FI_getTamaNoMemoriaCompartida();
	
	if((semaforo = semget(IPC_PRIVATE,num_semaforos,IPC_CREAT | 0600))==-1){
		perror("Error al crear looos semaforos\n");
	}
	if((mem_comp=shmget(IPC_PRIVATE,4*tamano_mem_comp,IPC_CREAT | 0600))==-1){
		perror("Error al crear la memoria compartida\n");
	}
	
	if((datos = malloc(sizeof(struct DatosSimulaciOn)))==NULL){
		perror("Error al reservar memoria\n");
	}
	
	ret=atoi(argv[1]);
	datos->maxFilOsofosEnPuente=9;
	datos->maxUnaDirecciOnPuente=2;
	datos->sitiosTemplo=15;
	datos->nTenedores=4;
	
	//recoger datos y mostrarlos por el canal de errores
	debugInfo[0] = datos->maxFilOsofosEnPuente;
	debugInfo[1] = datos->maxUnaDirecciOnPuente;
	debugInfo[2] = datos->sitiosTemplo;
	debugInfo[3] = datos->nTenedores;
	
	fprintf(stderr,"Datos de la simulacion: \n");
	fprintf(stderr,"Maximos filosofos en puente a la vez: %d\n",debugInfo[0]);
	fprintf(stderr,"Maximo numero de filosofos que van a esperar a la vez de un lado: %d\n", debugInfo[1]);
	fprintf(stderr,"Sitios en el templo: %d\n",debugInfo[2]);
	fprintf(stderr,"Numero de tenedores en el comedor: %d\n",debugInfo[3]);
	
	//inicio, creacion del mapa
	FI_inicio(ret, CLAVE,
              datos, semaforo, mem_comp,
              direccion);
        
        //creacion de los filosofos da error por algo      
	if((idfilosofo = FI_inicioFilOsofo(1))==-1){
		perror("error al crear filosofos\n");
	}
	
	
}
