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
#include "BRKey.h"
#include "BRUtil.h"
#include "BREthereumP2PCoder.h"

#define ETH_LOG_TOPIC "BREthereumP2PCoder"

//
// Public Functions
//
extern BRRlpData ethereumP2PHelloEncode(BREthereumP2PHello* message) {

    eth_log(ETH_LOG_TOPIC, "%s", "encoding hello message");
    
    BRRlpData data;
    
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem helloDataItems[5];
    
    /**
            Hello 0x00 [p2pVersion: P, clientId: B, [[cap1: B_3, capVersion1: P], [cap2: B_3, capVersion2: P], ...], listenPort: P, nodeId: B_64] First packet sent over the connection, and sent once by both sides. No other messages may be sent until a Hello is received.

            p2pVersion Specifies the implemented version of the P2P protocol. Now must be 1.
            clientId Specifies the client software identity, as a human-readable string (e.g. "Ethereum(++)/1.0.0").
            cap Specifies a peer capability name as a length-3 ASCII string. Current supported capabilities are eth, shh.
            capVersion Specifies a peer capability version as a positive integer. Current supported versions are 34 for eth, and 1 for shh.
            listenPort specifies the port that the client is listening on (on the interface that the present connection traverses). If 0 it indicates the client is not listening.
            nodeId is the Unique Identity of the node and specifies a 512-bit hash that identifies this node.
    **/
    /** Encode the following : [[cap1: B_3, capVersion1: P], [cap2: B_3, capVersion2: P], ...], */
    BRRlpItem caps[array_count(message->caps)];
    
    for(int i = 0; i < array_count(message->caps); ++i){
        BRRlpItem ethCapItems[2];
        ethCapItems[0] = rlpEncodeItemString(coder, message->caps[i].cap);
        ethCapItems[1] = rlpEncodeItemUInt64(coder, message->caps[i].capVersion, 0);
        BRRlpItem etheCapItemsEncoding = rlpEncodeListItems(coder, ethCapItems, 2);
        caps[i] = etheCapItemsEncoding;
    }
    BRRlpItem capsItem = rlpEncodeListItems(coder, caps, array_count(message->caps));
    
    /** Encode the following : [p2pVersion: P, clientId: B, [[cap1: B_3, capVersion1: P], [cap2: B_3, capVersion2: P], ...], listenPort: P, nodeId: B_64] */
    helloDataItems[0] = rlpEncodeItemUInt64(coder, message->version,0);
    helloDataItems[1] = rlpEncodeItemString(coder, message->clientId);
    helloDataItems[2] = capsItem;
    helloDataItems[3] = rlpEncodeItemUInt64(coder, 0x00,1);
    helloDataItems[4] = rlpEncodeItemBytes(coder, message->nodeId.u8, 64);

    /** Encode the following :  Hello 0x00 [p2pVersion: P, clientId: B, [[cap1: B_3, capVersion1: P], [cap2: B_3, capVersion2: P], ...], listenPort: P, nodeId: B_64] */
    BRRlpData listData, idData;
    rlpDataExtract(coder, rlpEncodeItemUInt64(coder, 0x00,1),&idData.bytes, &idData.bytesCount);
    
    rlpDataExtract(coder, rlpEncodeListItems(coder, helloDataItems, 5), &listData.bytes, &listData.bytesCount);
    
    uint8_t * rlpData = malloc(idData.bytesCount + listData.bytesCount);
    memcpy(rlpData, idData.bytes, idData.bytesCount);
    memcpy(&rlpData[idData.bytesCount], listData.bytes, listData.bytesCount);

    data.bytes = rlpData;
    data.bytesCount = idData.bytesCount + listData.bytesCount;
    
    rlpDataRelease(listData);
    rlpDataRelease(idData);
    rlpCoderRelease(coder);
    
    return data;
}

extern BRRlpData ethereumP2PDisconnectEncode(BREthereumDisconnect reason) {

    eth_log(ETH_LOG_TOPIC, "%s", "encoding disconnect message");
    
    BRRlpData data;
    
    BRRlpCoder coder = rlpCoderCreate();

    /** Disconnect 0x01 [reason: P] */
    
    BRRlpItem reasonList[1];
    reasonList[0] = rlpEncodeItemUInt64(coder, (uint64_t)reason, 0);
    BRRlpItem reasonItem = rlpEncodeListItems(coder, reasonList, 1);
    

    /** Encode the following :  0x01 [reason: P]  */
    BRRlpData listData, idData;
    rlpDataExtract(coder, rlpEncodeItemUInt64(coder, 0x01,0),&idData.bytes, &idData.bytesCount);
    rlpDataExtract(coder, reasonItem, &listData.bytes, &listData.bytesCount);
    
    uint8_t * rlpData = malloc(idData.bytesCount + listData.bytesCount);
    memcpy(rlpData, idData.bytes, idData.bytesCount);
    memcpy(&rlpData[idData.bytesCount], listData.bytes, listData.bytesCount);

    data.bytes = rlpData;
    data.bytesCount = idData.bytesCount + listData.bytesCount;
    
    rlpDataRelease(listData);
    rlpDataRelease(idData);
    rlpCoderRelease(coder);
    
    return data;
}
static BRRlpData _emptyMessage(uint64_t messageId) {

    BRRlpData data;
    
    BRRlpCoder coder = rlpCoderCreate();
    
    /** Disconnect 0x01 [reason: P] */
    
    BRRlpItem emptyList[1];
    BRRlpItem emptyItem = rlpEncodeListItems(coder, emptyList, 0);
    

    /** Encode the following : 0x02 [] */
    BRRlpData listData, idData;
    rlpDataExtract(coder, rlpEncodeItemUInt64(coder, messageId,0),&idData.bytes, &idData.bytesCount);
    rlpDataExtract(coder, emptyItem, &listData.bytes, &listData.bytesCount);
    
    uint8_t * rlpData = malloc(idData.bytesCount + listData.bytesCount);
    memcpy(rlpData, idData.bytes, idData.bytesCount);
    memcpy(&rlpData[idData.bytesCount], listData.bytes, listData.bytesCount);

    data.bytes = rlpData;
    data.bytesCount = idData.bytesCount + listData.bytesCount;
    
    rlpDataRelease(listData);
    rlpDataRelease(idData);
    rlpCoderRelease(coder);
    
    return data;

}
extern BRRlpData ethereumP2PPingEncode(void) {

    eth_log(ETH_LOG_TOPIC, "%s", "encoding ping message");
    
    return _emptyMessage(0x02);
}
extern BRRlpData ethereumP2PPongEncode(void) {

    eth_log(ETH_LOG_TOPIC, "%s", "encoding pong message");
    
    return _emptyMessage(0x03);
}
extern BREthereumP2PHello ethereumP2PHelloDecode(BRRlpCoder coder, BRRlpData data) {
    
    BREthereumP2PHello retHello;

    eth_log(ETH_LOG_TOPIC, "%s", "decoding hello message");
    
    BRRlpItem item = rlpGetItem (coder, data);
    
    //Set default values for optional status values
    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    retHello.version      = rlpDecodeItemUInt64(coder, items[0],1);
    retHello.clientId     = rlpDecodeItemString(coder,items[1]);
    retHello.listenPort   = rlpDecodeItemUInt64(coder, items[3],1);
    BRRlpData nodeIdBytes = rlpDecodeItemBytes(coder, items[4]);
    memcpy(retHello.nodeId.u8, nodeIdBytes.bytes, nodeIdBytes.bytesCount);
    rlpDataRelease(nodeIdBytes);
    
    size_t capsCount;
    const BRRlpItem *capItems = rlpDecodeList(coder, items[2], &capsCount);
    BREthereumCapabilities*caps;
    
    array_new(caps, capsCount);
    
    for(int i = 0; i < capsCount; ++i){
        size_t capsItemCount;
        const BRRlpItem* capsItem = rlpDecodeList(coder, capItems[i], &capsItemCount);
        BREthereumCapabilities cap;
        cap.cap = rlpDecodeItemString(coder, capsItem[0]);
        cap.capVersion = rlpDecodeItemUInt64(coder, capsItem[1], 1);
        array_add(caps, cap);
    }
    retHello.caps = caps;
    
    return retHello;
}
extern BREthereumDisconnect ethereumP2PDisconnectDecode(BRRlpCoder coder, BRRlpData data) {

    eth_log(ETH_LOG_TOPIC, "%s", "decoding disconnect message");
    
    BRRlpItem item = rlpGetItem (coder, data);
    
    //Set default values for optional status values
    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    BREthereumDisconnect reason = (BREthereumDisconnect)rlpDecodeItemUInt64(coder, items[0],0);

    return reason;
    
}
extern char* ethereumP2PDisconnectToString(BREthereumDisconnect reason) {

    switch(reason){
    case BRE_DISCONNECT_REQUESTED:
        return "Disconnect requested";
    break;
    case BRE_TCP_ERROR:
        return "TCP sub-system error";
    break;
    case BRE_BREACH_PROTO:
        return "Breach of protocol, e.g. a malformed message, bad RLP, incorrect magic number &c.";
    break;
    case BRE_USELESS_PEER:
        return "Useless peer";
    break;
    case BRE_TOO_MANY_PEERS:
        return "Too many peers";
    break;
    case BRE_ALREADY_CONNECTED:
        return "Already connected";
    break;
    case BRE_INCOMPATIBLE_P2P:
        return "Incompatible P2P protocol version";
    break;
    case BRE_NULL_NODE:
        return "Null node identity received - this is automatically invalid";
    break;
    case BRE_CLIENT_QUIT:
        return "Client quitting";
    break;
    case BRE_UNEXPECTED_ID:
        return "Unexpected identity (i.e. a different identity to a previous connection/what a trusted peer told us)";
    break;
    case BRE_ID_SAME:
        return "Identity is the same as this node (i.e. connected to itself)";
    break;
    case BRE_TIMEOUT:
        return "Timeout on receiving a message (i.e. nothing received since sending last ping)";
    break;
    case BRE_UNKNOWN:
        return "Some other reason specific to a subprotocol";
    break;
    default:
        return "Invalid reason code...";
    break;
    }
}
