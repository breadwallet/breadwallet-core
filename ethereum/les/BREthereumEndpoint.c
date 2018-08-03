//
//  BREthereumEndpoint.c
//  breadwallet
//
//  Created by Lamont Samuels on 5/23/18.
//  Copyright (c) 2018 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "../util/BRUtil.h"
#include "BREthereumEndpoint.h"

#define ETH_LOG_TOPIC "Endpoint"
#define MAX_HOST_NAME 1024

struct BREthereumEndpointContext {
    int addr_family;  //  BE encoded 4-byte or 16-byte address (size determines ipv4 vs ipv6)
    char hostname[MAX_HOST_NAME];
    BREthereumBoolean isIPV4Address; // Determines whether the address is a IPV4 (True) or IPV6 (False)
    uint16_t udpPort; // BE encoded 16-bit unsigned
    uint16_t tcpPort; // BE encoded 16-bit unsigned
};
extern BREthereumEndpoint ethereumEndpointCreate(BREthereumBoolean isIPV4Address, char*address, uint16_t udpPort, uint16_t tcpPort) {

    BREthereumEndpoint endpoint = (BREthereumEndpoint)calloc(1,sizeof(struct BREthereumEndpointContext));
    assert(endpoint != NULL);
    assert(address != NULL);
    endpoint->tcpPort = tcpPort;
    endpoint->udpPort = udpPort;
    strcpy(endpoint->hostname, address);
    endpoint->isIPV4Address = isIPV4Address;
    return endpoint;
}
extern void ethereumEndpointRelease(BREthereumEndpoint endpoint) {
    free(endpoint);
}
extern uint16_t ethereumEndpointGetTCP(BREthereumEndpoint endpoint) {
    return endpoint->tcpPort;
}
extern uint16_t ethereumEndpointGetUDP(BREthereumEndpoint endpoint) {
    return endpoint->udpPort;
}
extern const char* ethereumEndpointGetHost(BREthereumEndpoint endpoint) {
    return endpoint->hostname;
}
extern BREthereumBoolean ethereumEndpointIsIPV4(BREthereumEndpoint endpoint) {
    return endpoint->isIPV4Address;
}
