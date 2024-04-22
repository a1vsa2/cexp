
#include <stdio.h>

#include "npcap_utils.h"


// https://npcap.com/guide/wpcap


int send_data(pcap_t* netHandler, char* data, int len) {
    if (!netHandler) {
        printf("invalid device handle\n");
        return -1;
    }

    if (pcap_sendpacket(netHandler, data, len) != 0) {
        fprintf(stderr, "\nError sending the packet: %s\n", pcap_geterr(netHandler));
        return -1;
    }

    printf("Packet sent successfully!\n");
    return 0;
}

void handle_recv(pcap_t *nethandle, char* filter_expression, pcap_handler callback) {
    struct pcap_pkthdr *header;
    const u_char *pkt_data;
    int ret;

    // https://npcap.com/guide/wpcap/pcap-filter.html
    struct bpf_program filter;
    if (pcap_compile(nethandle, &filter, filter_expression, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        printf("Couldn't parse filter %s: %s\n", filter_expression, pcap_geterr(nethandle));
        return;
    }
    if (pcap_setfilter(nethandle, &filter) == -1) {
        printf("Couldn't install filter %s: %s\n", filter_expression, pcap_geterr(nethandle));
        return;
    }

    
    ret = pcap_loop(nethandle, -1, callback, 0);

    // while(1) {
    //     res = pcap_next_ex(nethandle, &header, &pkt_data);
    //     if (res == 1) {
    //         printf("Response received!\n");
    //         // 处理应答数据包
    //         printf("recevied %d bytes :\n", header->len);
    //         for (int i = 0; i < header->len; i++) {
    //             printf("%02x ", pkt_data[i]);
    //         }
    //         printf("\n");
    //     } else if (res == 0) {
    //         printf("timeout for wait packet\n");
    //     } else if (res == -1 || res == -2) {
    //         printf("Error reading the packet: %s\n", pcap_geterr(nethandle));
    //         break;
    //     }
    // }
}

void pick_dev(char** name) {
    pcap_if_t *alldevs;
    pcap_if_t *d;
    char errbuf[PCAP_ERRBUF_SIZE];
    u_char count = 0;
    u_char ri[32] = {0};
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
        exit(1);
    }

    int i = 0;
    for (d = alldevs; d; d = d->next) {
        i++;
        if (!((d->flags & PCAP_IF_LOOPBACK) != 0 || (d->flags & 0x28) == 8)) {
            continue;
        }
        ri[i] = 1;
        ++count;
        printf("%d. ", count);
        if (d->description) {
            printf("%s\t", d->description);
        } else {
            printf("No Description\t");
        }
        if (d->addresses) {
            printf("%s\n", inet_ntoa(((SOCKADDR_IN*)(d->addresses->addr))->sin_addr));
        }
        printf("\t%s\n", d->name);
    }

    if (count == 0) {
        printf("\nNo interfaces found! Make sure Npcap is installed.\n");
        return;
    }

    UCHAR targetNum = 2;
    printf("Enter the interface number (1-%d):", count);
    scanf("%d", &targetNum);

    pcap_if_t targetCard;
    if (targetNum < 1 || targetNum > count) {
        printf("incorrect interface number\n");
        return;
    }

    for (int j = 1, k = 0; j <= i;j++) {
        if (ri[j] == 1) {
            k++;
        }
        if (k == targetNum) {
            targetNum = j;
            break;
        }
    }

    d = alldevs;
    for (int j = 0; j < targetNum - 1; j++, d=d->next);
    int nameLen = strlen(d->name) + 1;
    *name = malloc(nameLen);
    memcpy(*name, d->name, nameLen);
    // pcap_if_t *pre_dev = alldevs;
    // pcap_if_t *next_dev;

    // for (i = 0; i < targetNum - 1; i++) {
    //     pre_dev = d;
    //     d = d->next;
    // }
    // *updev = d;
    // if(!pre_dev) {   
    //     alldevs = d->next;
    // } else {
    //     pre_dev->next = (*updev)->next;
    // } 
    // (*updev)->next = NULL;
    pcap_freealldevs(alldevs);
}

int dev_listen(pcap_t **ppHandler, char* devName) {
    char errbuf[PCAP_ERRBUF_SIZE];

    // pack buffer timeout: 数据包缓冲超时,在数据包到达后延迟一段时间再传递,使一次能够处理多个数据包
    // pcap_set_timeout(1000) pcap_set_immediate_mode(1)
    int snapLen = 0x010000;
    int packet_buf_to_ms = 10000;
    int mode = PCAP_OPENFLAG_PROMISCUOUS;
    *ppHandler = pcap_open_live(devName, snapLen, mode, packet_buf_to_ms, errbuf);
    if (*ppHandler == NULL) {
        printf("\nUnable to open the adapter. %s is not supported by Npcap\n", devName);
        return 1;
    }
    printf("\nListening on %s...\n", devName);
    return 0;
}