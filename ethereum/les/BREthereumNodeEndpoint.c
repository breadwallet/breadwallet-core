//
//  BREthereumNodeEndpoint.c
//  Core
//
//  Created by Ed Gamble on 8/14/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

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
#include "ethereum/util/BRUtil.h"
#include "BREthereumNodeEndpoint.h"

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

/// MARK: - Create/Release

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
    memset (&key, 0, sizeof(BRKey));
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

/// MARK: - Getters/Setters

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

/// MARK: - Hello

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

/// MARK: - Status

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

/// MARK: - Open/Close

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

/// MARK: - Recv/Send Data

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

    // ten second timeout for send/receive
    // BTC: so thread doesn't block for too long.
    // ETH: we use select and thus won't/shouldn't block on reads nor writes.
    tv.tv_sec = 10;
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
