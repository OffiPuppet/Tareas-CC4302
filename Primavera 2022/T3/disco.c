#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "pss.h"
#include "disco.h"

// Defina aca sus variables globales

typedef struct {
    char *dama, *varon;
    int ready;
    pthread_cond_t w;
} Request;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

Queue *damasEnEspera;
Queue *varonesEnEspera;


void DiscoInit(void) {
    //inicializamos las queue
    damasEnEspera = makeQueue();
    varonesEnEspera = makeQueue();
}

void DiscoDestroy(void) {
    //Destruimos las queue para evitar memory leaks
    destroyQueue(damasEnEspera);
    destroyQueue(varonesEnEspera);
}

char *dama(char *nom){
    //Lock
    pthread_mutex_lock(&m);
    //Si hay varones en espera, entonces hacemos lo que sigue
    if (queueLength(varonesEnEspera) != 0){
        //Creamos una estructura tipo request donde hacemos get de los varones en espera
        Request *request = get(varonesEnEspera);
        //Asignamos al parámetro dama del request el nom de la función dama
        request->dama = nom;
        //Cambiamos el estado ready a 1
        request->ready = 1;
        //Creamos una variable tipo char la que contendrá el request->varon
        char *varon = request->varon;
        //Hacemos signal y unlock
        pthread_cond_signal(&request->w);
        pthread_mutex_unlock(&m);
        //Retornamos la variable creada varon
        return varon;
    }
    //En caso contrario, es decir, no hay varones en espera, entonces hacemos esto
    else{
        //Creamos otra estructura request
        Request request = {nom, NULL, 0, PTHREAD_COND_INITIALIZER};
        //Hacemos put de la queue de damas
        put(damasEnEspera, &request);
        while (!request.ready){
          pthread_cond_wait(&request.w, &m);
        }
        //Hacemos unlock
        pthread_mutex_unlock(&m);
        //Retornamos lo que está en el varon de la request
        return request.varon;
    }
}

char *varon(char *nom){
    //Analogo al caso de dama
    pthread_mutex_lock(&m);
    //Si hay damas en espera, entonces hacemos lo que sigue
    if (queueLength(damasEnEspera) != 0){
        //Creamos una estructura tipo request donde hacemos get de las damas en espera
        Request *request = get(damasEnEspera);
        //Asignamos al parámetro varon del request el nom de la función varon
        request->varon = nom;
        //Cambiamos el estado ready a 1
        request->ready = 1;
        //Creamos una variable tipo char la que contendrá el request->dama
        char *dama = request->dama;
        //Hacemos signal y unlock
        pthread_cond_signal(&request->w);
        pthread_mutex_unlock(&m);
        //Retornamos la variable creada dama
        return dama;
    }
    //En caso contrario, es decir, no hay damas en espera, entonces hacemos esto
    else{
        //Creamos otra estructura request
        Request request = {NULL, nom, 0, PTHREAD_COND_INITIALIZER};
        //Hacemos put de la queue de varones
        put(varonesEnEspera, &request);
        while (!request.ready){
          pthread_cond_wait(&request.w, &m);
        }
        //Unlock
        pthread_mutex_unlock(&m);
        //Retornamos lo que está en el dama de la request
        return request.dama;
    }
}
