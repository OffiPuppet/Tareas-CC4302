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

void entrar(int sexo) {
  int opuesto= !sexo;
  START_CRITICAL;
  if (adentro[opuesto] || !nth_emptyQueue((q[opuesto]))) {
    nThread thisTh = nSelf();
    nth_putBack(q[sexo], thisTh);
    suspend(WAIT_SEM);
    schedule();
  }
  else
    adentro[sexo]++;
  END_CRITICAL;
}

void salir(int sexo) {
  int opuesto= !sexo;
  START_CRITICAL;
  adentro[sexo]--;
  if (adentro[sexo]==0) {
    while (!nth_emptyQueue((q[opuesto]))) {
      nThread th = nth_getFront(q[opuesto]);
      setReady(th);
      adentro[opuesto]++;
      schedule();
    }
  }
  END_CRITICAL;
}
