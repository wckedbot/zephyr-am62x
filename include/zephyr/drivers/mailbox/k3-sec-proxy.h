#ifndef K3_SEC_PROXY_H
#define K3_SEC_PROXY_H


#include <zephyr/device.h>

struct k3_sec_proxy_msg 
{
    size_t len;
    uint32_t *buf;
};

#endif