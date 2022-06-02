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
                // iterujemy po liscie ogloszen i szukamy niezajetego zlecenia
                    // pthread_mutex_lock(&lista_ogloszenMut);
// #ifdef DEBUG_WG
//                     debug("Szukam pracy, ");
// #endif
//                     pthread_mutex_lock(&lista_ogloszenMut);
                        std::map<int, int>::iterator it = lista_ogloszen.begin();
                        while (it!=lista_ogloszen.end()){
                                if(it->second == -1){
                                    int idd = it->first;
#ifdef DEBUG_WK
                                    debug("Iteruje po: %d z rodzajem sprzetu: %d", idd, zlecenia[idd].rodzaj_sprzetu);
#endif
                                    break;
                                }
                        }
                        
//                     }
//                     pthread_mutex_unlock(&lista_ogloszenMut);
                    // sleep(1);
                break;
            }
            
            case waitingForEquipment:{
                debug("Ogrodnik: zaznajamiam sie z literatura na 10 sekundy");
                sleep(10); // zaznajamia sie z literatura zlecenia
                

                // o który sprzęt ubiega się ogrodnik?
                changeState(workingInGarden);
#ifdef DEBUG_WG
                debug(">>> Zmieniam stan na workingInGarden");
#endif            
                // pobrać sprzęt przed zaznajamianiem się z literaturą czy przed?
                break;
            }

            case workingInGarden:{
                debug("Ogrodnik: pracuje przez 2 sekundy");
                sleep(2); // pracuje
                
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