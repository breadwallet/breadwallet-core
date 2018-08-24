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

// #define NEED_TO_PRINT_SEND_DATA
// #define LES_LOG_RECV
// #define LES_LOG_SEND

#define CONNECTION_TIME 3.0

/** Forward Declarations */
static int
openSocket (BREthereumLESNodeEndpoint *endpoint, int *socket, int port, int domain, int type, double timeout);

//
// MARK: - LES Node Endpoint
//
extern BREthereumLESNodeEndpoint
nodeEndpointCreateDetailed (BREthereumDISEndpoint dis,
                            BRKey key,
                            BRKey ephemeralKey,
                            UInt256 nonce) {
    BREthereumLESNodeEndpoint endpoint;
    memset (&endpoint, 0, sizeof (BREthereumLESNodeEndpoint));

    endpoint.dis = dis;

    for (int i = 0; i < NUMBER_OF_NODE_ROUTES; i++)
        endpoint.sockets[i] = -1;

    endpoint.timestamp = 0;

    endpoint.key = key;
    endpoint.nonce = UINT256_ZERO;
    endpoint.ephemeralKey = ephemeralKey;

    inet_ntop (dis.domain, (void *) &dis.addr, endpoint.hostname, _POSIX_HOST_NAME_MAX + 1);

    return endpoint;
}

extern BREthereumLESNodeEndpoint
nodeEndpointCreate (BREthereumDISEndpoint dis,
                    BRKey key) {
    UInt256 nonce = UINT256_ZERO;
    BRKey ephemeralKey;
    BRKeyClean(&ephemeralKey);

    return nodeEndpointCreateDetailed (dis, key, ephemeralKey, nonce);
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

extern int // errno
nodeEndpointOpen (BREthereumLESNodeEndpoint *endpoint,
                  BREthereumLESNodeEndpointRoute route) {
    if (nodeEndpointIsOpen (endpoint, route)) return 0;
    
    return openSocket (endpoint,
                       &endpoint->sockets[route],
                       (route == NODE_ROUTE_TCP ? endpoint->dis.portTCP : endpoint->dis.portUDP),
                       endpoint->dis.domain,
                       (route == NODE_ROUTE_TCP ? SOCK_STREAM : SOCK_DGRAM),
                       CONNECTION_TIME);
}

extern int
nodeEndpointClose (BREthereumLESNodeEndpoint *endpoint,
                   BREthereumLESNodeEndpointRoute route) {
    int socket;

    socket = endpoint->sockets[route];
    if (socket >= 0) {
        endpoint->sockets[route] = -1;

        if (shutdown (socket, SHUT_RDWR) < 0) {
            eth_log (LES_LOG_TOPIC, "Socket %d (%s) Shutdown Error: %s", socket,
                     nodeEndpointRouteGetName (route),
                     strerror(errno));
            return errno;
        }
        if (close (socket) < 0) {
            eth_log (LES_LOG_TOPIC, "Socket %d (%s) Close Error: %s", socket,
                     nodeEndpointRouteGetName (route),
                     strerror(errno));
            return errno;
        }
    }
    return 0;
}

extern int
nodeEndpointIsOpen (BREthereumLESNodeEndpoint *endpoint,
                    BREthereumLESNodeEndpointRoute route) {
    return -1 != endpoint->sockets[route];
}

/// MARK: - Recv Data

extern int
nodeEndpointRecvData (BREthereumLESNodeEndpoint *endpoint,
                      BREthereumLESNodeEndpointRoute route,
                      uint8_t *bytes,
                      size_t *bytesCount,
                      int needBytesCount) {

    ssize_t totalCount = 0;
    int error = 0;

    int socket = endpoint->sockets[route];

    if (socket < 0) error = ENOTCONN;

    while (socket >= 0 && !error && totalCount < *bytesCount) {
        ssize_t n = recv (socket, &bytes[totalCount], *bytesCount - totalCount, 0);
        if (n == 0) error = ECONNRESET;
        else if (n < 0 && errno != EWOULDBLOCK) error = errno;
        else {
            totalCount += n;
            if (!needBytesCount) break;
        }

        socket = endpoint->sockets[route];
    }

    if (error)
        eth_log (LES_LOG_TOPIC, "Recv: %s %zu bytes <= %s Error: %s",
                 (route == NODE_ROUTE_TCP ? "TCP" : "UDP"),
                 *bytesCount,
                 endpoint->hostname,
                 strerror(error));
    else {
        // !! DON'T MISS THIS !!
        *bytesCount = totalCount;
#if defined (LES_LOG_RECV)
        eth_log (LES_LOG_TOPIC, "read (%zu bytes) from peer [%s], contents: %s", len, endpoint->hostname, "?");
#endif
    }

    return error;
}

/// MARK: - Send Data

extern int
nodeEndpointSendData (BREthereumLESNodeEndpoint *endpoint,
                      BREthereumLESNodeEndpointRoute route,
                      uint8_t *bytes,
                      size_t bytesCount) {
    int error = 0;
    size_t totalCount = 0;

    int socket = endpoint->sockets[route];

#if defined (NEED_TO_PRINT_SEND_DATA)
    {
        char hex[1 + 2 * bytesCount];
        encodeHex(hex, 1 + 2 * bytesCount, bytes, bytesCount);
        printf ("Bytes: %s\n", hex);
    }
#endif

    if (socket < 0) error = ENOTCONN;

    while (socket >= 0 && !error && totalCount < bytesCount) {
        ssize_t n = send (socket, &bytes[totalCount], bytesCount - totalCount, MSG_NOSIGNAL);
        if (n >= 0) totalCount += n;
        if (n < 0 && errno != EWOULDBLOCK) error = errno;

        socket = endpoint->sockets[route];
    }

    if (error)
        eth_log (LES_LOG_TOPIC, "Send: %s %zu bytes => %s Error: %s",
                 (route == NODE_ROUTE_TCP ? "TCP" : "UDP"),
                 bytesCount,
                 endpoint->hostname,
                 strerror(error));
    else {
#if defined (LES_LOG_SEND)
        eth_log (LES_LOG_TOPIC, "sent (%zu bytes) to peer[%s], contents: %s", totalCount, endpoint->hostname, "?");
#endif
    }

    return error;
}

// https://stackoverflow.com/questions/27014955/socket-connect-vs-bind
// http://www.microhowto.info/howto/listen_for_and_receive_udp_datagrams_in_c.html


static void
nodeEndpointFillSockAddr (BREthereumLESNodeEndpoint *endpoint,
                          int port,
                          struct sockaddr_storage *addr,
                          socklen_t *addrLen) {
    // Clear addr
    memset(addr, 0, sizeof(struct sockaddr_storage));

    switch (endpoint->dis.domain) {
        case AF_INET: {
            struct sockaddr_in *addr_in4 = (struct sockaddr_in *) addr;

            addr_in4->sin_family = AF_INET;
            addr_in4->sin_port = htons(port);
            addr_in4->sin_addr = * (struct in_addr *) endpoint->dis.addr.ipv4;

            *addrLen = sizeof (struct sockaddr_in);
            break;
        }
        case AF_INET6: {
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *) addr;

            addr_in6->sin6_family = AF_INET6;
            addr_in6->sin6_port = htons(port);
            addr_in6->sin6_addr = * (struct in6_addr *) endpoint->dis.addr.ipv6;

            *addrLen = sizeof (struct sockaddr_in6);
            break;
        }
        default:
            assert (0);
    }
}

static int
openSocketReportResult (BREthereumLESNodeEndpoint *endpoint, int port, int type, int error) {
    eth_log (LES_LOG_TOPIC, "Open: %s @ %d => %s %s%s",
             (type == SOCK_STREAM ? "TCP" : "UDP"),
             port,
             endpoint->hostname,
             (0 == error ? "Success " : "Error: "),
             (0 == error ? "" : strerror(error)));
    return error;
}

/**
 * Note: This function is a direct copy of Aaron's _BRPeerOpenSocket function with a few modifications to
 * work for the Ethereum Core.
 * TODO: May want to make this more modular to work for both etheruem and bitcoin
 */
static int
openSocket(BREthereumLESNodeEndpoint *endpoint, int *socketToAssign, int port, int domain, int type, double timeout)
{
    struct sockaddr_storage addr;
    struct timeval tv;
    fd_set fds;
    socklen_t addrLen, optLen;
    int count, arg = 0, err = 0, on = 1, r = 1;

    *socketToAssign = socket (domain, type, 0);

    if (*socketToAssign < 0) return openSocketReportResult (endpoint, port, type, errno);

    tv.tv_sec = 10; // one second timeout for send/receive, so thread doesn't block for too long
    tv.tv_usec = 0;
    setsockopt(*socketToAssign, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(*socketToAssign, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(*socketToAssign, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
#ifdef SO_NOSIGPIPE // BSD based systems have a SO_NOSIGPIPE socket option to supress SIGPIPE signals
    setsockopt(*socketToAssign, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif

    // Set the socket non-blocking (temporarily).
    arg = fcntl(*socketToAssign, F_GETFL, NULL);
    if (arg < 0 || fcntl(*socketToAssign, F_SETFL, arg | O_NONBLOCK) < 0)
        return openSocketReportResult (endpoint, port, type, errno);

    // Fill `addr` with the remote endpoint
    nodeEndpointFillSockAddr(endpoint, port, &addr, &addrLen);

    // Attempt to connect; if success, we are done
    if (0 == connect (*socketToAssign, (struct sockaddr *)&addr, addrLen) < 0) {
        fcntl (*socketToAssign, F_SETFL, arg); // restore socket non-blocking status
        return openSocketReportResult (endpoint, port, type, 0);
    }

    // If EINPROGRESS, wait until good to go and try again
    if (errno == EINPROGRESS) {
        err = 0;
        optLen = sizeof(err);
        tv.tv_sec = timeout;
        tv.tv_usec = (long)(timeout*1000000) % 1000000;
        FD_ZERO(&fds);
        FD_SET(*socketToAssign, &fds);
        count = select (*socketToAssign + 1, NULL, &fds, NULL, &tv);

        if (count <= 0 || getsockopt (*socketToAssign, SOL_SOCKET, SO_ERROR, &err, &optLen) < 0 || err) {
            if (count == 0) err = ETIMEDOUT;
            if (count < 0 || ! err) err = errno;
            r = 0;
        }

        fcntl (*socketToAssign, F_SETFL, arg);
        return openSocketReportResult (endpoint, port, type, err);
    }

    // We've failed
    fcntl (*socketToAssign, F_SETFL, arg);
    return openSocketReportResult (endpoint, port, type, errno);
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
