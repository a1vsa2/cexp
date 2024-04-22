#ifndef __NETS_H_
#define __NETS_H_

typedef struct _Interface_Brief {
    unsigned long long IfIdx;
    struct _Interface_Brief* Next;
    char *AdapterName;
    unsigned short *Description;
    unsigned short * FriendlyName;
} Intf_Brief;

Intf_Brief* list_devs();
void choose_dev(char** name);
char get_adapter_mac(wchar_t* friendlyName, char* mac);

#endif // __NETS_H_