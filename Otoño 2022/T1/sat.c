#include <pthread.h>
#include <stdio.h>
#include "sat.h"

typedef int (*BoolFun)(int x[]);
int gen(int x[], int i, int n, BoolFun f, int p);

// Función gen secuencial
// Se le agrega como argumento el entero cnt que es el contador.
int gen_sec(int x[], int i, int n, BoolFun f) {
  int cnt=0;
  if (i==n) {
    if ((*f)(x)) {
      cnt++;
      return cnt;
    }
  }
  else {
    x[i]= 0; cnt += gen_sec(x, i+1, n, f); 
    x[i]= 1; cnt += gen_sec(x, i+1, n, f); 
  }
  return cnt;
}

// PASO 1: ESTRUCTURA
typedef struct {
  int *x;
  int i;
  int n;
  int p;
  int cnt;
  BoolFun f;
} Args;

// PASO 2: Función thread 
void *thread_function(void *params) {
  Args *a = (Args *)params;
  int *x = a->x;
  int i = a->i;
  int n = a->n;
  int p = a->p;
  BoolFun f = a->f;
  // Se utiliza la función gen con threads
  a-> cnt = gen(x, i, n, f, p);
  return NULL;
}

// PASO 3: Función principal 
// Función gen con threads
int gen(int x[], int i, int n, BoolFun f, int p) {
  // Caso base: se hace de forma secuencial
  if (p == 0) {
    return gen_sec(x, i, n, f);
  // Caso recursivo
  }
  else { 
    int cnt2 = 0;
    // Copia de arreglo x
    int x_copia[n];
    for (int k = 0; k < n; k++) {
      x_copia[k] = x[k];
    }
    x[i] = 0;
    x_copia[i] = 1;
    
    // Se inicia thread t
    pthread_t t;
    // Se crea estructura con los argumentos correspondientes
    Args args = {x, i + 1, n, f, p - 1, 0};
    pthread_create(&t, NULL, thread_function, &args);
    cnt2 += gen(x_copia, i + 1, n, f, p - 1);
    pthread_join(t, NULL); 
    cnt2 += args.cnt; 
    return cnt2;
  }
}

int recuento(int n, BoolFun f, int p) {
  int i = 0;
  int x[n];
  return (gen(x, i, n, f, p));
}
