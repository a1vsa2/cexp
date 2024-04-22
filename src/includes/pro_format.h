#ifndef __PRO_FORMAT_H_
#define __PRO_FORMAT_H_



#define PRO_TYPE_ARP 0x0806
#define PRO_TYPE_IPV4 0x0800

#define ARP_REQ 0x0001
#define ARP_REP 0x0002

// 网络协议都是大端
typedef struct _ETHERNET_HDR {
	char dest_mac[6];
	char src_mac[6];
	short proType;
} ETHERNET_HDR;


// https://www.rfc-editor.org/rfc/rfc791
typedef struct _IP_HDR {
	char ver_ihl; 			// ver(4bit) << 4 | ihl
	char serviceType; 		// preference(3bit) delay throughput relibility reserved(2bit)
	unsigned short totalLength; 	// internet header and data
	unsigned short identification;	// assigned by the sender, assembling the fragments of a datagram.
	unsigned short flags_fragmentOffset; // fragmentOffset & 0xff |  (flags(3bit) << 5 | (fragmentOffset >> 8) & 0x1f)
	unsigned char time2live;
	char protocol;			// PROTO
	unsigned short hdr_chksum;
	long srcIp;
	long dstIp;
} IP_HDR;

typedef struct _PSEUDO_HDR{
	long srcIp;
	long dstIp;
	char unused;
	char protocol;
	unsigned short tcpLen;
} PSEUDO_HDR;

// https://www.rfc-editor.org/rfc/rfc9293#name-header-format
typedef struct _TCP_HDR {
	short srcPort;
	short dstPort;
	unsigned long seq;
	unsigned long ack;
	unsigned char dOffset_rsrvd;	// dataOffset(4bit): indicates where the data begins
	char flags;			// CRW ECE URG ACK PSH RST SYN FIN
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

// https://www.rfc-editor.org/rfc/rfc792
typedef struct _ECHO_HDR
{
	char type;
	char code;
	short checksum;
	short identifier;
	short seq;
} ECHO_HDR;

typedef struct _ARP_HDR {
	short hardwareType; // Ethernet: 0x0001
	short proType; 		// ipv4: 0x0800
	char hardwareSize; 	// mac size
	char protocolSize;	// ip size
	short opcode;		// 1: request 2: reply
	char senderMac[6];
	char senderIp[4];
	char targetMac[6];
	char targetIp[4];
} ARP_HDR;

#pragma pack(1)
typedef struct _TCP_HFRAME {
	ETHERNET_HDR eheader;
	IP_HDR ipHeader;
	TCP_HDR tcpHeader;
} TCP_HFRAME;

#pragma pack()

typedef struct _LOOPBACK_HFRAME {
	long ehdr;
	IP_HDR ipHeader;
	TCP_HDR tcpHeader;
	// payload
} LOOPBACK_HFRAME;

typedef struct _ARP_FRAME {
	ETHERNET_HDR eheader;
	ARP_HDR arpHeader;
} ARP_FRAME;

typedef struct _ECHO_HFRAME {
	ETHERNET_HDR eheader;
	IP_HDR ipHeader;
	ECHO_HDR echoHeader;
	// payload
} ECHO_HFRAME;


#endif // __PRO_FORMAT_H_