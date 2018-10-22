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
#include "../../mpt/BREthereumMPT.h"

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
messagePIPGetIdentifierName (BREthereumPIPMessage message) {
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

    switch (message.type) {
        case PIP_MESSAGE_STATUS:
        case PIP_MESSAGE_ANNOUNCE:
        case PIP_MESSAGE_UPDATE_CREDIT_PARAMETERS:
        case PIP_MESSAGE_ACKNOWLEDGE_UPDATE:
        case PIP_MESSAGE_RELAY_TRANSACTIONS:
            return messagePIPNames[message.type];

        case PIP_MESSAGE_REQUEST: {
            BREthereumPIPMessageRequest *request = &message.u.request;
            return (array_count (request->inputs) > 0
                    ? messagePIPGetRequestName(request->inputs[0].identifier)
                    : messagePIPNames[message.type]);
        }
        case PIP_MESSAGE_RESPONSE: {
            BREthereumPIPMessageResponse *response = &message.u.response;
            return (array_count(response->outputs) > 0
                    ? messagePIPGetRequestName(response->outputs[0].identifier)
                    : messagePIPNames[message.type]);
        }
    }
}

/// MARK: PIP Status

extern void
messagePIPStatusShow(BREthereumPIPMessageStatus *status) {
    messageP2PStatusShow(&status->p2p);
}

extern BRRlpItem
messagePIPStatusEncode (BREthereumPIPMessageStatus *status,
                        BREthereumMessageCoder coder) {
    return messageP2PStatusEncode(&status->p2p, coder);
}

extern BREthereumPIPMessageStatus
messagePIPStatusDecode (BRRlpItem item,
                        BREthereumMessageCoder coder) {
    return (BREthereumPIPMessageStatus) {
        messageP2PStatusDecode(item, coder, NULL)
    };
}

/// MARK: PIP Announce

extern BREthereumPIPMessageAnnounce
messagePIPAnnounceDecode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    BREthereumPIPMessageAnnounce message = {};

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    if (5 != itemsCount && 4 != itemsCount) rlpCoderSetFailed (coder.rlp);
    else {
        message.headHash = hashRlpDecode (items[0], coder.rlp);
        message.headNumber = rlpDecodeUInt64 (coder.rlp, items[1], 1);
        message.headTotalDifficulty = rlpDecodeUInt256 (coder.rlp, items[2], 1);
        message.reorgDepth = rlpDecodeUInt64 (coder.rlp, items[3], 1);

        // TODO: Decode Keys
//        if (5 == itemsCount) {
//            size_t pairCount = 0;
//            const BRRlpItem *pairItems = rlpDecodeList (coder.rlp, items[4], &pairCount);
//            array_new(message.pairs, pairCount);
//            for (size_t index = 0; index < pairCount; index++)
//                ;
//        }
    }

    return message;
}

/// MARK: PIP Request

static BRRlpItem
loose (BRRlpCoder coder, int discriminant, BRRlpItem item) {
    return rlpEncodeList2 (coder,
                           rlpEncodeUInt64(coder, discriminant, 1),
                           item);
}

extern BRRlpItem
messagePIPRequestInputEncode (BREthereumPIPRequestInput input,
                              BREthereumMessageCoder coder) {
    BRRlpItem item = NULL;
    switch (input.identifier) {
        case PIP_REQUEST_HEADERS:
            item = rlpEncodeList (coder.rlp, 4,
                                  loose(coder.rlp, 0,
                                        (input.u.headers.useBlockNumber
                                         ? rlpEncodeUInt64 (coder.rlp, input.u.headers.block.number, 1)
                                         : hashRlpEncode (input.u.headers.block.hash, coder.rlp))),
                                  rlpEncodeUInt64 (coder.rlp, input.u.headers.skip, 1),
                                  rlpEncodeUInt64 (coder.rlp, input.u.headers.max, 1),
                                  rlpEncodeUInt64 (coder.rlp, ETHEREUM_BOOLEAN_IS_TRUE (input.u.headers.reverse), 1));
            break;
            
        case PIP_REQUEST_HEADER_PROOF:
            item = rlpEncodeList1 (coder.rlp,
                                   loose (coder.rlp, 0, rlpEncodeUInt64 (coder.rlp, input.u.headerProof.blockNumber, 1)));
            break;
            
        case PIP_REQUEST_TRANSACTION_INDEX:
            item = rlpEncodeList1 (coder.rlp,
                                   loose (coder.rlp, 0, hashRlpEncode (input.u.transactionIndex.transactionHash, coder.rlp)));
            break;
            
        case PIP_REQUEST_BLOCK_RECEIPTS:
            item = rlpEncodeList1 (coder.rlp,
                                   loose (coder.rlp, 0, hashRlpEncode (input.u.blockReceipt.blockHash, coder.rlp)));
            break;
            
        case PIP_REQUEST_BLOCK_BODY:
            item = rlpEncodeList1 (coder.rlp,
                                   loose (coder.rlp, 0, hashRlpEncode (input.u.blockBody.blockHash, coder.rlp)));
            break;
            
        case PIP_REQUEST_ACCOUNT:
            item = rlpEncodeList2 (coder.rlp,
                                   loose (coder.rlp, 0, hashRlpEncode (input.u.account.blockHash, coder.rlp)),
                                   loose (coder.rlp, 0, hashRlpEncode (input.u.account.addressHash, coder.rlp)));
            break;
            
        case PIP_REQUEST_STORAGE:
            item = rlpEncodeList (coder.rlp, 3,
                                  loose (coder.rlp, 0, hashRlpEncode (input.u.storage.blockHash, coder.rlp)),
                                  loose (coder.rlp, 0, hashRlpEncode (input.u.storage.addressHash, coder.rlp)),
                                  loose (coder.rlp, 0, hashRlpEncode (input.u.storage.storageRootHash, coder.rlp)));
            break;
            
        case PIP_REQUEST_CODE:
            item = rlpEncodeList2 (coder.rlp,
                                   loose (coder.rlp, 0, hashRlpEncode (input.u.code.blockHash, coder.rlp)),
                                   loose (coder.rlp, 0, hashRlpEncode (input.u.code.codeHash, coder.rlp)));
            break;
            
        case PIP_REQUEST_EXECUTION:
            assert (0);
            break;
    }
    return rlpEncodeList2 (coder.rlp,
                           rlpEncodeUInt64 (coder.rlp, input.identifier, 1),
                           item);
}

static void
messagePIPRequestInputRelease (BREthereumPIPRequestInput *input) {
    switch (input->identifier) {
        case PIP_REQUEST_HEADERS:
            break;

        case PIP_REQUEST_HEADER_PROOF:
            break;

        case PIP_REQUEST_TRANSACTION_INDEX:
            break;

        case PIP_REQUEST_BLOCK_RECEIPTS:
            break;

        case PIP_REQUEST_BLOCK_BODY:
            break;

        case PIP_REQUEST_ACCOUNT:
            break;

        case PIP_REQUEST_STORAGE:
            break;

        case PIP_REQUEST_CODE:
            break;

        case PIP_REQUEST_EXECUTION:
            if (NULL != input->u.execution.callData)
                array_free (input->u.execution.callData);
            break;
    }
}

static void
messagePIPRequestInputsRelease (BRArrayOf(BREthereumPIPRequestInput) inputs) {
    if (NULL != inputs) {
        size_t count = array_count(inputs);
        for (size_t index = 0; index < count; index++)
            messagePIPRequestInputRelease (&inputs[index]);
        array_free (inputs);
    }
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

///
/// MARK: Response
///

extern BREthereumPIPRequestOutput
messagePIPRequestOutputDecode (BRRlpItem item,
                               BREthereumMessageCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    if (2 != itemsCount) { rlpCoderSetFailed (coder.rlp); return (BREthereumPIPRequestOutput) {}; }

    BREthereumPIPRequestType type = (BREthereumPIPRequestType) rlpDecodeUInt64 (coder.rlp, items[0], 1);
    switch (type) {
        case PIP_REQUEST_HEADERS:
            return (BREthereumPIPRequestOutput) {
                PIP_REQUEST_HEADERS,
                { .headers = { blockOmmersRlpDecode (items[1], coder.network, RLP_TYPE_NETWORK, coder.rlp) } }
            };

        case PIP_REQUEST_HEADER_PROOF: {
            size_t outputsCount;
            const BRRlpItem *outputItems = rlpDecodeList (coder.rlp, items[1], &outputsCount);
            if (3 != outputsCount) { rlpCoderSetFailed (coder.rlp); return (BREthereumPIPRequestOutput) {}; }

            // The MerkleProof, in outputItems[0] is an RLP list of bytes w/ the bytes being
            // individual RLP encodings of a MPT (w/ leafs, extensions, and branches).
            BREthereumMPTNodePath path = mptNodePathDecodeFromBytes (outputItems[0], coder.rlp);

            return (BREthereumPIPRequestOutput) {
                PIP_REQUEST_HEADER_PROOF,
                { .headerProof = {
                    // After all the above... we don't use the `path`
                    hashRlpDecode(outputItems[1], coder.rlp),
                    rlpDecodeUInt256 (coder.rlp, outputItems[2], 1)
                }}
            };
        }

        case PIP_REQUEST_TRANSACTION_INDEX: {
            size_t outputsCount;
            const BRRlpItem *outputItems = rlpDecodeList (coder.rlp, items[1], &outputsCount);
            if (3 != outputsCount) { rlpCoderSetFailed (coder.rlp); return (BREthereumPIPRequestOutput) {}; }

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
            if (2 != outputsCount)  { rlpCoderSetFailed (coder.rlp); return (BREthereumPIPRequestOutput) {}; }

            return (BREthereumPIPRequestOutput) {
                PIP_REQUEST_BLOCK_BODY,
                { .blockBody = {
                    blockOmmersRlpDecode (outputItems[1], coder.network, RLP_TYPE_NETWORK, coder.rlp),
                    blockTransactionsRlpDecode (outputItems[0], coder.network, RLP_TYPE_NETWORK, coder.rlp) }}
            };
        }

        case PIP_REQUEST_ACCOUNT: {
            size_t outputsCount;
            const BRRlpItem *outputItems = rlpDecodeList (coder.rlp, items[1], &outputsCount);
            if (5 != outputsCount)  { rlpCoderSetFailed (coder.rlp); return (BREthereumPIPRequestOutput) {}; }

            // TODO: Proof - outputItems[0]
            return (BREthereumPIPRequestOutput) {
                PIP_REQUEST_ACCOUNT,
                { .account = {
                    rlpDecodeUInt64 (coder.rlp, outputItems[1], 1),
                    rlpDecodeUInt256 (coder.rlp, outputItems[2], 1),
                    hashRlpDecode (outputItems[3], coder.rlp),
                    hashRlpDecode (outputItems[4], coder.rlp) }}
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

static void
messagePIPRequestOutputRelease (BREthereumPIPRequestOutput *output) {
    switch (output->identifier) {
        case PIP_REQUEST_HEADERS:
            blockHeadersRelease (output->u.headers.headers);
            break;

        case PIP_REQUEST_HEADER_PROOF:
            break;

        case PIP_REQUEST_TRANSACTION_INDEX:
            break;

        case PIP_REQUEST_BLOCK_RECEIPTS:
            transactionReceiptsRelease (output->u.blockReceipt.receipts);
            break;

        case PIP_REQUEST_BLOCK_BODY:
            blockHeadersRelease (output->u.blockBody.headers);
            transactionsRelease (output->u.blockBody.transactions);
            break;

        case PIP_REQUEST_ACCOUNT:
            break;

        case PIP_REQUEST_STORAGE:
            break;

        case PIP_REQUEST_CODE:
            if (NULL != output->u.code.bytecode)
                array_free (output->u.code.bytecode);
            break;

        case PIP_REQUEST_EXECUTION:
            break;
    }
}

static void
messagePIPRequestOutputsRelease (BRArrayOf(BREthereumPIPRequestOutput) outputs) {
    if (NULL != outputs) {
        size_t count = array_count(outputs);
        for (size_t index = 0; index < count; index++)
            messagePIPRequestOutputRelease (&outputs[index]);
        array_free (outputs);
    }
}

extern BREthereumPIPMessageResponse
messagePIPResponseDecode (BRRlpItem item,
                          BREthereumMessageCoder coder) {
    BREthereumPIPMessageResponse message = {};

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder.rlp, item, &itemsCount);
    if (3 != itemsCount) rlpCoderSetFailed (coder.rlp);
    else {
        message.reqId   = rlpDecodeUInt64 (coder.rlp, items[0], 1);
        message.credits = rlpDecodeUInt64 (coder.rlp, items[1], 1);

        size_t outputsCount = 0;
        const BRRlpItem *outputItems = rlpDecodeList (coder.rlp, items[2], &outputsCount);
        BRArrayOf(BREthereumPIPRequestOutput) outputs;
        array_new (outputs, outputsCount);

        for (size_t index = 0; index < outputsCount; index++)
            array_add (outputs, messagePIPRequestOutputDecode (outputItems[index], coder));

        message.outputs = outputs;
    }

    return message;
}
extern void
messagePIPResponseConsume (BREthereumPIPMessageResponse *message,
                           BRArrayOf(BREthereumPIPRequestOutput) *outputs) {
    if (NULL != outputs) { *outputs = message->outputs; message->outputs = NULL; }
}

extern void // special case - only 'output' with allocated memory.
messagePIPRequestHeadersOutputConsume (BREthereumPIPRequestHeadersOutput *output,
                                       BRArrayOf(BREthereumBlockHeader) *headers) {
    if (NULL != headers) { *headers = output->headers; output->headers = NULL; }
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
    return rlpEncodeList (coder.rlp, 0);
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

extern void
messagePIPRelease (BREthereumPIPMessage *message) {
    switch (message->type) {
        case PIP_MESSAGE_STATUS:
            messageP2PStatusRelease (&message->u.status.p2p);
            break;

        case PIP_MESSAGE_ANNOUNCE:
            if (NULL != message->u.announce.pairs)
                array_free (message->u.announce.pairs);
            break;

        case PIP_MESSAGE_REQUEST:
            messagePIPRequestInputsRelease (message->u.request.inputs);
            break;

        case PIP_MESSAGE_RESPONSE:
            messagePIPRequestOutputsRelease (message->u.response.outputs);
            break;

        case PIP_MESSAGE_UPDATE_CREDIT_PARAMETERS:
            break;

        case PIP_MESSAGE_ACKNOWLEDGE_UPDATE:
            break;

        case PIP_MESSAGE_RELAY_TRANSACTIONS:
            transactionsRelease (message->u.relayTransactions.transactions);
            break;
    }
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
