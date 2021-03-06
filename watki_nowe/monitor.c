#include "main.h"
#include "monitor.h"

void *startMonitor(void *ptr)
{
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    char *instring, *token, *saveptr;
    instring=malloc(100);
    int newline;
    while ( stan!=InFinish ) {
	debug("sobie monitoruję");
        fgets( instring, 99, stdin);
        newline = strcspn(instring, "\n");
        instring[newline]=0; 
	debug("string %s\n",instring);
        token = strtok_r(instring, " ", &saveptr);
        if ((strcmp(token,"exit")==0) ||
	    (strcmp(token,"quit")==0)) {
            int i;
            for (i=0;i<size;i++)
		sendPacket(0,i,FINISH);
        }
        else if ((strcmp(token,"stop")==0) ||
	    (strcmp(token,"wait")==0)) {
            int i;
            for (i=0;i<size;i++)
		sendPacket(0,i,INMONITOR);
        } else if ((strcmp(token,"resume")==0) ||
	    (strcmp(token,"run")==0)) {
            int i;
            for (i=0;i<size;i++)
		sendPacket(0,i,INRUN);
        } else if (strcmp(token,"send")==0) {
	    token = strtok_r(0, " ", &saveptr);
            int i=1, data=1,type=INRUN;
            if (token) i = atoi(token);
	    token = strtok_r(0, " ", &saveptr);
            if (token) {
                if ((strcmp(token,"app")==0) || (strcmp(token,"appmsg")==0)) {
                    type = APPMSG;
                } else if (strcmp(token,"finish")==0) {
                    type = FINISH;
                }
            }
	    token = strtok_r(0, " ", &saveptr);
            if (token){ 
                data = atoi(token);
            }
            
            debug( "wysyłam typ %d do %d z danymi %d", type, i, data);
	    packet_t *pkt = malloc(sizeof(packet_t));
            pkt->data=data;
	    sendPacket(pkt,i,type);
            free(pkt);
        } 
 //       free(instring);
    }
    free(instring);
}
