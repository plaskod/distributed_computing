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
                    lista_ogloszen[id] = -1; // dodajemy nowe zlecenie do listy ogloszen
                    zlecenia[id] = {id, rodzaj_sprzetu}; // dodajemy nowe zlecenie do slownika zlecen
                }

//                 pthread_mutex_lock( &stateMut );
                if(stan == waitingForJob){
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
                        it++;
                    }
                }
//                 pthread_mutex_unlock( &stateMut );

                
//                 if(shouldSendRequest(recv_pkt)){
// #ifdef DEBUG_WK
//                 debug(">>> Wysylam REQ na zlecenie: %d - ogrodnik potrzebuje zasobu: %d", id, rodzaj_sprzetu);
// #endif
//                     packet_t *new_pkt = preparePacket(lamportClock, id, rodzaj_sprzetu, -1);
//                     broadcastPacket(new_pkt, REQ_ZLECENIE);
//                     free(new_pkt);
//                     lista_ogloszen[id] = lamportClock;
//                 }
  

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
                    lista_ogloszen[id] = recv_pkt.ts; // jezeli odpowiadam to nie ubiegam sie
                    lista_ogloszen.erease(id);
                    
                }
                else{
                    
#ifdef DEBUG_WK
                    debug("Dodaje proces: %d do kolejki", recv_pkt.src);
#endif

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


                    changeState(waitingForEquipment);
                    ile_zgod = 0;
                    moje_zlecenie.id = id;
                    moje_zlecenie.rodzaj_sprzetu = rodzaj_sprzetu;
#ifdef DEBUG_WK
                    debug("Zaraz zaczne pracę nad id: %d szukac sprzetu: %d ", moje_zlecenie.id, moje_zlecenie.rodzaj_sprzetu);
#endif  
                    packet_t *new_pkt = preparePacket(lamportClock, id, rodzaj_sprzetu, -1);
                    switch(rodzaj_sprzetu){
                        case obslugaTrawnika: {
                            broadcastPacket(new_pkt, REQ_SP_TRAWNIK);
                            break;
                        }

                        case przycinanieZywoplotu: {
                            broadcastPacket(new_pkt, REQ_SP_PRZYCINANIE);
                            break;
                        }

                        case wyganianieSzkodnikow: {
                            broadcastPacket(new_pkt, REQ_SP_WYGANIANIE);
                            break;
                        }
                        default:
                            debug("Jakis dziwny sprzet");
                            break;
                    }
                    
                    free(new_pkt);
                    
                }
                break;
            }
            case REQ_SPRZET:{
                if(shouldGrantEquipment(pkt)){
                    packet_t *new_pkt = preparePacket(lamportClock, id, rodzaj_sprzetu, -1);
                    sendPacket(new_pkt, recv_pkt.src, REPLY_SPRZET);
                    free(new_pkt);
                }
                else{
                    // dodaj do kolejki czekajacych na ten sam sprzet
                    processWaitingForMyEquipment[recv_pkt.src] = rodzaj_sprzetu; // mutex here?
                }
                break;
            }

            case REPLY_SPRZET:{
                
                if(stan==waitingForEquipment){
                    ile_zgod_sprzet++;
                    if(ile_zgod_sprzet==size-1){ // 
#ifdef DEBUG_WK
                        debug("Czekajac na sprzet, odebralem juz wszystkie zgody! Zaraz zacznę pracę");
#endif
                        ile_zgod_sprzet = 0;
                        changeState(workingInGarden);

#ifdef DEBUG_WK
                    debug(">>> Zmieniłem stan na workingInGarden");
#endif       
                    
                }
                else{
                    debug("UWAGA! Nie poszukuje sprzetu!");
                }
                break;
            }
            case REL_SPRZET:{
                breakl
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
    //pthread_mutex_lock(&lista_ogloszenMut);
    std::map<int, int>::iterator it = lista_ogloszen.begin();
    while (it!=lista_ogloszen.end()){
        if(pkt.zlecenie_id == it->first){
            // pthread_mutex_unlock(&lista_ogloszenMut);
            return true;
        }
        it++;
    }
    //pthread_mutex_unlock(&lista_ogloszenMut);
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

    return false;
}

bool shouldGrantEquipment(packet_t pkt){
    if(rank==pkt.src) {return true;}
    else if(stan!=waitingForEquipment){ return true;}
    else if(stan==waitingForEquipment && moje_zlecenie.rodzaj_sprzetu!=pkt.rodzaj_sprzetu){ // mutex here?
        return true;
    }
    return false;
}