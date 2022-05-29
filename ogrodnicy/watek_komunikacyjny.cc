#include "main.hh"
#include "watek_komunikacyjny.hh"

void *startKomWatek(void *ptr)
{
    MPI_Status status;
    packet_t recv_pkt;
    while(1) {
#ifdef DEBUG_WK
                debug("*** LISTENING ***");
#endif
        MPI_Recv(&recv_pkt, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        pthread_mutex_lock(&lamportMut);
        lamportClock = std::max(lamportClock, recv_pkt.ts)+1;
        pthread_mutex_unlock(&lamportMut);

        switch (status.MPI_TAG) {       
            case NOWE_ZLECENIE_OD_INSTYTUTU: {
#ifdef DEBUG_WK
                debug(">>>Otrzymalem info o nowym zleceniu: %d - ogrodnik potrzebuje zasobu: %d", recv_pkt.zlecenie_id, recv_pkt.zlecenie_enum);
#endif
                packet_t *new_pkt = preparePacket(lamportClock, recv_pkt.zlecenie_id, recv_pkt.zlecenie_enum, -1);
                broadcastPacket(new_pkt, REQ_ZLECENIE);
                break;
            }
            case REQ_ZLECENIE:{    
#ifdef DEBUG_WK
                debug(">>> Otrzymalem REQ_ZLECENIE od: %d", recv_pkt.src);
#endif        
                if(shouldSendReply(recv_pkt)){
#ifdef DEBUG_WK
                debug(">>> Odpowiadam na REQ_ZLECENIE od: %d", recv_pkt.src);
#endif
                    packet_t *new_pkt = preparePacket(lamportClock, recv_pkt.zlecenie_id, recv_pkt.zlecenie_enum, -1);
                    sendPacket(new_pkt,recv_pkt.src, REPLY_ZLECENIE_ZGODA);
                    free(new_pkt);
                }
                else{
                    
#ifdef DEBUG_WK
                    debug("Dodaje proces: %d do kolejki", recv_pkt.src);
#endif
                    processWaitingForJob[recv_pkt.src] = recv_pkt.ts; // queue
                }
                
                break;
            }
            case REPLY_ZLECENIE_ZGODA:{
                // trzeba zliczac ile zgód się otrzymało
                ile_zgod++; // jezeli ile_zgod = size - 1 staraj sie wejsc do sekcji krytycznej
                if(ile_zgod==size-1){
                    changeState(waitingForEquipment);
                    ile_zgod = 0; // to chyba nie sprawdzi sie
                }
                break;
            }
            case REL_SP_TRAWNIK:{
                pthread_mutex_lock(&csMut);
                cs--; // sekcja krytyczna sie zmniejsza
                pthread_mutex_unlock(&csMut);
                changeState(waitingForJob);
                break;
            }
            case REL_SP_PRZYCINANIE:{
                pthread_mutex_lock(&csMut);
                cs--; // sekcja krytyczna sie zmniejsza
                pthread_mutex_unlock(&csMut);
                changeState(waitingForJob);
                break;
            }
            case REL_SP_WYGANIANIE:{
                pthread_mutex_lock(&csMut);
                cs--; // sekcja krytyczna sie zmniejsza
                pthread_mutex_unlock(&csMut);
                changeState(waitingForJob);
                break;
            }
            default:
                debug("O panie!");
                break;
        }
    }
}

bool shouldSendReply(packet_t pkt){
    if(stan==waitingForJob){
        if((pkt.ts < lamportClock) || (pkt.ts == lamportClock && pkt.src < rank)) {
            return true;
        }
    }
    // else{
    //     if(pkt.src == rank){return true;}
    // }
    return false;
}
