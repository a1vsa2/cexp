#ifndef __PRO_FORMAT_H_
#define __PRO_FORMAT_H_


// 网络协议都是小端
#define PRO_TYPE_ARP 0x0806
#define PRO_TYPE_IPV4 0x0800


#define ARP_REQ 0x0100
#define ARP_REP 0x0200


typedef struct _ETHERNET_HDR {
	unsigned char dest_mac[6];
	unsigned char src_mac[6];
	unsigned short proType;
} ETHERNET_HDR;


// https://www.rfc-editor.org/rfc/rfc791
typedef struct _IP_HDR {
	unsigned char ver_ihl; 			// ver(4bit) << 4 | ihl
	unsigned char serviceType; 		// preference(3bit) delay throughput relibility reserved(2bit)
	unsigned short totalLength; 	// internet header and data
	unsigned short identification;	// assigned by the sender, assembling the fragments of a datagram.
	unsigned short flags_fragmentOffset; // fragmentOffset & 0xff |  (flags(3bit) << 5 | (fragmentOffset >> 8) & 0x1f)
	unsigned char time2live;
	unsigned char protocol;			// 6: tcp
	unsigned short hdr_chksum;
	unsigned long srcIp;
	unsigned long dstIp;
} IP_HDR;

typedef struct _PSEUDO_HDR{
	unsigned long srcIp;
	unsigned long dstIp;
	unsigned char unused;
	unsigned char protocol;
	// unsigned short unused_protocol;
	unsigned short tcpLen;
} PSEUDO_HDR;

// https://www.rfc-editor.org/rfc/rfc9293#name-header-format
typedef struct _TCP_HDR {
	unsigned short srcPort;
	unsigned short dstPort;
	unsigned long seq;
	unsigned long ack;
	unsigned char dOffset_rsrvd;	// dataOffset(4bit): indicates where the data begins
	unsigned char flags;			// CRW ECE URG ACK PSH RST SYN FIN
	// unsigned short dOffset_rsrvd_flags;
	unsigned short window;
	unsigned short chksum;
	unsigned short urgentPointer;
	// options_padding
} TCP_HDR;


typedef struct _UDP_HDR {
	unsigned short source_p;
	unsigned short dest_p;
	unsigned short len;
	unsigned short checksum;
} UDP_HDR;

typedef struct _ICMP_HDR
{
	unsigned char type;
	unsigned char code;
	unsigned short checksum;
	unsigned long rest;
} ICMP_HDR;

typedef struct _ARP_DATA {
	unsigned char hardwareType[2]; 	// Ethernet: 0x0001
	unsigned short proType; 	// ipv4: 0x8000
	unsigned char hardwareSize; 	// mac size
	unsigned char protocolSize;	// ip size
	unsigned short opcode;			// 1: request 2: reply
	char senderMac[6];
	char senderIp[4];
	char targetMac[6];
	char targetIp[4];
} ARP_DATA;

#endif // __PRO_FORMAT_H_