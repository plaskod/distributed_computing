#include "main.hh"
#include "watek_glowny.hh"
#include <stdlib.h>


void mainLoop()
{
    int id_zlecenie = 1000;
    int rodzaj_sprzetu;
    while(1){
        switch (stan)
        {
            case inInstitute: {// wysyla co jakis czas zlecenia
                int r = 1 + rand() % 10;
                debug("Instytut: zasypiam na %d sekund", r);
                sleep(r);
                
                rodzaj_sprzetu = rand()%3; // losowanie zadania 0,1 lub 2
                debug("Instytut: rozsyłam zadanie: %d - ogrodnik potrzebuje zasobu: %d", id_zlecenie, rodzaj_sprzetu);

                packet_t *pkt = preparePacket(lamportClock, id_zlecenie, rodzaj_sprzetu, -1);
                broadcastPacket(pkt, NOWE_ZLECENIE_OD_INSTYTUTU);
                id_zlecenie++;
                pthread_mutex_lock(&csMut);
                cs++; // sekcja krytyczna sie powieksza
                pthread_mutex_unlock(&csMut);

                free(pkt);
                break;
            }

            case waitingForJob:{
                break;
            }
            
            case waitingForEquipment:{
                debug("Ogrodnik: zaznajamiam sie z literatura na 10 sekundy");
                sleep(10); // zaznajamia sie z literatura zlecenia
                

                
//                 changeState(workingInGarden);
// #ifdef DEBUG_WG
//                 debug(">>> Zmieniam stan na workingInGarden");
// #endif            
                break;
            }

            case workingInGarden:{
                int r = 1 + rand() % 5;
                debug("Ogrodnik: wykonuje - %s - przez %d sekund",tag2job_name[moje_zlecenie.rodzaj_sprzetu], r);
                sleep(r);

                cleanAfterJob();
                changeState(waitingForJob);
                debug("Zadanie wykonane");

                break;
            }
            default: {
                debug("Nie placa ci za obijanie sie")
                break;
            }
        }



        
    }
}

void cleanAfterJob(){
        moje_zlecenie.id = -1;
        moje_zlecenie.rodzaj_sprzetu = -1;
        pthread_mutex_lock(&equipmentMut);
        std::map<int, int>::iterator it = processWaitingForMyEquipment.begin();
        while (it!=waitingForEquipment.end()){ // wysyla kazdemu, moze powinien wyslac tylko jednemu?
            int dst = it->first; // src
            int id_zlec = it->second;
#ifdef DEBUG_WG
                debug("Juz nie potrzebuje sprzetu, wysylam REPLY do: %d", idd);
#endif
            packet_t *new_pkt = preparePacket(lamportClock, id_zlec, zlecenia[id_zlec].rodzaj_sprzetu, -1);
            sendPacket(new_pkt, idd, REPLY_SPRZET); // nie zachowuje kolejnosci
            free(new_pkt);
            processWaitingForMyEquipment.erase(dst);
            it++;
        }
        pthread_mutex_unlock(&equipmentMut);
            
}

