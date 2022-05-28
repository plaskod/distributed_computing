#include "main.h"
#include "watek_komunikacyjny.h"

void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pakiet;
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while ( stan!=InFinish ) {
	debug("czekam na recv");
        MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        switch ( status.MPI_TAG ) {
	    case FINISH: 
                changeState(InFinish);
	    break;
	    case APPMSG: 
                debug("Dostałem wiadomość od %d z danymi %d",pakiet.src, pakiet.data);
	    break;
	    case INMONITOR: 
                changeState( InMonitor );
                debug("Od tej chwili czekam na polecenia od monitora");
	    break;
	    case INRUN: 
                changeState( InRun );
                debug("Od tej chwili decyzję podejmuję autonomicznie i losowo");
	    break;
	    default:
	    break;
        }
    }
}
