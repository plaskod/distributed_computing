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
                int r = 1 + rand() % 10;
                debug("Instytut: zasypiam na %d sekund", r);
                sleep(r);
                
                int zlecenie_enum = rand()%3; // losowanie zadania 1,2 lub 3
                debug("Instytut: rozsyłam zadanie: %d - ogrodnik potrzebuje zasobu: %d", id_zlecenie, zlecenie_enum);

            
                packet_t *pkt = preparePacket(lamportClock, id_zlecenie, zlecenie_enum, -1);
                broadcastPacket(pkt, NOWE_ZLECENIE_OD_INSTYTUTU);
                id_zlecenie++;
                // MPI_Bcast( &pkt, 1, MPI_INT, 0, MPI_COMM_WORLD );
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
                debug("Zadanie wykonane");
                changeState(waitingForJob);
                break;
            }
            default: {
                debug("Nie placa ci za obijanie sie")
                break;
            }
        }



        
    }
}