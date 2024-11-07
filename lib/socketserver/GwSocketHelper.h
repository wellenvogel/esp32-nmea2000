#pragma once
#include <lwip/sockets.h>

class GwSocketHelper{
    public:
        static bool setKeepAlive(int socket, bool noDelay=true){
            int val=1;
            if (noDelay){
                if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(int)) != ESP_OK) return false;
            }
            if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&val, sizeof(int)) != ESP_OK) return false;
            val = 10; // seconds of idleness before first keepalive probe
            if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) != ESP_OK) return false;
            val = 5; // interval between first and subsequent keepalives
            if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) != ESP_OK) return false;
            val = 4; // number of lost keepalives before we close the socket
            if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) != ESP_OK) return false;
            return true;  
        }
        static bool isMulticast(const String &addr){
            in_addr iaddr;
            if (inet_pton(AF_INET,addr.c_str(),&iaddr) != 1) return false;
            return IN_MULTICAST(ntohl(iaddr.s_addr));
        }
};