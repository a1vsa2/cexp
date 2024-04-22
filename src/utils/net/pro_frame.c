/* 

#include "pro_format.h"


void fill_ip_header(IP_HDR* header, long srcIp, long dstIp, char ipProtocol, int payloadLen);
void fill_tcp_header(TCP_HDR* header, short srcPort, short dstPort, char flags);
void fill_ip_checkbuf(char* buf, IP_HDR* ipHeader);
void fill_tcp_check_buffer(char* checkBuf, IP_HDR* ipHeader, TCP_HDR* tcpHeader, \
            char* tcpPayload, short payloadLen);

char* get_tcp_frame(char* payload, short payloadLen, int* frameLen);

unsigned short cal_checksum(unsigned short *ptr, int nbytes);





char* get_tcp_frame(char* hframe, char* payload, short payloadLen, int hlen) {
    
    int ethLen;
    char *ethHeader;
    char* frame;
    if (hlen == 44) {
        // 回环地址
        frame = (LOOPBACK_HFRAME*)hframe;
    } else {     
        ethLen = sizeof(ETHERNET_HDR);
        ethHeader = (char*)calloc(1, ethLen);
        ethHeader[12] = PRO_TYPE_IPV4 >> 8;
        ethHeader[13] = PRO_TYPE_IPV4 & 0xff;
        // 获取mac地址
        printf("not support \n");
        return 0;
    }

    // 不考虑tcp options
    *frameLen = ethLen + sizeof(IP_HDR) + sizeof(TCP_HDR) + payloadLen;
    char *frame = (char*)calloc(1, *frameLen);
    IP_HDR ipHeader;
    TCP_HDR tcpHeader;
    PSEUDO_HDR pseudoHeader;
    char* checkBuf = malloc(32 + (payloadLen + 1 >> 1 << 1));
    fill_tcp_header(&tcpHeader, garg.srcPort, garg.dstPort, 0);
    fill_ip_header(&ipHeader, garg.srcIp, garg.dstIp, garg.ipProto, payloadLen);

    fill_tcp_check_buffer(checkBuf, &ipHeader, &tcpHeader, payload, payloadLen);
    short chksum = cal_checksum((unsigned short*)checkBuf, 32 + (payloadLen + 1 >> 1 << 1));
    tcpHeader.chksum = htons(chksum);

    fill_ip_checkbuf(checkBuf, &ipHeader);
    chksum = cal_checksum((unsigned short*)checkBuf, 20);
    ipHeader.hdr_chksum = htons(chksum);

    free(checkBuf);

    memcpy(frame, ethHeader, ethLen);
    memcpy(frame + ethLen, &ipHeader, 20);
    memcpy(frame + ethLen + 20, &tcpHeader, 20);
    memcpy(frame + ethLen + 40, payload, payloadLen);
    return frame;
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

void fill_ip_header(IP_HDR* header, long srcIp, long dstIp, char ipProtocol, int payloadLen) {
    header->ver_ihl = 0x45;
    header->serviceType = 0;
    header->totalLength = htons(sizeof(IP_HDR) + sizeof(TCP_HDR) + payloadLen);
    header->identification = garg.identification;
    header->flags_fragmentOffset = 2 << 5; // dont fragment
    header->time2live = 0x32;
    header->protocol = ipProtocol;
    header->hdr_chksum = 0;
    header->srcIp = srcIp;
    header->dstIp = dstIp;
}

void fill_tcp_header(TCP_HDR* header, short srcPort, short dstPort, char flags) {

	header->srcPort = srcPort;
	header->dstPort = dstPort;
	header->seq = garg.seq;
	header->ack = garg.ack;
    header->dOffset_rsrvd = 5 << 4;
    header->flags = 0x18; // PSH, notify read data
	header->window = 0xff << 8;
	header->chksum = 0;
	header->urgentPointer = 0;
    
}

unsigned short cal_checksum(unsigned short *ptr, int nbytes) {

    unsigned long sum = 0;
    unsigned short checksum;

    while (nbytes > 1) {
        printf("0x%04x, ", *ptr);
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1) {
        sum += *(unsigned char*)ptr;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    checksum = (unsigned short)~sum;
    return checksum;
}

void convert_byte_order(char* data, long val) {
    char* pv = (char*)&val;
    data[0] = pv[1];
    data[1] = pv[0];
    data[2] = pv[3];
    data[3] = pv[2];
} */