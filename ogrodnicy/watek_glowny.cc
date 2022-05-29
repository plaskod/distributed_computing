#include "main.hh"
#include "watek_glowny.hh"
#include <stdlib.h>


void mainLoop()
{
    int id_zlecenie = 1000;
    while(1){
        switch (stan)
        {
            case inInstitute: {// wysyla co jakis czas zlecenia
                int r = rand() % 11;
                debug("Instytut: zasypiam na %d sekund", r);
                sleep(r);
                
                int zlecenie_enum = rand()%3; // losowanie zadania 1,2 lub 3
                debug("Instytut: rozsyłam zadanie: %d - ogrodnik potrzebuje zasobu: %d", id_zlecenie, zlecenie_enum);

                id_zlecenie++;// losowy zaczynajac od 1000 (do rozronienia z innymi danymi)
#ifdef DEBUG_WG
                debug(">>>Inkremetnacja id_zlecenia");
#endif
                packet_t *pkt = preparePacket(lamportClock, id_zlecenie, zlecenie_enum, -1);
#ifdef DEBUG_WG
                debug(">>>1: Pakiet przygotowany");
                debug(">>>2: Pakiet ts: %d, id_zlec: %d, typ: %d",pkt->ts, pkt->zlecenie_id, pkt->zlecenie_enum);

#endif
                for (int i=1 ; i<size ; i++) {
#ifdef DEBUG_WG
                    debug(">>>Przed wyslaniem pakietu dla rank: %d, size=%d", i, size);
#endif
                    sendPacket(pkt, i, ACK_NOWE_ZLECENIE_OD_INSTYTUTU);
                }
                // MPI_Bcast( &pkt, 1, MPI_INT, 0, MPI_COMM_WORLD );
                pthread_mutex_lock(&csMut);
                cs++; // sekcja krytyczna sie powieksza
                pthread_mutex_unlock(&csMut);

                free(pkt);
                break;
            }

            case waitingForJob:{
                // packet_t *pkt = preparePacket(lamportClock, id_zlecenie, -1, -1);
                
                // for(int i=1 ; i<size ; i++) {
                //     sendPacket(pkt, i, REQ_ZLECENIE);
                // }

                // czy ten fragment powinien pojsc do nizszego case'a?
                // if(ile_zgod = size - 1) {
                //     if(zlecenie_enum == 0) {
                //         sendPacket(pkt, /*do kogo?*/ i, REQ_SP_TRAWNIK);
                //         // czy jest potrzeba pytania innych ogrodnikow o dostep do sprzetu? to juz jest sekcja krytyczna
                //     }
                //     if(zlecenie_enum == 1) {
                //         sendPacket(pkt, /*do kogo?*/ i, REQ_SP_PRZYCINANIE);
                //     }
                //     if(zlecenie_enum == 2) {
                //         sendPacket(pkt, /*do kogo?*/ i, REQ_SP_WYGANIANIE);
                //     }
                // }
                
                // odbierz zgodę od innych procesów
                // odbierz zadanie
                break;
            }
            
            case waitingForEquipment:{
                sleep(2); // zaznajamia sie z literatura zlecenia
                debug("Ogrodnik: zaznajamiam sie z literatura na 2 sekundy");

                // o który sprzęt ubiega się ogrodnik?


                // pobrać sprzęt przed zaznajamianiem się z literaturą czy przed?
            }

            case workingInGarden:{
                sleep(2); // pracuje
            }
            default: {
                debug("Nie placa ci za obijanie sie")
                break;
            }
        }



        
    }
}