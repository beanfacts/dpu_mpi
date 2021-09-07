#include "get_ip.h"

char *find_addr(char *ifname, int is_host)
{
    struct ifaddrs *ifaddrs;
    getifaddrs(&ifaddrs);
    int offset;

    switch (is_host)
    {

        // For custom setup
        case -2:
            offset = 2;
            break;

        // Find my own IP
        case -1:
            offset = 0;
            break;

        // Find the host IP from BlueField
        case 0:
            offset = -100;
            break;

        // Find the BlueField IP from host
        case 1:
            offset = 100;
            break;
    }

    do
    {
        if (strcmp(ifaddrs->ifa_name, ifname) == 0 && ifaddrs->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *bf_addr = (struct sockaddr_in *) ifaddrs->ifa_addr;
            //printf("My address: %s + %d\n", inet_ntoa(bf_addr->sin_addr), offset);
            bf_addr->sin_addr.s_addr = htonl(ntohl(bf_addr->sin_addr.s_addr) + offset);
            //printf("Transformed: %s + %d\n", inet_ntoa(bf_addr->sin_addr), offset);
            return inet_ntoa(bf_addr->sin_addr);
        }
        else
        {
            ifaddrs = ifaddrs->ifa_next;
        }
    }
    while (ifaddrs->ifa_next);

    return NULL;
}



char *offset_addr(char *ifname, int offset)
{
    struct ifaddrs *ifaddrs;
    getifaddrs(&ifaddrs);

    do
    {
        if (strcmp(ifaddrs->ifa_name, ifname) == 0 && ifaddrs->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *bf_addr = (struct sockaddr_in *) ifaddrs->ifa_addr;
            //printf("My address: %s + %d\n", inet_ntoa(bf_addr->sin_addr), offset);
            bf_addr->sin_addr.s_addr = htonl(ntohl(bf_addr->sin_addr.s_addr) + offset);
            //printf("Transformed: %s + %d\n", inet_ntoa(bf_addr->sin_addr), offset);
            return inet_ntoa(bf_addr->sin_addr);
        }
        else
        {
            ifaddrs = ifaddrs->ifa_next;
        }
    }
    while (ifaddrs->ifa_next);

    return NULL;
}