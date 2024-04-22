
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <strsafe.h>

#include "nets.h"

int my_copy(char* dst, char* src, int* max, BOOLEAN isW);

void choose_dev(char** ppAdapterName) {
    Intf_Brief *devs = list_devs();
    int i = 0;
    Intf_Brief *cur = devs;
    while(cur) {
        printf("%d %s %S %S\n", ++i, cur->AdapterName, cur->Description, cur->FriendlyName);
        cur = cur->Next;
    }
    int targetNum = 0;
    printf("Enter the interface number (1-%d):", i);
    scanf("%d", &targetNum);
    
    cur = devs;
    for (int j = 0; j < i - 1; cur = cur->Next, j++);
    int len = strlen(cur->AdapterName) + 1;
    *ppAdapterName = malloc(len);
    memcpy(*ppAdapterName, cur->AdapterName, len);
    free(devs);
}



short findAllAdapters(PIP_ADAPTER_ADDRESSES *pAddr) {
    int ret;
    u_long size = 0;
    
    ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, NULL, &size);
    if (ret != ERROR_BUFFER_OVERFLOW) {
        return 0;
    }
    *pAddr = (PIP_ADAPTER_ADDRESSES)malloc(size);
    if (!pAddr) {
        return 0;
    }
    
    ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, *pAddr, &size);
    if (ret != NO_ERROR) {
        return 0;
    }
}

char get_adapter_mac(wchar_t* friendlyName, char* mac) {
    int len = 0;
    PIP_ADAPTER_ADDRESSES pAddr = NULL;
    findAllAdapters(&pAddr);
    PIP_ADAPTER_ADDRESSES pCur = pAddr;
    while(pCur) {
        if (wcscmp(pCur->FriendlyName, friendlyName) == 0) {
            memcpy(mac, pCur->PhysicalAddress, 6);
            len = 6;
            break;
        }
        pCur = pCur->Next;
    }
    free(pAddr);
    return len;
}

Intf_Brief* list_devs() {
    
    PIP_ADAPTER_ADDRESSES pAddr = NULL;
    findAllAdapters(&pAddr);

    int len = sizeof(Intf_Brief);
    int bufsize = 0;
    PIP_ADAPTER_ADDRESSES pCur = pAddr;
    ULONG typeMask = IF_TYPE_IEEE80211 | IF_TYPE_ETHERNET_CSMACD | IF_TYPE_SOFTWARE_LOOPBACK;
    while(pCur) {
        if ((pCur->IfType & typeMask) != 0 && pCur->OperStatus == IfOperStatusUp && pCur->Ipv4Enabled) {
            // printf("%s %S %S\n", pCur->AdapterName, pCur->Description, pCur->FriendlyName);
            bufsize += strlen(pCur->AdapterName) + wcslen(pCur->Description) * 2 + wcslen(pCur->FriendlyName) * 2 + len + 5;
        }
        pCur = pCur->Next;
    }

    pCur = pAddr;    
    Intf_Brief* upDevs = (Intf_Brief*)calloc(1, bufsize);
    Intf_Brief* curDev = upDevs;
    char** ppEnd = 0;
    size_t pRem = 0;
    char* copyPos;
    int cn = 0;
    while(pCur) {
        if ((pCur->IfType & typeMask) != 0 && pCur->OperStatus == IfOperStatusUp && pCur->Ipv4Enabled) {     

            curDev->IfIdx = pCur->IfIndex;
            copyPos = (char*)curDev + len;
            bufsize -= len;

            cn = my_copy(copyPos, pCur->AdapterName, &bufsize, 0);
            curDev->AdapterName = copyPos;
            copyPos += cn;

            cn = my_copy(copyPos, (char*)pCur->Description, &bufsize, 1);
            curDev->Description = (unsigned short*)copyPos;
            copyPos += cn;

            cn = my_copy(copyPos, (char*)pCur->FriendlyName, &bufsize, 1);
            curDev->FriendlyName = (unsigned short*)copyPos;
            copyPos += cn;

            if (bufsize == 0) {
                break;
            }
            curDev->Next = (Intf_Brief*)(copyPos);
            curDev = curDev->Next;
        }
        pCur = pCur->Next;
    }
    curDev->Next = 0;
    free(pAddr);
    return upDevs;
}

int my_copy(char* dst, char* src, int* max, BOOLEAN isW) {
    
    int i = 0;
    if (!isW) {
        for (; i < *max; i++) {
            dst[i] = src[i];
            if (src[i] == 0) {
                i+=1;
                break;
            }       
        }
    } else {
        for (; i < *max; i+=2) {
            dst[i] = src[i];
            dst[i+1]=src[i+1];
            if (src[i] == 0 && src[i+1] == 0) {
                i+=2;
                break;
            }
        }
    }
    *max -= i;
    return i;
}

int get_interface_status(MIB_IFROW* pIfRow, u_long ifIdx) {
    MIB_IFTABLE *pIfTable = NULL;
    u_long tSize;
    int ret;
    ret = GetIfTable(0, &tSize, 0);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
        pIfTable = (MIB_IFTABLE *) malloc(tSize);
    }
    if (ret != ERROR_INSUFFICIENT_BUFFER || pIfTable == NULL) {
        return ret;
    }

    ret = GetIfTable(pIfTable, &tSize, 0);
    if (ret) {
        return ret;
    }

    u_long num = pIfTable->dwNumEntries;
    for (int i = 0; i < num; i++) {
        if (pIfTable->table[i].dwIndex == ifIdx) {
            pIfRow->dwIndex = pIfTable->table[i].dwIndex;
            ret = GetIfEntry(pIfRow);
            break;
        }
    }
    free(pIfTable);
    return ret;
}