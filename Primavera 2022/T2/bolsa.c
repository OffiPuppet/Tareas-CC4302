#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "bolsa.h"

// Declare aca sus variables globales

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; //mutex
pthread_cond_t w = PTHREAD_COND_INITIALIZER; //condición

int *purchase = 0; //seteamos con 0 a un puntero a purchase de tipo int
int precioActual = 0; //precio actual de posible venta
char *elVendedor; //name vendedor
char *elComprador; //name comprador

int vendo(int precio, char *vendedor, char *comprador) {
    pthread_mutex_lock(&m); //lock
    if(precio >= precioActual && precioActual != 0){ //Si el precio de un vendedor es mayor al de otro y el precio actual no es 0, entonces hacemos unlock
        pthread_mutex_unlock(&m);
        return 0;
    }
    else{//Caso contrario. Cuando comienza, no hay nadie que venda, por ende precioActual = 0 y entra al if porque se cumple la segunda condición
        if(purchase != 0) //Además, si la purchase is not NULL, lo seteamos a -1 (FALLA)
            *purchase = -1;
        int local_Purchase = 0; //Sino, usando el hint del profe, definimos una variable local, en este caso un int igual a 0 (ESPERANDO)
        precioActual = precio;
        elComprador = comprador; //cambiamos las variables globales por los parámetros de vendo(parametros)
        elVendedor = vendedor;
        //el nuevo purchase es el local
        purchase = &local_Purchase;
        //llamamos a todos con broadcast
        pthread_cond_broadcast(&w);
        while(local_Purchase == 0){ //mientras la local sea 0, los mandamos a mimir(a esperar)
            pthread_cond_wait(&w, &m);
        }
        if(local_Purchase == -1){ //Si la local tiene seteado -1, entonces hacemos unlock
            pthread_mutex_unlock(&m);
            return 0;
        }
        else{//Si es distinto a -1, llamamos a todos y después unlock
            pthread_cond_broadcast(&w);
            pthread_mutex_unlock(&m);
            return 1;
        }
    }
}

int compro(char *comprador, char *vendedor) {
  pthread_mutex_lock(&m); // lock
  if(precioActual != 0){ //Si no es 0, entonces hacemos una nueva variable llamada precio_compra la que almacenará el precio final de la aceptación del TRADE
    int precio_compra = precioActual;
    //Hacemos string copy con strcpy tanto del vendedor(parámetro de compro) como de la global del comprador
    strcpy(elComprador, comprador);
    strcpy(vendedor, elVendedor);
    //cambiamos el valor del puntero a purchase a 1(GANÓ), ya que es el más barato y hay que ahorrar, está todo caro
    *purchase = 1;
    //Luego se sigue con setear los punteros globales a NULL para evitar DATARACES Y DEADLOCKS se hace el cambio de v. globales en compro (como dijo el profe)
    precioActual = 0;
    purchase = NULL;
    elVendedor = NULL;
    //Por último, solo nos queda llamar a todos con broadcast y hacer unlock
    pthread_cond_broadcast(&w);
    pthread_mutex_unlock(&m);
    //Retornamos el precio de la compra
    return precio_compra;
  }
  else{ //Si el precio actual es 0, entonces solo hacemos unlock
    pthread_mutex_unlock(&m);
    return 0;
  }
}
