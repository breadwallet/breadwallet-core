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

#include "../util/BRUtil.h"
#include "BREthereumNodeEndpoint.h"

#ifndef MSG_NOSIGNAL   // linux based systems have a MSG_NOSIGNAL send flag, useful for supressing SIGPIPE signals
#define MSG_NOSIGNAL 0 // set to 0 if undefined (BSD has the SO_NOSIGPIPE sockopt, and windows has no signals at all)
#endif

// #define NEED_TO_PRINT_SEND_DATA
// #define LES_LOG_RECV
// #define LES_LOG_SEND

#define CONNECTION_TIME 3.0

/** Forward Declarations */
static int
openSocket (BREthereumNodeEndpoint *endpoint, int *socket, int port, int domain, int type, double timeout);

//
// MARK: - LES Node Endpoint
//
extern BREthereumNodeEndpoint
nodeEndpointCreateDetailed (BREthereumDISEndpoint dis,
                            BRKey key,
                            BRKey ephemeralKey,
                            UInt256 nonce) {
    BREthereumNodeEndpoint endpoint;
    memset (&endpoint, 0, sizeof (BREthereumNodeEndpoint));

    endpoint.dis = dis;

    for (int i = 0; i < NUMBER_OF_NODE_ROUTES; i++)
        endpoint.sockets[i] = -1;

    endpoint.timestamp = 0;

    endpoint.key = key;
    endpoint.nonce = nonce;
    endpoint.ephemeralKey = ephemeralKey;

    inet_ntop (dis.domain, (void *) &dis.addr, endpoint.hostname, _POSIX_HOST_NAME_MAX + 1);

    return endpoint;
}

extern BREthereumNodeEndpoint
nodeEndpointCreate (BREthereumDISEndpoint dis,
                    BRKey key) {
    UInt256 nonce = UINT256_ZERO;
    BRKey ephemeralKey;
    BRKeyClean(&ephemeralKey);

    return nodeEndpointCreateDetailed (dis, key, ephemeralKey, nonce);
}

extern BREthereumNodeEndpoint
nodeEndpointCreateLocal (BREthereumLESRandomContext randomContext) {
    BREthereumDISEndpoint dis = {
        AF_INET,
        {},
        LES_LOCAL_ENDPOINT_UDP_PORT,
        LES_LOCAL_ENDPOINT_TCP_PORT
    };

    inet_pton (dis.domain, LES_LOCAL_ENDPOINT_ADDRESS, &dis.addr);

    BRKey localKey, localEphemeralKey;
    UInt256 localNonce;

    randomGenPriKey  (randomContext, &localKey);
    randomGenPriKey  (randomContext, &localEphemeralKey);
    randomGenUInt256 (randomContext, &localNonce);

    assert (0 == localKey.compressed);

    return nodeEndpointCreateDetailed (dis, localKey, localEphemeralKey, localNonce);
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

    BREthereumDISEndpoint dis = {
        AF_INET,
        {},
        port,
        port
    };

    inet_pton (dis.domain, ip, &dis.addr);

    BRKey key;
    key.pubKey[0] = 0x04;
    key.compressed = 0;
    decodeHex(&key.pubKey[1], 64, id, 128);
    return nodeEndpointCreate(dis, key);
}

extern BRKey
nodeEndpointGetKey (BREthereumNodeEndpoint endpoint) {
    return endpoint.key;
}

extern void
nodeEndpointSetHello (BREthereumNodeEndpoint *endpoint,
                      BREthereumP2PMessage hello) {
    endpoint->hello = hello;
}

extern void
nodeEndpointSetStatus (BREthereumNodeEndpoint *endpoint,
                       BREthereumLESMessage status) {
    endpoint->status = status;
}

extern int // errno
nodeEndpointOpen (BREthereumNodeEndpoint *endpoint,
                  BREthereumNodeEndpointRoute route) {
    if (nodeEndpointIsOpen (endpoint, route)) return 0;
    
    return openSocket (endpoint,
                       &endpoint->sockets[route],
                       (route == NODE_ROUTE_TCP ? endpoint->dis.portTCP : endpoint->dis.portUDP),
                       endpoint->dis.domain,
                       (route == NODE_ROUTE_TCP ? SOCK_STREAM : SOCK_DGRAM),
                       CONNECTION_TIME);
}

extern int
nodeEndpointClose (BREthereumNodeEndpoint *endpoint,
                   BREthereumNodeEndpointRoute route,
                   int needShutdown) {
    int socket;

    socket = endpoint->sockets[route];
    if (socket >= 0) {
        endpoint->sockets[route] = -1;

        if (needShutdown && shutdown (socket, SHUT_RDWR) < 0) {
            eth_log (LES_LOG_TOPIC, "Socket %d (%s) Shutdown Error: %s", socket,
                     nodeEndpointRouteGetName (route),
                     strerror(errno));

            // Try to close anyways but don't lose the original error.
            int error = errno;
            close(socket);
            return error;
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
nodeEndpointIsOpen (BREthereumNodeEndpoint *endpoint,
                    BREthereumNodeEndpointRoute route) {
    return -1 != endpoint->sockets[route];
}

/// MARK: - Recv Data

extern int // errno
nodeEndpointRecvData (BREthereumNodeEndpoint *endpoint,
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

    if (error)
        eth_log (LES_LOG_TOPIC, "Recv: %s @ %5d => %15s %s%s",
                 nodeEndpointRouteGetName(route),
                 (NODE_ROUTE_UDP == route ? endpoint->dis.portUDP : endpoint->dis.portTCP),
                 endpoint->hostname,
                 "Error: ",
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
nodeEndpointSendData (BREthereumNodeEndpoint *endpoint,
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

    if (error)
        eth_log (LES_LOG_TOPIC, "Send: %s @ %5d => %15s %s%s",
                 nodeEndpointRouteGetName(route),
                 (NODE_ROUTE_UDP == route ? endpoint->dis.portUDP : endpoint->dis.portTCP),
                 endpoint->hostname,
                 "Error: ",
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
nodeEndpointFillSockAddr (BREthereumNodeEndpoint *endpoint,
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
openSocketReportResult (BREthereumNodeEndpoint *endpoint, int port, int type, int error) {
    eth_log (LES_LOG_TOPIC, "Open: %s @ %5d => %15s %s%s",
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
openSocket(BREthereumNodeEndpoint *endpoint, int *socketToAssign, int port, int domain, int type, double timeout)
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

/// MARK: Local/Bootstrap Enodes

const char *localLESEnode = "enode://x@1.1.1.1:30303";

const char *bootstrapLESEnodes[] = {
    // Localhost - Parity
    //    "enode://8ebe6a85d46737451c8bd9423f37dcb117af7316bbce1643856feeaf9f81a792ff09029e9ab1796b193eb477f938af3465f911574c57161326b71aaf0221f341@192.168.1.111:30303",

    // Localhost - GETH
    //    "enode://a40437d2f44ae655387009d1d69ba9fd07b748b7a6ecfc958c135008a34c0497466db35049c36c8296590b4bcf9b9058f9fa2a688a2c6566654b1f1dc42417e4@127.0.0.1:30303",

    // START - BRD
    // Full
    //"enode://e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851@104.197.99.24:30303",
    // Archival
    "enode://e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851@35.226.238.26:30303",
    // END - BRD

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
};

size_t NUMBER_OF_NODE_ENDPOINT_SPECS = (sizeof (bootstrapLESEnodes) / sizeof (char *));
