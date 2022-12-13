#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

// Use los estados predefinidos WAIT_ACCEDER, WAIT_ACCEDER_TIMEOUT y
// WAIT_COMPARTIR
// El descriptor de thread incluye el campo ptr para que Ud. lo use
// a su antojo.


// Defina aca sus variables globales.
// Para la cola de esperade nCompartir prefiera el tipo Queue.

static NthQueue *losQueEsperan;
static nThread elQueComparte = NULL;
static int esperan = 0;

// nth_compartirInit se invoca al partir nThreads para que Ud. inicialize
// sus variables globales

void nth_compartirInit(void) {
    losQueEsperan = nth_makeQueue();
}

void nCompartir(void *ptr) {
    START_CRITICAL; //Aquí hay cambio de contexto
    nThread thisTh = nSelf();
    thisTh->ptr = ptr; //usamos el ptr
    elQueComparte = thisTh; //cambiamos nuestro thread que comparte por thisTh
    while(!nth_emptyQueue(losQueEsperan)){ //Mientras que no esté varia la nthQueue de losQueEsperan
        nThread th = nth_getFront(losQueEsperan); //hacemos un nThread th con getFront
        if(th->status == WAIT_ACCEDER_TIMEOUT){ //si el status es WAIT_ACCEDER_TIMEOUT, entonces cancelamos el nThread
            nth_cancelThread(th);
        }
        setReady(th); //seteamos en ready el nThread creado
    }
    suspend(WAIT_COMPARTIR); //hacemos suspend con WAIT_COMPRATIR
    schedule(); //cambio de contexto
    END_CRITICAL;
}

static void f(nThread th) {
  // programe aca la funcion que usa nth_queryThread para consultar si
  // th esta en la cola de espera de nCompartir.  Si esta presente
  // eliminela con nth_delQueue.
  // Ver funciones en nKernel/nthread-impl.h y nKernel/pss.h
    if(nth_queryThread(losQueEsperan, th))
        nth_delQueue(losQueEsperan, th);
}

void *nAcceder(int max_millis) {
  // ...  use nth_programTimer(nanos, f);  f es la funcion de mas arriba
    START_CRITICAL;
    nThread thisTh = nSelf();
    esperan++;
    if (elQueComparte == NULL) { //si no hay nthread compartiendo
        nth_putBack(losQueEsperan, thisTh);
        if(max_millis >= 0){ //si max_millis es mayor a cero
            suspend(WAIT_ACCEDER_TIMEOUT); //hacemos suspend con WAIT_ACCEDER_TIMEOUT
            nth_programTimer(max_millis * (long long)1000000, f); //usamos el programTimer con la conversión 1000000LL
        }
        else{ //si es menor a cero
            suspend(WAIT_ACCEDER); //hacemos suspend con WAIT_ACCEDER
        }
        schedule(); //cambio de contexto
    }
    void *item;
    if(elQueComparte != NULL)//Si hay nthread compartiendo, entonces asignamos ese nthread al item creado
        item = elQueComparte->ptr; //usamos el ptr
    else{ //volvemos a revisar si es que no hay nthread compartiendo
        item = NULL; //seteamos a NULL el item creado y restamos 1 a la cantidad que espera
        esperan--;
        return item;
    }
    END_CRITICAL;
    return item; //retornamos el item
}

void nDevolver(void) {
    START_CRITICAL;
    esperan--;
    if(esperan == 0){
        setReady(elQueComparte);
        elQueComparte = NULL;
    }
    END_CRITICAL;
}
