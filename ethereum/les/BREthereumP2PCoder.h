//
//  BREthereumP2PHello.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 5/29/18.
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

#ifndef BR_Ethereum_P2P_Hello_h
#define BR_Ethereum_P2P_Hello_h

#include <stdio.h>
#include <inttypes.h>
#include "BREthereumBase.h"
#include "BRInt.h"
#include "BRArray.h"
#include "BRRlp.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    BRE_DISCONNECT_REQUESTED = 0x00, //0x00 Disconnect requested
    BRE_TCP_ERROR,                   //0x01 TCP sub-system error
    BRE_BREACH_PROTO,                //0x02 Breach of protocol, e.g. a malformed message, bad RLP, incorrect magic number &c.
    BRE_USELESS_PEER,                //0x03 Useless peer
    BRE_TOO_MANY_PEERS,              //0x04 Too many peers
    BRE_ALREADY_CONNECTED,           //0x05 Already connected
    BRE_INCOMPATIBLE_P2P,            //0x06 Incompatible P2P protocol version
    BRE_NULL_NODE,                   //0x07 Null node identity received - this is automatically invalid
    BRE_CLIENT_QUIT,                 //0x08 Client quitting
    BRE_UNEXPECTED_ID,               //0x09 Unexpected identity (i.e. a different identity to a previous connection/what a trusted peer told us)
    BRE_ID_SAME,                     //0x0a Identity is the same as this node (i.e. connected to itself);
    BRE_TIMEOUT,                     //0x0b Timeout on receiving a message (i.e. nothing received since sending last ping);
    BRE_UNKNOWN                      //0x10 Some other reason specific to a subprotocol.
}BREthereumDisconnect;




typedef enum {
    BRE_P2P_HELLO      = 0x00,
    BRE_P2P_DISCONNECT = 0x01,
    BRE_P2P_PING       = 0x02,
    BRE_P2P_PONG       = 0x03
}EthereumP2PMessages;

typedef struct BREthereumCapabilitiesRecord {
    char*cap;                 // cap Specifies a peer capability name as a length-3 ASCII string. Current supported capabilities are eth, shh.
    uint64_t capVersion;     // capVersion Specifies a peer capability version as a positive integer. Current supported versions are 34 for eth, and 1 for shh.
}BREthereumCapabilities;

typedef struct BREthereumP2PHelloRecord {
      uint64_t version;                 // p2pVersion Specifies the implemented version of the P2P protocol. Now must be 1.
      char* clientId;                   // clientId Specifies the client software identity, as a human-readable string (e.g. "Ethereum(++)/1.0.0").
      BREthereumCapabilities * caps;    // The capabilities for this hello message
      uint64_t listenPort;              // specifies the port that the client is listening on (on the interface that the present connection traverses). If 0 it indicates the client is not listening.
      UInt512 nodeId;                   // nodeId is the Unique Identity of the node and specifies a 512-bit hash that identifies this node.
}BREthereumP2PHello;

typedef uint64_t BREthereumP2PPing;
typedef uint64_t BREthereumP2PPong;


//Encoded messages
extern BRRlpData ethereumP2PHelloEncode(BREthereumP2PHello* message);
extern BRRlpData ethereumP2PDisconnectEncode(BREthereumDisconnect reason);
extern BRRlpData ethereumP2PPingEncode(void);
extern BRRlpData ethereumP2PPongEncode(void);

//Decoded messages
extern BREthereumP2PHello ethereumP2PHelloDecode(BRRlpCoder coder, BRRlpData data);
extern BREthereumDisconnect ethereumP2PDisconnectDecode(BRRlpCoder coder, BRRlpData data);
extern BRRlpData ethereumP2PPingEncode(void);
extern BRRlpData ethereumP2PPongEncode(void);

//Utils
extern char* ethereumP2PDisconnectToString(BREthereumDisconnect reason);


#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_P2P_Hello_h */
