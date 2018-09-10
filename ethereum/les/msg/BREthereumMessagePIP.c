//
//  BREthereumMessagePIP.c
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

#include "BREthereumMessagePIP.h"

extern const char *
messagePIPGetRequestName (BREthereumPIPRequestType type) {
    static const char *
    messagePIPRequestNames[] = {
        "Headers",
        "Headers Proof",
        "Tx Index",
        "Receipts",
        "Bodies",
        "Accounts",
        "Storage",
        "Code",
        "Execution"
    };

    return messagePIPRequestNames[type];
}

extern const char *
messagePIPGetIdentifierName (BREthereumPIPMessageType identifer) {
    static const char *
    messagePIPNames[] = {
        "Status",
        "Announce",
        "Request",
        "Response",
        "UpdCredit",
        "AckCredit",
        "RelayTx"
    };

    return messagePIPNames[identifer];
}

/// MARK: PIP Status

extern BRRlpItem
messagePIPStatusEncode (BREthereumPIPMessageStatus *status,
                        BREthereumMessageCoder coder) {

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
#if 0
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
#endif
    return rlpEncodeListItems (coder.rlp, items, index);
}

extern BREthereumPIPMessageStatus
messagePIPStatusDecode (BRRlpItem item,
                        BREthereumMessageCoder coder) {
    BREthereumPIPMessageStatus status = {};

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
#if 0
            } else if (strcmp(key, "announceType") == 0) {
                status.announceType = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
#endif
            } else if (strcmp(key, "headNum") == 0) {
                status.headNum = rlpDecodeUInt64(coder.rlp, keyPairs[1], 1);
            } else if (strcmp(key, "genesisHash") == 0) {
                status.genesisHash = hashRlpDecode(keyPairs[1], coder.rlp);
#if 0
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
                // TODO: Wrong for PIP
                status.flowControlMRR = malloc(sizeof(uint64_t));
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
#endif
            }
            free (key);
        }
    }
    return status;
}

extern void
messagePIPStatusShow(BREthereumPIPMessageStatus *message) {
    BREthereumHashString headHashString, genesisHashString;
    hashFillString (message->headHash, headHashString);
    hashFillString (message->genesisHash, genesisHashString);

    char *headTotalDifficulty = coerceString (message->headTd, 10);

    eth_log (LES_LOG_TOPIC, "StatusMessage:%s", "");
    eth_log (LES_LOG_TOPIC, "    ProtocolVersion: %llu", message->protocolVersion);
#if 0
    if (message->protocolVersion != 1)
        eth_log (LES_LOG_TOPIC, "    AnnounceType   : %llu", message->announceType);
#endif
    eth_log (LES_LOG_TOPIC, "    NetworkId      : %llu", message->chainId);
    eth_log (LES_LOG_TOPIC, "    HeadNum        : %llu", message->headNum);
    eth_log (LES_LOG_TOPIC, "    HeadHash       : %s",   headHashString);
    eth_log (LES_LOG_TOPIC, "    HeadTd         : %s",   headTotalDifficulty);
    eth_log (LES_LOG_TOPIC, "    GenesisHash    : %s",   genesisHashString);
#if 0
    if (ETHEREUM_BOOLEAN_IS_TRUE(message->serveHeaders))
        eth_log (LES_LOG_TOPIC, "    ServeHeaders   : %s",  "Yes" );
    if (NULL != message->serveChainSince)
        eth_log (LES_LOG_TOPIC, "    ServeChainSince: %llu", *message->serveChainSince);
    if (NULL != message->serveStateSince)
        eth_log (LES_LOG_TOPIC, "    ServeStateSince: %llu", *message->serveStateSince) ;
    if (ETHEREUM_BOOLEAN_IS_TRUE(message->txRelay))
        eth_log (LES_LOG_TOPIC, "    TxRelay        : %s",  "Yes");
    if (NULL != message->flowControlBL)
        eth_log (LES_LOG_TOPIC, "    FlowControl/BL : %llu", *message->flowControlBL);
    if (NULL != message->flowControlMRR)
        eth_log (LES_LOG_TOPIC, "    FlowControl/MRR: %llu", *message->flowControlMRR);

    size_t count = (NULL == message->flowControlMRCCount ? 0 : *(message->flowControlMRCCount));
    if (count != 0) {
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
    }
#endif
    free (headTotalDifficulty);
}

/// MARK: PIP Announce

extern BREthereumPIPMessageAnnounce
messagePIPAnnounceDecode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    BREthereumPIPMessageAnnounce message;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (5 == itemsCount || 4 == itemsCount);

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

    return message;
}

/// MARK: PIP Request

extern BRRlpItem
messagePIPRequestInputEncode (BREthereumPIPRequestInput input,
                              BREthereumMessageCoder coder) {
    BRRlpItem item = NULL;
    switch (input.identifier) {
        case PIP_REQUEST_HEADERS:
            item = rlpEncodeList (coder.rlp, 4,
                                  (input.u.headers.useBlockNumber
                                   ? rlpEncodeUInt64 (coder.rlp, input.u.headers.block.number, 1)
                                   : hashRlpEncode (input.u.headers.block.hash, coder.rlp)),
                                  rlpEncodeUInt64 (coder.rlp, input.u.headers.max, 1),
                                  rlpEncodeUInt64 (coder.rlp, input.u.headers.skip, 1),
                                  rlpEncodeUInt64 (coder.rlp, input.u.headers.reverse, 1));
            break;

        case PIP_REQUEST_HEADER_PROOF:
            item = rlpEncodeUInt64 (coder.rlp, input.u.headerProof.blockNumber, 1);
            break;

        case PIP_REQUEST_TRANSACTION_INDEX:
            item = hashRlpEncode (input.u.transactionIndex.transactionHash, coder.rlp);
            break;

        case PIP_REQUEST_BLOCK_RECEIPTS:
            item = hashRlpEncode (input.u.blockReceipt.blockHash, coder.rlp);
            break;

        case PIP_REQUEST_BLOCK_BODY:
            item = hashRlpEncode (input.u.blockBody.blockHash, coder.rlp);
            break;

        case PIP_REQUEST_ACCOUNT:
            item = rlpEncodeList2 (coder.rlp,
                                   hashRlpEncode (input.u.account.blockHash, coder.rlp),
                                   hashRlpEncode (input.u.account.addressHash, coder.rlp));
            break;

        case PIP_REQUEST_STORAGE:
            item = rlpEncodeList (coder.rlp, 3,
                                   hashRlpEncode (input.u.storage.blockHash, coder.rlp),
                                   hashRlpEncode (input.u.storage.addressHash, coder.rlp),
                                  hashRlpEncode (input.u.storage.storageRootHash, coder.rlp));
            break;

        case PIP_REQUEST_CODE:
            item = rlpEncodeList2 (coder.rlp,
                                   hashRlpEncode (input.u.code.blockHash, coder.rlp),
                                   hashRlpEncode (input.u.code.codeHash, coder.rlp));
            break;

        case PIP_REQUEST_EXECUTION:
            assert (0);
            break;
    }
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, input.identifier, 1),
                           rlpEncodeList1 (coder.rlp, item));
}


extern BRRlpItem
messagePIPRequestEncode (BREthereumPIPMessageRequest *request, BREthereumMessageCoder coder) {
    size_t itemsCount = array_count (request->inputs);
    BRRlpItem items[itemsCount];

    for (size_t index = 0; index < itemsCount; index++)
        items[index] = messagePIPRequestInputEncode (request->inputs[index], coder);

    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, request->reqId, 1),
                           rlpEncodeListItems (coder.rlp, items, itemsCount));
}

/// MARK: Response Decode

extern BREthereumPIPRequestOutput
messagePIPRequestOutputDecode (BRRlpItem item,
                               BREthereumMessageCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (2 == itemsCount);

    BREthereumPIPRequestType type = (BREthereumPIPRequestType) rlpDecodeUInt64 (coder.rlp, items[0], 1);
    switch (type) {
        case PIP_REQUEST_HEADERS:
            return (BREthereumPIPRequestOutput) {
                PIP_REQUEST_HEADERS,
                { .headers = { blockOmmersRlpDecode (items[1], coder.network, RLP_TYPE_NETWORK, coder.rlp) } }
            };

        case PIP_REQUEST_HEADER_PROOF:
            assert (0);

        case PIP_REQUEST_TRANSACTION_INDEX: {
            size_t outputsCount;
            const BRRlpItem *outputItems = rlpDecodeList (coder.rlp, items[1], &outputsCount);
            assert (3 == outputsCount);

            return (BREthereumPIPRequestOutput) {
                PIP_REQUEST_TRANSACTION_INDEX,
                { .transactionIndex = {
                    rlpDecodeUInt64 (coder.rlp, outputItems[0], 1),
                    hashRlpDecode(outputItems[1], coder.rlp),
                    rlpDecodeUInt64 (coder.rlp, outputItems[2], 1)
                }}
            };
        }

        case PIP_REQUEST_BLOCK_RECEIPTS:
            return (BREthereumPIPRequestOutput) {
                PIP_REQUEST_BLOCK_RECEIPTS,
                { .blockReceipt = { transactionReceiptDecodeList (items[1], coder.rlp) } }
            };

        case PIP_REQUEST_BLOCK_BODY: {
            size_t outputsCount;
            const BRRlpItem *outputItems = rlpDecodeList (coder.rlp, items[1], &outputsCount);
            assert (2 == outputsCount);

            return (BREthereumPIPRequestOutput) {
                PIP_REQUEST_BLOCK_BODY,
                { .blockBody = {
                    blockOmmersRlpDecode (outputItems[0], coder.network, RLP_TYPE_NETWORK, coder.rlp),
                    blockTransactionsRlpDecode (outputItems[1], coder.network, RLP_TYPE_NETWORK, coder.rlp) }}
            };
        }

        case PIP_REQUEST_ACCOUNT: {
            size_t outputsCount;
            const BRRlpItem *outputItems = rlpDecodeList (coder.rlp, items[1], &outputsCount);
            assert (4 == outputsCount);

            return (BREthereumPIPRequestOutput) {
                PIP_REQUEST_ACCOUNT,
                { .account = {
                    rlpDecodeUInt64 (coder.rlp, outputItems[0], 1),
                    rlpDecodeUInt256 (coder.rlp, outputItems[1], 1),
                    hashRlpDecode (outputItems[2], coder.rlp),
                    hashRlpDecode (outputItems[3], coder.rlp) }}
            };
        }

        case PIP_REQUEST_STORAGE:
            assert (0);

        case PIP_REQUEST_CODE:
            assert (0);

        case PIP_REQUEST_EXECUTION:
            assert (0);
    }
}

extern BREthereumPIPMessageResponse
messagePIPResponseDecode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    assert (3 == itemsCount);

    uint64_t reqId   = rlpDecodeUInt64 (coder.rlp, items[0], 1);
    uint64_t credits = rlpDecodeUInt64 (coder.rlp, items[1], 1);

    size_t outputsCount = 0;
    const BRRlpItem *outputItems = rlpDecodeList (coder.rlp, items[2], &outputsCount);
    BRArrayOf(BREthereumPIPRequestOutput) outputs;
    array_new (outputs, outputsCount);

    for (size_t index = 0; index < outputsCount; index++)
        array_add (outputs, messagePIPRequestOutputDecode (outputItems[index], coder));

    return (BREthereumPIPMessageResponse) {
        reqId,
        credits,
        outputs
    };
}

/// MARK: Update Credit Parameters

extern BREthereumPIPMessageUpdateCreditParameters
messagePIPUpdateCreditParametersDecode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    return (BREthereumPIPMessageUpdateCreditParameters) {};
}

/// MARK: Acknowledge Update

extern BRRlpItem
messagePIPAcknowledgeUpdateEncode (BREthereumPIPMessageAcknowledgeUpdate *message,
                                   BREthereumMessageCoder coder) {
    assert (0);
}

/// MARK: Relay Transaction

extern BRRlpItem
messagePIPRelayTransactionsEncode (BREthereumPIPMessageRelayTransactions *message,
                                   BREthereumMessageCoder coder) {
    size_t itemsCount = array_count (message->transactions);
    BRRlpItem items[itemsCount];

    for (size_t index = 0; index < itemsCount; index++)
        items[index] = transactionRlpEncode (message->transactions[index],
                                             coder.network,
                                             RLP_TYPE_TRANSACTION_SIGNED,
                                             coder.rlp);

    return rlpEncodeListItems (coder.rlp, items, itemsCount);
}


/// MARK:
extern BREthereumPIPMessage
messagePIPDecode (BRRlpItem item,
                  BREthereumMessageCoder coder,
                  BREthereumPIPMessageType type) {
    switch (type) {
        case PIP_MESSAGE_STATUS:
            return (BREthereumPIPMessage) {
                PIP_MESSAGE_STATUS,
                { .status = messagePIPStatusDecode (item, coder) }
            };

        case PIP_MESSAGE_ANNOUNCE:
            return (BREthereumPIPMessage) {
                PIP_MESSAGE_ANNOUNCE,
                { .announce = messagePIPAnnounceDecode (item, coder) }
            };

        case PIP_MESSAGE_REQUEST:
            assert (0);

        case PIP_MESSAGE_RESPONSE:
            return (BREthereumPIPMessage) {
                PIP_MESSAGE_RESPONSE,
                { .response = messagePIPResponseDecode (item, coder) }
            };

        case PIP_MESSAGE_UPDATE_CREDIT_PARAMETERS:
            return (BREthereumPIPMessage) {
                PIP_MESSAGE_UPDATE_CREDIT_PARAMETERS,
                { .updateCreditParameters = messagePIPUpdateCreditParametersDecode (item, coder) }
            };

        case PIP_MESSAGE_ACKNOWLEDGE_UPDATE:
            assert (0);

        case PIP_MESSAGE_RELAY_TRANSACTIONS:
            assert (0);
    }
}

extern BRRlpItem
messagePIPEncode (BREthereumPIPMessage message,
                  BREthereumMessageCoder coder) {
    BRRlpItem body = NULL;
    switch (message.type) {

        case PIP_MESSAGE_STATUS:
            body = messagePIPStatusEncode (&message.u.status, coder);
            break;

        case PIP_MESSAGE_ANNOUNCE:
            assert (0);

        case PIP_MESSAGE_REQUEST:
            body = messagePIPRequestEncode (&message.u.request, coder);
            break;

        case PIP_MESSAGE_RESPONSE:
            assert (0);

        case PIP_MESSAGE_UPDATE_CREDIT_PARAMETERS:
            assert (0);

        case PIP_MESSAGE_ACKNOWLEDGE_UPDATE:
            body = messagePIPAcknowledgeUpdateEncode (&message.u.acknowledgeUpdate, coder);
            break;

        case PIP_MESSAGE_RELAY_TRANSACTIONS:
            body = messagePIPRelayTransactionsEncode (&message.u.relayTransactions, coder);
            break;
    }

    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, message.type + coder.messageIdOffset, 1),
                           body);
}

extern uint64_t
messagePIPGetCredits (const BREthereumPIPMessage *message) {
    switch (message->type) {
        case PIP_MESSAGE_RESPONSE:
            return message->u.response.credits;
        case PIP_MESSAGE_STATUS:
        case PIP_MESSAGE_ANNOUNCE:
        case PIP_MESSAGE_REQUEST:
        case PIP_MESSAGE_UPDATE_CREDIT_PARAMETERS:
        case PIP_MESSAGE_ACKNOWLEDGE_UPDATE:
        case PIP_MESSAGE_RELAY_TRANSACTIONS:
            return 0;
    }
}

extern uint64_t
messagePIPGetCreditsCount (const BREthereumPIPMessage *message) {
    switch (message->type) {
        case PIP_MESSAGE_REQUEST: {
            uint64_t count = 0;
            for (size_t index = 0; index < array_count(message->u.request.inputs); index++) {
                uint64_t incr = 1;
                BREthereumPIPRequestInput *input = &message->u.request.inputs[index];
                switch (input->identifier) {
                    case PIP_REQUEST_HEADERS:
                        incr = input->u.headers.max;
                        break;
                    case PIP_REQUEST_HEADER_PROOF:
                    case PIP_REQUEST_TRANSACTION_INDEX:
                    case PIP_REQUEST_BLOCK_RECEIPTS:
                    case PIP_REQUEST_BLOCK_BODY:
                    case PIP_REQUEST_ACCOUNT:
                    case PIP_REQUEST_STORAGE:
                    case PIP_REQUEST_CODE:
                        incr = 1;
                        break;
                    case PIP_REQUEST_EXECUTION:
                        incr = 1;
                        break;
                }
                count += incr;
            }
            return count;
        }

        case PIP_MESSAGE_STATUS:
        case PIP_MESSAGE_ANNOUNCE:
        case PIP_MESSAGE_RESPONSE:
        case PIP_MESSAGE_UPDATE_CREDIT_PARAMETERS:
        case PIP_MESSAGE_ACKNOWLEDGE_UPDATE:
        case PIP_MESSAGE_RELAY_TRANSACTIONS:
            return 0;

    }
}

extern uint64_t
messagePIPGetRequestId (const BREthereumPIPMessage *message) {
    switch (message->type) {
        case PIP_MESSAGE_STATUS:   return PIP_MESSAGE_NO_REQUEST_ID;
        case PIP_MESSAGE_ANNOUNCE: return PIP_MESSAGE_NO_REQUEST_ID;
        case PIP_MESSAGE_REQUEST:  return message->u.request.reqId;
        case PIP_MESSAGE_RESPONSE: return message->u.response.reqId;
        case PIP_MESSAGE_UPDATE_CREDIT_PARAMETERS: return PIP_MESSAGE_NO_REQUEST_ID;
        case PIP_MESSAGE_ACKNOWLEDGE_UPDATE:       return PIP_MESSAGE_NO_REQUEST_ID;
        case PIP_MESSAGE_RELAY_TRANSACTIONS:       return PIP_MESSAGE_NO_REQUEST_ID;
    }
}
