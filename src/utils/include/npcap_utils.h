#ifndef __NPCAP_UTILS_H
#define __NPCAP_UTILS_H_

#include <pcap.h>

void pick_dev(char** adapterName);
int dev_listen(pcap_t **netHandler, char* devName);
int send_data(pcap_t* netHandler, char* data, int len);
void handle_recv(pcap_t *nethandle, char* filter_expression, pcap_handler callback);

#endif // __NPCAP_UTILS_H