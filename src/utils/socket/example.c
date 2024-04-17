#include <stdio.h> 
#include <winsock2.h>   
#include <ws2tcpip.h> 
#include <time.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>   


#define false 0
#define IPVER   4           //IP协议预定
#define MAX_BUFF_LEN 65500  //发送缓冲区最大值

typedef struct ip_hdr    //定义IP首部 
{
	UCHAR h_verlen;            //4位首部长度,4位IP版本号 
	UCHAR tos;                //8位服务类型TOS 
	USHORT total_len;        //16位总长度（字节） 
	USHORT ident;            //16位标识 
	USHORT frag_and_flags;    //3位标志位和13位偏移大小 
	UCHAR ttl;                //8位生存时间 TTL 
	UCHAR proto;            //8位协议 (TCP, UDP 或其他) 
	USHORT checksum;        //16位IP首部校验和 
	ULONG sourceIP;            //32位源IP地址 
	ULONG destIP;            //32位目的IP地址 
}IP_HEADER; 

typedef struct tsd_hdr //定义TCP伪首部 
{ 
	ULONG saddr;    //源地址
	ULONG daddr;    //目的地址 
	UCHAR mbz;        //
	UCHAR ptcl;        //协议类型 
	USHORT tcpl;    //TCP长度 
}PSD_HEADER; 

typedef struct tcp_hdr //定义TCP首部 
{ 
	USHORT th_sport;            //16位源端口 
	USHORT th_dport;            //16位目的端口 
	ULONG th_seq;                //32位序列号 
	ULONG th_ack;                //32位确认号 
	UCHAR th_lenres;            //4位首部长度/6位保留字 
	UCHAR th_flag;                //6位标志位 
	USHORT th_win;                //16位窗口大小 
	USHORT th_sum;                //16位校验和 
	USHORT th_urp;                //16位紧急数据偏移量 
}TCP_HEADER; 




//CheckSum:计算校验和的子函数 
USHORT checksum(USHORT *buffer, int size) 
{ 
    unsigned long cksum=0; 
    while(size >1) 
    { 
        cksum+=*buffer++; 
        size -=sizeof(USHORT); 
    } 
    if(size) 
    { 
        cksum += *(UCHAR*)buffer; 
    } 

    cksum = (cksum >> 16) + (cksum & 0xffff); 
    cksum += (cksum >>16); 
    return (USHORT)(~cksum); 
} 

int main(int argc, char* argv[]) 
{ 
    WSADATA WSAData;   //一类数据结构，存放windows socket初始化信息 
    SOCKET sock;      
	//socket（）函数创建一个能够进行网络通信的套接字,对于TCP/IP协议族，该参数置AF_INET,第二个参数指定要创建的套接字类型,第三个参数指定应用程序所使用的通信协议 
	//该函数如果调用成功就返回新创建的套接字的描述符，如果失败就返回INVALID_SOCKET。套接字描述符是一个整数类型的值。每个进程的进程空间里都有一个套接字描述符表，该表中存放着套接字描述符和套接字数据结构的对应关系。该表中有一个字段存放新创建的套接字的描述符，另一个字段存放套接字数据结构的地址，因此根据套接字描述符就可以找到其对应的套接字数据结构。每个进程在自己的进程空间里都有一个套接字描述符表但是套接字数据结构都是在操作系统的内核缓冲里。 
    //一般编程中并不直接针对sockaddr数据结构操作，而是使用另一个与sockaddr等价的数据结构，sockaddr_in,其结构体如下: 
	    //sin_family指代协议族，在socket编程中只能是AF_INET
        //sin_port存储端口号（使用网络字节顺序）
        //sin_addr存储IP地址，使用in_addr这个数据结构
        //sin_zero是为了让sockaddr与sockaddr_in两个数据结构保持大小相同而保留的空字节。
        
    IP_HEADER ipHeader; 
    TCP_HEADER tcpHeader; 
    PSD_HEADER psdHeader; 

    char Sendto_Buff[MAX_BUFF_LEN];  //发送缓冲区
    unsigned short check_Buff[MAX_BUFF_LEN]; //检验和缓冲区
    const char tcp_send_data[]={"This is my homework of networt,I am happy!"};

    BOOL flag; 
    int rect,nTimeOver; 
    
//    BOOL Flag=TRUE; 
//    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (char *)&Flag, sizeof(Flag));
//    int timeout=1000;
//    setsockopt(sock, SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout, sizeof(timeout));

    if (argc!= 5) 
    {
        printf("Useage: sendtcp soruce_ip source_port dest_ip dest_port \n"); 
        return false; 
    } 

    if (WSAStartup(MAKEWORD(2,2), &WSAData)!=0) 
	//第一个参数Windows Sockets API提供的调用方可使用的最高版本号
	//第二个参数指向WSADATA数据结构的指针,用来接收Windows Sockets实现的细节.
	//函数允许应用程序或DLL指明Windows Sockets API的版本号及获得特定Windows Sockets实现的细节，应用程序或DLL只能在一次成功的WSAStartup()调用之后才能调用进一步的Windows Sockets API函数. 
    { 
        printf("WSAStartup Error!\n"); 
        return false; 
    } 
	if((sock=WSASocket(AF_INET,SOCK_RAW,IPPROTO_RAW,NULL,0,WSA_FLAG_OVERLAPPED))==INVALID_SOCKET) 
	//socket必须是raw socket才能发送自定义的数据包
	//WSASocket()的发送操作和接收操作都可以被重叠使用。接收函数可以被多次调用，发出接收缓冲区，准备接收到来的数据。发
	//送函数也可以被多次调用，组成一个发送缓冲区队列。可是socket()却只能发过之后等待回消息才可做下一步操作
	//创建一个与指定传送服务提供者捆绑的套接口，可选地创建和/或加入一个套接口组。其功能都是创建一个原始套接字 
	//af:[in]一个地址族规范。目前仅支持AF_INET格式，亦即ARPA Internet地址格式。
	//type:新套接口的类型描述。
    //protocol:套接口使用的特定协议，这里IPPROTO_RAW，表示这个socket只能用来发送IP包，而不能接收任何的数据。发送的数据需要自己填充IP包头，并且自己计算校验和。
	//IPPROTO_IP为用于接收任何的IP数据包。其中的校验和和协议分析由程序自己完成。 
    //lpProtocolInfo:一个指向PROTOCOL_INFO结构的指针，该结构定义所创建套接口的特性。如果本参数非零，则前三个参数(af, type, protocol)被忽略，可以为空。
    //g:保留给未来使用的套接字组。套接口组的标识符。
	//iFlags:套接口属性描述 ，WSA_FLAG_OVERLAPPED表明可以使用发送接收超时设置。 
	//该函数如果调用成功就返回新创建的套接字的描述符，如果失败就返回INVALID_SOCKET。 
    { 
        printf("Socket Setup Error!\n"); 
        printf("send error:%d\n", WSAGetLastError());
        return false; 
    } 
    flag=1; 
	//设置选项值  IP_HDRINCL为要设置的选项值
    if(setsockopt(sock,IPPROTO_IP,IP_HDRINCL,(char*)&flag,sizeof(flag))==SOCKET_ERROR) 
    // 获取或者设置与某个套接字关联的选项。选项可能存在于多层协议中，它们总会出现在最上面的套接字层。
	//当操作套接字选项选项位于的层和选项的名称必须给出。为了操作套接字层的选项，应该将层的值指定为SOL_SOCKET。为了操作其它层的选项，控制选项的合适协议号必须给出。 
	    //sock：将要被设置或者获取选项的套接字。
        //level：选项所在的协议层。
        //optname：需要访问的选项名。
        //optval：指向包含新选项值的缓冲。
        //optlen：现选项的长度 
    //这里表示为操作一个由IP协议解析的代码，所以层应该设定为IP层。即 IPPROTO_IP，设置的控制的方式（选项值）为IP_HDRINCL，意思为在数据包中包含IP首部，表明自己来构造IP头。
	//注意，如果设置IP_HDRINCL选项，那么必须拥有administrator权限。 
	{ 
        printf("setsockopt IP_HDRINCL error!\n"); 
        return false; 
    } 
    nTimeOver=1000; 
	//设置选项值  SO_SNDTIMEO为要设置的选项值
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&nTimeOver, sizeof(nTimeOver))==SOCKET_ERROR) 
    { 
        printf("setsockopt SO_SNDTIMEO error!\n"); 
        return false; 
    } 
    //在send()过程中有时由于网络状况等原因，发送不能预期进行,而设置收发时限，在这里我们使用基本套接字SOL_SOCKET，设置SO_SNDTIMEO表示使用发送超时设置，超时时间设置为1000ms 
    //经过一次性设置后，以后调用send系列的函数，最多只能阻塞 1 秒 
    
    //填充IP首部 
    ipHeader.h_verlen=(IPVER<<4 | sizeof(ipHeader)/sizeof(unsigned long));  
    ipHeader.tos=(UCHAR)0; 
    ipHeader.total_len=htons((unsigned short)sizeof(ipHeader)+sizeof(tcpHeader)+sizeof(tcp_send_data)); 
    ipHeader.ident=0;       //16位标识，用来标识是否分片。 
    ipHeader.frag_and_flags=0; //3位标志位，最低位为MF，0则表示后面没有分片了，DF表示不能分片，分片时置为1。 
    ipHeader.ttl=128; //8位生存时间  
    ipHeader.proto=IPPROTO_TCP; //协议类型，协议类型是什么就交给哪个协议进行处理 
    ipHeader.checksum=0; //检验和暂时为0
    ipHeader.sourceIP=inet_addr(argv[1]);  //32位源IP地址
    ipHeader.destIP=inet_addr(argv[3]);    //32位目的IP地址
    //inet_addr() 函数的作用是将点分十进制的IPv4地址转换成网络字节序列的长整型,将高位和地位交换 
	//网络字节序定义：收到的第一个字节被当作高位看待，这就要求发送端发送的第一个字节应当是高位。a.b.c.d先发送a,再b、c、d。 
    
	//计算IP头部检验和
    memset(check_Buff,0,MAX_BUFF_LEN);
    //将一段内存空间全部设置为某个字符，一般用在对定义的字符串进行初始化，此处是将check_buff全部初始化为‘0’ 
    memcpy(check_Buff,&ipHeader,sizeof(IP_HEADER));
    //用做内存拷贝，可以拿它拷贝任何数据类型的对象，可以指定拷贝的数据长度,strcpy只能拷贝字符串，遇到'/0'就结束拷贝
    ipHeader.checksum=checksum(check_Buff,sizeof(IP_HEADER));
    

    //构造TCP伪首部
    psdHeader.saddr=ipHeader.sourceIP;
    psdHeader.daddr=ipHeader.destIP;
    psdHeader.mbz=0;
    psdHeader.ptcl=ipHeader.proto;
    psdHeader.tcpl=htons(sizeof(TCP_HEADER)+sizeof(tcp_send_data));  //将主机的无符号短整形数转换成网络字节顺序,简单地说,htons()就是将一个数的高低位互换

    //填充TCP首部 
    tcpHeader.th_dport=htons(atoi(argv[4])); //16位目的端口号，atoi()函数用于把字符串转换成整型数，函数会扫描参数字符串，跳过前面的空白字符（例如空格，tab缩进）等，直到遇上数字或正负符号才开始做转换，而在遇到非数字或字符串结束符('\0')才结束转换，并将结果返回 
    tcpHeader.th_sport=htons(atoi(argv[2])); //16位源端口号 
    tcpHeader.th_seq=0;                         //SYN序列号
    tcpHeader.th_ack=0;                         //ACK序列号置为0
    //TCP首部的长度和保留位
    tcpHeader.th_lenres=(sizeof(tcpHeader)/sizeof(unsigned long)<<4|0); 
    tcpHeader.th_flag=2; //修改这里来实现不同的标志位探测，2是SYN，1是//FIN，16是ACK探测 等等 
    tcpHeader.th_win=htons((unsigned short)8192);     //窗口大小
    tcpHeader.th_urp=0;                            //紧急指针，指出本报文段中的紧急数据的字节数    
    tcpHeader.th_sum=0;                            //检验和暂时填为0
    
    //计算TCP校验和 
    memset(check_Buff,0,MAX_BUFF_LEN);
    memcpy(check_Buff,&psdHeader,sizeof(psdHeader)); 
    memcpy(check_Buff+sizeof(psdHeader),&tcpHeader,sizeof(tcpHeader));    //直接从 check_Buff+sizeof(psdHeader)出开始，数组名的值是个指针常量,也就是数组第一个元素的地址。 
    memcpy(check_Buff+sizeof(PSD_HEADER)+sizeof(TCP_HEADER),tcp_send_data,sizeof(tcp_send_data));
    tcpHeader.th_sum=checksum(check_Buff,sizeof(PSD_HEADER)+sizeof(TCP_HEADER)+sizeof(tcp_send_data)); 

    //填充发送缓冲区
    memset(Sendto_Buff,0,MAX_BUFF_LEN);
    memcpy(Sendto_Buff,&ipHeader,sizeof(IP_HEADER));
    memcpy(Sendto_Buff+sizeof(IP_HEADER),&tcpHeader,sizeof(TCP_HEADER)); 
    memcpy(Sendto_Buff+sizeof(IP_HEADER)+sizeof(TCP_HEADER),tcp_send_data,sizeof(tcp_send_data));
    int datasize=sizeof(IP_HEADER)+sizeof(TCP_HEADER)+sizeof(tcp_send_data);
    
    //发送数据报的目的地址
    SOCKADDR_IN dest;    
    memset(&dest,0,sizeof(dest));
    dest.sin_family=AF_INET; 
    dest.sin_addr.s_addr=inet_addr(argv[3]); //inet_addr() 函数的作用是将点分十进制的IPv4地址转换成网络字节序列的长整型,将高位和地位交换 
    dest.sin_port=htons(atoi(argv[4]));
	rect=sendto(sock,Sendto_Buff,datasize, 0,(struct sockaddr*)&dest, sizeof(dest)); 
	if (rect==SOCKET_ERROR) 
	{  
      printf("send error!:%d\n",WSAGetLastError()); 
      return false; 
    } 
    else 
    printf("send ok!\n");
    closesocket(sock);    //关闭套接口，释放套接描述字 
    WSACleanup();   //解除与Socket库的绑定并且释放socket库所占的系统资源 
    return 1; 

}

