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
        // lamportClock = (recv_pkt.ts > lamportClock ? recv_pkt.ts : lamportClock) + 1;
        lamportClock = std::max(recv_pkt.ts, lamportClock);
        lamportClock++;
        pthread_mutex_unlock(&lamportMut);
#ifdef DEBUG_WK
                
                debug("++++++++++++++++++++ WSZYSTKO: SRC %d, TS %d, DATA %d, TAG %d ",recv_pkt.src, recv_pkt.ts, recv_pkt.data, status.MPI_TAG);
#endif
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
#ifdef DEBUG_WK
                
                debug(">>>Widzialem juz zlecenie: %d", id );
#endif
                    break;
                }


                if(stan == waitingForJob){
                    std::map<int, int>::iterator it = lista_ogloszen.begin();
                    while (it!=lista_ogloszen.end()){
#ifdef DEBUG_TASKLIST 
                        printf("Ogrodnik : %d na mojej liscie ogloszen, zlecenie: %d ma wartosc: %d\n", rank, it->first, it->second);
#endif
                        if(it->second == -1){
                            
                            int idd = it->first;
                            lista_ogloszen[idd] = lamportClock; // lamportClock kiedy wyslalem request 
#ifdef DEBUG_WK
                            debug("Iteruje po: %d z rodzajem sprzetu: %d", idd, zlecenia[idd].rodzaj_sprzetu);
#endif
                            packet_t *new_pkt = preparePacket(lamportClock, idd, zlecenia[idd].rodzaj_sprzetu, lamportClock);
                            broadcastPacket(new_pkt, REQ_ZLECENIE);
                            free(new_pkt);
                            break;
                        }
                        it++;
                    }
                }

                
                
            }break;
            case REQ_ZLECENIE:{    
#ifdef DEBUG_WK
                debug("------------------------------- Otrzymalem REQ_ZLECENIE od: %d z ts=%d na zlecenie: %d", recv_pkt.src, recv_pkt.data, id);
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
                debug("------------------------------- Odpowiadam REPLY_ZLECENIE na REQ_ZLECENIE od: %d z ts=%d na zlecenie: %d", recv_pkt.src, recv_pkt.data, id);
#endif
                    packet_t *new_pkt = preparePacket(lamportClock, id, rodzaj_sprzetu, lamportClock);
                    sendPacket(new_pkt,recv_pkt.src, REPLY_ZLECENIE_ZGODA);
                    free(new_pkt);

                    //lista_ogloszen[id] = -2; // zaznaczam

                    if(recv_pkt.src!=rank){
#ifdef DEBUG_WK
                        debug("------------------------------- NIE POWINIENEM ZAJAC SIE TYM ZLECENIEM: %d, zegar lamporta ogrodnika %d wynosi %d, a moj: %d", id, recv_pkt.src, recv_pkt.data, lista_ogloszen[recv_pkt.zlecenie_id]);
#endif
                        replies[id] = 0;
                        lista_ogloszen[id] = -2;
                    }
                    
                }
                else{
                    
#ifdef DEBUG_WK
                    debug("------------------------------- IGNORUJE ogrodnika: %d z z ts=%d, ubiegajacego sie o zlecenie %d",recv_pkt.src, recv_pkt.ts, id);
#endif
                    
                }
                
                
            }break;
            case REPLY_ZLECENIE_ZGODA:{
                replies[id] = replies[id]+1;
#ifdef DEBUG_WK
                    debug("------------------------------- Otrzymalem zgode na zadanie %d od %d z ts=%d, w sumie: %d z %d", id, recv_pkt.src, recv_pkt.ts, replies[id], size-1);
#endif
                if(replies[id]==size-1 && stan==waitingForJob){ 
                    changeState(waitingForEquipment);
                    replies[id] = 0;
                    moje_zlecenie.id = id;
                    moje_zlecenie.rodzaj_sprzetu = rodzaj_sprzetu;
                    int request_lamport_clock = lamportClock;
#ifdef DEBUG_WK
                    debug("------------------------------- Zaraz zaczne pracę nad id: %d ide szukac sprzetu: %d ", moje_zlecenie.id, moje_zlecenie.rodzaj_sprzetu);
#endif  

                    
                    packet_t *new_pkt = preparePacket(lamportClock, id, rodzaj_sprzetu, request_lamport_clock);
                    for(int i = 1; i<size; i++){ // broadcast to all, exlucding me that I want equipment
                        if(i!=rank){
#ifdef DEBUG_BROADCAST
                            debug("Sending packet to: %d", i);
#endif
                            sendPacket(new_pkt, i, REQ_SPRZET);
                        }                             
                    }
                    free(new_pkt);
                    
                    ack_counter++;
#ifdef DEBUG_WK
                    debug("------------------------------- BROADCAST REQ_SPRZET DONE (EXCLUDING ME)");
#endif

                    equipmentQueue[rodzaj_sprzetu].insert({rank, request_lamport_clock});
                    pthread_mutex_lock(&readingMut);
                    readLiterature = true;
                    pthread_mutex_unlock(&readingMut);

                    if(ack_counter == size-1){
                        sortEquipmentQueue(rodzaj_sprzetu);
                        ack_counter = 0;
                        if(canTakeEquipment(recv_pkt))
                        {
#ifdef DEBUG_WK
                            debug("-------------------------------------------------------------- Mogę wejść do sekcji krytycznej od razu po zgodzie na zlecenie! Zaczynam pracę nad zleceniem: %d", id);
#endif
                            changeState(workingInGarden);
                        }
                    }

                    
                    

                }
                
                
            }break;

            case REQ_SPRZET:{ // REQ_SPRZET shouldn't come from me
#ifdef DEBUG_WK
                    debug("-------------------------------------------------------------- Otrzymalem REQ_SPRZET od %d z ts=%d, DODAJE OGRODNIKA DO KOLEJKI!", recv_pkt.src, recv_pkt.ts);
#endif
                    equipmentQueue[rodzaj_sprzetu].insert({recv_pkt.src, recv_pkt.data});
                    packet_t *new_pkt = preparePacket(lamportClock, id, rodzaj_sprzetu, lamportClock);
                    sendPacket(new_pkt, recv_pkt.src, ACK_SPRZET);
                    free(new_pkt);
#ifdef DEBUG_WK
                    debug("-------------------------------------------------------------- Wysylalem ACK_SPRZET do %d", recv_pkt.src);
#endif
                    
            }break;

            case ACK_SPRZET:{         
                if(stan==waitingForEquipment)
                {
                    ack_counter++;
#ifdef DEBUG_WK
                        debug("--------------------------------------------------------------Czekajac na sprzet, odebralem jeden ACK od %d! Razem: %d z %d",recv_pkt.src, ack_counter, size-1);
#endif               
                    if(ack_counter==size-1){ 
#ifdef DEBUG_WK
                        debug("--------------------------------------------------------------Czekajac na sprzet, odebralem juz wszystkie ACK! Zaraz zacznę pracę nad zleceniem: %d", id);
#endif
#ifdef DEBUG_SORT
                    printf("Sortowanie rozpoczete przez ogrodnika: %d\n", rank);
                    printf("PRZED SORTOWANIEM\n: ");
                    int i = 0;
                    std::map<int, int>::iterator it = equipmentQueue[rodzaj_sprzetu].begin();
                    while (it!=equipmentQueue[rodzaj_sprzetu].end()){
                        
                        printf("Iteracja: %d id ogrodnika: %d ts: %d \n", i, it->first, it->second);
                        i++;
                        it++;
                    }
#endif
                        
                        
                        std::vector<std::pair<int, int> > sorted_vector = sortEquipmentQueue(rodzaj_sprzetu);
                        equipmentQueue[rodzaj_sprzetu].clear();
                        for (auto& it_sorted : sorted_vector) {
                            equipmentQueue[rodzaj_sprzetu][it_sorted.first]= it_sorted.second;
                        }
                        // equipmentQueue[rodzaj_sprzetu] = sortEquipmentQueue(rodzaj_sprzetu);
#ifdef DEBUG_SORT
                    printf("PO POSORTOWANIU:\n ");
                    int i2 = 0;
                    std::map<int, int>::iterator it2 = equipmentQueue[rodzaj_sprzetu].begin();
                    while (it2!=equipmentQueue[rodzaj_sprzetu].end()){
                        
                        printf("Iteracja: %d id ogrodnika: %d ts: %d \n", i2, it2->first, it2->second);
                        i2++;
                        it2++;
                    }
#endif
                        ack_counter = 0;
                        if(canTakeEquipment(recv_pkt)){
#ifdef DEBUG_WK
                        debug("-------------------------------------------------------------- Mogę wejść do sekcji krytycznej! Zaczynam pracę nad zleceniem: %d", id);
#endif
                            changeState(workingInGarden);

#ifdef DEBUG_WK
                    debug("-------------------------------------------------------------- Zmieniłem stan na workingInGarden, zajmuje sie zleceniem: %d, ack_counter= %d", id, ack_counter);
#endif                                
                        }
                        else{
#ifdef DEBUG_WK
                    debug("-------------------------------------------------------------- NIE MOGE JESZCZE WEJSC DO SEKCJI KRYTYCZNEJ, CZEKAM NA SPRZET!");
#endif       
                        }

                    
                    }
                }
                else
                {
                    debug("UWAGA! Nie poszukuje sprzetu!");
                }
            
                
                
            }break;

            case RELEASE_SPRZET:{
#ifdef DEBUG_WK
                    debug("-------------------------------------------------------------- Otrzymałem RELEASE_SPRZET od %d na rodzaj sprzetu: %d", recv_pkt.src, rodzaj_sprzetu);
#endif     
                equipmentQueue[rodzaj_sprzetu].erase(recv_pkt.src);
                if(rank!=recv_pkt.src && stan==waitingForEquipment && canTakeEquipment(recv_pkt)){
                    changeState(workingInGarden);
                }
            }break;

            default:
                debug("O panie!");
                break;
        } 
    }
}

bool heardAboutThisJob(packet_t pkt){
    std::map<int, int>::iterator it = lista_ogloszen.begin();
    while (it!=lista_ogloszen.end()){
        if(pkt.zlecenie_id == it->first){
            return true;
        }
        it++;
    }
    return false;
}

bool shouldSendReply(packet_t pkt){
    if(rank == pkt.src){ return true;}
    if(lista_ogloszen[pkt.zlecenie_id]!=-1 && lista_ogloszen[pkt.zlecenie_id]!=-2){
        if((pkt.data < lista_ogloszen[pkt.zlecenie_id]) || (pkt.data == lista_ogloszen[pkt.zlecenie_id] && pkt.src < rank))
        {
            return true;
        }
    }
    else{
        if((pkt.data < lamportClock) || (pkt.data == lamportClock && pkt.src < rank))
        {
            return true;
        }
    }
    
    return false;
}

bool shouldGrantEquipment(packet_t pkt){
    if(rank == pkt.src) { return true; }
    else if(stan!=waitingForEquipment){ return true;}
    else if(stan==waitingForEquipment && moje_zlecenie.rodzaj_sprzetu!=pkt.rodzaj_sprzetu){ // mutex here?
        return true;
    }
    return false;
}

bool cmp(std::pair<int, int>& a,
         std::pair<int, int>& b)
{
    if (a.second == b.second){
        return a.first < b.first;
    }
    return a.second < b.second;
}

std::vector<std::pair<int, int> > sortEquipmentQueue(int equipment_id){

    std::vector<std::pair<int, int> > A;
  
    for (auto& it : equipmentQueue[equipment_id]) {
        A.push_back(it);
    }

    std::sort(A.begin(), A.end(), cmp);

    // equipmentQueue[equipment_id].clear();
    // std::map<int, int> sortedEquipmentQueue;
    // for (auto& it_sorted : A) {
    //     sortedEquipmentQueue.insert({it_sorted.first, it_sorted.second});
    // }

    return A;

}

bool canTakeEquipment(packet_t pkt){
    int position = std::distance(equipmentQueue[pkt.rodzaj_sprzetu].begin(), equipmentQueue[pkt.rodzaj_sprzetu].find(rank));
    if (position>=0){
        
        if(pkt.rodzaj_sprzetu == obslugaTrawnika){
#ifdef DEBUG_EQUIPMENT
            printf("Position is found: im on %dth place for equipment: %d with max: %d\n", position, pkt.rodzaj_sprzetu, SP_TRAWNIK-1);
#endif
            if(position <= SP_TRAWNIK-1){
                return true;
            }
        }
        else if(pkt.rodzaj_sprzetu == przycinanieZywoplotu){
            if(position <= SP_PRZYCINANIE-1){
                return true;
            }
        }
        else if(pkt.rodzaj_sprzetu == wyganianieSzkodnikow){
            if(position <= SP_WYGANIANIE-1){
                return true;
            }
        }
    }

    return false;
}