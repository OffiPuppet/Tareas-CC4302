#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

// Use los estados predefinidos WAIT_ACCEDER y WAIT_COMPARTIR
// El descriptor de thread incluye el campo ptr para que Ud. lo use
// a su antojo.

static NthQueue *losQueEsperan;
static nThread elQueComparte = NULL;
static int esperan = 0;


// nth_compartirInit se invoca al partir nThreads para Ud. inicialize
// sus variables globales

void nth_compartirInit(void) {
    losQueEsperan = nth_makeQueue();
}

void nCompartir(void *ptr) {
    START_CRITICAL; //Aquí hay cambio de contexto
    nThread thisTh = nSelf();
    thisTh->ptr = ptr; //usamos el ptr
    elQueComparte = thisTh; //cambiamos nuestro thread que comparte por thisTh
    int nth_length = nth_queueLength(losQueEsperan);
    if (nth_length > 0) { //Si hay esperando
        for(int i = 0; i < nth_length; i++){
            nThread th = nth_getFront(losQueEsperan);
            setReady(th);
        }
    }
    suspend(WAIT_ACCEDER);
    schedule();
    END_CRITICAL;
}

void *nAcceder(int max_millis) {
    START_CRITICAL;
    nThread thisTh = nSelf();
    esperan++;
    if (elQueComparte == NULL) { //queue de si hay compartiendo
        nth_putBack(losQueEsperan, thisTh);
        suspend(WAIT_COMPARTIR);
        schedule();
    }
    void *item = elQueComparte->ptr; //usamos el ptr
    END_CRITICAL;
    return item;
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
