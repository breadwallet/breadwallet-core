//
//  BREthereumLESBase.h
//  Core
//
//  Created by Ed Gamble on 9/1/18.
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

#ifndef BR_Ethereum_LES_Base_H
#define BR_Ethereum_LES_Base_H

#include "BRSet.h"
#include "BRArray.h"
#include "../util/BRUtil.h"
#include "../rlp/BRRlp.h"
#include "../base/BREthereumHash.h"
#include "../blockchain/BREthereumNetwork.h"

#define BRArrayOf(type)    type*
#define BRSetOf(type)      BRSet*

#define DEFAULT_UDPPORT     (30303)
#define DEFAULT_TCPPORT     (30303)

#define LES_LOCAL_ENDPOINT_ADDRESS    "1.1.1.1"
#define LES_LOCAL_ENDPOINT_TCP_PORT   DEFAULT_TCPPORT
#define LES_LOCAL_ENDPOINT_UDP_PORT   DEFAULT_UDPPORT
#define LES_LOCAL_ENDPOINT_NAME       "BRD Light Client"

#define LES_LOG_TOPIC "LES"

/** */
#define LES_SUPPORT_PARITY

/** */
#define LES_SUPPORT_GETH        // not until GetProofs{V1,V2} 'works'

/**
 * The Supported P2P version - this applies to both DIS and P2P messaging.
 *
 * https://github.com/ethereum/devp2p/blob/master/discv4.md
 */
#define P2P_MESSAGE_VERSION     0x04

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
