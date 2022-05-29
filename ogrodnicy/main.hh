#pragma once
#ifndef GLOBALH
#define GLOBALH

#define _GNU_SOURCE
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
/* odkomentować, jeżeli się chce DEBUGI */
//#define DEBUG 
/* boolean */
#define TRUE 1
#define FALSE 0
#define ROOT 0

#define SP_TRAWNIK 2
#define SP_PRZYCINANIE 1
#define SP_WYGANIANIE 1

#define LICZBA_OGRODNIKOW 2
// stany w ktorych znajduja sie ogrodnicy + instytut
typedef enum {inInstitute, waitingForJob, waitingForEquipment, workingInGarden, InFinish} state_t;
// rodzaj pracy
enum {obslugaTrawnika, przycinanieZywoplotu, wyganianieSzkodnikow};

extern state_t stan;
extern int rank;
extern int size;
extern int lamportClock; //powielenie definicji w main.cc
extern int cs;
extern int ile_zgod;


extern pthread_mutex_t stateMut;
extern pthread_mutex_t lamportMut;
extern pthread_mutex_t csMut;


// typy wiadmości
#define ACK_NOWE_ZLECENIE_OD_INSTYTUTU 100// tag nowe zlecenie od instytutu
#define REQ_ZLECENIE 110 // tag request o zlecenie
#define ACK_ZLECENIE_ZGODA 120 // tag zgoda lub odmowa -- NIEWIEM
#define ACK_ZLECENIE_ODMOWA 130
#define REQ_SP_TRAWNIK 140 // tag request o sprzet T
#define REQ_SP_PRZYCINANIE 150 // tag request o sprzet P
#define REQ_SP_WYGANIANIE 160 // tag request o sprzet W
#define REL_SP_TRAWNIK 170 // tag release o sprzet T
#define REL_SP_PRZYCINANIE 180 // tag release o sprzet P
#define REL_SP_WYGANIANIE 190 // tag release o sprzet W


/* to może przeniesiemy do global... */
typedef struct {
    int ts;       /* timestamp (zegar lamporta */
    int src;      /* pole nie przesyłane, ale ustawiane w main_loop */
    int zlecenie_id;
    int zlecenie_enum;
    int data;     /* przykładowe pole z danymi; można zmienić nazwę na bardziej pasującą */

} packet_t;
extern MPI_Datatype MPI_PAKIET_T;

#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif



#define P_WHITE printf("%c[%d;%dm",27,1,37);
#define P_BLACK printf("%c[%d;%dm",27,1,30);
#define P_RED printf("%c[%d;%dm",27,1,31);
#define P_GREEN printf("%c[%d;%dm",27,1,33);
#define P_BLUE printf("%c[%d;%dm",27,1,34);
#define P_MAGENTA printf("%c[%d;%dm",27,1,35);
#define P_CYAN printf("%c[%d;%d;%dm",27,1,36);
#define P_SET(X) printf("%c[%d;%dm",27,1,31+(6+X)%7);
#define P_CLR printf("%c[%d;%dm",27,0,37);

#define println(FORMAT, ...) printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, ##__VA_ARGS__, 27,0,37);

void sendPacket(packet_t *pkt, int destination, int tag);
void changeState( state_t );
packet_t *preparePacket(int lamportClock, int zlecenie_id, int zlecenie_enum, int data);

#endif