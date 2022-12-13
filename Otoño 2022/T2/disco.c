#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "disco.h"

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t w = PTHREAD_COND_INITIALIZER;

void discoInit(void) {
}

void discoDestroy(void) {
}


int ticketsEntregadosMujeres = 0;
int ticketsEntregadosHombres = 0;
int displayMujeres = 0;
int displayHombres = 0;

char *proximaParejaH;
char *proximaParejaM;

char *dama(char *nom) {
	pthread_mutex_lock(&m);
	char *pareja;
	int ticketPersonal = ticketsEntregadosMujeres++;

	/*
	Espera su turno
	Si displayMujeres y displayHombres son iguales, 
	indican que el turno anterior ya fue resuelto, por lo que se puede continuar
	*/
	while(ticketPersonal != displayMujeres || ticketPersonal != displayHombres) {
		pthread_cond_wait(&w, &m);
	}

	proximaParejaM = nom;

	/* Espera que exista su pareja */
	while(proximaParejaH == NULL) {
		pthread_cond_wait(&w, &m);
	}

	pareja = proximaParejaH;
	proximaParejaH = NULL;
	displayMujeres++;
	pthread_cond_broadcast(&w);
	pthread_mutex_unlock(&m);
	return pareja;
}

char *varon(char *nom) {
	pthread_mutex_lock(&m);
	char *pareja;
	int ticketPersonal = ticketsEntregadosHombres++;

	/*Espera su turno*/
	while(ticketPersonal != displayHombres || ticketPersonal != displayMujeres) {
		pthread_cond_wait(&w, &m);
	}
	proximaParejaH = nom;

	/* Espera que exista su pareja */
	while(proximaParejaM == NULL) {
		pthread_cond_wait(&w, &m);
	}

	displayHombres++;
	pareja = proximaParejaM;
	proximaParejaM = NULL;
	pthread_cond_broadcast(&w);
	pthread_mutex_unlock(&m);
	return pareja;
}
