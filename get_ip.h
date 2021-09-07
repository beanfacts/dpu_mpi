#ifndef GET_IP_H
#define GET_IP_H

#include <sys/types.h>
#include <ifaddrs.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

int worldsize;
/*
    Get the BlueField/Host address of this node
    [in]    ifname  -> Interface name
    [in]    is_host -> Are you the host or BlueField? - or to find your own address, pass in -1
    [out]   bf_addr -> BlueField address string
*/
char *find_addr(char *ifname, int is_host);

char *offset_addr(char *ifname, int offset);

#endif