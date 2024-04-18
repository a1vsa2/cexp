
#include <stdlib.h>

#include <winsock2.h>
#include <iphlpapi.h>

int get_interface_status(MIB_IFROW* pIfRow, u_long ifIdx);


void get_ip_interface_info(u_long ip4Addr) {
    u_long ret = 0;
    u_long ifIdx;
    ret = GetBestInterface(ip4Addr, ifIdx);
    if (ret) {
        return;
    }
    

    // PIP_ADAPTER_ADDRESSES pAddr = NULL;
    // u_long size = 0;
    // ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, NULL, &size);
    // if (ret != ERROR_BUFFER_OVERFLOW) {
    //     return;
    // }
    // pAddr = (PIP_ADAPTER_ADDRESSES)malloc(size);
    // if (!pAddr) {
    //     return;
    // }
    // ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddr, &size);
    // if (ret == NO_ERROR) {
    //     return;
    // }
    // while(pAddr) {
    //     if (pAddr->IfIndex == ifIdx) {
    //         break;
    //     }
    //     pAddr = pAddr->Next;
    // }
    
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