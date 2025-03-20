#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <sys/ipc.h>    
#include <sys/shm.h>    
#include <sys/sem.h>    
#include <signal.h>     
#include <unistd.h>     
#include <errno.h> 
#include <sys/wait.h>     
#include "filosofar.h"  
#define CLAVE 41211294392005

void manejador_señales(int);

int semaforo,mem_comp;

//funcion para registrar la señal SIGINT
void manejador_señales(int sig){
	fprintf(stderr,"Estoy en la manejadora");
	if((shmctl(mem_comp,IPC_RMID,NULL))==-1){
		perror("Error al eliminar la memoria compartida");
	}
	
	if((semctl(semaforo,0,IPC_RMID))==-1){
		perror("Error al eliminar los semaforos ");
	}
	
	kill(getppid(),SIGINT);
}

int main (int argc,char *argv[]){
	int ret,direccion[10]={0},debugInfo[4], nFilOsofo, filosofos[10],zonaActual, *punt_mem;
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
	
	nFilOsofo = atoi(argv[2]);
	
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
        
        for(int i=0;i<nFilOsofo;i++){
        	filosofos[i] = fork();
        	if(filosofos[i] == 0){
        		//creamos los filosofos
        		FI_inicioFilOsofo(i);
        		//codigo para andar en campo
        		zonaActual=CAMPO;
        		while(zonaActual == CAMPO){
				if((FI_puedoAndar())==100){
				FI_pausaAndar();
				zonaActual = FI_andar();
				}
			}
			//codigo para andar en puente
			zonaActual = PUENTE;
			if(zonaActual == PUENTE){
				while(zonaActual == PUENTE){
					if((FI_puedoAndar())==100){
					FI_pausaAndar();
					zonaActual = FI_andar();
					}
				}
			}
			//codigo para andar en campo despues de puente
			zonaActual=CAMPO;
        		while(zonaActual == CAMPO){
				if((FI_puedoAndar())==100){
				FI_pausaAndar();
				zonaActual = FI_andar();
				}
			}
			//codigo antesala
			zonaActual=ANTESALA;
        		while(zonaActual == ANTESALA){
				if((FI_puedoAndar())==100){
				FI_pausaAndar();
				zonaActual = FI_andar();
				}
			}
			
			FI_entrarAlComedor(1);
			//codigo para andar en la entrada del comedor
			zonaActual=ENTRADACOMEDOR;
        		while(zonaActual == ENTRADACOMEDOR){
				if((FI_puedoAndar())==100){
				FI_pausaAndar();
				zonaActual = FI_andar();
				}
				
				FI_cogerTenedor(TENEDORDERECHO);
				FI_cogerTenedor(TENEDORIZQUIERDO);
				//while(FI_comer() 
			}
			
        	}
        	else{
        		printf("Error al crear filosofo");
        	}
        	
        }
        
        //codigo del padre
        for(int i=0;i<nFilOsofo;i++){
        	waitpid(filosofos[i],NULL,0);
        }
        
        //FI_fin();
	
	
}
