#ifndef __HELLOWORLD_H__
#define __HELLOWORLD_H__

struct henv
{
    char msg[6];
#ifdef __cplusplus
    int getVersion() { return 12;}
#endif

};

#endif // __HELLOWORLD_H__