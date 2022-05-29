#include "main.hh"
#include "watek_komunikacyjny.hh"


void *startKomWatek(void *ptr)
{
    
    MPI_Status status;
    packet_t *pkt;
    // ile_zgod = 0;
    while(1) {
        MPI_Recv(&pkt, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        pthread_mutex_lock(&lamportMut);
        lamportClock = std::max(lamportClock, pkt->ts)+1;
        pthread_mutex_unlock(&lamportMut);

        switch (status.MPI_TAG) {
            
            case ACK_NOWE_ZLECENIE_OD_INSTYTUTU: // chyba tylko niepracujacy ogrodnicy powinni sie ubiegac o to zlecenie, ale moze wystarczy lamport
                for(int i=1 ; i<size ; i++) {
                    sendPacket(pkt, i, REQ_ZLECENIE);
                }
                break;
            case REQ_ZLECENIE: 
                if((pkt->ts < lamportClock) || (pkt->ts == lamportClock && pkt->src < rank)) {
                    // packet_t *pakiet = preparePacket(lamportClock, )
                    sendPacket(pkt, status.MPI_SOURCE, ACK_ZLECENIE_ZGODA);
                }
                break;
            case ACK_ZLECENIE_ZGODA:
                // trzeba zliczac ile zgód się otrzymało
                // ile_zgod++; // jezeli ile_zgod = size - 1 staraj sie wejsc do sekcji krytycznej

                break;
                
            case REL_SP_TRAWNIK:
                pthread_mutex_lock(&csMut);
                cs--; // sekcja krytyczna sie zmniejsza
                pthread_mutex_unlock(&csMut);
                changeState(waitingForJob);
                break;
            case REL_SP_PRZYCINANIE:
                pthread_mutex_lock(&csMut);
                cs--; // sekcja krytyczna sie zmniejsza
                pthread_mutex_unlock(&csMut);
                changeState(waitingForJob);
                break;
            case REL_SP_WYGANIANIE:
                pthread_mutex_lock(&csMut);
                cs--; // sekcja krytyczna sie zmniejsza
                pthread_mutex_unlock(&csMut);
                changeState(waitingForJob);
                break;
            default:
                debug("O panie!");
                break;
        }
    }
}

bool shouldSendReply(packet_t *pkt){
    if(pkt->src == rank){return true;}
    return false;

}

