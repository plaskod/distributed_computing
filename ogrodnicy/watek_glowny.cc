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

                packet_t *pkt = preparePacket(lamportClock, id_zlecenie, zlecenie_enum, -1);
                for (int i=1 ; i<size ; i++) {
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

                // odbierz zgodę od innych procesów
                // odbierz zadanie
                break;
            }
            
            case waitingForEquipment:{
                sleep(2); // zaznajamia sie z literatura zlecenia
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