#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>

#include "pro_format.h"

void tcp_rep_frame(TCP_FRAME* hf, const unsigned char* bytes, int blen, char* payload, short payloadLen);

void fill_ip_header(IP_HDR* header, int payloadLen);
void fill_tcp_header(TCP_HDR* header);
void fill_ip_checkbuf(char* buf, IP_HDR* ipHeader);
void fill_tcp_check_buffer(char* checkBuf, IP_HDR* ipHeader, TCP_HDR* tcpHeader, \
            char* tcpPayload, short payloadLen);

unsigned short cal_checksum(unsigned short *ptr, int nbytes);


// unsigned char g_check_buf[4096];

void tcp_rep_frame(TCP_FRAME* hf, const unsigned char* bytes, int blen, char* payload, short payloadLen) {
    // EnterCriticalSection()
    if (*((long*)bytes) == 2) { // loopback frame
        memcpy((char*)&hf->eheader + 10, bytes, 4);
        bytes+=4;
    } else {
        memcpy(hf->eheader.dest_mac, bytes + 6, 6);
        memcpy(hf->eheader.src_mac, bytes, 6); // get local mac
        hf->eheader.proType = bytes[12] | (bytes[13]) << 8;
        bytes += 14;
    }
    hf->ipHeader.identification = (bytes[4] | (bytes[5] << 8)) + 1; // todo discard if repeat
    hf->ipHeader.srcIp = bytes[16] | (bytes[17] << 8) | (bytes[18] << 16) | (bytes[19] << 24);
    hf->ipHeader.dstIp = bytes[12] | (bytes[13] << 8) | (bytes[14] << 16) | (bytes[15] << 24);
    bytes += 20; // tcp header
    hf->tcpHeader.srcPort = bytes[2] | (bytes[3] << 8);
    hf->tcpHeader.dstPort = bytes[0] | (bytes[1] << 8);
    hf->tcpHeader.seq = bytes[8] | (bytes[9] << 8) | (bytes[10] << 16) | (bytes[11] << 24);  // todo fragment
    hf->tcpHeader.ack = (bytes[4] | (bytes[5] << 8) | (bytes[6] << 16) | (bytes[7] << 24)) + blen - 44;

    char* checkBuf = malloc(32 + (payloadLen + 1 >> 1 << 1));

    // todo remove
    fill_tcp_header(&hf->tcpHeader);
    fill_ip_header(&hf->ipHeader, payloadLen);

    hf->tcpHeader.flags = 0x18;

    fill_tcp_check_buffer(checkBuf, &hf->ipHeader, &hf->tcpHeader, payload, payloadLen);
    short chksum = cal_checksum((unsigned short*)checkBuf, 32 + (payloadLen + 1 >> 1 << 1));
    hf->tcpHeader.chksum = htons(chksum);
    fill_ip_checkbuf(checkBuf, &hf->ipHeader);
    chksum = cal_checksum((unsigned short*)checkBuf, 20);
    hf->ipHeader.hdr_chksum = htons(chksum);

    free(checkBuf);
    memcpy((char*)hf + 56, payload, payloadLen);
}


void fill_ip_header(IP_HDR* header, int payloadLen) {
    header->ver_ihl = 0x45;
    header->serviceType = 0;
    header->totalLength = htons(sizeof(IP_HDR) + sizeof(TCP_HDR) + payloadLen);
    header->flags_fragmentOffset = 2 << 5; // dont fragment
    header->time2live = 0x32;
    header->protocol = IPPROTO_TCP;
    header->hdr_chksum = 0;
}

void fill_tcp_header(TCP_HDR* header) {

	// header->srcPort = srcPort;
	// header->dstPort = dstPort;
	// header->seq = garg.seq;
	// header->ack = garg.ack;
    header->dOffset_rsrvd = 5 << 4;
    header->flags = 0x18; // PSH, notify read data
	header->window = 0xff << 8;
	header->chksum = 0;
	header->urgentPointer = 0;
    
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

unsigned short cal_checksum(unsigned short *ptr, int nbytes) {

    unsigned long sum = 0;
    unsigned short checksum;

    while (nbytes > 1) {
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