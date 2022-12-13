// Esta solucion funciona pero no sirve para la tarea 4!
// Esta basada en los monitores y condiciones de nSystem que
// son basicamente mutex y condiciones.
// Tiene que reprogramar este codigo para basarse unicamente
// en las funciones de bajo nivel de nThreads para programar
// herramientas de sincronizacion.  Basese en la implementacion
// de los semaforos, mutex/condiciones y mensajes.


#define _XOPEN_SOURCE 500
#include "nthread-impl.h"


#define entrar nEntrar
#define salir nSalir

static NthQueue *q[2];
static int adentro[2]= { 0, 0 };

void nth_iniPub(void) {
  q[0]= nth_makeQueue();
  q[1]= nth_makeQueue();
}

void nth_endPub(void) {
	nth_destroyQueue(q[0]);
	nth_destroyQueue(q[1]);
}

void wakeUpFun(nThread th) {
	if (nth_queryThread(q[0], th)) {
		nth_delQueue(q[0], th);
	}
	else if (nth_queryThread(q[1], th)){
		nth_delQueue(q[1], th);
	}
}

int nEntrarTimeout(int sexo, long long delayNanos){
	int opuesto= !sexo;
	if (delayNanos == 0) {
  		return 1;
  	}
  	START_CRITICAL;
  	if (adentro[opuesto] || !nth_emptyQueue(q[opuesto])) {
	    nThread thisTh = nSelf();
	    thisTh->send.rc = 1;
	    nth_putBack(q[sexo], thisTh);
	    suspend(WAIT_PUB_TIMEOUT);
	    nth_programTimer(delayNanos, wakeUpFun);
	    schedule();
	    int response = thisTh->send.rc;
	    END_CRITICAL;
	    return response;
  	}
  	else
	    adentro[sexo]++;
  		END_CRITICAL;
  		return 0;
}

void entrar(int sexo) {
  int opuesto= !sexo;
  START_CRITICAL;
  if (adentro[opuesto] || !nth_emptyQueue(q[opuesto])) {
    nThread thisTh = nSelf();
    nth_putBack(q[sexo], thisTh);
    suspend(WAIT_PUB);
    schedule();
  }
  else
    adentro[sexo]++;
  END_CRITICAL;
  return;
}

void salir(int sexo) {
  int opuesto= !sexo;
  START_CRITICAL;
  adentro[sexo]--;
  if (adentro[sexo]==0) {
    while (!nth_emptyQueue(q[opuesto])) {
      nThread th = nth_getFront(q[opuesto]);
      if (th->status == WAIT_PUB_TIMEOUT) {
      	nth_cancelThread(th);
      	th->send.rc = 0;
      }
      setReady(th);
      adentro[opuesto]++;
      schedule();
    }
  }
  END_CRITICAL;
  return;
}
