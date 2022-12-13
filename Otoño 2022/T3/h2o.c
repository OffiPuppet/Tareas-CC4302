#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include "pss.h"
#include "h2o.h"

typedef struct 
{
    Oxygen *o;
    H2O *h2o;
    pthread_cond_t w;
}OxyRequest;

typedef struct 
{
    Hydrogen *h;
    H2O *h2o;
    pthread_cond_t w;
}HydroRequest;

Queue *oxyq;
Queue *hydroq;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void initH2O(void) {
    oxyq = makeQueue();
    hydroq = makeQueue();
}

void endH2O(void) {
    destroyQueue(oxyq);
    destroyQueue(hydroq);
}

H2O *combineOxy(Oxygen *o) {
    pthread_mutex_lock(&m);
    if (queueLength(hydroq) > 1) {
        HydroRequest * h1 = get(hydroq);
        HydroRequest * h2 = get(hydroq);
        H2O * h2o = makeH2O(h1->h, h2->h,o);
        h1->h2o = h2o;
        h2->h2o = h2o;
        pthread_cond_signal(&h1->w);
        pthread_cond_signal(&h2->w);
        pthread_mutex_unlock(&m);
        return h2o;
    }
    else {
        OxyRequest request = {o, NULL, PTHREAD_COND_INITIALIZER};
        put(oxyq, &request);
        while(request.h2o == NULL) {
            pthread_cond_wait(&request.w, &m);
        }
        H2O * h2o = request.h2o;
        pthread_mutex_unlock(&m);
        return h2o;
    }
}

H2O *combineHydro(Hydrogen *h) {
    pthread_mutex_lock(&m);
    if (queueLength(hydroq) > 0 && queueLength(oxyq) > 0) {
        HydroRequest * h1 = get(hydroq);
        OxyRequest * o1 = get(oxyq);
        H2O * h2o = makeH2O(h1->h, h, o1->o);
        h1->h2o = h2o;
        o1->h2o = h2o;
        pthread_cond_signal(&h1->w);
        pthread_cond_signal(&o1->w);
        pthread_mutex_unlock(&m);
        return h2o;
    }
    else {
        HydroRequest request = {h, NULL, PTHREAD_COND_INITIALIZER};
        put(hydroq, &request);
        while(request.h2o == NULL) {
            pthread_cond_wait(&request.w, &m);
        }
        H2O * h2o = request.h2o;
        pthread_mutex_unlock(&m);
        return h2o;
    }
}
