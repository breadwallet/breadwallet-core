//
//  BREthereumNodeEndpoint.c
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
#include <limits.h>

#ifndef HOST_NAME_MAX
# if defined(_POSIX_HOST_NAME_MAX)
#  define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
# elif defined(MAXHOSTNAMELEN)
#  define HOST_NAME_MAX MAXHOSTNAMELEN
# elif defined(_SC_HOST_NAME_MAX)
#  define HOST_NAME_MAX _SC_HOST_NAME_MAX
# else
#  error HOST_NAME_MAX is undefined
# endif
#endif /* HOST_NAME_MAX */

#include "../util/BRUtil.h"
#include "BREthereumNodeEndpoint.h"

#ifndef MSG_NOSIGNAL   // linux based systems have a MSG_NOSIGNAL send flag, useful for supressing SIGPIPE signals
#define MSG_NOSIGNAL 0 // set to 0 if undefined (BSD has the SO_NOSIGPIPE sockopt, and windows has no signals at all)
#endif

/** Forward Declarations */
static int
openSocket (BREthereumNodeEndpoint endpoint, int *socket, int port, int domain, int type, double timeout);

//
// Endpoint
//
struct BREthereumNodeEndpointRecord {
    /** The hash used to identify this Node Endpoint as a BRSet item */
    BREthereumHash hash;

    /** An optional hostname - if not provided this will be a IP addr string */
    char hostname[HOST_NAME_MAX + 1];

    /** The 'Discovery Endpoint' */
    BREthereumDISNeighbor dis;

    /** The sockets - will be -1 if not connected */
    int sockets[NUMBER_OF_NODE_ROUTES];

    /** */
    uint64_t timestamp;

    /** Public Key (for the remote peer ) */
    // BRKey key;

    /** The ephemeral public key */
    BRKey ephemeralKey;

    /** The nonce */
    UInt256 nonce;

    /** The Hello P2P message */
    BREthereumP2PMessageHello hello;

    /** The Status P2P message */
    BREthereumP2PMessageStatus status;

    /** */
    BREthereumBoolean discovered;
};

///
/// MARK: - Create/Release
///

extern BREthereumNodeEndpoint
nodeEndpointCreateDetailed (BREthereumDISNeighbor dis,
                            BRKey ephemeralKey,
                            UInt256 nonce) {
    BREthereumNodeEndpoint endpoint = calloc (1, sizeof (struct BREthereumNodeEndpointRecord));

    endpoint->dis = dis;
    endpoint->hash = neighborDISHash(dis);

    for (int i = 0; i < NUMBER_OF_NODE_ROUTES; i++)
        endpoint->sockets[i] = -1;

    endpoint->timestamp = 0;

    endpoint->nonce = nonce;
    endpoint->ephemeralKey = ephemeralKey;

    inet_ntop (dis.node.domain, (void *) &dis.node.addr, endpoint->hostname, HOST_NAME_MAX + 1);

    endpoint->discovered = ETHEREUM_BOOLEAN_FALSE;
    return endpoint;
}

extern BREthereumNodeEndpoint
nodeEndpointCreate (BREthereumDISNeighbor dis) {
    UInt256 nonce = UINT256_ZERO;
    BRKey ephemeralKey;
    BRKeyClean(&ephemeralKey);

    return nodeEndpointCreateDetailed (dis, ephemeralKey, nonce);
}

extern BREthereumNodeEndpoint
nodeEndpointCreateLocal (BREthereumLESRandomContext randomContext) {
    BREthereumDISEndpoint disEndpoint = {
        AF_INET,
        { .ipv6 = { 0,0,0,0,   0,0,0,0,   0,0,0,0,   0,0,0,0 } },
        LES_LOCAL_ENDPOINT_UDP_PORT,
        LES_LOCAL_ENDPOINT_TCP_PORT
    };

    inet_pton (disEndpoint.domain, LES_LOCAL_ENDPOINT_ADDRESS, &disEndpoint.addr);

    BRKey localKey, localEphemeralKey;
    UInt256 localNonce;

    randomGenPriKey  (randomContext, &localKey);
    randomGenPriKey  (randomContext, &localEphemeralKey);
    randomGenUInt256 (randomContext, &localNonce);

    assert (0 == localKey.compressed);

    BREthereumDISNeighbor dis = { disEndpoint, localKey };

    return nodeEndpointCreateDetailed (dis, localEphemeralKey, localNonce);
}

extern BREthereumNodeEndpoint
nodeEndpointCreateEnode (const char *enode) {
    size_t enodeLen = strlen (enode);
    assert (enodeLen < 1024);

    char buffer[1024], *buf = buffer;
    assert (1 == sscanf (enode, "enode://%s", buffer));

    char *id = strsep (&buf, "@:");
    char *ip = strsep (&buf, "@:");
    char *pt = strsep (&buf, "@:");
    int port = atoi (pt);

    BREthereumDISEndpoint disEndpoint = {
        AF_INET,
        { .ipv6 = { 0,0,0,0,   0,0,0,0,   0,0,0,0,   0,0,0,0 } },
        port,
        port
    };

    inet_pton (disEndpoint.domain, ip, &disEndpoint.addr);

    BRKey key;
    key.pubKey[0] = 0x04;
    key.compressed = 0;
    decodeHex(&key.pubKey[1], 64, id, 128);

    return nodeEndpointCreate((BREthereumDISNeighbor) { disEndpoint, key });
}

extern void
nodeEndpointRelease (BREthereumNodeEndpoint endpoint) {
    // If we where unable to communicate w/ this endpoint - and certainly if we couldn't exchange
    // status messages - then one or both of `hello` and `status` will never have been assigned.

    messageP2PHelloRelease (&endpoint->hello);
    messageP2PStatusRelease (&endpoint->status);
    free (endpoint);
}

///
/// MARK: - Getters/Setters
///

extern BREthereumHash
nodeEndpointGetHash (BREthereumNodeEndpoint endpoint) {
    return endpoint->hash;
}

extern const char *
nodeEndpointGetHostname (BREthereumNodeEndpoint endpoint) {
    return endpoint->hostname;
}

extern int // remote.dis.node.portTCP)
nodeEndpointGetPort (BREthereumNodeEndpoint endpoint,
                     BREthereumNodeEndpointRoute route) {
    switch (route) {
        case NODE_ROUTE_UDP: return endpoint->dis.node.portUDP;
        case NODE_ROUTE_TCP: return endpoint->dis.node.portTCP;
    }
}

extern int
nodeEndpointGetSocket (BREthereumNodeEndpoint endpoint,
                       BREthereumNodeEndpointRoute route) {
    return endpoint->sockets[route];
}

extern BREthereumDISNeighbor
nodeEndpointGetDISNeighbor (BREthereumNodeEndpoint endpoint) {
    return endpoint->dis;
}

extern BRKey *
nodeEndpointGetKey (BREthereumNodeEndpoint endpoint) {
    return &endpoint->dis.key;
}

extern BRKey *
nodeEndpointGetEphemeralKey (BREthereumNodeEndpoint endpoint) {
    return &endpoint->ephemeralKey;
}

extern UInt256 *
nodeEndpointGetNonce (BREthereumNodeEndpoint endpoint) {
    return &endpoint->nonce;
}

// Support BRSet
extern size_t
nodeEndpointHashValue (const void *h) {
    return hashSetValue(&((BREthereumNodeEndpoint) h)->hash);
}

// Support BRSet
extern int
nodeEndpointHashEqual (const void *h1, const void *h2) {
    return h1 == h2 || hashSetEqual (&((BREthereumNodeEndpoint) h1)->hash,
                                     &((BREthereumNodeEndpoint) h2)->hash);
}

///
/// MARK: - Hello
///
extern BREthereumP2PMessageHello
nodeEndpointGetHello (const BREthereumNodeEndpoint endpoint) {
    return endpoint->hello;
}

extern void
nodeEndpointSetHello (BREthereumNodeEndpoint endpoint,
                      OwnershipGiven BREthereumP2PMessageHello hello) {
    endpoint->hello = hello;
}

extern void
nodeEndpointDefineHello (BREthereumNodeEndpoint endpoint,
                         const char *name,
                         OwnershipGiven BRArrayOf(BREthereumP2PCapability) capabilities) {
    assert (NULL == endpoint->hello.capabilities);
    assert (NULL == endpoint->hello.clientId);

    endpoint->hello = (BREthereumP2PMessageHello) {
        P2P_MESSAGE_VERSION,
        strdup (name),
        capabilities,
        endpoint->dis.node.portTCP,
        {} // UInt512 nodeId
    };

    // The NodeID is the 64-byte (uncompressed) public key
    uint8_t pubKey[65];
    assert (65 == BRKeyPubKey (&endpoint->dis.key, pubKey, 65));
    memcpy (endpoint->hello.nodeId.u8, &pubKey[1], 64);
}

extern void
nodeEndpointShowHello (BREthereumNodeEndpoint endpoint) {
    messageP2PHelloShow (&endpoint->hello);
}

extern BREthereumBoolean
nodeEndpointHasHelloCapability (BREthereumNodeEndpoint endpoint,
                                const char *name,
                                uint32_t version) {
    for (size_t index = 0; index < array_count(endpoint->hello.capabilities); index++)
        if (0 == strcmp (name, endpoint->hello.capabilities[index].name) &&
            version == endpoint->hello.capabilities[index].version)
            return ETHEREUM_BOOLEAN_TRUE;
    return ETHEREUM_BOOLEAN_FALSE;
}

extern const BREthereumP2PCapability *
nodeEndpointHasHelloMatchingCapability (BREthereumNodeEndpoint source,
                                        BREthereumNodeEndpoint target) {
    for (size_t index = 0; index < array_count(source->hello.capabilities); index++) {
        BREthereumP2PCapability *scap = &source->hello.capabilities[index];
        if (ETHEREUM_BOOLEAN_IS_TRUE (nodeEndpointHasHelloCapability (target, scap->name, scap->version)))
            return scap;
    }
    return NULL;
}

///
/// MARK: - Status
///
extern BREthereumP2PMessageStatus
nodeEndpointGetStatus (const BREthereumNodeEndpoint endpoint) {
    return endpoint->status;
}

extern void
nodeEndpointSetStatus (BREthereumNodeEndpoint endpoint,
                       OwnershipGiven BREthereumP2PMessageStatus status) {
    endpoint->status = status;
}

//extern void
//nodeEndpointDefineStatus (BREthereumNodeEndpoint endpoint,
//                          uint64_t protocolVersion,
//                           uint64_t chainId,
//                           uint64_t headNum,
//                           BREthereumHash headHash,
//                           UInt256 headTd,
//                           BREthereumHash genesisHash,
//                          uint64_t announceType) {
//
//    endpoint->status = (BREthereumP2PMessageStatus) {
//        protocolVersion,
//        chainId,
//        headNum,
//        headHash,
//        headTd,
//        genesisHash,
//        NULL
//    };
//}

extern void
nodeEndpointShowStatus (BREthereumNodeEndpoint endpoint) {
    messageP2PStatusShow (&endpoint->status);
}

///
/// MARK: - Open/Close
///
extern int // errno
nodeEndpointOpen (BREthereumNodeEndpoint endpoint,
                  BREthereumNodeEndpointRoute route) {
    if (nodeEndpointIsOpen (endpoint, route)) return 0;
    
    return openSocket (endpoint,
                       &endpoint->sockets[route],
                       (route == NODE_ROUTE_TCP ? endpoint->dis.node.portTCP : endpoint->dis.node.portUDP),
                       endpoint->dis.node.domain,
                       (route == NODE_ROUTE_TCP ? SOCK_STREAM : SOCK_DGRAM),
                       NODE_ENDPOINT_OPEN_SOCKET_TIMEOUT);
}

extern int
nodeEndpointClose (BREthereumNodeEndpoint endpoint,
                   BREthereumNodeEndpointRoute route,
                   int needShutdown) {
    int socket;

    socket = endpoint->sockets[route];
    if (socket >= 0) {
        // TOOO: Only assign -1 if closer is successful?
        endpoint->sockets[route] = -1;

        if (needShutdown && shutdown (socket, SHUT_RDWR) < 0) {
            eth_log (LES_LOG_TOPIC, "Socket %d (%s) Shutdown Error: %s", socket,
                     nodeEndpointRouteGetName (route),
                     strerror(errno));

            // Save the error
            int error = errno;

            // Try to close anyways; if unsuccessful return the original error.
            return 0 == close (socket) ? 0 : error;
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
nodeEndpointIsOpen (BREthereumNodeEndpoint endpoint,
                    BREthereumNodeEndpointRoute route) {
    return -1 != endpoint->sockets[route];
}

///
/// MARK: - Recv/Send Data
///

extern int // errno
nodeEndpointRecvData (BREthereumNodeEndpoint endpoint,
                      BREthereumNodeEndpointRoute route,
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
        else if (n < 0 && errno == EWOULDBLOCK) continue;
        else {
            totalCount += n;
            if (!needBytesCount) break;
        }

        socket = endpoint->sockets[route];
    }

    if (!error) {
        // !! DON'T MISS THIS !!
        *bytesCount = totalCount;
#if defined (LES_LOG_RECV)
        eth_log (LES_LOG_TOPIC, "read (%zu bytes) from peer [%s], contents: %s", len, endpoint->hostname, "?");
#endif
    }
#if defined (LES_LOG_SEND_RECV_ERROR)
    else {
        eth_log (LES_LOG_TOPIC, "Recv: [ %s, %15d ] => %15s %s%s",
                 nodeEndpointRouteGetName(route),
                 (NODE_ROUTE_UDP == route ? endpoint->dis.node.portUDP : endpoint->dis.node.portTCP),
                 endpoint->hostname,
                 "Error: ",
                 strerror(error));
    }
#endif
    return error;
}

extern int
nodeEndpointSendData (BREthereumNodeEndpoint endpoint,
                      BREthereumNodeEndpointRoute route,
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

    if (!error) {
#if defined (LES_LOG_SEND)
        eth_log (LES_LOG_TOPIC, "sent (%zu bytes) to peer[%s], contents: %s", totalCount, endpoint->hostname, "?");
#endif
    }
#if defined (LES_LOG_SEND_RECV_ERROR)
    else {
        eth_log (LES_LOG_TOPIC, "Send: %s @ %5d => %15s %s%s",
                 nodeEndpointRouteGetName(route),
                 (NODE_ROUTE_UDP == route ? endpoint->dis.node.portUDP : endpoint->dis.node.portTCP),
                 endpoint->hostname,
                 "Error: ",
                 strerror(error));
    }
#endif
    return error;
}

// https://stackoverflow.com/questions/27014955/socket-connect-vs-bind
// http://www.microhowto.info/howto/listen_for_and_receive_udp_datagrams_in_c.html

static void
nodeEndpointFillSockAddr (BREthereumNodeEndpoint endpoint,
                          int port,
                          struct sockaddr_storage *addr,
                          socklen_t *addrLen) {
    // Clear addr
    memset(addr, 0, sizeof(struct sockaddr_storage));

    switch (endpoint->dis.node.domain) {
        case AF_INET: {
            struct sockaddr_in *addr_in4 = (struct sockaddr_in *) addr;

            addr_in4->sin_family = AF_INET;
            addr_in4->sin_port = htons(port);
            addr_in4->sin_addr = * (struct in_addr *) endpoint->dis.node.addr.ipv4;

            *addrLen = sizeof (struct sockaddr_in);
            break;
        }
        case AF_INET6: {
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *) addr;

            addr_in6->sin6_family = AF_INET6;
            addr_in6->sin6_port = htons(port);
            addr_in6->sin6_addr = * (struct in6_addr *) endpoint->dis.node.addr.ipv6;

            *addrLen = sizeof (struct sockaddr_in6);
            break;
        }
        default:
            assert (0);
    }
}

static int
openSocketReportResult (BREthereumNodeEndpoint endpoint, int port, int type, int error) {
#if defined (LES_LOG_REPORT_OPEN_SOCKET)
    eth_log (LES_LOG_TOPIC, "Open: [ %s, %15d ] => %15s %s%s",
             (type == SOCK_STREAM ? "TCP" : "UDP"),
             port,
             endpoint->hostname,
             (0 == error ? "Success " : "Error: "),
             (0 == error ? "" : strerror(error)));
#endif
    return error;
}

/**
 * Note: This function is a direct copy of Aaron's _BRPeerOpenSocket function with a few modifications to
 * work for the Ethereum Core.
 * TODO: May want to make this more modular to work for both etheruem and bitcoin
 */
static int
openSocket(BREthereumNodeEndpoint endpoint, int *socketToAssign, int port, int domain, int type, double timeout)
{
    struct sockaddr_storage addr;
    struct timeval tv;
    fd_set fds;
    socklen_t addrLen, optLen;
    int count, arg = 0, err = 0, on = 1;

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
        }

        fcntl (*socketToAssign, F_SETFL, arg);
        return openSocketReportResult (endpoint, port, type, err);
    }

    // We've failed
    fcntl (*socketToAssign, F_SETFL, arg);
    return openSocketReportResult (endpoint, port, type, errno);
}

#if defined (GET_WIFI_ADDRESS) // never defined
//func getWiFiAddress() -> String? {
//    var address : String?
//
//    // Get list of all interfaces on the local machine:
//    var ifaddr : UnsafeMutablePointer<ifaddrs>?
//    guard getifaddrs(&ifaddr) == 0 else { return nil }
//    guard let firstAddr = ifaddr else { return nil }
//
//    // For each interface ...
//    for ifptr in sequence(first: firstAddr, next: { $0.pointee.ifa_next }) {
//        let interface = ifptr.pointee
//
//        // Check for IPv4 or IPv6 interface:
//        let addrFamily = interface.ifa_addr.pointee.sa_family
//        if addrFamily == UInt8(AF_INET) || addrFamily == UInt8(AF_INET6) {
//
//            // Check interface name:
//            let name = String(cString: interface.ifa_name)
//            if  name == "en0" {
//
//                // Convert interface address to a human readable string:
//                var hostname = [CChar](repeating: 0, count: Int(NI_MAXHOST))
//                getnameinfo(interface.ifa_addr, socklen_t(interface.ifa_addr.pointee.sa_len),
//                            &hostname, socklen_t(hostname.count),
//                            nil, socklen_t(0), NI_NUMERICHOST)
//                address = String(cString: hostname)
//            }
//        }
//    }
//    freeifaddrs(ifaddr)
//
//    return address
//    }

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

#endif // defined (GET_WIFI_ADDRESS)

///
/// MARK: - Local/Bootstrap Enodes
///

//
// LES/PIP Nodes hosted locally, when enabled (these are my NodeId; not yours).
//
const char *bootstrapLCLEnodes[] = {
#if defined (LES_SUPPORT_PARITY)
    // Localhost - Parity
//  "enode://4483ac6134c85ecbd31d14168f1c97b82bdc45c442e81277f52428968de41add46549f8d6c9c8c3432f3b8834b018c350ac37d87d70d67e599f42f68a96717fc@127.0.0.1:30303",     // SSD Archive
    "enode://4483ac6134c85ecbd31d14168f1c97b82bdc45c442e81277f52428968de41add46549f8d6c9c8c3432f3b8834b018c350ac37d87d70d67e599f42f68a96717fc@192.168.1.126:30303", // SSD Archive
    // 192.168.1.126
#endif

#if defined (LES_SUPPORT_GETH)
    // Localhost - GETH
    "enode://654580048e9de8f7743ca38035c7ab7fbf2d59b6acd5b92cc031e4571b2c441fe9fc5bb261ada112fb39ca32c1ac7716d91a211b992693c9472ad6af42c5302a@127.0.0.1:30304",
#endif
    NULL
};

//
// LES/PIP Nodes hosted by BRD
//
const char *bootstrapBRDEnodes[] = {
#if defined (LES_SUPPORT_PARITY)
    // Archival
    "enode://6b3d1d0d0830f399167af983585a671e433e2080c1c3ebe4f859ee31aebcac88dbad59495155b01ed5b6b292df86b48e2302d896238671893faee60b5a47dda2@167.99.160.105:30303",
    "enode://b253e581a0d9e515164c9bf9a62db85ccc8893fd9a2b6968fac4910bd7dfe334072d4d9eb5a14a53d4647b87c0164e32d80c2f387d6d2c54e8437062a2a5855d@35.239.120.179:30303",
#endif

#if defined (LES_SUPPORT_GETH)
    // Archival
    "enode://3d0bce4775635c65733b7534f1bccd48720632f5d66a44030c1d13e2e5883262d9d22cdb8365c03137e8d5fbbf5355772acf35b08d6f9b5ad69bb24ad52a20cc@35.184.255.33:30303",
    // Full
//    "enode://e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851@104.197.99.24:30303",
//    "enode://e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851@35.193.192.189:30303",
#endif
    NULL
};

//
// LES/PIP Nodes - 'Well Known', a mixture of PIP and LES nodes.
//
const char *bootstrapLESEnodes[] = {
    // START -- https://gist.github.com/rfikki/e2a8c47f4460668557b1e3ec8bae9c11
    "enode://03f178d5d4511937933b50b7af683b467abaef8cfc5f7c2c9b271f61e228578ae192aaafc7f0d8035dfa994e734c2c2f72c229e383706be2f4fa43efbe9f94f4@163.172.149.200:30303",
    "enode://0f740f471e876020566c2ce331c81b4128b9a18f636b1d4757c4eaea7f077f4b15597a743f163280293b0a7e35092064be11c4ec199b9905541852a36be9004b@206.221.178.149:30303",
    "enode://16d92fc94f4ec4386aca44d255853c27cbe97a4274c0df98d2b642b0cc4b2f2330e99b00b46db8a031da1a631c85e2b4742d52f5eaeca46612cd28db41fb1d7f@91.223.175.173:30303",
    "enode://1d70e87a2ee28a2762f1b2cd56f1b9134824a84264030539bba297f67a5bc9ec7ae3016b5f900dc59b1c27b4e258a63fc282a37b2dd6e25a8377473530513394@208.88.169.151:30303",
    "enode://242b68a4e37b4478c46901c3512315f36bd1aa513566d1f061939b202258b55d63d66367bc5807e62ec03ae673bead9a351846e3f23284ce79537ff7afa65615@34.201.26.61:30303",
    "enode://2af1ef12967d112f527648819f89e55bfe61f77f5920a0edc1c21de274092bc4839a68405b13d845a0c133b101050c5fb04f5b4a8683663fc20d9ccc5f68d0f3@34.239.156.26:30303",
    "enode://31b5db1136a0ebceeb0ab6879e95dc66e8c52bcce9c8de50e2f722b5868f782aa0306b6b137b9e0c6271a419c5562a194d7f2abd78e22dcd1f55700dfc30c46a@35.165.17.127:30303",
    "enode://3afdfd40713a8b188a94e4c7a9ddc61bc6ef176c3abbb13d1dd35eb367725b95329a7570039044dbffa49c50d4aa65f0a1f99ee68e46b8e2f09100d11d4fc85a@31.17.196.138:30303",
    "enode://3d0bce4775635c65733b7534f1bccd48720632f5d66a44030c1d13e2e5883262d9d22cdb8365c03137e8d5fbbf5355772acf35b08d6f9b5ad69bb24ad52a20cc@35.184.255.33:30303",
    "enode://4baa9b4ea9f3219e595f52c817ce4829ae916e7b1ea0f356a543c73de0c7d7ff889b6360f4b7dfbbcae7d2f60b51a16bc02ccc510df6be0aee63cba94ff5a923@18.207.138.205:30303",
    "enode://4c2b5c5d9503b7f4e76a551c827f19200f7f9ebb62f2cb5078c352de1e8d4d1006efa8fc143f9ccf2c8fd85836198dc1c69729dfa1c54d63f5d1d57fd8781bf8@62.151.178.212:30303",
    "enode://63acf19ecd1f7a365176cc4ccf0b410e8fa05a60a5b298102a7a0194e86570a6f9e15abbb23cb3791fd92ddd4e25d32dba7a6c6887f6b76e4b266288fa99cf98@76.170.48.252:30303",
    "enode://89495deb21261a4542d50167d6e69cf3b1a585609e6843a23becbd349d92755bd2ddcc55bb1f2c017099b774454d95ef5ebccbed1859fc530fb34843ddfc32e2@52.39.91.131:30303",
    "enode://95176fe178be55d40aae49a5c11f21aa58968e13c681a6b1f571b2bb3e45927a7fb3888361bef85c0e28a52ea0e4afa17dcaa9d6c61baf504b3559f056f78581@163.172.145.241:30303",
    "enode://a979fb575495b8d6db44f750317d0f4622bf4c2aa3365d6af7c284339968eef29b69ad0dce72a4d8db5ebb4968de0e3bec910127f134779fbcb0cb6d3331163c@52.16.188.185:30303",
    "enode://ae1d9252428fa66371bc68e9c4fc0f9c60d09943b521cede6c60b50c67fd6dc1d614525c07030afe52586cbf35d43ad83368ad71c57639125698c3392f8b4a89@121.140.198.219:30303",
    "enode://bfad505cbb2bde72e161a7cff044d66d20ceb85c8a61047b50037881f289bd2dcc064189ade2077daddd5b20fd2fc6dee7208f227ae2a34361bf51751d225e8e@51.15.220.91:30303",
    "enode://d324187ba8da3ac7ad454eeb9aa395eae610fc032bccf9dae63c1e3206458cf55c7e9e454ce23acf9706fb89d0ce9d47038ab261676776b5c6fa1b76c6cf829c@198.58.126.224:30303",
    "enode://d5d63b7b26027d54f1d03656d8aed536b3c914999cbedddf7a4733e1286984ae99ebe2e7a1b3ada1ae4b10af4ddd9c5ed235ef908795f7142ef2061ca1751a11@198.74.52.106:30303",
    "enode://d70756f1aa07246a61731c8b0ce3e89046e07e8a18c497172dd4baa4b380998b4ee669396140effe65affbcd79bb269ec3f2c698b97507656291c88e7f8e1bc3@50.116.21.236:30303",
    // BRD: "enode://e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851@104.197.99.24:30303",
    "enode://ea1737bf696928b4b686a2ccf61a6f2295d149281a80b0d83a9bce242e7bb084434c0837a2002d4cc2840663571ecf3e45517545499c466e4373c69951d090fe@163.172.181.92:30303",
    "enode://f251404ab66f10df6f541d69d735616a7d78e04673ec40cdfe6bf3d1fb5d84647ba627f22a1e8c5e2aa45629c88e33bc394cc1633a63fed11d84304892e51fe9@196.54.41.2:38065",
    // END -- https://gist.github.com/rfikki/e2a8c47f4460668557b1e3ec8bae9c11

    // Random
    "enode://3e9301c797f3863d7d0f29eec9a416f13956bd3a14eec7e0cf5eb56942841526269209edf6f57cd1315bef60c4ebbe3476bc5457bed4e479cac844c8c9e375d3@109.232.77.21:30303", // GETH
    "enode://81863f47e9bd652585d3f78b4b2ee07b93dad603fd9bc3c293e1244250725998adc88da0cef48f1de89b15ab92b15db8f43dc2b6fb8fbd86a6f217a1dd886701@193.70.55.37:30303",  // Parity
    NULL
};

// Parity Nodes (default PIP 'on'; but not necessarily PIP 'on')
//
// From: https://github.com/paritytech/parity-ethereum/blob/master/ethcore/res/ethereum/foundation.json
// Retrieved: 12 Sept 2018
const char *bootstrapParityEnodes[] = {
#if defined (LES_SUPPORT_PARITY)
    "enode://81863f47e9bd652585d3f78b4b2ee07b93dad603fd9bc3c293e1244250725998adc88da0cef48f1de89b15ab92b15db8f43dc2b6fb8fbd86a6f217a1dd886701@193.70.55.37:30303",
    "enode://4afb3a9137a88267c02651052cf6fb217931b8c78ee058bb86643542a4e2e0a8d24d47d871654e1b78a276c363f3c1bc89254a973b00adc359c9e9a48f140686@144.217.139.5:30303",
    "enode://c16d390b32e6eb1c312849fe12601412313165df1a705757d671296f1ac8783c5cff09eab0118ac1f981d7148c85072f0f26407e5c68598f3ad49209fade404d@139.99.51.203:30303",
    "enode://4faf867a2e5e740f9b874e7c7355afee58a2d1ace79f7b692f1d553a1134eddbeb5f9210dd14dc1b774a46fd5f063a8bc1fa90579e13d9d18d1f59bac4a4b16b@139.99.160.213:30303",
    "enode://6a868ced2dec399c53f730261173638a93a40214cf299ccf4d42a76e3fa54701db410669e8006347a4b3a74fa090bb35af0320e4bc8d04cf5b7f582b1db285f5@163.172.131.191:30303",
    "enode://66a483383882a518fcc59db6c017f9cd13c71261f13c8d7e67ed43adbbc82a932d88d2291f59be577e9425181fc08828dc916fdd053af935a9491edf9d6006ba@212.47.247.103:30303",
    "enode://cd6611461840543d5b9c56fbf088736154c699c43973b3a1a32390cf27106f87e58a818a606ccb05f3866de95a4fe860786fea71bf891ea95f234480d3022aa3@163.172.157.114:30303",
    "enode://1d1f7bcb159d308eb2f3d5e32dc5f8786d714ec696bb2f7e3d982f9bcd04c938c139432f13aadcaf5128304a8005e8606aebf5eebd9ec192a1471c13b5e31d49@138.201.223.35:30303",
    "enode://a979fb575495b8d6db44f750317d0f4622bf4c2aa3365d6af7c284339968eef29b69ad0dce72a4d8db5ebb4968de0e3bec910127f134779fbcb0cb6d3331163c@52.16.188.185:30303",
    "enode://3f1d12044546b76342d59d4a05532c14b85aa669704bfe1f864fe079415aa2c02d743e03218e57a33fb94523adb54032871a6c51b2cc5514cb7c7e35b3ed0a99@13.93.211.84:30303",
    "enode://78de8a0916848093c73790ead81d1928bec737d565119932b98c6b100d944b7a95e94f847f689fc723399d2e31129d182f7ef3863f2b4c820abbf3ab2722344d@191.235.84.50:30303",
    "enode://158f8aab45f6d19c6cbf4a089c2670541a8da11978a2f90dbf6a502a4a3bab80d288afdbeb7ec0ef6d92de563767f3b1ea9e8e334ca711e9f8e2df5a0385e8e6@13.75.154.138:30303",
    "enode://1118980bf48b0a3640bdba04e0fe78b1add18e1cd99bf22d53daac1fd9972ad650df52176e7c7d89d1114cfef2bc23a2959aa54998a46afcf7d91809f0855082@52.74.57.123:30303",
    "enode://979b7fa28feeb35a4741660a16076f1943202cb72b6af70d327f053e248bab9ba81760f39d0701ef1d8f89cc1fbd2cacba0710a12cd5314d5e0c9021aa3637f9@5.1.83.226:30303",
    "enode://0cc5f5ffb5d9098c8b8c62325f3797f56509bff942704687b6530992ac706e2cb946b90a34f1f19548cd3c7baccbcaea354531e5983c7d1bc0dee16ce4b6440b@40.118.3.223:30305",
    "enode://1c7a64d76c0334b0418c004af2f67c50e36a3be60b5e4790bdac0439d21603469a85fad36f2473c9a80eb043ae60936df905fa28f1ff614c3e5dc34f15dcd2dc@40.118.3.223:30308",
    "enode://85c85d7143ae8bb96924f2b54f1b3e70d8c4d367af305325d30a61385a432f247d2c75c45c6b4a60335060d072d7f5b35dd1d4c45f76941f62a4f83b6e75daaf@40.118.3.223:30309",
    "enode://de471bccee3d042261d52e9bff31458daecc406142b401d4cd848f677479f73104b9fdeb090af9583d3391b7f10cb2ba9e26865dd5fca4fcdc0fb1e3b723c786@54.94.239.50:30303",
    "enode://4cd540b2c3292e17cff39922e864094bf8b0741fcc8c5dcea14957e389d7944c70278d872902e3d0345927f621547efa659013c400865485ab4bfa0c6596936f@138.201.144.135:30303",
    "enode://01f76fa0561eca2b9a7e224378dd854278735f1449793c46ad0c4e79e8775d080c21dcc455be391e90a98153c3b05dcc8935c8440de7b56fe6d67251e33f4e3c@51.15.42.252:30303",
    "enode://2c9059f05c352b29d559192fe6bca272d965c9f2290632a2cfda7f83da7d2634f3ec45ae3a72c54dd4204926fb8082dcf9686e0d7504257541c86fc8569bcf4b@163.172.171.38:30303",
    "enode://efe4f2493f4aff2d641b1db8366b96ddacfe13e7a6e9c8f8f8cf49f9cdba0fdf3258d8c8f8d0c5db529f8123c8f1d95f36d54d590ca1bb366a5818b9a4ba521c@163.172.187.252:30303",
    "enode://bcc7240543fe2cf86f5e9093d05753dd83343f8fda7bf0e833f65985c73afccf8f981301e13ef49c4804491eab043647374df1c4adf85766af88a624ecc3330e@136.243.154.244:30303",
    "enode://ed4227681ca8c70beb2277b9e870353a9693f12e7c548c35df6bca6a956934d6f659999c2decb31f75ce217822eefca149ace914f1cbe461ed5a2ebaf9501455@88.212.206.70:30303",
    "enode://cadc6e573b6bc2a9128f2f635ac0db3353e360b56deef239e9be7e7fce039502e0ec670b595f6288c0d2116812516ad6b6ff8d5728ff45eba176989e40dead1e@37.128.191.230:30303",
    "enode://595a9a06f8b9bc9835c8723b6a82105aea5d55c66b029b6d44f229d6d135ac3ecdd3e9309360a961ea39d7bee7bac5d03564077a4e08823acc723370aace65ec@46.20.235.22:30303",
    "enode://029178d6d6f9f8026fc0bc17d5d1401aac76ec9d86633bba2320b5eed7b312980c0a210b74b20c4f9a8b0b2bf884b111fa9ea5c5f916bb9bbc0e0c8640a0f56c@216.158.85.185:30303",
    "enode://fdd1b9bb613cfbc200bba17ce199a9490edc752a833f88d4134bf52bb0d858aa5524cb3ec9366c7a4ef4637754b8b15b5dc913e4ed9fdb6022f7512d7b63f181@212.47.247.103:30303",
#endif
    NULL
};

//
// GETH Nodes (default LES 'off'; but not necessarily LES 'off')
//
// From: https://github.com/ethereum/go-ethereum/blob/master/params/bootnodes.go
// Retrieved: 12 Sept 2018
const char *bootstrapGethEnodes[] = {
#if defined (LES_SUPPORT_GETH)
    // Ethereum Foundation Go Bootnodes
    "enode://a979fb575495b8d6db44f750317d0f4622bf4c2aa3365d6af7c284339968eef29b69ad0dce72a4d8db5ebb4968de0e3bec910127f134779fbcb0cb6d3331163c@52.16.188.185:30303", // IE
    "enode://3f1d12044546b76342d59d4a05532c14b85aa669704bfe1f864fe079415aa2c02d743e03218e57a33fb94523adb54032871a6c51b2cc5514cb7c7e35b3ed0a99@13.93.211.84:30303",  // US-WEST
    "enode://78de8a0916848093c73790ead81d1928bec737d565119932b98c6b100d944b7a95e94f847f689fc723399d2e31129d182f7ef3863f2b4c820abbf3ab2722344d@191.235.84.50:30303", // BR
    "enode://158f8aab45f6d19c6cbf4a089c2670541a8da11978a2f90dbf6a502a4a3bab80d288afdbeb7ec0ef6d92de563767f3b1ea9e8e334ca711e9f8e2df5a0385e8e6@13.75.154.138:30303", // AU
    "enode://1118980bf48b0a3640bdba04e0fe78b1add18e1cd99bf22d53daac1fd9972ad650df52176e7c7d89d1114cfef2bc23a2959aa54998a46afcf7d91809f0855082@52.74.57.123:30303",  // SG

    // Ethereum Foundation C++ Bootnodes
    "enode://979b7fa28feeb35a4741660a16076f1943202cb72b6af70d327f053e248bab9ba81760f39d0701ef1d8f89cc1fbd2cacba0710a12cd5314d5e0c9021aa3637f9@5.1.83.226:30303", // DE
#endif
    NULL
};

///
/// MARK: Enode Sets
///

//
// Determine which of the above enodes to boot with.
//
#if defined(LES_SUPPORT_PARITY) && !defined(LES_BOOTSTRAP_LCL_ONLY) && !defined(LES_BOOTSTRAP_BRD_ONLY)
#define LES_BOOTSTRAP_PARITY
#endif

#if defined(LES_SUPPORT_GETH) && !defined(LES_BOOTSTRAP_LCL_ONLY) && !defined(LES_BOOTSTRAP_BRD_ONLY)
#define LES_BOOTSTRAP_GETH
#endif

#if !defined(LES_BOOTSTRAP_LCL_ONLY) && !defined(LES_BOOTSTRAP_BRD_ONLY)
#define LES_BOOTSTRAP_COMMON
#endif

#if !defined (LES_BOOTSTRAP_BRD_ONLY) || defined (LES_BOOTSTRAP_LCL_ONLY)
#define LES_BOOTSTRAP_LCL
#endif

#if !defined (LES_BOOTSTRAP_LCL_ONLY) || defined (LES_BOOTSTRAP_BRD_ONLY)
#define LES_BOOTSTRAP_BRD
#endif

//
// Enode Sets - Order is important here.  Generally inserted by 'NodeID distance' except that
// BRD and LCL nodes will be inserted so as to be connected first.
//
const char **bootstrapMainnetEnodeSets[] = {
#if defined (LES_BOOTSTRAP_COMMON)
    bootstrapLESEnodes,
#endif

#if defined (LES_BOOTSTRAP_PARITY)
    bootstrapParityEnodes,
#endif

#if defined (LES_BOOTSTRAP_GETH)
    bootstrapGethEnodes,
#endif

#if defined (LES_BOOTSTRAP_BRD)
    bootstrapBRDEnodes,
#endif

#if defined (LES_BOOTSTRAP_LCL)
    bootstrapLCLEnodes,
#endif
};
size_t NUMBER_OF_NODE_ENDPOINT_SETS = (sizeof (bootstrapMainnetEnodeSets) / sizeof (char **));

// TODO: Known bad enodes - really the code should find and disable on the fly, based on the node's behavior.
const char *bootstrapDisableEnodes[] = {
    "enode://5de9c43e8554ea2d8451f01614fca69799c8e758d1c65bead526d4c3beec695356f5548b20bee6fb05919c463bde6486abc1c73903796b5f8f041bd8ac483711@45.63.26.208:30304",

    // Futuristic block numbes
    "enode://1b1a26ebe85c34469ba25893053cc8249d59ec9916eed8c8b7de21f12edb92dd550b704f326b49bed679b521525c948be4f6c5477839da23d88d2090d549d6bd@34.201.251.249:30303",
    NULL
};

