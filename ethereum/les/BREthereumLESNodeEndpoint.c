//
//  BREthereumLESNodeEndpoint.c
//  Core
//
//  Created by Ed Gamble on 8/14/18.
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <assert.h>

#include "../util/BRUtil.h"
#include "BREthereumLESNodeEndpoint.h"

#ifndef MSG_NOSIGNAL   // linux based systems have a MSG_NOSIGNAL send flag, useful for supressing SIGPIPE signals
#define MSG_NOSIGNAL 0 // set to 0 if undefined (BSD has the SO_NOSIGPIPE sockopt, and windows has no signals at all)
#endif

#define CONNECTION_TIME 3.0

/** Forward Declarations */
static int
openSocket (BREthereumLESNodeEndpoint *endpoint, int *socket, int domain, int type, double timeout, int *error);

//
// MARK: - LES Node Endpoint
//
extern BREthereumLESNodeEndpoint
nodeEndpointCreateRaw (const char *address,
                       uint16_t portUDP,
                       uint16_t portTCP,
                       BRKey key,
                       BRKey ephemeralKey,
                       UInt256 nonce) {
    BREthereumLESNodeEndpoint endpoint = nodeEndpointCreate(address, portUDP, portTCP, key);
    endpoint.ephemeralKey = ephemeralKey;
    endpoint.nonce = nonce;

    return endpoint;
}

extern BREthereumLESNodeEndpoint
nodeEndpointCreate (const char *address,
                    uint16_t portUDP,
                    uint16_t portTCP,
                    BRKey key) {
    BREthereumLESNodeEndpoint endpoint;
    memset (&endpoint, 0, sizeof (BREthereumLESNodeEndpoint));

    endpoint.addr_family = AF_INET; // ??
    strncpy (endpoint.hostname, address, MAX_HOST_NAME);
    endpoint.isIPV4Address = 1;

    endpoint.portUDP = portUDP;
    endpoint.socketUDP = -1;

    endpoint.portTCP = portTCP;
    endpoint.socketTCP = -1;

    endpoint.key = key;
    endpoint.nonce = UINT256_ZERO;
    BRKeyClean(&endpoint.ephemeralKey);

    return endpoint;
}

extern BRKey
nodeEndpointGetKey (BREthereumLESNodeEndpoint endpoint) {
    return endpoint.key;
}

extern void
nodeEndpointSetHello (BREthereumLESNodeEndpoint *endpoint,
                      BREthereumP2PMessage hello) {
    endpoint->hello = hello;
}

extern void
nodeEndpointSetStatus (BREthereumLESNodeEndpoint *endpoint,
                       BREthereumLESMessage status) {
    endpoint->status = status;
}

extern void
nodeEndpointOpen (BREthereumLESNodeEndpoint *endpoint) {
    int error = 0;

    if (endpoint->portTCP) assert (openSocket(endpoint, &endpoint->socketTCP, PF_INET, SOCK_STREAM, CONNECTION_TIME, &error));
    if (endpoint->portUDP) assert (openSocket(endpoint, &endpoint->socketUDP, PF_INET, SOCK_DGRAM,  CONNECTION_TIME, &error));
}

extern void
nodeEndpointClose (BREthereumLESNodeEndpoint *endpoint) {
    int socket;

    // Close the TCP socket.
    socket = endpoint->socketTCP;
    if (socket >= 0) {
        endpoint->socketTCP = -1;
        if (shutdown (socket, SHUT_RDWR) < 0){
            eth_log (LES_LOG_TOPIC, "Socket TCP Shutdown Error: %s", strerror(errno));
        }
        close (socket);
    }

    // Close the UDP socket
    socket = endpoint->socketUDP;
    if (socket >= 0) {
        endpoint->socketUDP = -1;
        if (shutdown (socket, SHUT_RDWR) < 0){
            eth_log (LES_LOG_TOPIC, "Socket UDP Shutdown Error: %s", strerror(errno));
        }
        close (socket);
    }
}

extern int
nodeEndpointIsOpen (BREthereumLESNodeEndpoint *endpoint) {
    return -1 != endpoint->socketTCP || -1 != endpoint->socketUDP;
}

extern int
nodeEndpointHasRecvDataAvailable (BREthereumLESNodeEndpoint *endpoint,
                                  fd_set *readFds) {
    return (NULL == readFds ||
            FD_ISSET (endpoint->socketTCP, readFds) ||
            FD_ISSET (endpoint->socketUDP, readFds));
}

extern void 
nodeEndpointSetRecvDataAvailableFDS (BREthereumLESNodeEndpoint *endpoint,
                                     fd_set *readFds) {
    FD_SET (endpoint->socketTCP, readFds);
    FD_SET (endpoint->socketUDP, readFds);
}

extern void
nodeEndpointClrRecvDataAvailableFDS (BREthereumLESNodeEndpoint *endpoint,
                                     fd_set *readFds) {
    FD_CLR (endpoint->socketTCP, readFds);
    FD_CLR (endpoint->socketUDP, readFds);
}

extern int
nodeEndpointGetRecvDataAvailableFDSNum (BREthereumLESNodeEndpoint *endpoint) {
    return 1 + (endpoint->socketTCP > endpoint->socketUDP
                ? endpoint->socketTCP
                : endpoint->socketUDP);
}

/// MARK: - Recv Data

extern int
nodeEndpointRecvData (BREthereumLESNodeEndpoint *endpoint,
                            int socket,
                            uint8_t *bytes,
                            size_t *bytesCount,
                            int needBytesCount) {

    ssize_t totalCount = 0;
    int error = 0;

    if (socket < 0) error = ENOTCONN;

    while (socket >= 0 && !error && totalCount < *bytesCount) {
        ssize_t n = recv (socket, &bytes[totalCount], *bytesCount - totalCount, 0);
        if (n == 0) error = ECONNRESET;
        else if (n < 0 && errno != EWOULDBLOCK) error = errno;
        else {
            totalCount += n;
            if (!needBytesCount) break;
        }

        socket = endpoint->socketTCP;
    }

    if (error)
        eth_log(LES_LOG_TOPIC, "[READ (TCP) FROM PEER ERROR]:%s", strerror(error));
    else {
        *bytesCount = totalCount;
#if defined (LES_LOG_RECV)
        eth_log(LES_LOG_TOPIC, "read (%zu bytes) from peer [%s], contents: %s", len, endpoint->hostname, "?");
#endif
    }

    return error;
}

/// MARK: - Send Data

extern int
nodeEndpointSendData (BREthereumLESNodeEndpoint *endpoint,
                      int socket,
                      uint8_t *bytes,
                      size_t bytesCount) {

    ssize_t n = 0;
    int error = 0;
    size_t offset = 0;

#if defined (NEED_TO_PRINT_UDP_SEND_DATA)
    {
        char hex[1 + 2 * bytesCount];
        encodeHex(hex, 1 + 2 * bytesCount, bytes, bytesCount);
        printf ("Bytes: %s\n", hex);
    }
#endif

    if (socket < 0) error = ENOTCONN;

    while (socket >= 0 && !error && offset <  bytesCount) {
        n = send (socket, &bytes[offset], bytesCount - offset, MSG_NOSIGNAL);
        if (n >= 0) offset += n;
        if (n < 0 && errno != EWOULDBLOCK) error = errno;

    }

    if (error)
        eth_log(LES_LOG_TOPIC, "[WRITE TO PEER ERROR]:%s", strerror(error));
    else {
#if defined (LES_LOG_SEND)
        eth_log(LES_LOG_TOPIC, "sent (%zu bytes) to peer[%s], contents: %s", offset, endpoint->hostname, "?");
#endif
    }

    return error;
}

// https://stackoverflow.com/questions/27014955/socket-connect-vs-bind
// http://www.microhowto.info/howto/listen_for_and_receive_udp_datagrams_in_c.html


/**
 * Note: This function is a direct copy of Aaron's _BRPeerOpenSocket function with a few modifications to
 * work for the Ethereum Core.
 * TODO: May want to make this more modular to work for both etheruem and bitcoin
 */
static int
openSocket(BREthereumLESNodeEndpoint *endpoint, int *socketToAssign, int domain, int type, double timeout, int *error)
{
    struct sockaddr_storage addr;
    struct timeval tv;
    fd_set fds;
    socklen_t addrLen, optLen;
    int count, arg = 0, err = 0, on = 1, r = 1;

    *socketToAssign = socket(domain, type, 0);

    if (*socketToAssign < 0) {
        err = errno;
        r = 0;
    }
    else {
        tv.tv_sec = 10; // one second timeout for send/receive, so thread doesn't block for too long
        tv.tv_usec = 0;
        setsockopt(*socketToAssign, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(*socketToAssign, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        setsockopt(*socketToAssign, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
#ifdef SO_NOSIGPIPE // BSD based systems have a SO_NOSIGPIPE socket option to supress SIGPIPE signals
        setsockopt(*socketToAssign, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
        arg = fcntl(*socketToAssign, F_GETFL, NULL);
        if (arg < 0 || fcntl(*socketToAssign, F_SETFL, arg | O_NONBLOCK) < 0) r = 0; // temporarily set socket non-blocking
        if (! r) err = errno;
    }

    if (r) {
        memset(&addr, 0, sizeof(addr));
        uint16_t port = endpoint->portTCP; // endpointGetTCP(node->peer.endpoint);
        const char* address = endpoint->hostname; // endpointGetHost(node->peer.endpoint);

        if (domain == PF_INET6) {
            struct sockaddr_in6 * addr_in6 = ((struct sockaddr_in6 *)&addr);
            addr_in6->sin6_family = AF_INET6;
            inet_pton(AF_INET6, address, &(addr_in6->sin6_addr));
            addr_in6->sin6_port = htons(port);
            addrLen = sizeof(struct sockaddr_in6);
        }
        else {
            struct sockaddr_in* addr_in4 = ((struct sockaddr_in *)&addr);
            addr_in4->sin_family = AF_INET;
            inet_pton(AF_INET, address, &(addr_in4->sin_addr));
            addr_in4->sin_port = htons(port);
            addrLen = sizeof(struct sockaddr_in);
        }
        if (connect(*socketToAssign, (struct sockaddr *)&addr, addrLen) < 0) {
            err = errno;
        }

        if (err == EINPROGRESS) {
            err = 0;
            optLen = sizeof(err);
            tv.tv_sec = timeout;
            tv.tv_usec = (long)(timeout*1000000) % 1000000;
            FD_ZERO(&fds);
            FD_SET(*socketToAssign, &fds);
            count = select(*socketToAssign + 1, NULL, &fds, NULL, &tv);

            if (count <= 0 || getsockopt(*socketToAssign, SOL_SOCKET, SO_ERROR, &err, &optLen) < 0 || err) {
                if (count == 0) err = ETIMEDOUT;
                if (count < 0 || ! err) err = errno;
                r = 0;
            }
        }
        else if (err && domain == PF_INET && endpoint->isIPV4Address) {
            if (*socketToAssign >= 0) { close (*socketToAssign); *socketToAssign = -1; }
            return openSocket(endpoint, socketToAssign, PF_INET6, type, timeout, error); // fallback to IPv4
        }
        else if (err) r = 0;

        if (r) {
            eth_log(LES_LOG_TOPIC,"%s","ethereum socket TCP connected");
        }
        fcntl(*socketToAssign, F_SETFL, arg); // restore socket non-blocking status
    }

    if (! r && err) {
        eth_log( LES_LOG_TOPIC, "ethereum connect TCP error: %s", strerror(err));
    }
    if (error && err) *error = err;
    return r;
}

#if 0
func getWiFiAddress() -> String? {
    var address : String?

    // Get list of all interfaces on the local machine:
    var ifaddr : UnsafeMutablePointer<ifaddrs>?
    guard getifaddrs(&ifaddr) == 0 else { return nil }
    guard let firstAddr = ifaddr else { return nil }

    // For each interface ...
    for ifptr in sequence(first: firstAddr, next: { $0.pointee.ifa_next }) {
        let interface = ifptr.pointee

        // Check for IPv4 or IPv6 interface:
        let addrFamily = interface.ifa_addr.pointee.sa_family
        if addrFamily == UInt8(AF_INET) || addrFamily == UInt8(AF_INET6) {

            // Check interface name:
            let name = String(cString: interface.ifa_name)
            if  name == "en0" {

                // Convert interface address to a human readable string:
                var hostname = [CChar](repeating: 0, count: Int(NI_MAXHOST))
                getnameinfo(interface.ifa_addr, socklen_t(interface.ifa_addr.pointee.sa_len),
                            &hostname, socklen_t(hostname.count),
                            nil, socklen_t(0), NI_NUMERICHOST)
                address = String(cString: hostname)
            }
        }
    }
    freeifaddrs(ifaddr)

    return address
    }
#endif


#if 0
#include <stdio.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

    extern int
    main(int argc, const char **argv) {
        struct ifaddrs *interfaces = NULL;
        if (getifaddrs(&interfaces) == 0) {
            printf ("%7s %10s %25s\n", "name", "family", "address");
            for (struct ifaddrs *ifa = interfaces; ifa; ifa = ifa->ifa_next) {
                char buf[128];
                if (ifa->ifa_addr->sa_family == AF_INET) {
                    inet_ntop(AF_INET, (void *)&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr,
                              buf, sizeof(buf));
                } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                    inet_ntop(AF_INET6, (void *)&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr,
                              buf, sizeof(buf));
                } else {
                    continue;
                }

                printf ("%7s %8u %40s\n",
                        ifa->ifa_name,
                        (unsigned)ifa->ifa_addr->sa_family,
                        buf);
            }
        }
        freeifaddrs(interfaces);
        return 0;
    }

    //ebg@ebg(93)$ ./a.out
    //  name     family                   address
    //  lo0        2                                127.0.0.1
    //  lo0       30                                      ::1
    //  lo0       30                                  fe80::1
    //  en0       30                 fe80::4a1:bcaa:1aa2:6c8e
    //  en0        2                              172.20.10.2
    //awdl0       30                fe80::e4ac:5fff:fed0:d48b
    //utun0       30                fe80::c78b:cd3f:18a6:dd87
    //utun1       30                fe80::31e2:25c9:1c31:8293
    //utun1       30    fdc7:e48b:d7:aa3a:31e2:25c9:1c31:8293
    //utun2       30                fe80::31e2:25c9:1c31:8293
    //utun2       30  fdc8:3cab:5fba:b1b0:31e2:25c9:1c31:8293
    //utun3       30                fe80::2d34:ab2e:f86b:f077

#endif
