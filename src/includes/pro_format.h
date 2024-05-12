#ifndef __PRO_FORMAT_H_
#define __PRO_FORMAT_H_



#define PRO_ARP 0x0806
#define PRO_IPV4 0x0800
#define PRO_IPV6 0X86DD

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
	unsigned char ver_ihl; 			// ver(4bit) << 4 | ihl
	unsigned char serviceType; 		// preference(3bit) delay throughput relibility reserved(2bit)
	unsigned short totalLength; 	// internet header and data
	unsigned short identification;	// assigned by the sender, assembling the fragments of a datagram.
	unsigned short flags_fragmentOffset; // fragmentOffset & 0xff |  (flags(3bit) << 5 | (fragmentOffset >> 8) & 0x1f)
	unsigned char time2live;
	unsigned char protocol;			// PROTO
	unsigned short hdr_chksum;
	unsigned long srcIp;
	unsigned long dstIp;
} IP_HDR;

typedef struct _PSEUDO_HDR{
	unsigned long srcIp;
	unsigned long dstIp;
	char unused;
	unsigned char protocol;
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
	unsigned char type;
	unsigned char code;
	unsigned short checksum;
	unsigned short identifier;
	unsigned short seq;
} ECHO_HDR;

typedef struct _ARP_HDR {
	unsigned short hardwareType; // Ethernet: 0x0001
	unsigned short proType; 		// ipv4: 0x0800
	unsigned char hardwareSize; 	// mac size
	unsigned char protocolSize;	// ip size
	unsigned short opcode;		// 1: request 2: reply
	unsigned char senderMac[6];
	unsigned char senderIp[4];
	unsigned char targetMac[6];
	unsigned char targetIp[4];
} ARP_HDR;

// #pragma pack(1)
typedef struct _TCP_FRAME {
	short padding;
	ETHERNET_HDR eheader;
	IP_HDR ipHeader;
	TCP_HDR tcpHeader;
	char payload[4096];
} TCP_FRAME;

// #pragma pack()

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