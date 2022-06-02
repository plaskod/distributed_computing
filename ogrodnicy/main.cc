#include "main.hh"
#include "watek_komunikacyjny.hh"
#include "watek_glowny.hh"
/* wątki */
#include <pthread.h>

state_t stan=waitingForJob;
int size, rank;
MPI_Datatype MPI_PAKIET_T;
pthread_t threadKom;

// mutexy
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER; // na zmiane stanu 
pthread_mutex_t lamportMut = PTHREAD_MUTEX_INITIALIZER; // zmiana zegaru lamporta
pthread_mutex_t csMut = PTHREAD_MUTEX_INITIALIZER; // zmiana rozmiaru sekcji krytycznej
pthread_mutex_t lista_ogloszenMut = PTHREAD_MUTEX_INITIALIZER;

int lamportClock = 0;
int cs = 0;
int ile_zgod = 0;
int timestamps[LICZBA_OGRODNIKOW] = {0};
std::map<int, int> processWaitingForJob, processWaitingForMyEquipment;
std::map<int, int> lista_ogloszen;
std::map<int, zlecenie_t> zlecenia;
zlecenie_t moje_zlecenie = {-1, -1};
void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: %d\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("Brak wsparcia dla wątków, kończę\n");
            /* Nie ma co, trzeba wychodzić */
	    fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
	    MPI_Finalize();
	    exit(-1);
	    break;
        case MPI_THREAD_FUNNELED: 
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	    break;
        case MPI_THREAD_SERIALIZED: 
            /* Potrzebne zamki wokół wywołań biblioteki MPI */
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	    break;
        case MPI_THREAD_MULTIPLE: printf("Pełne wsparcie dla wątków\n");
	    break;
        default: printf("Nikt nic nie wie\n");
    }
}

void inicjuj(int *argc, char ***argv)
{
    int provided;
    MPI_Init_thread(argc, argv,MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);


    /* Stworzenie typu */
    /* Poniższe (aż do MPI_Type_commit) potrzebne tylko, jeżeli
       brzydzimy się czymś w rodzaju MPI_Send(&typ, sizeof(pakiet_t), MPI_BYTE....
    */
    /* sklejone z stackoverflow */
    const int nitems=5; /* bo packet_t ma trzy pola */
    int       blocklengths[5] = {1,1,1,1,1};
    MPI_Datatype typy[5] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint     offsets[5]; 
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, zlecenie_id);
    offsets[3] = offsetof(packet_t, rodzaj_sprzetu);
    offsets[4] = offsetof(packet_t, data);

    MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &MPI_PAKIET_T);
    MPI_Type_commit(&MPI_PAKIET_T);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    srand(rank);

    if(rank!=ROOT){
        pthread_create( &threadKom, NULL, startKomWatek , 0);
        debug("Komuikacja otwarta");
    }
    else { 
        changeState(inInstitute);
        debug("Nowy instytut");
        return;
    }
    debug("Nowy ogrodnik");
}

void finalizuj()
{
    pthread_mutex_destroy( &stateMut);
    pthread_mutex_destroy( &lamportMut);
    pthread_mutex_destroy( &csMut);
    pthread_mutex_destroy( &lista_ogloszenMut);
    /* Czekamy, aż wątek potomny się zakończy */
    println("czekam na wątek \"komunikacyjny\"\n" );
    pthread_join(threadKom,NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}

void sendPacket(packet_t *pkt, int destination, int tag)
{
    
    // bool freepkt=false;
    // if (pkt==0) { pkt = (packet_t*)malloc(sizeof(packet_t)); freepkt=true;}
    // pkt->src = rank;
    pthread_mutex_lock(&lamportMut);
    lamportClock++;
    pkt->ts = lamportClock; // byc moze przypisanie do pkt.ts wartosci lamportClock 
                            // powinno odbyc sie po zamknieciu mutexa
    
    MPI_Send( pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    pthread_mutex_unlock(&lamportMut);
    // if (freepkt) free(pkt);
    
}

void broadcastPacket(packet_t *pkt, int tag){
    for(int i=1 ; i<size ; i++) {
#ifdef DEBUG_BROADCAST
        debug("Sending packet to: %d", 1);
#endif
        sendPacket(pkt, i, tag);
    }
}

void changeState( state_t newState )
{
    pthread_mutex_lock( &stateMut );
    if (stan==InFinish) { 
	    pthread_mutex_unlock( &stateMut );
        return;
    }
    if (stan==workingInGarden && newState==waitingForJob){
        moje_zlecenie.id = -1;
        moje_zlecenie.rodzaj_sprzetu = -1;
    }
    stan = newState;
    pthread_mutex_unlock( &stateMut );
}

packet_t *preparePacket(int lamportClock, int zlecenie_id=-1, int rodzaj_sprzetu=-1, int data=-1){
    packet_t *pkt = (packet_t*)malloc(sizeof(packet_t));
    pkt->ts = lamportClock;
    pkt->src = rank;
    pkt->zlecenie_id = zlecenie_id;
    pkt->rodzaj_sprzetu = rodzaj_sprzetu;
    pkt->data = data;
    return pkt;
}

int main(int argc, char **argv)
{
    /* Tworzenie wątków, inicjalizacja itp */
    inicjuj(&argc,&argv); // tworzy wątek komunikacyjny w "watek_komunikacyjny.cc"
    mainLoop();          // w pliku "watek_glowny.cc"

    finalizuj();
    return 0;
}

