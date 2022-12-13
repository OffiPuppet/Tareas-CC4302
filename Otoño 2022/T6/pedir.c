#include <stdio.h>

#include "pss.h"
#include "spin-locks.h"
#include "pedir.h"

// Defina aca sus variables globales
Queue* cola[2];
int mtx;
int busy;
void iniciar() {
  cola[0]=makeQueue();
  cola[1]=makeQueue();
  mtx = OPEN;
  busy = -1;
}

void terminar() {
  destroyQueue(cola[0]);
  destroyQueue(cola[1]);
}

void pedir(int cat) {
  spinLock(&mtx);
  if(busy==-1){
    busy=cat;
    spinUnlock(&mtx);
  }
  else{
    int lk= CLOSED;
    put(cola[cat], &lk);
    spinUnlock(&mtx);
    spinLock(&lk);
  }
}

void devolver() {
  spinLock(&mtx);
  if(!emptyQueue(cola[!busy])){
    int *pw=get(cola[!busy]);
    spinUnlock(pw);
    busy=!busy;
  }
  else if(!emptyQueue(cola[busy])){
    int *pw=get(cola[busy]);
    spinUnlock(pw);
  }
  else{
    busy=-1;
  }
  spinUnlock(&mtx);
}
