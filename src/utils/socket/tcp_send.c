#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <iphlpapi.h> 
#include <pcap.h>

#include "pro_format.h"

// https://npcap.com/guide/wpcap

// gcc tcp_send.c -o ts.exe -Ld:/ztestfiles/libs -lwpcap -lPacket -lws2_32 -I../../includes -Id:/ztestfiles/libs/include


void fill_ip_header(IP_HDR* header, char* srcIp, char* dstIp, u_char ipProtocol, int payloadLen);
void fill_tcp_header(TCP_HDR* header, short srcPort, short dstPort, u_char flags);
void fill_ip_checkbuf(char* buf, IP_HDR* ipHeader);
void fill_tcp_check_buffer(char* checkBuf, IP_HDR* ipHeader, TCP_HDR* tcpHeader, \
            char* tcpPayload, short payloadLen);

char* get_tcp_frame(char* srcIp, char* dstIp, short srcPort, short dstPort, \
            char* payload, short payloadLen, int* frameLen);

USHORT cal_checksum(USHORT *ptr, int nbytes);

pcap_t* open_listen();
int send_data(char* data, int len);
void handle_recv(pcap_t *nethandle, char* filter_expression);
void get_mac();

pcap_t* gnetHandler;
pcap_if_t *target_dev;

char** gv;

int main(int argc, char** argv) {

    gnetHandler = open_listen();
    // handle_recv(gnetHandler, "dst port 8888");
    return 0;

    char *srcIp = "127.0.0.1";
    char *dstIp = "127.0.0.1";
    short srcPort = 0x2324;
    short dstPort = 0x2526;
    char* payload = "ab";
    short payloadLen = strlen(payload);
    if (argc < 9) {
        return -1;
    }
    gv = argv;
    srcIp = argv[1];
    dstIp = argv[2];
    srcPort = atoi(argv[3]);
    dstPort = atoi(argv[4]);
    payload = argv[8];
    payloadLen = strlen(payload);

    int frameLen;
    char* frame = get_tcp_frame(srcIp, dstIp, srcPort, dstPort, payload, payloadLen, &frameLen);
    send_data(frame, frameLen);

    free(frame);
    pcap_freealldevs(target_dev);
    pcap_close(gnetHandler);
}

char* get_tcp_frame(char* srcIp, char* dstIp, short srcPort, short dstPort, 
            char* payload, short payloadLen, int* frameLen) {
    
    int ethLen;
    char *ethHeader;
    if (!strcmp(srcIp, "127.0.0.1") && !strcmp(dstIp, "127.0.0.1")) {
        // 回环地址
        ethLen = 4;
        char loopback[4] = {0x02};
        ethHeader = loopback;
    } else {     
        ethLen = sizeof(ETHERNET_HDR);
        ethHeader = (char*)calloc(1, ethLen);
        ethHeader[12] = PRO_TYPE_IPV4 >> 8;
        ethHeader[13] = PRO_TYPE_IPV4 & 0xff;
        // 获取mac地址

    }

    // 不考虑tcp options
    *frameLen = ethLen + sizeof(IP_HDR) + sizeof(TCP_HDR) + payloadLen;
    u_char *frame = (char*)calloc(1, *frameLen);
    IP_HDR ipHeader;
    TCP_HDR tcpHeader;
    PSEUDO_HDR pseudoHeader;
    char* checkBuf = malloc(32 + (payloadLen + 1 >> 1 << 1));
    fill_ip_header(&ipHeader, srcIp, dstIp, IPPROTO_TCP, payloadLen);
    fill_tcp_header(&tcpHeader, srcPort, dstPort, 0);

    u_short identification = (u_short)atoi(gv[5]);
    u_long seq = atol(gv[6]);
    u_long ack = atol(gv[7]);
    ipHeader.identification = htons(identification);
    tcpHeader.seq = htonl(seq);
    tcpHeader.ack = htonl(ack);

    fill_tcp_check_buffer(checkBuf, &ipHeader, &tcpHeader, payload, payloadLen);
    u_short chksum = cal_checksum((u_short*)checkBuf, 32 + (payloadLen + 1 >> 1 << 1));
    tcpHeader.chksum = htons(chksum);

    fill_ip_checkbuf(checkBuf, &ipHeader);
    chksum = cal_checksum((u_short*)checkBuf, 20);
    ipHeader.hdr_chksum = htons(chksum);

    free(checkBuf);

    memcpy(frame, ethHeader, ethLen);
    memcpy(frame + ethLen, &ipHeader, 20);
    memcpy(frame + ethLen + 20, &tcpHeader, 20);
    memcpy(frame + ethLen + 40, payload, payloadLen);
    return frame;
}

void convert_byte_order(char* data, long val) {
    char* pv = (char*)&val;
    data[0] = pv[1];
    data[1] = pv[0];
    data[2] = pv[3];
    data[3] = pv[2];
}

void fill_tcp_check_buffer(char* checkBuf, IP_HDR* ipHeader, TCP_HDR* tcpHeader, char* tcpPayload, short payloadLen) {
    convert_byte_order(checkBuf, ipHeader->srcIp);
    convert_byte_order(checkBuf + 4, ipHeader->dstIp);
    checkBuf[8] = ipHeader->protocol;
    checkBuf[9] = 0;
    checkBuf[10] = (payloadLen + 20) & 0xff;
    checkBuf[11] = (payloadLen + 20) >> 8;
    checkBuf[12] = tcpHeader->srcPort >> 8;
    checkBuf[13] = tcpHeader->srcPort & 0xff;
    checkBuf[14] = tcpHeader->dstPort >> 8;
    checkBuf[15] = tcpHeader->dstPort & 0xff;
    convert_byte_order(checkBuf + 16, tcpHeader->seq);
    convert_byte_order(checkBuf + 20, tcpHeader->ack);
    checkBuf[24] = tcpHeader->flags;
    checkBuf[25] = tcpHeader->dOffset_rsrvd;
    checkBuf[26] = tcpHeader->window >> 8;
    checkBuf[27] = tcpHeader->window & 0xff;
    checkBuf[28] = 0;
    checkBuf[29] = 0;
    checkBuf[30] = 0;
    checkBuf[31] = 0;

	checkBuf += 32;
    int even = payloadLen % 2 == 0 ? payloadLen: payloadLen - 1 >> 1 << 1;
    for (int i = 0; i < even; i+=2) {
        checkBuf[i] = tcpPayload[i + 1];
        checkBuf[i+1] = tcpPayload[i];
    }
    checkBuf += even;
    if (payloadLen - even) {
        checkBuf[even] = 0;
        checkBuf[even + 1] = tcpPayload[payloadLen - 1];
    }
}

void fill_ip_checkbuf(char* buf, IP_HDR* ipHeader) {
    buf[1] = ipHeader->ver_ihl;
    buf[0] = ipHeader->serviceType;
    buf[2] = ipHeader->totalLength >> 8;
    buf[3] = ipHeader->totalLength & 0xff;
    buf[4] = ipHeader->identification >> 8;
    buf[5] = ipHeader->identification & 0xff;
    buf[6] = ipHeader->flags_fragmentOffset >> 8;
    buf[7] = ipHeader->flags_fragmentOffset & 0xff;
    buf[9] = ipHeader->time2live;
    buf[8] = ipHeader->protocol;
    buf[10] = 0;
    buf[11] = 0;
    convert_byte_order(buf + 12, ipHeader->srcIp);
    convert_byte_order(buf + 16, ipHeader->dstIp);
}

void fill_ip_header(IP_HDR* header, char* srcIp, char* dstIp, u_char ipProtocol, int payloadLen) {
    header->ver_ihl = 0x45;
    header->serviceType = 0;
    header->totalLength = htons(sizeof(IP_HDR) + sizeof(TCP_HDR) + payloadLen);
    header->identification = 0;
    header->flags_fragmentOffset = 2 << 5; // dont fragment
    header->time2live = 0x32;
    header->protocol = ipProtocol;
    header->hdr_chksum = 0;
    header->srcIp = inet_addr(srcIp);
    header->dstIp = inet_addr(dstIp);
}

void fill_tcp_header(TCP_HDR* header, short srcPort, short dstPort, u_char flags) {

	header->srcPort = htons(srcPort);
	header->dstPort = htons(dstPort);
	header->seq = 0;
	header->ack = 0;
    header->dOffset_rsrvd = 5 << 4;
    header->flags = 0x18; // PSH, notify read data
	header->window = 0xff << 8;
	header->chksum = 0;
	header->urgentPointer = 0;
}

int send_data(char* data, int len) {
    if (!gnetHandler) {
        printf("invalid device handle\n");
        return -1;
    }

    if (pcap_sendpacket(gnetHandler, data, len) != 0) {
        fprintf(stderr, "\nError sending the packet: %s\n", pcap_geterr(gnetHandler));
        return -1;
    }

    printf("Packet sent successfully!\n");
    return 0;
}

pcap_t* open_listen() {
    pcap_if_t *alldevs;
    pcap_if_t *d;

    pcap_t *netHandler;
    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
        exit(1);
    }
    
    u_char count = 0;
    u_char ri[32] = {0};
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
        return 0;
    }

    UCHAR targetNum = 2;
    printf("Enter the interface number (1-%d):", count);
    scanf("%d", &targetNum);

    pcap_if_t targetCard;
    if (targetNum < 1 || targetNum > count) {
        printf("incorrect interface number\n");
        return 0;
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
    pcap_if_t *pre_dev = alldevs;
    pcap_if_t *next_dev;

    for (i = 0; i < targetNum - 1; i++) {
        pre_dev = d;
        d = d->next;
    }
    target_dev = d;
    if(!pre_dev) {   
        alldevs = d->next;
    } else {
        pre_dev->next = target_dev->next;
    } 
    target_dev->next = NULL;
    pcap_freealldevs(alldevs);

    // pack buffer timeout: 数据包缓冲超时,在数据包到达后延迟一段时间再传递,使一次能够处理多个数据包
    // pcap_set_timeout(1000) pcap_set_immediate_mode(1)
    int snapLen = 0x010000;
    int packet_buf_to_ms = 10000;
    int mode = PCAP_OPENFLAG_PROMISCUOUS;
    if ((netHandler = pcap_open_live(target_dev->name, snapLen, mode, packet_buf_to_ms, errbuf)) == NULL) {
        fprintf(stderr, "\nUnable to open the adapter. %s is not supported by Npcap\n", d->name);
        pcap_freealldevs(alldevs);
        return 0;
    }
    printf("\nListening on %s...\n", d->description);
    return netHandler;
}


void handle_recv(pcap_t *nethandle, char* filter_expression) {
    struct pcap_pkthdr *header;
    const u_char *pkt_data;
    int res;

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

    // 捕获数据包
    while(1) {
    res = pcap_next_ex(nethandle, &header, &pkt_data);
    if (res == 1) {
        printf("Response received!\n");
        // 处理应答数据包
        printf("recevied %d bytes :\n", header->len);
        for (int i = 0; i < header->len; i++) {
            printf("%02x ", pkt_data[i]);
        }
        printf("\n");
    } else if (res == 0) {
        // 超时，继续等待
        printf("timeout for wait packet\n");
    } else if (res == -1 || res == -2) {
        printf("Error reading the packet: %s\n", pcap_geterr(nethandle));
        break;
    }
    }
}

//    struct MY_FRAME req_frame= {
//       .ehdr = {
//         // 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
//         // 本机 0xC0, 0xB6, 0xF9, 0x89, 0xEC, 0xE8
//         // 手机 0xEA, 0x92, 0xA4, 0xFA, 0x43, 0x7C
//         .dest_mac = {0xC0, 0xB6, 0xF9, 0x89, 0xEC, 0xE8},
//         .src_mac = {0xC0, 0xB6, 0xF9, 0x89, 0xEC, 0xE8},
//         .proType = PRO_TYPE_IPV4
//       },
//       .arpData = {
//         .hardwareType = {0, 1},
//         .proType = PRO_TYPE_ARP,
//         .hardwareSize = 6,
//         .protocolSize = 4,
//         .opcode = ARP_REP,
//         .senderMac = {0xC0, 0xB6, 0xF9, 0x89, 0xEC, 0xE8},
//         .senderIp = {0xC0, 0xA8, 0x0, 0x1},
//         .targetMac = {0xEA, 0x92, 0xA4, 0xFA, 0x43, 0x7C},
//         .targetIp = {0xC0, 0xA8, 0x0, 0x65}
//       }
//     };

USHORT cal_checksum(USHORT *ptr, int nbytes) {

    ULONG sum = 0;
    USHORT checksum;

    while (nbytes > 1) {
        printf("0x%04x, ", *ptr);
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1) {
        sum += *(UCHAR*)ptr;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    checksum = (USHORT)~sum;
    printf("checksum:0x%04x\n", checksum);
    return checksum;
}
