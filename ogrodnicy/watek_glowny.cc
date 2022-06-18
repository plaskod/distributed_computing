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
                int r = 1 + rand() % 8;
                debug("Instytut: zasypiam na %d sekund", r);
                sleep(r);
                
                rodzaj_sprzetu = rand()%3; // losowanie zadania 0,1 lub 2
                debug("Instytut: rozsyłam zadanie: %d - ogrodnik potrzebuje zasobu: %d", id_zlecenie, rodzaj_sprzetu);

                packet_t *pkt = preparePacket(-1, id_zlecenie, rodzaj_sprzetu, -1);
                broadcastPacket(pkt, NOWE_ZLECENIE_OD_INSTYTUTU);
                id_zlecenie++;

                free(pkt);
                
            }break;

            case waitingForJob:{
                
            }break;
            
            case waitingForEquipment:{
                if(readLiterature){
                    debug("Ogrodnik: zaznajamiam sie z literatura");
                    pthread_mutex_lock(&readingMut);
                    readLiterature = false;
                    pthread_mutex_unlock(&readingMut);
                    sleep(5);
                    debug("Ogrodnik: skonczylem czytac literature");
                }
                           
                
            }break;

            case workingInGarden:{
                int r = 1 + rand() % 5;
                debug("Ogrodnik: wykonuje zlecenie - %d - przez %d sekund", moje_zlecenie.id, r);
                sleep(r);
                wykonaneZlecenia.push_back(moje_zlecenie.id);
                debug("Zadanie %d wykonane", moje_zlecenie.id);
                
#ifdef DEBUG_WG
                printf("Wszystkie zadania wykonane przez %d:", rank);
                print_vector(wykonaneZlecenia);
#endif
                cleanAfterJob();
                
                

                
            } break;
            default: {
                debug("Nie placa ci za obijanie sie")
                break;
            }
        }



        
    }
}

void cleanAfterJob(){
        changeState(waitingForJob);
        packet_t *new_pkt = preparePacket(lamportClock, moje_zlecenie.id, zlecenia[moje_zlecenie.id].rodzaj_sprzetu, -1);
        broadcastPacket(new_pkt, RELEASE_SPRZET);
        free(new_pkt);
#ifdef DEBUG_RELEASE
    debug("[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[--RELEASE--]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]] Nie potrzebuje juz sprzetu: %d potrzebnego mi w zleceniu %d, WYSLALEM BROADCAST", moje_zlecenie.id, zlecenia[moje_zlecenie.id].rodzaj_sprzetu);
#endif
        moje_zlecenie.id = -1;
        moje_zlecenie.rodzaj_sprzetu = -1;
        

            
}

