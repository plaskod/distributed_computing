#pragma once
#ifndef GLOBALH
#define GLOBALH

// #define _GNU_SOURCE
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <queue>
#include <vector>
#include <algorithm>
/* odkomentować, jeżeli się chce DEBUGI */
#define DEBUG_WG
#define DEBUG_WK
#define DEBUG_BROADCAST

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
extern int lamportClock; 
extern int cs;
extern int ile_zgod;
extern int ack_counter;
extern std::map <int, std::string> tag2job_name;
extern std::map <int, int> replies;
extern bool readLiterature;

extern pthread_mutex_t stateMut;
extern pthread_mutex_t lamportMut;
extern pthread_mutex_t readingMut;
extern pthread_mutex_t equipmentMut;

// typy wiadmości
#define NOWE_ZLECENIE_OD_INSTYTUTU 100// tag nowe zlecenie od instytutu
#define REQ_ZLECENIE 110 // tag request o zlecenie
#define REPLY_ZLECENIE_ZGODA 120 // tag zgoda lub odmowa
#define REPLY_EQUIPMMENT_ZGODA 130
#define REQ_SP_TRAWNIK 140 // tag request o sprzet T
#define REQ_SP_PRZYCINANIE 150 // tag request o sprzet P
#define REQ_SP_WYGANIANIE 160 // tag request o sprzet W
#define REL_SP_TRAWNIK 170 // tag release o sprzet T
#define REL_SP_PRZYCINANIE 180 // tag release o sprzet P
#define REL_SP_WYGANIANIE 190 // tag release o sprzet W
#define ACK_SPRZET 200
#define REQ_SPRZET 210
#define RELEASE_SPRZET 220

/* to może przeniesiemy do global... */

typedef struct {
    int id;
    int rodzaj_sprzetu;
} zlecenie_t;
extern zlecenie_t moje_zlecenie;

typedef struct {
    int ts;       /* timestamp (zegar lamporta */
    int src;      /* pole nie przesyłane, ale ustawiane w main_loop */
    int zlecenie_id;
    int rodzaj_sprzetu;
    int data;

} packet_t;
extern MPI_Datatype MPI_PAKIET_T;

extern std::map<int, int> lista_ogloszen;
extern std::map<int, zlecenie_t> zlecenia;
// extern std::vector<std::map<int,int>> sprzet;
extern std::map<int, std::map<int, int> > equipmentQueue;

extern std::map<int, int> processWaitingForMyEquipment;
extern std::vector<int> wykonaneZlecenia;



extern zlecenie_t moje_zlecenie;

#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [%d][ts=%d][stan=%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamportClock, stan, ##__VA_ARGS__, 27,0,37);
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
packet_t *preparePacket(int lamportClock, int zlecenie_id, int rodzaj_sprzetu,int data);
void broadcastPacket(packet_t *pkt, int tag);
void print_vector(const std::vector<int> & vec);
#endif