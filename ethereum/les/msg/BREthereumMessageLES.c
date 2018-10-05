//
//  BREthereumMessageLES.c
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

#include "../../blockchain/BREthereumBlockChain.h"
#include "BREthereumMessageLES.h"

// GETH Limits
// MaxHeaderFetch           = 192 // Amount of block headers to be fetched per retrieval request
// MaxBodyFetch             = 32  // Amount of block bodies to be fetched per retrieval request
// MaxReceiptFetch          = 128 // Amount of transaction receipts to allow fetching per request
// MaxCodeFetch             = 64  // Amount of contract codes to allow fetching per request
// MaxProofsFetch           = 64  // Amount of merkle proofs to be fetched per retrieval request
// MaxHelperTrieProofsFetch = 64  // Amount of merkle proofs to be fetched per retrieval request
// MaxTxSend                = 64  // Amount of transactions to be send per request
// MaxTxStatus              = 256 // Amount of transactions to queried per request

/// MARK: - LES (Light Ethereum Subprotocol) Messages

// Static
BREthereumLESMessageSpec messageLESSpecs [NUMBER_OF_LES_MESSAGE_IDENTIFIERS] = {
    { "Status",           LES_MESSAGE_USE_STATUS           },
    { "Announce",         LES_MESSAGE_USE_STATUS           },
    { "GetBlockHeaders",  LES_MESSAGE_USE_REQUEST,    192  },
    { "BlockHeaders",     LES_MESSAGE_USE_RESPONSE         },
    { "GetBlockBodies",   LES_MESSAGE_USE_REQUEST,     32  },
    { "BlockBodies",      LES_MESSAGE_USE_RESPONSE         },
    { "GetReceipts",      LES_MESSAGE_USE_REQUEST,    128  },
    { "Receipts",         LES_MESSAGE_USE_RESPONSE         },
    { "GetProofs",        LES_MESSAGE_USE_REQUEST,     64  },
    { "Proofs",           LES_MESSAGE_USE_RESPONSE         },
    { "GetContractCodes", LES_MESSAGE_USE_REQUEST,     64  },
    { "ContractCodes",    LES_MESSAGE_USE_RESPONSE         },
    { "SendTx",           LES_MESSAGE_USE_STATUS,      64  }, // has cost, no response
    { "GetHeaderProofs",  LES_MESSAGE_USE_REQUEST,     64  },
    { "HeaderProofs",     LES_MESSAGE_USE_RESPONSE         },
    { "GetProofsV2",      LES_MESSAGE_USE_REQUEST,     64  },
    { "ProofsV2",         LES_MESSAGE_USE_RESPONSE         },
    { "GetHelperTrieProofs", LES_MESSAGE_USE_REQUEST,  64  },
    { "HelperTrieProofs",    LES_MESSAGE_USE_RESPONSE      },
    { "SendTx2",          LES_MESSAGE_USE_REQUEST,     64  },
    { "GetTxStatus",      LES_MESSAGE_USE_REQUEST,    256  },
    { "TxStatus",         LES_MESSAGE_USE_RESPONSE         }
};
extern const char *
messageLESGetIdentifierName (BREthereumLESMessageIdentifier identifer) {
    return messageLESSpecs[identifer].name;
}

//
// MARK: LES Status
//

extern void
messageLESStatusShow(BREthereumLESMessageStatus *status) {
    messageP2PStatusShow(&status->p2p);

    eth_log (LES_LOG_TOPIC, "    FlowControl/MRC:%s", "");
    for (size_t index = 0; index < NUMBER_OF_LES_MESSAGE_IDENTIFIERS; index++)
        if (index == status->costs[index].msgCode) {
            const char *label = messageLESGetIdentifierName ((BREthereumLESMessageIdentifier) status->costs[index].msgCode);
            if (NULL != label) {
                eth_log (LES_LOG_TOPIC, "        Request : %llu (%s)", status->costs[index].msgCode, label);
                eth_log (LES_LOG_TOPIC, "        BaseCost: %llu", status->costs[index].baseCost);
                eth_log (LES_LOG_TOPIC, "        ReqCost : %llu", status->costs[index].reqCost);
                eth_log (LES_LOG_TOPIC, "%s", "");
            }
        }
}

extern BRRlpItem
messageLESStatusEncode (BREthereumLESMessageStatus *status,
                        BREthereumMessageCoder coder) {
    return messageP2PStatusEncode(&status->p2p, coder);
#if 0
    //flowControl/MRC
    if(status->flowControlBL != NULL) {
        size_t count = *(status->flowControlMRCCount);
        BRRlpItem mrcItems[count];

        for(int idx = 0; idx < count; ++idx){
            BRRlpItem mrcElements [3];
            mrcElements[0] = rlpEncodeUInt64(coder.rlp,status->flowControlMRC[idx].msgCode,1);
            mrcElements[1] = rlpEncodeUInt64(coder.rlp,status->flowControlMRC[idx].baseCost,1);
            mrcElements[2] = rlpEncodeUInt64(coder.rlp,status->flowControlMRC[idx].reqCost,1);
            mrcItems[idx] = rlpEncodeListItems(coder.rlp, mrcElements, 3);
        }

        items[index++] = rlpEncodeList2 (coder.rlp,
                                         rlpEncodeString(coder.rlp, "flowControl/MRC"),
                                         rlpEncodeListItems(coder.rlp, mrcItems, count));
    }
#endif
}

extern BREthereumLESMessageStatus
messageLESStatusDecode (BRRlpItem item,
                        BREthereumMessageCoder coder) {
    BRRlpItem costItems = NULL;

    // Get the LES Status filled with the baseline P2P Status
    BREthereumLESMessageStatus status = {
        messageP2PStatusDecode(item, coder, &costItems)
    };

    memset (&status.costs, 0, sizeof (status.costs));

    if (NULL != costItems) {

        // Get the individual cost items.
        size_t count;
        const BRRlpItem* items = rlpDecodeList(coder.rlp, costItems, &count);

        // process each cost items
        for (size_t index = 0; index < count; index++) {
            // get the individual cost
            size_t elementCount = 0;
            const BRRlpItem* elements = rlpDecodeList (coder.rlp, items[index], &elementCount);

            // Ignore if RLP decode fails
            if (3 != elementCount) break;

            // fill in status.
            size_t msgCode = rlpDecodeUInt64(coder.rlp, elements[0], 1);
            status.costs[msgCode].msgCode = msgCode;
            status.costs[msgCode].baseCost = rlpDecodeUInt64(coder.rlp, elements[1], 1);
            status.costs[msgCode].reqCost  = rlpDecodeUInt64(coder.rlp, elements[2], 1);
        }
    }

    return status;
}

extern BREthereumLESMessageStatus
messageLESStatusCreate (uint64_t protocolVersion,
                        uint64_t chainId,
                        uint64_t headNum,
                        BREthereumHash headHash,
                        UInt256 headTd,
                        BREthereumHash genesisHash,
                        uint64_t announceType) {
    BRArrayOf(BREthereumP2PMessageStatusKeyValuePair) pairs;
    BREthereumP2PMessageStatusKeyValuePair pair = {
        P2P_MESSAGE_STATUS_ANNOUNCE_TYPE,
        {
            P2P_MESSAGE_STATUS_VALUE_INTEGER,
            announceType
        }
    };

    array_new (pairs, 1);
    array_add (pairs, pair);

    return (BREthereumLESMessageStatus) {
        protocolVersion,            // If protocolVersion is LESv2 ...
        chainId,
        headNum,
        headHash,
        headTd,
        genesisHash,

        pairs
    };
}

/// Mark: LES Announce

//
// Announce
//
extern BREthereumLESMessageAnnounce
messageLESAnnounceDecode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    BREthereumLESMessageAnnounce message = {};

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    if (5 != itemsCount && 4 != itemsCount) rlpCoderSetFailed (coder.rlp);
    else {
        message.headHash = hashRlpDecode (items[0], coder.rlp);
        message.headNumber = rlpDecodeUInt64 (coder.rlp, items[1], 1);
        message.headTotalDifficulty = rlpDecodeUInt256 (coder.rlp, items[2], 1);
        message.reorgDepth = rlpDecodeUInt64 (coder.rlp, items[3], 1);

        // TODO: Decode Keys
        if (5 == itemsCount) {
            size_t pairCount = 0;
            const BRRlpItem *pairItems = rlpDecodeList (coder.rlp, items[4], &pairCount);
            array_new(message.pairs, pairCount);
            for (size_t index = 0; index < pairCount; index++)
                ;
        }
    }
    return message;
}

/// MARK: LES Get Block Headers

//
// Get Block Headers
//
extern BREthereumLESMessageGetBlockHeaders
messageLESGetBlockHeadersCreate (uint64_t reqId,
                                 uint64_t number,
                                 uint32_t maxHeaders,
                                 uint64_t skip,
                                 uint8_t  reverse) {
    return (BREthereumLESMessageGetBlockHeaders) {
        reqId,
        1,
        { .number = number },
        maxHeaders,
        skip,
        reverse};
}

extern BRRlpItem
messageLESGetBlockHeadersEncode (BREthereumLESMessageGetBlockHeaders message,
                                 BREthereumMessageCoder coder) {
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           rlpEncodeList (coder.rlp, 4,
                                          (message.useBlockNumber
                                           ? rlpEncodeUInt64 (coder.rlp, message.block.number, 1)
                                           : hashRlpEncode (message.block.hash, coder.rlp)),
                                          rlpEncodeUInt64 (coder.rlp, message.maxHeaders, 1),
                                          rlpEncodeUInt64 (coder.rlp, message.skip, 1),
                                          rlpEncodeUInt64 (coder.rlp, message.reverse, 1)));
}

extern BREthereumLESMessageGetBlockHeaders
messageLESGetBlockHeadersDecode (BRRlpItem item,
                                 BREthereumMessageCoder coder) {
    assert (0);
}

/// MARK: BlockHeaders

extern BREthereumLESMessageBlockHeaders
messageLESBlockHeadersDecode (BRRlpItem item,
                              BREthereumMessageCoder coder) {
    BREthereumLESMessageBlockHeaders message = {};
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    if (3 != itemsCount) rlpCoderSetFailed (coder.rlp);
    else {
        message.reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
        message.bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

        size_t headerItemsCount = 0;
        const BRRlpItem *headerItems = rlpDecodeList (coder.rlp, items[2], &headerItemsCount);

        BRArrayOf(BREthereumBlockHeader) headers;
        array_new (headers, headerItemsCount);
        for (size_t index = 0; index < headerItemsCount; index++)
            array_add (headers, blockHeaderRlpDecode (headerItems[index], RLP_TYPE_NETWORK, coder.rlp));

        message.headers = headers;
    }

    return message;
}

/// MARK: LES GetBlockBodies

static BRRlpItem
messageLESGetBlockBodiesEncode (BREthereumLESMessageGetBlockBodies message, BREthereumMessageCoder coder) {
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           hashEncodeList (message.hashes, coder.rlp));
}

/// MARK: LES BlockBodies

static BREthereumLESMessageBlockBodies
messageLESBlockBodiesDecode (BRRlpItem item,
                             BREthereumMessageCoder coder) {
    BREthereumLESMessageBlockBodies message = {};

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    if (3 != itemsCount) rlpCoderSetFailed (coder.rlp);
    else {
        message.reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
        message.bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

        size_t pairItemsCount = 0;
        const BRRlpItem *pairItems = rlpDecodeList (coder.rlp, items[2], &pairItemsCount);

        BRArrayOf(BREthereumBlockBodyPair) pairs;
        array_new(pairs, pairItemsCount);
        for (size_t index = 0; index < pairItemsCount; index++) {
            size_t bodyItemsCount;
            const BRRlpItem *bodyItems = rlpDecodeList (coder.rlp, pairItems[index], &bodyItemsCount);
            if (2 != bodyItemsCount) { rlpCoderSetFailed (coder.rlp); break /* for */; }
            else {
                BREthereumBlockBodyPair pair = {
                    blockTransactionsRlpDecode (bodyItems[0], coder.network, RLP_TYPE_NETWORK, coder.rlp),
                    blockOmmersRlpDecode (bodyItems[1], coder.network, RLP_TYPE_NETWORK, coder.rlp)
                };
                array_add(pairs, pair);
            }
        }

        message.pairs = pairs;
    }

    return message;
}

/// MARK: LES GetReceipts

static BRRlpItem
messageLESGetReceiptsEncode (BREthereumLESMessageGetReceipts message, BREthereumMessageCoder coder) {
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           hashEncodeList (message.hashes, coder.rlp));
}

/// MARK: LES Receipts

static BREthereumLESMessageReceipts
messageLESReceiptsDecode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    BREthereumLESMessageReceipts message = {};

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    if (3 != itemsCount) rlpCoderSetFailed (coder.rlp);
    else {
        message.reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
        message.bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

        size_t arrayItemsCount = 0;
        const BRRlpItem *arrayItems = rlpDecodeList (coder.rlp, items[2], &arrayItemsCount);

        BRArrayOf(BREthereumLESMessageReceiptsArray) arrays;
        array_new(arrays, arrayItemsCount);
        for (size_t index = 0; index < arrayItemsCount; index++) {
            BREthereumLESMessageReceiptsArray array = {
                transactionReceiptDecodeList (arrayItems[index], coder.rlp)
            };
            array_add (arrays, array);
        }

        message.arrays = arrays;
    }

    return message;
}

/// MARK: LES GetProofs

static BRRlpItem
proofsSpecEncode (BREthereumLESMessageGetProofsSpec spec,
                  BREthereumMessageCoder coder) {
    return rlpEncodeList (coder.rlp, 4,
                          hashRlpEncode (spec.blockHash, coder.rlp),
                          rlpEncodeBytes (coder.rlp, spec.key1.bytes, spec.key1.bytesCount),
                          rlpEncodeBytes (coder.rlp, spec.key2.bytes, spec.key2.bytesCount),
                          rlpEncodeUInt64 (coder.rlp, spec.fromLevel, 1));
}

static BRRlpItem
proofsSpecEncodeList (BRArrayOf(BREthereumLESMessageGetProofsSpec) specs,
                      BREthereumMessageCoder coder) {
    size_t itemsCount = array_count(specs);
    BRRlpItem items[itemsCount];

    for (size_t index = 0; index < itemsCount; index++)
        items[index] = proofsSpecEncode (specs[index], coder);

    return rlpEncodeListItems (coder.rlp, items, itemsCount);
}

static BRRlpItem
messageLESGetProofsEncode (BREthereumLESMessageGetProofs message, BREthereumMessageCoder coder) {
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           proofsSpecEncodeList(message.specs, coder));
}

/// MARK: LES Proofs

static BREthereumLESMessageProofs
messageLESProofsDecode (BRRlpItem item,
                        BREthereumMessageCoder coder) {
    BREthereumLESMessageProofs message = {};
    // rlpShowItem (coder.rlp, item, "LES Proofs");
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    if (3 != itemsCount) rlpCoderSetFailed (coder.rlp);
    else {
        message.reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
        message.bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);
        message.paths = mptProofDecodeList (items[2], coder.rlp);
    }

    return message;
}

/// MARK: LES GetContractCodes

/// MARK: LES ContractCodes

/// MARK: LES SendTx

static BRRlpItem
messageLESSendTxEncode (BREthereumLESMessageSendTx message, BREthereumMessageCoder coder) {
    size_t itemsCount = array_count(message.transactions);
    BRRlpItem items[itemsCount];

    for (size_t index = 0; index < itemsCount; index++)
        items[index] = transactionRlpEncode(message.transactions[index],
                                            coder.network,
                                            RLP_TYPE_TRANSACTION_SIGNED,
                                            coder.rlp);

    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           rlpEncodeListItems (coder.rlp, items, itemsCount));
}

/// MARK: LES GetHeaderProofs

/// MARK: LES HeaderProofs

/// MARK: LES GetProofsV2

static BRRlpItem
messageLESGetProofsV2Encode (BREthereumLESMessageGetProofsV2 message, BREthereumMessageCoder coder) {
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           proofsSpecEncodeList(message.specs, coder));
}

/// MARK: LES ProofsV2

static BREthereumLESMessageProofsV2
messageLESProofsV2Decode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    // rlpShowItem (coder.rlp, item, "LES ProofsV2");
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

    return (BREthereumLESMessageProofsV2) {
        reqId,
        bv,
        mptProofDecodeList (items[2], coder.rlp)
    };
}

/// MARK: LES GetHelperTrieProofs

/// MARK: LES HelperTrieProofs

/// MARK: LES SendTx2

static BRRlpItem
messageLESSendTx2Encode (BREthereumLESMessageSendTx2 message, BREthereumMessageCoder coder) {
    size_t itemsCount = array_count(message.transactions);
    BRRlpItem items[itemsCount];

    for (size_t index = 0; index < itemsCount; index++)
        items[index] = transactionRlpEncode(message.transactions[index],
                                            coder.network,
                                            RLP_TYPE_TRANSACTION_SIGNED,
                                            coder.rlp);

    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           rlpEncodeListItems (coder.rlp, items, itemsCount));
}

/// MARK: LES GetTxStatus

static BRRlpItem
messageLESGetTxStatusEncode (BREthereumLESMessageGetTxStatus message, BREthereumMessageCoder coder) {
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.reqId, 1),
                           hashEncodeList (message.hashes, coder.rlp));
}

/// MARK: LES TxStatus

static BREthereumLESMessageTxStatus
messageLESTxStatusDecode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    BREthereumLESMessageTxStatus message = {};

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    if (3 != itemsCount) rlpCoderSetFailed (coder.rlp);
    else {
        message.reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
        message.bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);
        message.stati = transactionStatusDecodeList (items[2], coder.rlp);
    }

    return message;
}

/// MARK: LES Messages

extern BREthereumLESMessage
messageLESDecode (BRRlpItem item,
                  BREthereumMessageCoder coder,
                  BREthereumLESMessageIdentifier identifier) {
    switch (identifier) {
        case LES_MESSAGE_STATUS:
            return (BREthereumLESMessage) {
                LES_MESSAGE_STATUS,
                { .status = messageLESStatusDecode (item, coder) }
            };

        case LES_MESSAGE_ANNOUNCE:
            return (BREthereumLESMessage) {
                LES_MESSAGE_ANNOUNCE,
                { .announce = messageLESAnnounceDecode (item, coder) }
            };

        case LES_MESSAGE_BLOCK_HEADERS:
            return (BREthereumLESMessage) {
                LES_MESSAGE_BLOCK_HEADERS,
                { .blockHeaders = messageLESBlockHeadersDecode (item, coder)} };

        case LES_MESSAGE_BLOCK_BODIES:
            return (BREthereumLESMessage) {
                LES_MESSAGE_BLOCK_BODIES,
                { .blockBodies = messageLESBlockBodiesDecode (item, coder)} };

        case LES_MESSAGE_RECEIPTS:
            return (BREthereumLESMessage) {
                LES_MESSAGE_RECEIPTS,
                { .receipts = messageLESReceiptsDecode (item, coder)} };

        case LES_MESSAGE_PROOFS:
            return (BREthereumLESMessage) {
                LES_MESSAGE_PROOFS,
                { .proofs = messageLESProofsDecode (item, coder)} };

        case LES_MESSAGE_PROOFS_V2:
            // rlpShowItem (coder.rlp, item, "LES ProofsV2");
            return (BREthereumLESMessage) {
                LES_MESSAGE_PROOFS_V2,
                { .proofsV2 = messageLESProofsV2Decode (item, coder)} };

        case LES_MESSAGE_TX_STATUS:
            return (BREthereumLESMessage) {
                LES_MESSAGE_TX_STATUS,
                { .txStatus = messageLESTxStatusDecode (item, coder)} };

        default:
            assert (0);
    }
}

extern BRRlpItem
messageLESEncode (BREthereumLESMessage message,
                  BREthereumMessageCoder coder) {
    BRRlpItem body = NULL;
    switch (message.identifier) {
        case LES_MESSAGE_STATUS:
            body = messageLESStatusEncode(&message.u.status, coder);
            break;

        case LES_MESSAGE_GET_BLOCK_HEADERS:
            body = messageLESGetBlockHeadersEncode (message.u.getBlockHeaders, coder);
            break;

        case LES_MESSAGE_GET_BLOCK_BODIES:
            body = messageLESGetBlockBodiesEncode (message.u.getBlockBodies, coder);
            break;

        case LES_MESSAGE_GET_RECEIPTS:
            body = messageLESGetReceiptsEncode (message.u.getReceipts, coder);
            break;

        case LES_MESSAGE_GET_PROOFS:
            body = messageLESGetProofsEncode (message.u.getProofs, coder);
            // rlpShowItem (coder.rlp, body, "LES GetProofs");
            break;

        case LES_MESSAGE_SEND_TX:
            body = messageLESSendTxEncode (message.u.sendTx, coder);
            break;

        case LES_MESSAGE_GET_PROOFS_V2:
            body = messageLESGetProofsV2Encode (message.u.getProofsV2, coder);
            //rlpShowItem (coder.rlp, body, "LES GetProofsV2");
            break;

        case LES_MESSAGE_GET_TX_STATUS:
            body = messageLESGetTxStatusEncode (message.u.getTxStatus, coder);
            break;

        case LES_MESSAGE_SEND_TX2:
            body = messageLESSendTx2Encode (message.u.sendTx2, coder);
            break;

        default:
            assert (0);
    }

    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.identifier + coder.messageIdOffset, 1),
                           body);
}

extern int
messageLESHasUse (const BREthereumLESMessage *message,
                  BREthereumLESMessageUse use) {
    return use == messageLESSpecs[message->identifier].use;
}

// 0 if not response
extern uint64_t
messageLESGetCredits (const BREthereumLESMessage *message) {
    switch (message->identifier) {
        case LES_MESSAGE_BLOCK_HEADERS:  return message->u.blockHeaders.bv;
        case LES_MESSAGE_BLOCK_BODIES:   return message->u.blockBodies.bv;
        case LES_MESSAGE_RECEIPTS:       return message->u.receipts.bv;
        case LES_MESSAGE_PROOFS:         return message->u.proofs.bv;
        case LES_MESSAGE_CONTRACT_CODES: return message->u.contractCodes.bv;
        case LES_MESSAGE_HEADER_PROOFS:  return message->u.headerProofs.bv;
        case LES_MESSAGE_PROOFS_V2:      return message->u.proofsV2.bv;
        case LES_MESSAGE_HELPER_TRIE_PROOFS: return message->u.helperTrieProofs.bv;
        case LES_MESSAGE_TX_STATUS:      return message->u.txStatus.bv;
        default: return 0;
    }
}

extern uint64_t
messageLESGetCreditsCount (const BREthereumLESMessage *lm) {
    switch (lm->identifier) {
        case LES_MESSAGE_GET_BLOCK_HEADERS:  return lm->u.getBlockHeaders.maxHeaders;
        case LES_MESSAGE_GET_BLOCK_BODIES:   return array_count (lm->u.getBlockBodies.hashes);
        case LES_MESSAGE_GET_RECEIPTS:       return array_count(lm->u.getReceipts.hashes);
        case LES_MESSAGE_GET_PROOFS:         return array_count(lm->u.getProofs.specs);
        case LES_MESSAGE_GET_CONTRACT_CODES: return 0;
        case LES_MESSAGE_SEND_TX:            return array_count(lm->u.sendTx.transactions);
        case LES_MESSAGE_GET_HEADER_PROOFS:  return 0;
        case LES_MESSAGE_GET_PROOFS_V2:      return array_count(lm->u.getProofsV2.specs);
        case LES_MESSAGE_GET_HELPER_TRIE_PROOFS: return 0;
        case LES_MESSAGE_SEND_TX2:           return array_count(lm->u.sendTx2.transactions);
        case LES_MESSAGE_GET_TX_STATUS:      return array_count(lm->u.getTxStatus.hashes);
        default:
            return 0;
    }
}

extern uint64_t
messageLESGetRequestId (const BREthereumLESMessage *message) {
    switch (message->identifier) {
        case LES_MESSAGE_STATUS: return LES_MESSAGE_NO_REQUEST_ID;
        case LES_MESSAGE_ANNOUNCE: return LES_MESSAGE_NO_REQUEST_ID;
        case LES_MESSAGE_GET_BLOCK_HEADERS: return message->u.getBlockBodies.reqId;
        case LES_MESSAGE_BLOCK_HEADERS: return message->u.blockHeaders.reqId;
        case LES_MESSAGE_GET_BLOCK_BODIES: return message->u.getBlockBodies.reqId;
        case LES_MESSAGE_BLOCK_BODIES: return message->u.blockBodies.reqId;
        case LES_MESSAGE_GET_RECEIPTS: return message->u.getReceipts.reqId;
        case LES_MESSAGE_RECEIPTS: return message->u.receipts.reqId;
        case LES_MESSAGE_GET_PROOFS: return message->u.getProofs.reqId;
        case LES_MESSAGE_PROOFS: return message->u.proofs.reqId;
        case LES_MESSAGE_GET_CONTRACT_CODES: return message->u.getContractCodes.reqId;
        case LES_MESSAGE_CONTRACT_CODES: return message->u.contractCodes.reqId;
        case LES_MESSAGE_SEND_TX: return message->u.sendTx.reqId;
        case LES_MESSAGE_GET_HEADER_PROOFS: return message->u.getHeaderProofs.reqId;
        case LES_MESSAGE_HEADER_PROOFS: return message->u.headerProofs.reqId;
        case LES_MESSAGE_GET_PROOFS_V2: return message->u.getProofsV2.reqId;
        case LES_MESSAGE_PROOFS_V2: return message->u.proofsV2.reqId;
        case LES_MESSAGE_GET_HELPER_TRIE_PROOFS: return message->u.getHelperTrieProofs.reqId;
        case LES_MESSAGE_HELPER_TRIE_PROOFS: return message->u.helperTrieProofs.reqId;
        case LES_MESSAGE_SEND_TX2: return message->u.sendTx2.reqId;
        case LES_MESSAGE_GET_TX_STATUS: return message->u.getTxStatus.reqId;
        case LES_MESSAGE_TX_STATUS: return message->u.txStatus.reqId;
    }
}
