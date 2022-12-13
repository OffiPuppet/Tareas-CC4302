#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "disk.h"
#include "priqueue.h"
#include "spinlocks.h"

// Le sera de ayuda la clase sobre semáforos:
// https://www.u-cursos.cl/ingenieria/2022/2/CC4302/1/novedades/detalle?id=431689
// Le serviran la solucion del productor/consumidor resuelto con el patron
// request y la solucion de los lectores/escritores, tambien resuelto con
// el patron request.  Puede substituir los semaforos de esas soluciones
// por spin-locks, porque esos semaforos almacenan a lo mas una sola ficha.


// Holi, estaba haciendo la tarea con Queue's y me tira error (dice que debe ser PriQueue)
// no entiendo porqué, pero lo usaré uwu


//  Definir estructura solo con un w y track
typedef struct {
    int w;
    int track;
} Request;

// Declare los tipos que necesite

PriQueue *mayor_igual_track; //Hacemos las dos colas del enunciado con PriQueue
PriQueue *menor_track;

// Definimos 3 int que nos servirán uno para el spinLock/Unlock, otro para condiciones
// y el último para almacenar la última track
 
int mtx;
int ocupado;
int last_track;

// Agregue aca las funciones requestDisk y releaseDisk

void iniDisk(void) {
    // ... inicializa las variables globales que cree, en este caso las PriQueue
    // el mtx con OPEN, y ocupado/last_track con 0
    mayor_igual_track = makePriQueue();
    menor_track = makePriQueue();
    mtx = OPEN;
    ocupado = last_track = 0;
}

void requestDisk(int track) {
    spinLock(&mtx); // Hacemos el spinLock SIEMPRE al inicio uwu
    if(ocupado){ // Si está ocupado, entonces
        Request req = {CLOSED, track}; // Hacemos una estructura con CLOSED y track
        // (el ingresado en la función)
        if (track < last_track) { // Si el track ingresado es menor que el último, entonces
        // hacemos priPut de la PriQueue menor_track con la dir de req y track
            priPut(menor_track, &req, track);
        } // Sino, entonces lo mismo que arriba pero con la otra PriQueue
        else{
            priPut(mayor_igual_track, &req, track);
        }
        // Luego, hacemos spinUnlock con mtx y hacemos spinLock con el w que está en la estructura
        spinUnlock(&mtx);
        spinLock(&req.w);
        return; // Retornamos, en este caso nada, sino arroja error, ya que es tipo void la función
    }
    else{ // Si no está ocupado, entonces el last_track será el track ingresado y a ocupado se le cambia a 1
        last_track = track;
        ocupado = 1;
    }
    spinUnlock(&mtx); // Y al final también hacemos spinUnlock y retornamos
    return;
}

void releaseDisk() {
    spinLock(&mtx); // Hacemos el spinLock SIEMPRE al inicio nwn
    if(!emptyPriQueue(mayor_igual_track)){ // Si la PriQueue mayor_igual_track está con elementos, entonces
        Request *req = priGet(mayor_igual_track); // Hacemos un tipo request con priGet de la cola con los mayores o iguales
        last_track = req->track; // Cambiamos el valor del último con el que tenemos en la request
        spinUnlock(&req->w); // Hacemos spinUnlock del &req->w
    }
    else if(!emptyPriQueue(menor_track)){ // Ahora, si la cola que no está vacía es la de los menores, entonces
        Request *req = priGet(menor_track); // Hacemos una request con priGet de la cola con los menores
        last_track = req->track; // Cambiamos el valor del último con el que tenemos en la request
        while (!emptyPriQueue(menor_track)) { // Mientras la cola menor no esté vacía
            Request *req = priGet(menor_track); // Hacemos una request con priGet de la cola con los menores
            priPut(mayor_igual_track, req, req->track); // Y priPut de la mayor o iguales con req y req->track
        }
        spinUnlock(&req->w); // Hacemos spinUnlock con req->w
    }
    else{ //Caso contrario, entonces ocupado cambia a 0
        ocupado = 0;
    }
    //Hacemos el último spinLock con mtx y terminamos uwu
    spinUnlock(&mtx);    
}
