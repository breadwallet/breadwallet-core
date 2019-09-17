//
//  BREthereumLESBase.h
//  Core
//
//  Created by Ed Gamble on 9/1/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_LES_Base_H
#define BR_Ethereum_LES_Base_H

#include "support/BRSet.h"
#include "support/BRArray.h"
#include "ethereum/util/BRUtil.h"
#include "ethereum/rlp/BRRlp.h"
#include "ethereum/base/BREthereumHash.h"
#include "ethereum/blockchain/BREthereumNetwork.h"

#define DEFAULT_UDPPORT     (30303)
#define DEFAULT_TCPPORT     (30303)

#define LES_LOCAL_ENDPOINT_ADDRESS    "1.1.1.1"
#define LES_LOCAL_ENDPOINT_TCP_PORT   DEFAULT_TCPPORT
#define LES_LOCAL_ENDPOINT_UDP_PORT   DEFAULT_UDPPORT
#define LES_LOCAL_ENDPOINT_NAME       "BRD Light Client"

#define LES_LOG_TOPIC "LES"

/** */
//#define LES_SUPPORT_PARITY
#define LES_SUPPORT_PARITY_VERSION      (1)

/** */
#define LES_SUPPORT_GETH
#define LES_SUPPORT_GETH_VERSION        (2)
#define LES_SUPPORT_GETH_ANNOUNCE_TYPE  (1)

/**
 * We can optionally only bootstrap from a BRD server.  Setting this overrides the subsequent
 * LES_BOOTSTRAP_LCL_ONLY
 */
#define LES_BOOTSTRAP_BRD_ONLY

/**
 * For debugging only, we can optionally only bootstrap from a LCL (local) server.
 */
#if defined (LES_BOOTSTRAP_BRD_ONLY) || defined (NDEBUG) || !defined (DEBUG)
#undef LES_BOOTSTRAP_LCL_ONLY
#endif

//#define LES_BOOTSTRAP_LCL_ONLY

/**
 * For debugging only, we can optionally disable P2P Node Discovery.  This is very useful for
 * performance and memory allocation/leak analysis
 */
#if defined (NDEBUG) || !defined (DEBUG)
#undef LES_DISABLE_DISCOVERY
#endif

#define LES_DISABLE_DISCOVERY

/**
 * If we attempt to open a socket to a node endpoint and the socket reports EINPROGRESS, we'll
 * select() on the socket with this timeout.  See CORE-265.  This occurs on the LES thread and
 * is a blocking operation - blocking *all* other nodes.  We should consider attempting to resolve
 * an EINPROGRESS by introducing another 'connecting' state (between OPEN and AUTH).  That new
 * state will wait on the 'write file descriptor' and then do `getsockopt()` - if success then
 * move to AUTH otherwise error.
 *
 * But, the above is too onerous at this time.  We can wait; nobody is going any where.
 */
#define NODE_ENDPOINT_OPEN_SOCKET_TIMEOUT 3.0

/**
 * The Supported P2P version - this applies to both DIS and P2P messaging.  There is a V5
 * discovery protocol (at least in Geth) that is defined so as to facilitate discovery of LES
 * nodes... fact is, if we need to explore one node to find a LES node, then we might as well
 * explore N nodes.  And, if Geth wants to faclitate LES nodes, then just turn on LES by default.
 *
 * https://github.com/ethereum/devp2p/blob/master/discv4.md
 */
#define P2P_MESSAGE_VERSION     0x04

// The number of nodes that should be maintained as 'active' (aka 'connected').  Once connected we
// will get announcements of new blocks.  When a transaction is submitted we'll submit it to *all*
// active nodes.  We will need multiple active nodes for submissions as we *routinely* see nodes
// simply dropping submission outright, quietly.
#define LES_ACTIVE_NODE_COUNT 3

// When discovering nodes (on UDP) don't allow more then LES_ACTIVE_NODE_UDP_LIMIT nodes to be
// actively discovering at once.
#define LES_ACTIVE_NODE_UDP_LIMIT 3

// The number of nodes that should be available.  We'll discover nodes until we reach this count.
// Note that this doesn mean that the nodes are LESv2 or PIPv1 nodes - they are just nodes.  As
// we explore individual nodes they may become unavailable and we'll need to discover more.
#define LES_AVAILABLE_NODES_COUNT     (100)

// For a request with a nodeIndex that is not LES_PREFERRED_NODE_INDEX, the intention is to send
// the same message multiple times.  If the message was to be sent to 3 nodes, but only two are
// connected, how many times should the message be sent and to what nodes?  If the following,
// LES_WRAP_SEND_WHEN_MULTIPLE_INDEX, is '1' when we'll send 3 times, including multiple times.  If
// the following is '0' then we'll multiple times up to the number of connected nodes.
//
// TODO: Unused
#define LES_WRAP_SEND_WHEN_MULTIPLE_INDEX    1


#ifdef __cplusplus
extern "C" {
#endif


// BREthereumMessageCoder - when RLP encoding and decoding messages, we need both the
// RLP Coder, the Network and a LES message offset.  The network is required for decoding
// transactions - where the signature encodes the network's chain id.  The LES message offset
// is required to offset a LES message id (away from the P2P id space).
//
// We could have considered modifying BRRlpCoder to include the network - however, the RLP
// module *absolutely does not* depend on anything...  So we'll use this 'MessageCoder' abstraction
// to bundle all the LES specific needs.
//
typedef struct {
    BRRlpCoder rlp;
    BREthereumNetwork network;

    // The offset for LES messages.  This is determined by 'negotiating' subprotocols
    // https://github.com/ethereum/wiki/wiki/ÐΞVp2p-Wire-Protocol
    //
    // "ÐΞVp2p is designed to support arbitrary sub-protocols (aka capabilities) over the basic
    // wire protocol. Each sub-protocol is given as much of the message-ID space as it needs (all
    // such protocols must statically specify how many message IDs they require). On connection and
    // reception of the Hello message, both peers have equivalent information about what
    // subprotocols they share (including versions) and are able to form consensus over the
    // composition of message ID space.
    //
    // "Message IDs are assumed to be compact from ID 0x10 onwards (0x00-0x10 is reserved for
    // ÐΞVp2p messages) and given to each shared (equal-version, equal name) sub-protocol in
    // alphabetic order. Sub-protocols that are not shared are ignored. If multiple versions are
    // shared of the same (equal name) sub-protocol, the numerically highest wins, others are
    // ignored."
    //
    // Generally, we have one protocol specified
    uint64_t messageIdOffset;
} BREthereumMessageCoder;


#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_Base_H */
