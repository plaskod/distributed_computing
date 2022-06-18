#ifndef WATEK_KOMUNIKACYJNY_H
#define WATEK_KOMUNIKACYJNY_H

/* wątek komunikacyjny: odbieranie wiadomości i reagowanie na nie poprzez zmiany stanu */
void *startKomWatek(void *ptr);
bool heardAboutThisJob(packet_t pkt);
bool shouldSendRequest(packet_t pkt);
bool shouldSendReply(packet_t pkt);
bool shouldGrantEquipment(packet_t pkt);
bool cmp(const personInLine_t & a, const personInLine_t & b);
std::vector<std::pair<int, int> > sortEquipmentQueue(int equip_id);
bool canTakeEquipment(packet_t pkt);
void removePersonFromQueue(std::vector<personInLine_t> & peopleWaitingForEquipment, int id_to_remove);
#endif
