#include "main.hh"
#include "watek_komunikacyjny.hh"

void *startKomWatek(void *ptr)
{
    MPI_Status status;
    packet_t recv_pkt;
    int id;
    int rodzaj_sprzetu;

    while(1) {
#ifdef DEBUG_WK
                debug("*** LISTENING ***");
#endif
        MPI_Recv(&recv_pkt, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        pthread_mutex_lock(&lamportMut);
        // lamportClock = std::max(lamportClock, recv_pkt.ts)+1;
        lamportClock = (recv_pkt.ts > lamportClock ? recv_pkt.ts : lamportClock) + 1;
        pthread_mutex_unlock(&lamportMut);

        id = recv_pkt.zlecenie_id;
        rodzaj_sprzetu = recv_pkt.rodzaj_sprzetu;
        
        switch (status.MPI_TAG) {       
            case NOWE_ZLECENIE_OD_INSTYTUTU: {
#ifdef DEBUG_WK
                
                debug(">>>Otrzymalem info o nowym zleceniu: %d - ogrodnik potrzebuje zasobu: %d", id, rodzaj_sprzetu);
#endif
                if(!heardAboutThisJob(recv_pkt)){
#ifdef DEBUG_WK
                
                debug("!!! TUTAJ0");
#endif
                    lista_ogloszen[id] = -1; // dodajemy nowe zlecenie do listy ogloszen
                    zlecenia[id] = {id, rodzaj_sprzetu}; // dodajemy nowe zlecenie do slownika zlecen
                }
#ifdef DEBUG_WK
                
                debug("!!! TUTAJ1");
#endif
                pthread_mutex_lock( &stateMut );
#ifdef DEBUG_WK
                
                debug("!!! TUTAJ2");
#endif
                if(stan == waitingForJob){
#ifdef DEBUG_WK
                    debug("*******Patrze na liste ogloszen");
#endif
                    std::map<int, int>::iterator it = lista_ogloszen.begin();
                    while (it!=lista_ogloszen.end()){
                        if(it->second == -1){
                            int idd = it->first;
#ifdef DEBUG_WK
                            debug("Iteruje po: %d z rodzajem sprzetu: %d", idd, zlecenia[idd].rodzaj_sprzetu);
#endif
                            packet_t *new_pkt = preparePacket(lamportClock, idd, zlecenia[idd].rodzaj_sprzetu, -1);
                            broadcastPacket(new_pkt, REQ_ZLECENIE);
                            lista_ogloszen[idd] = lamportClock;
                            free(new_pkt);
                            break;
                        }
                    }
                }
                pthread_mutex_unlock( &stateMut );

                
//                 if(shouldSendRequest(recv_pkt)){
// #ifdef DEBUG_WK
//                 debug(">>> Wysylam REQ na zlecenie: %d - ogrodnik potrzebuje zasobu: %d", id, rodzaj_sprzetu);
// #endif
//                     packet_t *new_pkt = preparePacket(lamportClock, id, rodzaj_sprzetu, -1);
//                     broadcastPacket(new_pkt, REQ_ZLECENIE);
//                     free(new_pkt);
//                     lista_ogloszen[id] = lamportClock;
//                 }
#ifdef DEBUG_WK
                debug("!!! WYchodze!: %d", recv_pkt.src);
#endif        

                break;
            }
            case REQ_ZLECENIE:{    
#ifdef DEBUG_WK
                debug(">>> Otrzymalem REQ_ZLECENIE od: %d", recv_pkt.src);
#endif        

                if(!heardAboutThisJob(recv_pkt)){
                    lista_ogloszen[id] = -1; 
                    zlecenia[id] = {id, rodzaj_sprzetu};// dodajemy nowe zlecenie do slownika zlecen
                }
                
                if(shouldSendReply(recv_pkt)){
#ifdef DEBUG_WK
                debug(">>> Odpowiadam na REQ_ZLECENIE od: %d", recv_pkt.src);
#endif
                    packet_t *new_pkt = preparePacket(lamportClock, id, rodzaj_sprzetu, -1);
                    sendPacket(new_pkt,recv_pkt.src, REPLY_ZLECENIE_ZGODA);
                    free(new_pkt);
                    pthread_mutex_lock(&lista_ogloszenMut);
                    lista_ogloszen[id] = recv_pkt.ts; // jezeli odpowiadam to nie ubiegam sie
                    pthread_mutex_unlock(&lista_ogloszenMut);
                    
                }
                else{
                    
#ifdef DEBUG_WK
                    debug("Dodaje proces: %d do kolejki", recv_pkt.src);
#endif
                    // processWaitingForJob[recv_pkt.src] = recv_pkt.ts; // TODO: queue
                    // processWaitingForJob.push(recv_pkt); // dane = tag z reply
                }
                
                break;
            }
            case REPLY_ZLECENIE_ZGODA:{
                // trzeba zliczac ile zgód się otrzymało
                ile_zgod++; // jezeli ile_zgod = size - 1 staraj sie wejsc do sekcji krytycznej
#ifdef DEBUG_WK
                    debug("Otrzymalem zgode, w sumie: %d ", ile_zgod);
#endif
                if(ile_zgod==size-1){ // and cs-rozmiar_kolejki!=0
#ifdef DEBUG_WK
                    debug("Zaraz zaczne szukac sprzetu: %d ", ile_zgod);
#endif
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

bool heardAboutThisJob(packet_t pkt){
    pthread_mutex_lock(&lista_ogloszenMut);
    std::map<int, int>::iterator it = lista_ogloszen.begin();
    while (it!=lista_ogloszen.end()){
        if(pkt.zlecenie_id == it->first){
            pthread_mutex_unlock(&lista_ogloszenMut);
            return true;
        }
    }
    pthread_mutex_unlock(&lista_ogloszenMut);
    return false;
}

bool shouldSendRequest(packet_t pkt){
    if(stan==waitingForJob){
        return true;
    }
    return false;
}

bool shouldSendReply(packet_t pkt){
    if(stan==waitingForJob){
        if(rank = pkt.src){ return true;}
        else if((pkt.ts < lamportClock) || (pkt.ts == lamportClock && pkt.src < rank)) {
            return true;
        }
    }
    // else if(stan==waitingForEquipment && pkt.src!=rank){
    //     return true;
    // }

    return false;
}

void updateJobList(packet_t pkt){

}