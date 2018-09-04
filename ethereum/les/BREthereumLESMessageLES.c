//
//  BREthereumLESMessageLES.c
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

#include "../blockchain/BREthereumBlockChain.h"
#include "BREthereumLESMessageLES.h"

// GETH Limits
// MaxHeaderFetch           = 192 // Amount of block headers to be fetched per retrieval request
// MaxBodyFetch             = 32  // Amount of block bodies to be fetched per retrieval request
// MaxReceiptFetch          = 128 // Amount of transaction receipts to allow fetching per request
// MaxCodeFetch             = 64  // Amount of contract codes to allow fetching per request
// MaxProofsFetch           = 64  // Amount of merkle proofs to be fetched per retrieval request
// MaxHelperTrieProofsFetch = 64  // Amount of merkle proofs to be fetched per retrieval request
// MaxTxSend                = 64  // Amount of transactions to be send per request
// MaxTxStatus              = 256 // Amount of transactions to queried per request

static BREthereumLESMessageStatusKey lesMessageStatusKeys[] = {
    "protocolVersion",
    "networkId",
    //...
    "flowControl/MRR"
};

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

#if 0
typedef BRRlpItem (*BREthereumLESMessageStatusValueRLPEncoder) (BREthereumLESMessageStatusValue value, BREthereumMessageCoder coder);
typedef BREthereumLESMessageStatusValue (*BREthereumLESMessageStatusValueRLPDecoder) (BRRlpItem item, BREthereumMessageCoder coder);

static BRRlpItem
messageStatusValueRlpEncodeNumber (BREthereumLESMessageStatusValue value, BREthereumMessageCoder coder) {
    return rlpEncodeUInt64(coder.rlp, value.number, 1);
}

static BREthereumLESMessageStatusValue
messageStatusValueRlpDecodeNumber (BRRlpItem item, BREthereumMessageCoder coder) {
    return (BREthereumLESMessageStatusValue) { .number = (uint32_t) rlpDecodeUInt64 (coder.rlp, item, 1) };
}

static BRRlpItem
messageStatusValueRlpEncodeBoolean (BREthereumLESMessageStatusValue value, BREthereumMessageCoder coder) {
    return rlpEncodeUInt64(coder.rlp, value.boolean, 1);
}

static BREthereumLESMessageStatusValue
messageStatusValueRlpDecodeBoolean(BRRlpItem item, BREthereumMessageCoder coder) {
    return (BREthereumLESMessageStatusValue) { .boolean = (int) rlpDecodeUInt64 (coder.rlp, item, 1) };
}

static BRRlpItem
messageStatusValueRlpEncodeBignum (BREthereumLESMessageStatusValue value, BREthereumMessageCoder coder) {
    return rlpEncodeUInt256(coder.rlp, value.bignum, 1);
}

static BREthereumLESMessageStatusValue
messageStatusValueRlpDecodeBignum(BRRlpItem item, BREthereumMessageCoder coder) {
    return (BREthereumLESMessageStatusValue) { .bignum = rlpDecodeUInt256 (coder.rlp, item, 1) };
}


static BRRlpItem
messageStatusValueRlpEncodeHash (BREthereumLESMessageStatusValue value, BREthereumMessageCoder coder) {
    return hashRlpEncode(value.hash, coder);
}

static BREthereumLESMessageStatusValue
messageStatusValueRlpDecodeHash(BRRlpItem item, BREthereumMessageCoder coder) {
    return (BREthereumLESMessageStatusValue) { .hash = hashRlpDecode (item, coder) };
}

struct {
    char *key;
    BREthereumLESMessageStatusValueRLPEncoder encoder;
    BREthereumLESMessageStatusValueRLPDecoder decoder;
} messageStatusKeyValueHandler[] = {
    { "protocolVersion", messageStatusValueRlpEncodeNumber,  messageStatusValueRlpDecodeNumber  },
    { "networkId",       messageStatusValueRlpEncodeNumber,  messageStatusValueRlpDecodeNumber  },
    { "headTd",          messageStatusValueRlpEncodeBignum,  messageStatusValueRlpDecodeBignum  },
    { "headHash",        messageStatusValueRlpEncodeHash,    messageStatusValueRlpDecodeHash    },
    { "headNum" },
    { "genesisHash", },
    { "announceType", },
};
extern BRRlpItem
messageLESStatusRLPEncode (BREthereumLESMessageStatus status, BREthereumMessageCoder coder) {
    size_t itemsCount = array_count(status.pairs);
    BRRlpItem items [itemsCount];

    for (size_t index = 0; index < itemsCount; index++) {
        BREthereumLESMessageStatusKeyValuePair pair = status.pairs[index];
        items[index] = rlpEncodeList2 (coder.rlp,
                                       rlpEncodeString (coder.rlp, (char *) pair.key),
                                       foo);
    }
}
#endif

//
// MARK: LES Status
//

extern BREthereumLESMessageStatus
messageLESStatusCreate (uint64_t protocolVersion,
                        uint64_t chainId,
                        uint64_t headNum,
                        BREthereumHash headHash,
                        UInt256 headTd,
                        BREthereumHash genesisHash,
                        uint64_t announceType) {
    return (BREthereumLESMessageStatus) {
        protocolVersion,            // 2
        chainId,
        headNum,
        headHash,
        headTd,
        genesisHash,

        ETHEREUM_BOOLEAN_FALSE,
        NULL,
        NULL,

        ETHEREUM_BOOLEAN_FALSE,

        NULL,
        NULL,
        NULL,
        NULL,

        announceType                // 1
    };
}

extern BRRlpItem
messageLESStatusEncode (BREthereumLESMessageStatus *status, BREthereumMessageCoder coder) {

    size_t index = 0;
    BRRlpItem items[15];

    items[index++] = rlpEncodeList2 (coder.rlp,
                                     rlpEncodeString(coder.rlp, "protocolVersion"),
                                     rlpEncodeUInt64(coder.rlp, status->protocolVersion,1));

    items[index++] = rlpEncodeList2 (coder.rlp,
                                     rlpEncodeString(coder.rlp, "networkId"),
                                     rlpEncodeUInt64(coder.rlp, status->chainId,1));

    items[index++] = rlpEncodeList2 (coder.rlp,
                                     rlpEncodeString(coder.rlp, "headTd"),
                                     rlpEncodeUInt256(coder.rlp, status->headTd,1));

    items[index++] = rlpEncodeList2(coder.rlp,
                                    rlpEncodeString(coder.rlp, "headHash"),
                                    hashRlpEncode(status->headHash, coder.rlp));

    items[index++] = rlpEncodeList2(coder.rlp,
                                    rlpEncodeString(coder.rlp, "headNum"),
                                    rlpEncodeUInt64(coder.rlp, status->headNum,1));

    items[index++] = rlpEncodeList2 (coder.rlp,
                                     rlpEncodeString(coder.rlp, "genesisHash"),
                                     hashRlpEncode(status->genesisHash, coder.rlp));

    if(ETHEREUM_BOOLEAN_IS_TRUE(status->serveHeaders))
        items[index++] = rlpEncodeList1 (coder.rlp,
                                         rlpEncodeString(coder.rlp, "serveHeaders"));

    if (status->serveChainSince != NULL)
        items[index++] = rlpEncodeList2 (coder.rlp,
                                         rlpEncodeString(coder.rlp, "serveChainSince"),
                                         rlpEncodeUInt64(coder.rlp, *(status->serveChainSince),1));

    if (status->serveStateSince != NULL)
        items[index++] = rlpEncodeList2 (coder.rlp,
                                         rlpEncodeString(coder.rlp, "serveStateSince"),
                                         rlpEncodeUInt64(coder.rlp, *(status->serveStateSince),1));

    if(ETHEREUM_BOOLEAN_IS_TRUE(status->txRelay))
        items[index++] = rlpEncodeList1 (coder.rlp, rlpEncodeString(coder.rlp, "txRelay"));

    //flowControl/BL
    if (status->flowControlBL != NULL)
        items[index++] = rlpEncodeList2 (coder.rlp,
                                         rlpEncodeString(coder.rlp, "flowControl/BL"),
                                         rlpEncodeUInt64(coder.rlp, *(status->flowControlBL),1));

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
    //flowControl/MRR
    if (status->flowControlMRR != NULL)
        items[index++] = rlpEncodeList2 (coder.rlp,
                                         rlpEncodeString(coder.rlp, "flowControl/MRR"),
                                         rlpEncodeUInt64(coder.rlp, *(status->flowControlMRR),1));

    if (status->protocolVersion == 0x02)
        items[index++] = rlpEncodeList2 (coder.rlp,
                                         rlpEncodeString(coder.rlp, "announceType"),
                                         rlpEncodeUInt64(coder.rlp, status->announceType,1));

    return rlpEncodeListItems (coder.rlp, items, index);
}

extern BREthereumLESMessageStatus
messageLESStatusDecode (BRRlpItem item, BREthereumMessageCoder coder) {
    BREthereumLESMessageStatus status = {};

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder.rlp, item, &itemsCount);

    for(int i= 0; i < itemsCount; ++i) {
        size_t keyPairCount;
        const BRRlpItem *keyPairs = rlpDecodeList (coder.rlp, items[i], &keyPairCount);
        if (keyPairCount > 0) {
            char *key = rlpDecodeString(coder.rlp, keyPairs[0]);

            if (strcmp(key, "protocolVersion") == 0) {
                status.protocolVersion = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "networkId") == 0) {
                status.chainId = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "headTd") == 0) {
                status.headTd = rlpDecodeUInt256(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "headHash") == 0) {
                status.headHash = hashRlpDecode(keyPairs[1], coder.rlp);
            } else if (strcmp(key, "announceType") == 0) {
                status.announceType = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "headNum") == 0) {
                status.headNum = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "genesisHash") == 0) {
                status.genesisHash = hashRlpDecode(keyPairs[1], coder.rlp);
            } else if (strcmp(key, "serveHeaders") == 0) {
                status.serveHeaders = ETHEREUM_BOOLEAN_TRUE;
            } else if (strcmp(key, "serveChainSince") == 0) {
                status.serveChainSince = malloc(sizeof(uint64_t));
                *(status.serveChainSince) = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "serveStateSince") == 0) {
                status.serveStateSince = malloc(sizeof(uint64_t));
                *(status.serveStateSince) = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "txRelay") == 0) {
                status.txRelay = ETHEREUM_BOOLEAN_TRUE;
            } else if (strcmp(key, "flowControl/BL") == 0) {
                status.flowControlBL = malloc(sizeof(uint64_t));
                *(status.flowControlBL) = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "flowControl/MRC") == 0) {
                //status.flowControlMRR = malloc(sizeof(uint64_t));
                size_t mrrItemsCount  = 0;
                const BRRlpItem* mrrItems = rlpDecodeList(coder.rlp, keyPairs[1], &mrrItemsCount);
                BREthereumLESMessageStatusMRC *mrcs = NULL;
                if(mrrItemsCount > 0){
                    mrcs = (BREthereumLESMessageStatusMRC*) calloc (mrrItemsCount, sizeof(BREthereumLESMessageStatusMRC));
                    for(int mrrIdx = 0; mrrIdx < mrrItemsCount; ++mrrIdx){
                        size_t mrrElementsCount  = 0;
                        const BRRlpItem* mrrElements = rlpDecodeList(coder.rlp, mrrItems[mrrIdx], &mrrElementsCount);
                        mrcs[mrrIdx].msgCode  = rlpDecodeUInt64(coder.rlp, mrrElements[0], 1);
                        mrcs[mrrIdx].baseCost = rlpDecodeUInt64(coder.rlp, mrrElements[1], 1);
                        mrcs[mrrIdx].reqCost  = rlpDecodeUInt64(coder.rlp, mrrElements[2], 1);
                    }
                }
                status.flowControlMRCCount = malloc (sizeof (size_t));
                *status.flowControlMRCCount = mrrItemsCount;
                status.flowControlMRC = mrcs;
            } else if (strcmp(key, "flowControl/MRR") == 0) {
                status.flowControlMRR = malloc(sizeof(uint64_t));
                *(status.flowControlMRR) = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            }
            free (key);
        }
    }
    return status;
}

extern void
messageLESStatusShow(BREthereumLESMessageStatus *message) {
    BREthereumHashString headHashString, genesisHashString;
    hashFillString (message->headHash, headHashString);
    hashFillString (message->genesisHash, genesisHashString);

    char *headTotalDifficulty = coerceString (message->headTd, 10);

    eth_log (LES_LOG_TOPIC, "StatusMessage:%s", "");
    eth_log (LES_LOG_TOPIC, "    ProtocolVersion: %llu", message->protocolVersion);
    eth_log (LES_LOG_TOPIC, "    AnnounceType   : %llu", message->announceType);
    eth_log (LES_LOG_TOPIC, "    NetworkId      : %llu", message->chainId);
    eth_log (LES_LOG_TOPIC, "    HeadNum        : %llu", message->headNum);
    eth_log (LES_LOG_TOPIC, "    HeadHash       : %s",   headHashString);
    eth_log (LES_LOG_TOPIC, "    HeadTD         : %s",   headTotalDifficulty);
    eth_log (LES_LOG_TOPIC, "    GenesisHash    : %s",   genesisHashString);
    eth_log (LES_LOG_TOPIC, "    ServeHeaders   : %s", ETHEREUM_BOOLEAN_IS_TRUE(message->serveHeaders) ? "Yes" : "No");
    eth_log (LES_LOG_TOPIC, "    ServeChainSince: %llu", (NULL != message->serveChainSince ? *message->serveChainSince : -1)) ;
    eth_log (LES_LOG_TOPIC, "    ServeStateSince: %llu", (NULL != message->serveStateSince ? *message->serveStateSince : -1)) ;
    eth_log (LES_LOG_TOPIC, "    TxRelay        : %s", ETHEREUM_BOOLEAN_IS_TRUE(message->txRelay) ? "Yes" : "No");
    eth_log (LES_LOG_TOPIC, "    FlowControl/BL : %llu", (NULL != message->flowControlBL  ? *message->flowControlBL  : -1));
    eth_log (LES_LOG_TOPIC, "    FlowControl/MRR: %llu", (NULL != message->flowControlMRR ? *message->flowControlMRR : -1));

    size_t count = (NULL == message->flowControlMRCCount ? 0 : *(message->flowControlMRCCount));
    eth_log (LES_LOG_TOPIC, "    FlowControl/MRC:%s", "");
    for (size_t index = 0; index < count; index++) {
        const char *label = messageLESGetIdentifierName ((BREthereumLESMessageIdentifier) message->flowControlMRC[index].msgCode);
        if (NULL != label) {
            eth_log (LES_LOG_TOPIC, "      %2d", (BREthereumLESMessageIdentifier) message->flowControlMRC[index].msgCode);
            eth_log (LES_LOG_TOPIC, "        Request : %s", label);
            eth_log (LES_LOG_TOPIC, "        BaseCost: %llu", message->flowControlMRC[index].baseCost);
            eth_log (LES_LOG_TOPIC, "        ReqCost : %llu", message->flowControlMRC[index].reqCost);
        }
    }

    free (headTotalDifficulty);
}

/// Mark: LES Announce

//
// Announce
//
extern BREthereumLESMessageAnnounce
messageLESAnnounceDecode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    BREthereumLESMessageAnnounce message;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (5 == itemsCount);

    message.headHash = hashRlpDecode (items[0], coder.rlp);
    message.headNumber = rlpDecodeUInt64 (coder.rlp, items[1], 1);
    message.headTotalDifficulty = rlpDecodeUInt256 (coder.rlp, items[2], 1);
    message.reorgDepth = rlpDecodeUInt64 (coder.rlp, items[3], 1);

    // TODO: Decode Keys
    size_t pairCount = 0;
    const BRRlpItem *pairItems = rlpDecodeList (coder.rlp, items[4], &pairCount);
    array_new(message.pairs, pairCount);
    for (size_t index = 0; index < pairCount; index++)
        ;

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
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

    size_t headerItemsCount = 0;
    const BRRlpItem *headerItems = rlpDecodeList (coder.rlp, items[2], &headerItemsCount);

    BRArrayOf(BREthereumBlockHeader) headers;
    array_new (headers, headerItemsCount);
    for (size_t index = 0; index < headerItemsCount; index++)
        array_add (headers, blockHeaderRlpDecode (headerItems[index], RLP_TYPE_NETWORK, coder.rlp));

    return (BREthereumLESMessageBlockHeaders) {
        reqId,
        bv,
        headers
    };
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
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

    size_t pairItemsCount = 0;
    const BRRlpItem *pairItems = rlpDecodeList (coder.rlp, items[2], &pairItemsCount);

    BRArrayOf(BREthereumBlockBodyPair) pairs;
    array_new(pairs, pairItemsCount);
    for (size_t index = 0; index < pairItemsCount; index++) {
        size_t bodyItemsCount;
        const BRRlpItem *bodyItems = rlpDecodeList (coder.rlp, pairItems[index], &bodyItemsCount);
        assert (2 == bodyItemsCount);

        BREthereumBlockBodyPair pair = {
            blockTransactionsRlpDecode (bodyItems[0], coder.network, RLP_TYPE_NETWORK, coder.rlp),
            blockOmmersRlpDecode (bodyItems[1], coder.network, RLP_TYPE_NETWORK, coder.rlp)
        };
        array_add(pairs, pair);
    }
    return (BREthereumLESMessageBlockBodies) {
        reqId,
        bv,
        pairs
    };
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
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

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
    return (BREthereumLESMessageReceipts) {
        reqId,
        bv,
        arrays
    };
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
    // rlpShowItem (coder.rlp, item, "LES Proofs");
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

    return (BREthereumLESMessageProofs) {
        reqId,
        bv,
        mptProofDecodeList (items[2], coder.rlp)
    };
}

/// MARK: LES GetContractCodes

/// MARK: LES ContractCodes

/// MARK: LES SendTx

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
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t bv    = rlpDecodeUInt64 (coder.rlp, items[1], 1);

    return (BREthereumLESMessageTxStatus) {
        reqId,
        bv,
        transactionStatusDecodeList (items[2], coder.rlp)
    };
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
                           rlpEncodeUInt64 (coder.rlp, message.identifier + coder.lesMessageIdOffset, 1),
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
        case LES_MESSAGE_CONTRACT_CODES: return 0;
        case LES_MESSAGE_HEADER_PROOFS:  return 0;
        case LES_MESSAGE_PROOFS_V2:      return message->u.proofsV2.bv;
        case LES_MESSAGE_HELPER_TRIE_PROOFS: return 0;
        case LES_MESSAGE_TX_STATUS:      return message->u.txStatus.bv;
        default: return 0;
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


