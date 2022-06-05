#include "main.hh"
#include "watek_komunikacyjny.hh"

void *startKomWatek(void *ptr)
{
    MPI_Status status;
    packet_t recv_pkt;
    int id;
    int rodzaj_sprzetu;
    sleep(1 + rand() % 3);
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

#ifdef DEBUG_WK
                
                debug(">>>Widze nowe zlecenie: %d dodaje na liste ogloszen", id);
#endif
                    lista_ogloszen[id] = -1; // dodajemy nowe zlecenie do listy ogloszen
                    zlecenia[id] = {id, rodzaj_sprzetu}; // dodajemy nowe zlecenie do slownika zlecen
                    replies[id] = 0;
                }
                else{
                    break;
                }


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
                            lista_ogloszen[idd] = lamportClock; // nie usuwam z listy ogloszen, kazdy kolejny REQ ponownie by to dodawal
                            free(new_pkt);
                            goto outer;
                        }
                        it++;
                    }
                }

                outer:
                break;
            }
            case REQ_ZLECENIE:{    
#ifdef DEBUG_WK
                debug("------------------------------- Otrzymalem REQ_ZLECENIE od: %d z ts=%d na zlecenie: %d", recv_pkt.src, recv_pkt.ts, id);
#endif        

                if(!heardAboutThisJob(recv_pkt)){
#ifdef DEBUG_WK
                debug("************ Pierwszy raz slysze o zleceniu %d od: %d", id, recv_pkt.src);
#endif        
                    lista_ogloszen[id] = -1; 
                    zlecenia[id] = {id, rodzaj_sprzetu};// dodajemy nowe zlecenie do slownika zlecen
                    replies[id] = 0;
                }
                
                if(shouldSendReply(recv_pkt)){
#ifdef DEBUG_WK
                debug("------------------------------- Odpowiadam REPLY_ZLECENIE na REQ_ZLECENIE od: %d z ts=%d na zlecenie: %d", recv_pkt.src, recv_pkt.ts, id);
#endif
                    packet_t *new_pkt = preparePacket(lamportClock, id, rodzaj_sprzetu, -1);
                    sendPacket(new_pkt,recv_pkt.src, REPLY_ZLECENIE_ZGODA);
                    free(new_pkt);
                    lista_ogloszen[id] = recv_pkt.ts; // zaznaczam

                    if(recv_pkt.src!=rank){
#ifdef DEBUG_WK
                        debug("------------------------------- USUWAM zlecenie: %d, zegar lamporta ogrodnika: %d == %d", id, recv_pkt.src, recv_pkt.ts);
#endif
                        replies[id] = 0;
                    }

                    
                    // lista_ogloszen.erease(id); // jezeli odpowiadam to nie ubiegam sie
                    
                }
                else{
                    
#ifdef DEBUG_WK
                    debug("------------------------------- IGNORUJE ogrodnika: %d z z ts=%d, ubiegajacego sie o zlecenie %d",recv_pkt.src, recv_pkt.ts, id);
#endif
                    
                }
                
                break;
            }
            case REPLY_ZLECENIE_ZGODA:{
                // trzeba zliczac ile zgód się otrzymało
                replies[id] = replies[id]+1;
                // ile_zgod++; // jezeli ile_zgod = size - 1 staraj sie wejsc do sekcji krytycznej
#ifdef DEBUG_WK
                    debug("------------------------------- Otrzymalem zgode na zadanie %d od %d z ts=%d, w sumie: %d z %d", id, recv_pkt.src, recv_pkt.ts, replies[id], size-1);
#endif
                if(replies[id]==size-1 && stan==waitingForJob){ // and cs-rozmiar_kolejki!=0
                    changeState(waitingForEquipment);
                    // replies[id] = 0;
                    moje_zlecenie.id = id;
                    moje_zlecenie.rodzaj_sprzetu = rodzaj_sprzetu;
#ifdef DEBUG_WK
                    debug("------------------------------- Zaraz zaczne pracę nad id: %d szukac sprzetu: %d ", moje_zlecenie.id, moje_zlecenie.rodzaj_sprzetu);
#endif  
                    packet_t *new_pkt = preparePacket(lamportClock, id, rodzaj_sprzetu, -1);
                    broadcastPacket(new_pkt, REQ_SPRZET);
#ifdef DEBUG_WK
                    debug("------------------------------- BROADCAST REQ_SPRZET");
#endif
                    pthread_mutex_lock(&readingMut);
                    readLiterature = true;
                    pthread_mutex_unlock(&readingMut);
                    free(new_pkt);

                }
                break;
                
            }

            case REQ_SPRZET:{
#ifdef DEBUG_WK
                    debug("-------------------------------------------------------------- Otrzymalem REQ_SPRZET od %d z ts=%d", recv_pkt.src, recv_pkt.ts);
#endif
                if(shouldGrantEquipment(recv_pkt)){
#ifdef DEBUG_WK
                    debug("-------------------------------------------------------------- Udzielam REPLY_SPRZET ogrodnikowi: %d z ts=%d", recv_pkt.src, recv_pkt.ts);
#endif
                    packet_t *new_pkt = preparePacket(lamportClock, id, rodzaj_sprzetu, -1);
                    sendPacket(new_pkt, recv_pkt.src, REPLY_SPRZET);
                    free(new_pkt);
                }
                else{  // dodaj do kolejki czekajacych na ten sam sprzet
#ifdef DEBUG_WK
                    debug("-------------------------------------------------------------- DODAJE ogrodnika do kolejki czekajacych na sprzet: %d", recv_pkt.src);
#endif

                    pthread_mutex_lock(&equipmentMut);
                    processWaitingForMyEquipment[recv_pkt.src] = id; // mutex here?
                    pthread_mutex_unlock(&equipmentMut);
                }
                break;
            }

            case REPLY_SPRZET:{         
                if(stan==waitingForEquipment)
                {
                    ile_zgod_sprzet++;
                    if(ile_zgod_sprzet==size-1){ // 
#ifdef DEBUG_WK
                        debug("--------------------------------------------------------------Czekajac na sprzet, odebralem juz wszystkie zgody! Zaraz zacznę pracę");
#endif
                        ile_zgod_sprzet = 0;
                        changeState(workingInGarden);

#ifdef DEBUG_WK
                    debug("-------------------------------------------------------------- Zmieniłem stan na workingInGarden");
#endif       
                    
                    }
                }
                else
                {
                    debug("UWAGA! Nie poszukuje sprzetu!");
                }
            
                
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
            return true;
        }
        it++;
    }
    return false;
}

bool shouldSendRequest(packet_t pkt){
    if(stan==waitingForJob){
        return true;
    }
    return false;
}

bool shouldSendReply(packet_t pkt){
    if(rank = pkt.src){ return true;}
    if(stan==waitingForJob){
        if((pkt.ts < lamportClock) || (pkt.ts == lamportClock && pkt.src < rank)) {
            return true;
        }
        else{
            return false;
        }
    }

    return true;
}

bool shouldGrantEquipment(packet_t pkt){
    if(rank==pkt.src) {return true;}
    else if(stan!=waitingForEquipment){ return true;}
    else if(stan==waitingForEquipment && moje_zlecenie.rodzaj_sprzetu!=pkt.rodzaj_sprzetu){ // mutex here?
        return true;
    }
    return false;
}