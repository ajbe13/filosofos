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

void liberaripc(void);
void manejador_senales(int);
int crear_semaforos(int);
void crear_mem_comp(int);
void sem_wait(int);
void sem_signal(int);
void coger_tenedores(int);
void dejar_tenedores(int);
int caminar_hasta_silla(int,int);
int entrar_templo(int,int);
int salir_templo(int,int);


union semun{
  int val;
  struct semid_ds *buf;
  unsigned short *array;
};
struct DatosSimulaciOn *datos;


int semaforo,mem_comp;
struct memoriaCompartidaTemplo{
    int puesto1;
    int puesto2;
    int puesto3;
};
struct memoriaCompartidaComedor{
    int puesto1;
    int puesto2;
    int puesto3;
    int puesto4;
};
struct memoriaCompartidaPuente {
    int direccion_actual;       // Dirección actual del puente (1 o -1)
    int filosofos_en_direccion; // Número de filósofos cruzando en una dirección
    int esperando_izq;          // Filosofos esperando cruzar izq->der
    int esperando_der;          // Filosofos esperando cruzar der->izq
};

void entrar_puente(int,struct memoriaCompartidaPuente *);
void salir_puente(struct memoriaCompartidaPuente *);
int coger_sitio(struct memoriaCompartidaComedor *);
void liberar_sitio(int,struct memoriaCompartidaComedor *);
int coger_sTemplo(struct memoriaCompartidaTemplo *);
void liberar_sTemplo(int,struct memoriaCompartidaTemplo *);
int salir_comedor(int,int,struct memoriaCompartidaComedor *);


void liberaripc(){
  if((shmctl(mem_comp,IPC_RMID,NULL))==-1){
    perror("Error al eliminar la memoria compartida");
  }
  
  if((semctl(semaforo,0,IPC_RMID))==-1){
    perror("Error al eliminar los semaforos ");
  }
}
//funcion para registrar la señal SIGINT
void manejador_senales(int sig){
	fprintf(stderr,"Estoy en la manejadora");
	//shmdt(sitios);
	if((shmctl(mem_comp,IPC_RMID,NULL))==-1){
		perror("Error al eliminar la memoria compartida");
	}
	
	if((semctl(semaforo,0,IPC_RMID))==-1){
		perror("Error al eliminar los semaforos ");
	}
	
	kill(getppid(),SIGINT);
    exit(EXIT_SUCCESS);
}

//funcion para crear el array de semaforos
int crear_semaforos(int num_semaforos){
	int sem;
	if((sem = semget(IPC_PRIVATE,num_semaforos,IPC_CREAT | 0600))==-1){
			perror("Error al crear looos semaforos\n");
		}
	return sem;
}

//funcion para crear la memoria compartida
void crear_mem_comp(int tamano_mem_comp){
	if((mem_comp=shmget(IPC_PRIVATE,4*tamano_mem_comp,IPC_CREAT | 0600))==-1){
		perror("Error al crear la memoria compartida\n");
	}
}

//funcion para hacer wait
void sem_wait(int sem_num){
	struct sembuf sops = {sem_num,-1,0};
	semop(semaforo,&sops,1);
}

//funcion para hacer signal
void sem_signal(int sem_num){
	struct sembuf sops = {sem_num,1,0};
	semop(semaforo,&sops,1);
}

void entrar_puente(int dir, struct memoriaCompartidaPuente *mcp) {
    sem_wait(5);  // Semaforo para proteger memoria compartida

    if (dir == 1){ 
        mcp->esperando_izq++;
    }else{ 
        mcp->esperando_der++;
    }

    if (mcp->direccion_actual != 0 && mcp->direccion_actual != dir){
        sem_signal(5);  // Liberar mutex mientras espera
        sem_wait(10);   // Pequeña espera para evitar busy-waiting
        sem_wait(5);
        mcp->direccion_actual=dir;
    }
    // Esperar si hay más de 9 en la misma dirección y hay esperando en la otra
    if ((mcp->filosofos_en_direccion >= 9 && ((dir == 1 && mcp->esperando_der > 0) || (dir == -1 && mcp->esperando_izq > 0)))) {
        sem_signal(5);  // Liberar mutex mientras espera
        usleep(1000);   // Pequeña espera para evitar busy-waiting
        sem_wait(5);
        mcp->direccion_actual=dir;
    }

    // Actualizar dirección y contar filósofos en esa dirección
    if (mcp->direccion_actual == 0){
        sem_wait(10);
        mcp->direccion_actual = dir;     
    }

    mcp->filosofos_en_direccion++;

    if (dir == 1){
        mcp->esperando_izq--;
    }else{ 
        mcp->esperando_der--;
    }

    sem_signal(5); // Liberar controlador de acceso a la memoria compartida
    sem_wait(2);   // Esperar espacio en el puente
}

void salir_puente(struct memoriaCompartidaPuente *mcp) {
    sem_wait(5); // Bloquear acceso a memoria compartida
    mcp->filosofos_en_direccion--;

    // Cambiar dirección si el puente queda vacío y hay filósofos esperando en la otra dirección
    if (mcp->filosofos_en_direccion == 0) {
        if (mcp->esperando_izq > 0){ 
            mcp->direccion_actual = 1;
        }else if (mcp->esperando_der > 0){ 
            mcp->direccion_actual = -1;
        }else{ 
            mcp->direccion_actual = 0;
        }
        sem_signal(10);
    }

    sem_signal(5); // Liberar controlador de acceso a la memoria compartida
    sem_signal(2); // Liberar un espacio en el puente
}


//funcion para coger un sitio
int coger_sitio(struct memoriaCompartidaComedor *mcc) {
    int sitioSeleccionado;
    sem_wait(5);  // Bloquear memoria compartida
    if((mcc->puesto1==-1)&&(mcc->puesto2==-1)&&(mcc->puesto3==-1)&&(mcc->puesto4==-1)){
        sitioSeleccionado = -1;
    }else if(mcc->puesto1==1){
        sitioSeleccionado = 1;
        mcc->puesto1 =  -1;
    }else if(mcc->puesto2==2){
        sitioSeleccionado = 2;
        mcc->puesto2 =  -1;
    }else if(mcc->puesto3==3){
        sitioSeleccionado = 3;
        mcc->puesto3 =  -1;
    }else if(mcc->puesto4==4){
        sitioSeleccionado = 4;
        mcc->puesto4 =  -1;
    }
    sem_signal(5);  // Liberar memoria compartida
    return sitioSeleccionado;
    	    
}


//funcion para liberar el sitio
void liberar_sitio(int sitio,struct memoriaCompartidaComedor *mcc){
    sem_wait(5);
    if(sitio==1){
        mcc->puesto1 =  1;
    }else if(sitio==2){
        mcc->puesto2 =  2;
    }else if(sitio==3){
        mcc->puesto3 =  3;
    }else if(sitio==4){
        mcc->puesto4 =  4;
    }
    sem_signal(5);
}

int caminar_hasta_silla(int sitio, int zonaActual){
    if(sitio==1){
        sem_wait(11);
        if(zonaActual==ENTRADACOMEDOR){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
            sem_signal(3);
            
        }
       if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_signal(11);
        while(zonaActual == ENTRADACOMEDOR){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
        }
    }else if(sitio==2){
        sem_wait(11);
        sem_wait(12);
        sem_wait(13);
        if(zonaActual==ENTRADACOMEDOR){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
            sem_signal(3);
            
        }
        while(zonaActual == ENTRADACOMEDOR){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
        }
        sem_signal(11);
        sem_signal(12);
        sem_signal(13);
    }else if(sitio==3){
        sem_wait(11);
        sem_wait(12);
        if(zonaActual==ENTRADACOMEDOR){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
            sem_signal(3);
            
        }
        while(zonaActual == ENTRADACOMEDOR){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
        }
        sem_signal(11);
        sem_signal(12);
    
    }else{
        sem_wait(11);
        if(zonaActual==ENTRADACOMEDOR){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
            sem_signal(3);
            
        }
        while(zonaActual == ENTRADACOMEDOR){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
        }
        sem_signal(11);
    } 
    return zonaActual;
}

int salir_comedor(int puesto,int zonaActual,struct memoriaCompartidaComedor *mcc){
  int w,x,y,z,m,n,t;
    if(puesto==1){
        if(zonaActual!=PUENTE){ 
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
            liberar_sitio(puesto,mcc);
            sem_signal(4);  //liberar un hueco en el comedor
            for(w=0;w<5;w++){
                if((FI_puedoAndar())==100){
                    FI_pausaAndar();
                    zonaActual = FI_andar();
                }
            }
            sem_wait(15);
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
            sem_signal(15);
        }
    }else if(puesto==2){
        if(zonaActual!=PUENTE){
            sem_wait(14);
            if(zonaActual!=PUENTE){ 
                if((FI_puedoAndar())==100){
                    FI_pausaAndar();
                    zonaActual = FI_andar();
                }
                liberar_sitio(puesto,mcc);
                sem_signal(4);  //liberar un hueco en el comedor
            }
            sem_signal(14);
            for(x=0;x<2;x++){
                if((FI_puedoAndar())==100){
                    FI_pausaAndar();
                    zonaActual = FI_andar();
                }
            }
            sem_wait(15);
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
            sem_signal(15);
        }

    }else if(puesto==3){
        if(zonaActual!=PUENTE){
            sem_wait(13);
            if(zonaActual!=PUENTE){ 
                if((FI_puedoAndar())==100){
                    FI_pausaAndar();
                    zonaActual = FI_andar();
                }
                liberar_sitio(puesto,mcc);
                sem_signal(4);  //liberar un hueco en el comedor
            }
            sem_signal(13);
            for(y=0;y<8;y++){
                if((FI_puedoAndar())==100){
                    FI_pausaAndar();
                    zonaActual = FI_andar();
                }
            }
            sem_wait(14);
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
            sem_signal(14);
            for(z=0;z<2;z++){
                if((FI_puedoAndar())==100){
                    FI_pausaAndar();
                    zonaActual = FI_andar();
                }
            }
            sem_wait(15);
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
            sem_signal(15);
        }
    }else {
        if(zonaActual!=PUENTE){
            sem_wait(12);
            if(zonaActual!=PUENTE){ 
                if((FI_puedoAndar())==100){
                    FI_pausaAndar();
                    zonaActual = FI_andar();
                }
                liberar_sitio(puesto,mcc);
                sem_signal(4);  //liberar un hueco en el comedor
            }
            sem_signal(12);
            for(m=0;m<3;m++){
                if((FI_puedoAndar())==100){
                    FI_pausaAndar();
                    zonaActual = FI_andar();
                }
            }
            sem_wait(13);
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
            sem_signal(13);
            for(n=0;n<8;n++){
                if((FI_puedoAndar())==100){
                    FI_pausaAndar();
                    zonaActual = FI_andar();
                }
            }
            sem_wait(14);
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
            sem_signal(14);
            for(t=0;t<2;t++){
                if((FI_puedoAndar())==100){
                    FI_pausaAndar();
                    zonaActual = FI_andar();
                }
            }
            sem_wait(15);
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
            sem_signal(15);
        }
    }
    return zonaActual;
}

void coger_tenedores(int sitio){
    if(sitio==1){
        FI_cogerTenedor(TENEDORDERECHO);
        sem_wait(6);
		FI_cogerTenedor(TENEDORIZQUIERDO);
    }else if(sitio==2){
        sem_wait(6);
        FI_cogerTenedor(TENEDORDERECHO);
        sem_wait(7);
		FI_cogerTenedor(TENEDORIZQUIERDO);
    }else if(sitio==3){
        sem_wait(7);
        FI_cogerTenedor(TENEDORDERECHO);
        sem_wait(8);
		FI_cogerTenedor(TENEDORIZQUIERDO);
    }else{
        FI_cogerTenedor(TENEDORIZQUIERDO);
        sem_wait(8);
        FI_cogerTenedor(TENEDORDERECHO);
    }
}

void dejar_tenedores(int sitio){
    if(sitio==1){
        FI_dejarTenedor(TENEDORDERECHO);
		FI_dejarTenedor(TENEDORIZQUIERDO);
        sem_signal(6);
    }else if(sitio==2){
        FI_dejarTenedor(TENEDORDERECHO);
        sem_signal(6);
		FI_dejarTenedor(TENEDORIZQUIERDO);
        sem_signal(7);
    }else if(sitio==3){
        FI_dejarTenedor(TENEDORDERECHO);
        sem_signal(7);
		FI_dejarTenedor(TENEDORIZQUIERDO);
        sem_signal(8);
    }else{
        FI_dejarTenedor(TENEDORIZQUIERDO);
        FI_dejarTenedor(TENEDORDERECHO);
        sem_signal(8);
    }
}

int coger_sTemplo(struct memoriaCompartidaTemplo *mct) {
    int sitioSeleccionado;
    sem_wait(5);  // Bloquear memoria compartida
    if((mct->puesto1==-1)&&(mct->puesto2==-1)&&(mct->puesto3==-1)){
        sitioSeleccionado = -1;
    }else if(mct->puesto1==0){
        sitioSeleccionado = 0;
        mct->puesto1 =  -1;
    }else if(mct->puesto2==1){
        sitioSeleccionado = 1;
        mct->puesto2 =  -1;
    }else{
        sitioSeleccionado = 2;
        mct->puesto3 =  -1;
    }
    sem_signal(5);  // Liberar memoria compartida
    return sitioSeleccionado;   	    
}

void liberar_sTemplo(int sitio,struct memoriaCompartidaTemplo *mct){
    sem_wait(5);
    if(sitio==1){
        mct->puesto1 =  0;
    }else if(sitio==2){
        mct->puesto2 =  1;
    }else{
        mct->puesto3 =  2;
    }
    sem_signal(5);
}

int entrar_templo(int sitio,int zonaActual){
	int i;
    if(sitio==0){
        for(i=0;i<2;i++){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
        }
        sem_wait(17);
        if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_signal(17);
        for(i=0;i<4;i++){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
        }
        sem_wait(18);
        if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_signal(18);
    }else if(sitio==1){
        for(i=0;i<2;i++){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
        }
        sem_wait(17);
        if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_signal(17);
    }
    return zonaActual;
}

int salir_templo(int sitio,int zonaActual){  
	int i;
    if(sitio==0){
        for(i=0;i<9;i++){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
        }
        sem_wait(19);
        if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_signal(19);
        for(i=0;i<4;i++){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
        }
        sem_wait(20);
        if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_signal(20);
    }else if(sitio==1){
        for(i=0;i<2;i++){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
        }
        sem_wait(18);
        if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_signal(18);
        if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_wait(19);
        if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_signal(19);
        for(i=0;i<4;i++){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
        }
        sem_wait(20);
        if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_signal(20);
    }else{
        for(i=0;i<2;i++){
            if((FI_puedoAndar())==100){
                FI_pausaAndar();
                zonaActual = FI_andar();
            }
        }
        sem_wait(17);
        if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_signal(17);
        if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_wait(20);
        if((FI_puedoAndar())==100){
            FI_pausaAndar();
            zonaActual = FI_andar();
        }
        sem_signal(20);
    } 
    return zonaActual;
}

int main (int argc,char *argv[]){
	int j,ret,direccion[10]={0},debugInfo[4], nFilOsofo, nVueltas,filosofos[MAXFILOSOFOS],zonaActual, *punt_mem,valor_comer,valor_meditar,sitio, dir, sitioTemplo,i,k;
	int num_semaforos,tamano_mem_comp;
    size_t size;
    union semun sem_union;
    //unsigned short valores_sem[]={1,1,2,4,4,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

	//comprobacion de argumentos que se le pasan al programa
	if(argc!=4){
		fprintf(stderr,"Debes usar <lentitud> <numFilosofos> <numVueltas>\n");
	}
    ret=atoi(argv[1]);
    if(((atoi(argv[2]))<1)||((atoi(argv[2]))>MAXFILOSOFOS)){
        fprintf(stderr,"El numero de filososfos debe estar comprendido entre 1 y 21 \n");
    }else{
	    nFilOsofo = atoi(argv[2]);
    }
	
	//registrar señal SIGINT
	signal(SIGINT,manejador_senales);
	
	//creacion de semaforos y memoria compartida
	num_semaforos = FI_getNSemAforos();
	tamano_mem_comp = FI_getTamaNoMemoriaCompartida();
	
	semaforo = crear_semaforos(num_semaforos+23);
    size=sizeof(struct memoriaCompartidaPuente);
    int sizeMCPuente = (int) size;
    size=sizeof(struct memoriaCompartidaComedor);
    int sizeMCComedor = (int) size;
    size=sizeof(struct memoriaCompartidaTemplo);
    int sizeMCTemplo = (int) size;
	crear_mem_comp(tamano_mem_comp+sizeMCPuente+sizeMCComedor+sizeMCTemplo);
	
	
	sem_union.val=1;
	int ctl=semctl(semaforo,0,SETVAL,sem_union);
	if(ctl == -1){
		perror("Error al inicializar el semaforo semPantalla");
		return 1;
	}
	sem_union.val=1;
	ctl=semctl(semaforo,1,SETVAL,sem_union);
	if(ctl == -1){
		perror("Error al inicializar el semaforo semTotal");
		return 1;
	}
	sem_union.val=2;
    ctl=semctl(semaforo,2,SETVAL,sem_union);
	if(ctl == -1){
		perror("Error al inicializar el semaforo de la capacidad del puente");
		return 1;
	}
	sem_union.val=4;
	ctl=semctl(semaforo,3,SETVAL,sem_union);
	if(ctl == -1){
		perror("Error al inicializar el semaforo de la antesala");
		return 1;
	}
	sem_union.val=4;
    ctl=semctl(semaforo,4,SETVAL,sem_union);
	if(ctl == -1){
		perror("Error al inicializar el semaforo de la entrada al comedor");
		return 1;
	}
	sem_union.val=1;
	ctl=semctl(semaforo,5,SETVAL,sem_union);
	if(ctl == -1){
		perror("Error al inicializar el semaforo de control de la memoria compartida");
		return 1;
    }
    sem_union.val=1;
    ctl=semctl(semaforo,6,SETVAL,sem_union);
	if(ctl == -1){
		perror("Error al inicializar el semaforo del tenedor 2");
		return 1;
    }
    sem_union.val=1;
    ctl=semctl(semaforo,7,SETVAL,sem_union);
	if(ctl == -1){
		perror("Error al inicializar el semaforo del tenedor 3");
		return 1;
    }
    sem_union.val=1;
    ctl=semctl(semaforo,8,SETVAL,sem_union);
	if(ctl == -1){
		perror("Error al inicializar el semaforo del tenedor 4");
		return 1;
    }
    sem_union.val=3;
    ctl=semctl(semaforo,9,SETVAL,sem_union);
	if(ctl == -1){
		perror("Error al inicializar el semaforo del templo");
		return 1;
    }
    sem_union.val=1;
    ctl=semctl(semaforo,10,SETVAL,sem_union);
	if(ctl == -1){
		perror("Error al inicializar el semaforo de la direccion");
		return 1;
    }
    sem_union.val=1;
    for(i=11;i<24;i++){
        ctl=semctl(semaforo,i,SETVAL,sem_union);
	    if(ctl == -1){
		perror("Error al inicializar el semaforo de la direccion");
		return 1;
        }
    }
	//creamos y asociamos la memoria compartida y le damos valor
	void *punteroMC = shmat(mem_comp, 0, 0);
    struct memoriaCompartidaPuente *mcPuente = (struct memoriaCompartidaPuente *) ((char*)punteroMC +4*tamano_mem_comp);
    //Inicializamos valores compartidos
    mcPuente->direccion_actual = 0;
    mcPuente->filosofos_en_direccion = 0;
    mcPuente->esperando_izq = 0;
    mcPuente->esperando_der = 0;

    struct memoriaCompartidaComedor *sitios = (struct memoriaCompartidaComedor *) ((char*)punteroMC +4*tamano_mem_comp+4*sizeMCPuente);
    //Inicializamos valores compartidos
    sitios->puesto1=1;
    sitios->puesto2=2;
    sitios->puesto3=3;
    sitios->puesto4=4;

    struct memoriaCompartidaTemplo *sitiosTemplo = (struct memoriaCompartidaTemplo *) ((char*)punteroMC +4*tamano_mem_comp+4*sizeMCPuente+4*sizeMCComedor);
    //Inicializamos valores compartidos
    sitiosTemplo->puesto1=0;
    sitiosTemplo->puesto2=1;
    sitiosTemplo->puesto3=2;

	if((datos = malloc(sizeof(struct DatosSimulaciOn)))==NULL){
		perror("Error al reservar memoria\n");
	}
	
	nVueltas = atoi(argv[3]);


	//inicio, creacion del mapa
	FI_inicio(ret, CLAVE,datos, semaforo, mem_comp,direccion); 
	
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
        
        for(i=0;i<nFilOsofo;i++){
        	filosofos[i] = fork();
        	if(filosofos[i] == 0){
        		//creamos los filosofos
        		FI_inicioFilOsofo(i);
        		sem_wait(16);
        		if((FI_puedoAndar())==100){
			            FI_pausaAndar();
			            zonaActual = FI_andar();			            
			        }
			        sem_signal(16);
                //codigo para andar en campo
        		zonaActual=CAMPO;
        		while(zonaActual == CAMPO){
        			
			        if((FI_puedoAndar())==100){			          
			            FI_pausaAndar();
			            zonaActual = FI_andar();

			        }
    	        }
                for(j=0;j<nVueltas;j++){
        		    
			        //codigo para andar en puente
			        if(zonaActual == PUENTE){
				        dir=-1;
                        entrar_puente(dir,mcPuente);
                        while(zonaActual == PUENTE){
					        if((FI_puedoAndar())==100){
					        FI_pausaAndar();
					        zonaActual = FI_andar();
					        }
                        }    
                    }
			    

			        //codigo para andar en campo despues de puente
			        if(zonaActual==CAMPO){
                        if((FI_puedoAndar())==100){
                            FI_pausaAndar();
                            zonaActual = FI_andar();
                        }
                        salir_puente(mcPuente);
				        while(zonaActual == CAMPO){
					        if((FI_puedoAndar())==100){
					            FI_pausaAndar();
					            zonaActual = FI_andar();
					        }
				        }
			        }
			        //codigo antesala
			        if(zonaActual==ANTESALA){
                		sem_wait(3); //esperar fuera de la antesala si esta esta llena
				        while(zonaActual == ANTESALA){
					        if((FI_puedoAndar())==100){
					            FI_pausaAndar();
					            zonaActual = FI_andar();
					        }
				        }
                        sem_wait(4); //Esperar en la antesala si el comedor esta lleno
                        sitio=coger_sitio(sitios);
                        if(sitio==-1){
                            do{
                                sitio=coger_sitio(sitios);
                            }while(sitio==-1);
                        } 
			        }
              		if (sitio < 1 || sitio > 4) {
			            fprintf(stderr, "Error: Se intentó entrar al comedor con un sitio inválido (%d)\n", sitio);
			            exit(1);
			        }
			        FI_entrarAlComedor(sitio);

			        //codigo para andar en la entrada del comedor
			        zonaActual=caminar_hasta_silla(sitio,zonaActual);
			        //nos sentamos en el comedor cogemos los cubiertos y comemos
			        if(zonaActual==SILLACOMEDOR){
                        coger_tenedores(sitio);
				        valor_comer = FI_comer();
				        while(valor_comer == SILLACOMEDOR){
					        valor_comer = FI_comer();
				        }
                        dejar_tenedores(sitio);
			        }
			        //salimos del comedor de vuelta al templo
			        if(zonaActual!=PUENTE){ 
                        salir_comedor(sitio,zonaActual,sitios);
                        while(zonaActual!=PUENTE){
				            if((FI_puedoAndar())==100){
					            FI_pausaAndar();
					            zonaActual = FI_andar();
					        }
			            }
                    }
                    //entramos de nuevo al puente
			        if(zonaActual==PUENTE){
                        dir=1;
                        entrar_puente(dir,mcPuente);
				        while(zonaActual == PUENTE){
					        if((FI_puedoAndar())==100){
					            FI_pausaAndar();
					            zonaActual = FI_andar();
					        }
				        }
			        }

			        if(zonaActual==CAMPO){
                        if((FI_puedoAndar())==100){
                            FI_pausaAndar();
                            zonaActual = FI_andar();
                        }
                        salir_puente(mcPuente);
				        while(zonaActual == CAMPO){
					        if((FI_puedoAndar())==100){
					            FI_pausaAndar();
					            zonaActual = FI_andar();
					        }
				        }
                        sem_wait(9);
                        sitioTemplo=coger_sTemplo(sitiosTemplo);
                        if(sitioTemplo==-1){
                            do{
                                sitioTemplo=coger_sTemplo(sitiosTemplo);
                            }while(sitioTemplo==-1);
                        }
						if(sitioTemplo==0){
                            sem_wait(21);
                        }else if(sitioTemplo==1){
                            sem_wait(22);
                        }else if(sitioTemplo==2){
                            sem_wait(23);
                        }
			        }

			        //elegimos sitio del templo y meditamos
			        if(zonaActual==TEMPLO){
				        FI_entrarAlTemplo(sitioTemplo);
						zonaActual=entrar_templo(sitioTemplo,zonaActual);
				        while(zonaActual == TEMPLO){
					        if((FI_puedoAndar())==100){
					            FI_pausaAndar();
					            zonaActual = FI_andar();
					        }
				        }
				        valor_meditar = FI_meditar();
				        while(valor_meditar==SITIOTEMPLO){
					        valor_meditar=FI_meditar();
				        }
                        liberar_sTemplo(sitioTemplo,sitiosTemplo);
                    }
                    
                    if(j!=nVueltas-1){
        		        zonaActual=CAMPO;
                        salir_templo(sitioTemplo,zonaActual);
                        sem_signal(9);
                        if(sitioTemplo==0){
                            sem_signal(21);
                        }else if(sitioTemplo==1){
                            sem_signal(22);
                        }else if(sitioTemplo==2){
                            sem_signal(23);
                        }
        		        while(zonaActual == CAMPO){
				            if((FI_puedoAndar())==100){
				                FI_pausaAndar();
				                zonaActual = FI_andar();
				            }
			            }
                    }
                }
                sem_signal(9);
				if(sitioTemplo==0){
                    sem_signal(21);
                }else if(sitioTemplo==1){
                    sem_signal(22);
                }else if(sitioTemplo==2){
                    sem_signal(23);
                }
                FI_finFilOsofo();
                exit(0);
            }else{
		
        	}	
        }      
        	//codigo del padre
		for(k=0;k<nFilOsofo;k++){
			waitpid(filosofos[k],NULL,0);
		}
		if(k == nFilOsofo){
		liberaripc();
		FI_fin();	
		}
}
