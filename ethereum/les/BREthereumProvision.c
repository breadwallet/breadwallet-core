//
//  BREthereumProvision.c
//  Core
//
//  Created by Ed Gamble on 9/4/18.
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

#include "BREthereumProvision.h"

static size_t minimum (size_t x, size_t  y) { return x < y ? x : y; }

static BREthereumAccountState
hackFakeAccountStateLESProofs (uint64_t number);

extern BREthereumLESMessageIdentifier
provisionGetMessageLESIdentifier (BREthereumProvisionType type) {
    switch (type) {
        case PROVISION_BLOCK_HEADERS:        return LES_MESSAGE_GET_BLOCK_HEADERS;
        case PROVISION_BLOCK_BODIES:         return LES_MESSAGE_GET_BLOCK_BODIES;
        case PROVISION_TRANSACTION_RECEIPTS: return LES_MESSAGE_GET_RECEIPTS;
        case PROVISION_ACCOUNTS:             return LES_MESSAGE_GET_PROOFS_V2;
        case PROVISION_TRANSACTION_STATUSES: return LES_MESSAGE_GET_TX_STATUS;
        case PROVISION_SUBMIT_TRANSACTION:   return LES_MESSAGE_SEND_TX2;
    }
}

extern BREthereumPIPRequestType
provisionGetMessagePIPIdentifier (BREthereumProvisionType type) {
    switch (type) {
        case PROVISION_BLOCK_HEADERS:        return PIP_REQUEST_HEADERS;
        case PROVISION_BLOCK_BODIES:         return PIP_REQUEST_BLOCK_BODY;
        case PROVISION_TRANSACTION_RECEIPTS: return PIP_REQUEST_BLOCK_RECEIPTS;
        case PROVISION_ACCOUNTS:             return PIP_REQUEST_ACCOUNT;
        case PROVISION_TRANSACTION_STATUSES: return PIP_REQUEST_TRANSACTION_INDEX;
        case PROVISION_SUBMIT_TRANSACTION:   assert (0); return -1;
    }
}

/// MARK: - LES

extern BREthereumMessage
provisionCreateMessageLES (BREthereumProvision *provisionMulti,
                           size_t messageContentLimit,
                           uint64_t messageIdBase,
                           size_t index) {
    uint64_t messageId = messageIdBase + index;
    switch (provisionMulti->type) {
        case PROVISION_BLOCK_HEADERS: {
            BREthereumProvisionHeaders *provision = &provisionMulti->u.headers;

            if (NULL == provision->headers) {
                array_new (provision->headers, provision->limit);
                array_set_count (provision->headers, provision->limit);
            }

            uint64_t start = provision->start + index * messageContentLimit;
            uint64_t count = provision->limit - index * messageContentLimit;

            return (BREthereumMessage) {
                MESSAGE_LES,
                { .les = {
                    LES_MESSAGE_GET_BLOCK_HEADERS,
                    { .getBlockHeaders = {
                        messageId,
                        1, // use 'number'
                        { .number = start },
                        (uint32_t) minimum (count, messageContentLimit),
                        provision->skip,
                        ETHEREUM_BOOLEAN_IS_TRUE (provision->reverse)
                    }}}}
            };
        }

        case PROVISION_BLOCK_BODIES: {
            BREthereumProvisionBodies *provision = &provisionMulti->u.bodies;

            BRArrayOf(BREthereumHash) hashes = provision->hashes;
            size_t hashesCount = array_count(hashes);

            if (NULL == provision->pairs) {
                array_new (provision->pairs, hashesCount);
                array_set_count (provision->pairs, hashesCount);
            }

            BRArrayOf(BREthereumHash) messageHashes;
            array_new(messageHashes, messageContentLimit);

            size_t hashesOffset = index * messageContentLimit;

            for (size_t i = 0; i < minimum (messageContentLimit, hashesCount - hashesOffset); i++)
                array_add (messageHashes, hashes[(hashesOffset + i)]);

            return (BREthereumMessage) {
                MESSAGE_LES,
                { .les = {
                    LES_MESSAGE_GET_BLOCK_BODIES,
                    { .getBlockBodies = { messageId, messageHashes }}}}
            };
        }

        case PROVISION_TRANSACTION_RECEIPTS: {
            BREthereumProvisionReceipts *provision = &provisionMulti->u.receipts;

            BRArrayOf(BREthereumHash) hashes = provision->hashes;
            size_t hashesCount = array_count(hashes);

            if (NULL == provision->receipts) {
                array_new (provision->receipts, hashesCount);
                array_set_count (provision->receipts, hashesCount);
            }

            BRArrayOf(BREthereumHash) messageHashes;
            array_new(messageHashes, messageContentLimit);

            size_t hashesOffset = index * messageContentLimit;

            for (size_t i = 0; i < minimum (messageContentLimit, hashesCount - hashesOffset); i++)
                array_add (messageHashes, hashes[(hashesOffset + i)]);

            return (BREthereumMessage) {
                MESSAGE_LES,
                { .les = {
                    LES_MESSAGE_GET_RECEIPTS,
                    { .getReceipts = { messageId, messageHashes }}}}
            };
        }

        case PROVISION_ACCOUNTS: {
            BREthereumProvisionAccounts *provision = &provisionMulti->u.accounts;

            BREthereumAddress address = provision->address;
            BRArrayOf(BREthereumHash) hashes = provision->hashes;
            BRArrayOf(uint64_t) numbers = provision->numbers;
            size_t hashesCount = array_count(hashes);

            if (NULL == provision->accounts) {
                array_new (provision->accounts, hashesCount);
                array_set_count (provision->accounts, hashesCount);
            }

            BRArrayOf(BREthereumHash) messageHashes;
            array_new(messageHashes, messageContentLimit);

            size_t hashesOffset = index * messageContentLimit;

            BRArrayOf(BREthereumLESMessageGetProofsSpec) specs;
            array_new (specs, hashesCount);

            // HACK
            BREthereumAddress *addr = malloc (sizeof (BREthereumAddress));
            memcpy (addr, &address, sizeof (BREthereumAddress));

            BRRlpData key1 = (BRRlpData) { 0, NULL };
            BRRlpData key2 = (BRRlpData) { sizeof (BREthereumAddress), addr->bytes };

            for (size_t i = 0; i < minimum (messageContentLimit, hashesCount - hashesOffset); i++) {
                BREthereumLESMessageGetProofsSpec spec = {
                    hashes[index],
                    key1,
                    key2,
                    0,
                    numbers[index],  // HACK
                    address
                };
                array_add (specs, spec);
            }

            return (BREthereumMessage) {
                MESSAGE_LES,
                { .les = {
                    LES_MESSAGE_GET_PROOFS_V2,
                    { .getProofsV2 = { messageId, specs }}}}
            };
        }

        case PROVISION_TRANSACTION_STATUSES: {
            BREthereumProvisionStatuses *provision = &provisionMulti->u.statuses;

            BRArrayOf(BREthereumHash) hashes = provision->hashes;
            size_t hashesCount = array_count(hashes);

            if (NULL == provision->statuses) {
                array_new (provision->statuses, hashesCount);
                array_set_count (provision->statuses, hashesCount);
            }

            BRArrayOf(BREthereumHash) messageHashes;
            array_new(messageHashes, messageContentLimit);

            size_t hashesOffset = index * messageContentLimit;

            for (size_t i = 0; i < minimum (messageContentLimit, hashesCount - hashesOffset); i++)
                array_add (messageHashes, hashes[(hashesOffset + i)]);

            return (BREthereumMessage) {
                MESSAGE_LES,
                { .les = {
                    LES_MESSAGE_GET_TX_STATUS,
                    { .getTxStatus = { messageId, messageHashes }}}}
            };
        }

        case PROVISION_SUBMIT_TRANSACTION: {
            BREthereumProvisionSubmission *provision = &provisionMulti->u.submission;

            switch (index) {
                case 0: {
                    BRArrayOf(BREthereumTransaction) transactions;
                    array_new (transactions, 1);
                    array_add (transactions, provision->transaction);

                    return (BREthereumMessage) {
                        MESSAGE_LES,
                        { .les = {
                            LES_MESSAGE_SEND_TX,
                            { .sendTx2 = { messageId, transactions }}}}
                    };
                }
                case 1: {
                    BRArrayOf(BREthereumHash) hashes;
                    array_new (hashes, 1);
                    array_add (hashes, transactionGetHash (provision->transaction));

                    return (BREthereumMessage) {
                        MESSAGE_LES,
                        { .les = {
                            LES_MESSAGE_GET_TX_STATUS,
                            { .getTxStatus = { messageId, hashes }}}}
                    };
                }
                default:
                    assert (0);
            }
        }
    }
}

extern void
provisionHandleMessageLES (BREthereumProvision *provisionMulti,
                           BREthereumLESMessage message,
                           size_t messageContentLimit,
                           uint64_t messageIdBase) {
    switch (provisionMulti->type) {
        case PROVISION_BLOCK_HEADERS: {
            assert (LES_MESSAGE_BLOCK_HEADERS == message.identifier);

            BREthereumProvisionHeaders *provision = &provisionMulti->u.headers;
            BRArrayOf(BREthereumBlockHeader) provisionHeaders = provision->headers;

            BREthereumProvisionIdentifier identifier = messageLESGetRequestId (&message);

            BRArrayOf(BREthereumBlockHeader) messageHeaders  = message.u.blockHeaders.headers;
            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messageHeaders); index++)
                provisionHeaders[offset + index] = messageHeaders[index];
            break;
        }

        case PROVISION_BLOCK_BODIES: {
            assert (LES_MESSAGE_BLOCK_BODIES == message.identifier);

            BREthereumProvisionBodies *provision = &provisionMulti->u.bodies;
            BRArrayOf(BREthereumBlockBodyPair) provisionPairs = provision->pairs;

            BREthereumProvisionIdentifier identifier = messageLESGetRequestId (&message);

            BRArrayOf(BREthereumBlockBodyPair) messagePairs = message.u.blockBodies.pairs;
            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messagePairs); index++)
                provisionPairs[offset + index] = messagePairs[index];
            break;
        }

        case PROVISION_TRANSACTION_RECEIPTS: {
            assert (LES_MESSAGE_RECEIPTS == message.identifier);

            BREthereumProvisionReceipts *provision = &provisionMulti->u.receipts;
            BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) provisionPairs = provision->receipts;

            BREthereumProvisionIdentifier identifier = messageLESGetRequestId (&message);

            BRArrayOf(BREthereumLESMessageReceiptsArray) messagePairs = message.u.receipts.arrays;
            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messagePairs); index++)
                provisionPairs[offset + index] = messagePairs[index].receipts;
            break;
        }

        case PROVISION_ACCOUNTS: {
            assert (LES_MESSAGE_PROOFS_V2 == message.identifier);
            BREthereumProvisionAccounts *provision = &provisionMulti->u.accounts;
            BRArrayOf(BREthereumAccountState) provisionAccounts = provision->accounts;

            BREthereumProvisionIdentifier identifier = messageLESGetRequestId (&message);

            // HACK: This is empty
            BRArrayOf(BREthereumMPTNodePath) messagePaths = message.u.proofsV2.paths;
            size_t offset = messageContentLimit * (identifier - messageIdBase);

            for (size_t index = 0; index <  messageContentLimit; index++) {
                if (offset + index < array_count(provisionAccounts)) {
                    uint64_t number = provision->numbers [offset + index];   // HACK
                    provisionAccounts[offset + index] = hackFakeAccountStateLESProofs(number); // HACK
                }
            }
            break;
        }

        case PROVISION_TRANSACTION_STATUSES: {
            assert (LES_MESSAGE_TX_STATUS == message.identifier);

            BREthereumProvisionStatuses *provision = &provisionMulti->u.statuses;
            BRArrayOf(BREthereumTransactionStatus) provisionPairs = provision->statuses;

            BREthereumProvisionIdentifier identifier = messageLESGetRequestId (&message);

            BRArrayOf(BREthereumTransactionStatus) messagePairs = message.u.txStatus.stati;
            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messagePairs); index++)
                provisionPairs[offset + index] = messagePairs[index];

            break;
        }
        case PROVISION_SUBMIT_TRANSACTION: {
            assert (LES_MESSAGE_TX_STATUS == message.identifier);

            BREthereumProvisionSubmission *provision = &provisionMulti->u.submission;
            provision->status = message.u.txStatus.stati[0];
            break;
          }
    }
}

/// MARK: PIP

extern BREthereumMessage
provisionCreateMessagePIP (BREthereumProvision *provisionMulti,
                           size_t messageContentLimit,
                           uint64_t messageIdBase,
                           size_t index) {
    uint64_t messageId = messageIdBase + index;

    switch (provisionMulti->type) {
        case PROVISION_BLOCK_HEADERS: {
            BREthereumProvisionHeaders *provision = &provisionMulti->u.headers;

            if (NULL == provision->headers) {
                array_new (provision->headers, provision->limit);
                array_set_count(provision->headers, provision->limit);
            }

            uint64_t start = provision->start + index * messageContentLimit;
            uint64_t count = provision->limit - index * messageContentLimit;

            BREthereumPIPRequestInput input = {
                PIP_REQUEST_HEADERS,
                { .headers = {
                    1,
                    { .number = start },
                    provision->skip,
                    (uint32_t) minimum (count, messageContentLimit),
                    provision->reverse }}
            };

            BRArrayOf(BREthereumPIPRequestInput) inputs;
            array_new (inputs, 1);
            array_add (inputs, input);

            return (BREthereumMessage) {
                MESSAGE_PIP,
                { .pip = {
                    PIP_MESSAGE_REQUEST,
                    { .request = { messageId, inputs }}}}
            };
        }

        case PROVISION_BLOCK_BODIES: {
            BREthereumProvisionBodies *provision = &provisionMulti->u.bodies;

            BRArrayOf(BREthereumHash) hashes = provision->hashes;
            size_t hashesCount = array_count(hashes);

            if (NULL == provision->pairs) {
                array_new (provision->pairs, hashesCount);
                array_set_count (provision->pairs, hashesCount);
            }

            BRArrayOf(BREthereumHash) messageHashes;
            array_new(messageHashes, messageContentLimit);

            size_t hashesOffset = index * messageContentLimit;

            BRArrayOf(BREthereumPIPRequestInput) inputs;
            array_new (inputs, messageContentLimit);
            for (size_t i = 0; i < minimum (messageContentLimit, hashesCount - hashesOffset); i++) {
                BREthereumPIPRequestInput input = {
                    PIP_REQUEST_BLOCK_BODY,
                    { .blockBody = { hashes[hashesOffset + i]}}
                };
                array_add (inputs, input);
            }

            return (BREthereumMessage) {
                MESSAGE_PIP,
                { .pip = {
                    PIP_MESSAGE_REQUEST,
                    { .request = { messageId, inputs }}}}
            };
        }
        case PROVISION_TRANSACTION_RECEIPTS: {
            BREthereumProvisionReceipts *provision = &provisionMulti->u.receipts;

            BRArrayOf(BREthereumHash) hashes = provision->hashes;
            size_t hashesCount = array_count(hashes);

            if (NULL == provision->receipts) {
                array_new (provision->receipts, hashesCount);
                array_set_count (provision->receipts, hashesCount);
            }

            size_t hashesOffset = index * messageContentLimit;

            BRArrayOf(BREthereumPIPRequestInput) inputs;
            array_new (inputs, messageContentLimit);
            for (size_t i = 0; i < minimum (messageContentLimit, hashesCount - hashesOffset); i++) {
                BREthereumPIPRequestInput input = {
                    PIP_REQUEST_BLOCK_RECEIPTS,
                    { .blockReceipt = { hashes[hashesOffset + i]}}
                };
                array_add (inputs, input);
            }

            return (BREthereumMessage) {
                MESSAGE_PIP,
                { .pip = {
                    PIP_MESSAGE_REQUEST,
                    { .request = { messageId, inputs }}}}
            };
        }

        case PROVISION_ACCOUNTS: {
            BREthereumProvisionAccounts *provision = &provisionMulti->u.accounts;

            BREthereumHash addressHash = addressGetHash(provision->address);
            BRArrayOf(BREthereumHash) hashes = provision->hashes;
            size_t hashesCount = array_count(hashes);

            if (NULL == provision->accounts) {
                array_new (provision->accounts, hashesCount);
                array_set_count (provision->accounts, hashesCount);
            }

            size_t hashesOffset = index * messageContentLimit;

            BRArrayOf(BREthereumPIPRequestInput) inputs;
            array_new (inputs, messageContentLimit);
            for (size_t i = 0; i < minimum (messageContentLimit, hashesCount - hashesOffset); i++) {
                BREthereumPIPRequestInput input = {
                    PIP_REQUEST_ACCOUNT,
                    { .account = { hashes[hashesOffset + i], addressHash }}
                };
                array_add (inputs, input);
            }

            return (BREthereumMessage) {
                MESSAGE_PIP,
                { .pip = {
                    PIP_MESSAGE_REQUEST,
                    { .request = { messageId, inputs }}}}
            };
        }

        case PROVISION_TRANSACTION_STATUSES: {
            BREthereumProvisionStatuses *provision = &provisionMulti->u.statuses;

            BRArrayOf(BREthereumHash) hashes = provision->hashes;
            size_t hashesCount = array_count(hashes);

            if (NULL == provision->statuses) {
                array_new (provision->statuses, hashesCount);
                array_set_count (provision->statuses, hashesCount);
            }

            size_t hashesOffset = index * messageContentLimit;

            BRArrayOf(BREthereumPIPRequestInput) inputs;
            array_new (inputs, messageContentLimit);
            for (size_t i = 0; i < minimum (messageContentLimit, hashesCount - hashesOffset); i++) {
                BREthereumPIPRequestInput input = {
                    PIP_REQUEST_TRANSACTION_INDEX,
                    { .transactionIndex = { hashes[hashesOffset + i]}}
                };
                array_add (inputs, input);
            }

            return (BREthereumMessage) {
                MESSAGE_PIP,
                { .pip = {
                    PIP_MESSAGE_REQUEST,
                    { .request = { messageId, inputs }}}}
            };
        }
        case PROVISION_SUBMIT_TRANSACTION: {
            BREthereumProvisionSubmission *provision = &provisionMulti->u.submission;

            switch (index) {
                case 0: {
                    BRArrayOf(BREthereumTransaction) transactions;
                    array_new (transactions, 1);
                    array_add (transactions, provision->transaction);

                    return (BREthereumMessage) {
                        MESSAGE_PIP,
                        { .pip = {
                            PIP_MESSAGE_RELAY_TRANSACTIONS,
                            { .relayTransactions = { transactions }}}}
                    };
                }

                case 1: {
                    BRArrayOf(BREthereumPIPRequestInput) inputs;
                    array_new (inputs, 1);

                    BREthereumPIPRequestInput input = {
                        PIP_REQUEST_TRANSACTION_INDEX,
                        { .transactionIndex = { transactionGetHash (provision->transaction) }}
                    };
                    array_add (inputs, input);

                    return (BREthereumMessage) {
                        MESSAGE_PIP,
                        { .pip = {
                            PIP_MESSAGE_REQUEST,
                            { .request = { messageId, inputs }}}}
                    };
                }

                default:
                    assert (0);
            }
        }
    }
}

extern void
provisionHandleMessagePIP (BREthereumProvision *provisionMulti,
                           BREthereumPIPMessage message,
                           size_t messageContentLimit,
                           uint64_t messageIdBase) {
    switch (provisionMulti->type) {
        case PROVISION_BLOCK_HEADERS: {
            assert (PIP_MESSAGE_RESPONSE == message.type);

            BREthereumProvisionHeaders *provision = &provisionMulti->u.headers;
            BRArrayOf(BREthereumBlockHeader) provisionHeaders = provision->headers;

            BREthereumProvisionIdentifier identifier = messagePIPGetRequestId(&message);

            // TODO: Not likely.
            assert (1 == array_count(message.u.response.outputs));
            BREthereumPIPRequestOutput output = message.u.response.outputs[0];

            assert (PIP_REQUEST_HEADERS == output.identifier);
            BRArrayOf(BREthereumBlockHeader) messageHeaders = output.u.headers.headers;
            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messageHeaders); index++)
                provisionHeaders[offset + index] = messageHeaders[index];
            break;
        }

        case PROVISION_BLOCK_BODIES: {
            BREthereumProvisionBodies *provision = &provisionMulti->u.bodies;
            BRArrayOf(BREthereumBlockBodyPair) provisionPairs = provision->pairs;

            BREthereumProvisionIdentifier identifier = messagePIPGetRequestId(&message);

            BRArrayOf(BREthereumPIPRequestOutput) messageOutputs = message.u.response.outputs;
            //assert (array_count(provisionPairs) == array_count(messageOutputs));

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messageOutputs); index++) {
                provisionPairs[offset + index].transactions = messageOutputs[index].u.blockBody.transactions;
                provisionPairs[offset + index].uncles = messageOutputs[index].u.blockBody.headers;
            }
            break;
        }

        case PROVISION_TRANSACTION_RECEIPTS: {
            BREthereumProvisionReceipts *provision = &provisionMulti->u.receipts;
            BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) provisionReceiptsArray = provision->receipts;

            BREthereumProvisionIdentifier identifier = messagePIPGetRequestId(&message);

            BRArrayOf(BREthereumPIPRequestOutput) messageOutputs = message.u.response.outputs;

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messageOutputs); index++)
                provisionReceiptsArray[offset + index]= messageOutputs[index].u.blockReceipt.receipts;
            break;
        }

        case PROVISION_ACCOUNTS: {
            BREthereumProvisionAccounts *provision = &provisionMulti->u.accounts;
            BRArrayOf(BREthereumAccountState) provisionAccounts= provision->accounts;

            BREthereumProvisionIdentifier identifier = messagePIPGetRequestId(&message);

            BRArrayOf(BREthereumPIPRequestOutput) messageOutputs = message.u.response.outputs;

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messageOutputs); index++)
                provisionAccounts[offset + index] =
                accountStateCreate (messageOutputs[index].u.account.nonce,
                                    etherCreate(messageOutputs[index].u.account.balance),
                                    messageOutputs[index].u.account.storageRootHash,
                                    messageOutputs[index].u.account.codeHash);
            break;
        }

        case PROVISION_TRANSACTION_STATUSES: {
            BREthereumProvisionStatuses *provision = &provisionMulti->u.statuses;
            BRArrayOf(BREthereumTransactionStatus) provisionStatuses= provision->statuses;

            BREthereumProvisionIdentifier identifier = messagePIPGetRequestId(&message);

            BRArrayOf(BREthereumPIPRequestOutput) messageOutputs = message.u.response.outputs;

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messageOutputs); index++)
                provisionStatuses[offset + index] =
                transactionStatusCreateIncluded (gasCreate(0),
                                                 messageOutputs[index].u.transactionIndex.blockHash,
                                                 messageOutputs[index].u.transactionIndex.blockNumber,
                                                 messageOutputs[index].u.transactionIndex.transactionIndex);
            break;
        }

        case PROVISION_SUBMIT_TRANSACTION: {
            break;
        }
    }

}

/// MARK: - Create / Handle

extern BREthereumMessage
provisionCreateMessage (BREthereumProvision *provision,
                        BREthereumMessageIdentifier type,
                        size_t messageContentLimit,
                        uint64_t messageIdBase,
                        size_t index) {
    switch (type) {
        case MESSAGE_P2P:
        case MESSAGE_DIS:
        case MESSAGE_ETH:
            assert (0);

        case MESSAGE_LES:
            return provisionCreateMessageLES (provision, messageContentLimit, messageIdBase, index);

        case MESSAGE_PIP:
            return provisionCreateMessagePIP (provision, messageContentLimit, messageIdBase, index);
            break;
    }
}

extern void
provisionHandleMessage (BREthereumProvision *provision,
                        BREthereumMessage message,
                        size_t messageContentLimit,
                        uint64_t messageIdBase) {
    switch (message.identifier) {
        case MESSAGE_P2P:
        case MESSAGE_DIS:
        case MESSAGE_ETH:
            break;

        case MESSAGE_LES:
            provisionHandleMessageLES (provision, message.u.les, messageContentLimit, messageIdBase);
            break;

        case MESSAGE_PIP:
            provisionHandleMessagePIP (provision, message.u.pip, messageContentLimit, messageIdBase);
            break;
    }
}

extern BREthereumBoolean
provisionMatches (BREthereumProvision *provision1,
                  BREthereumProvision *provision2) {
    return AS_ETHEREUM_BOOLEAN (provision1->type == provision2->type &&
                                provision1->identifier == provision2->identifier
                                // bodies?
                                );
}

/// MARK: HACK

struct BlockStateMap {
    uint64_t number;
    BREthereumAccountState state;
};

// Address: 0xa9de3dbD7d561e67527bC1Ecb025c59D53b9F7Ef
static struct BlockStateMap map[] = {
    { 0, { 0 }},
    { 5506602, { 1 }}, // <- ETH, 0xa9d8724bb9db4b5ad5a370201f7367c0f731bfaa2adf1219256c7a40a76c8096
    { 5506764, { 2 }}, // -> KNC, 0xaca2b09703d7816753885fd1a60e65c6426f9d006ba2d8dd97f7c845e0ffa930
    { 5509990, { 3 }}, // -> KNC, 0xe5a045bdd432a8edc345ff830641d1b75847ab5c9d8380241323fa4c9e6cee1e
    { 5511681, { 4 }}, // -> KNC, 0x04d93a1addec69da4a0589bd84d5157a0b47369ce6084c06d66fbd0afc8591dc
    { 5539808, { 5 }}, // -> KNC, 0x932faac9e5bf5cead0492afbe290ff0cd7d2ab5d7b351ad1bccae8aac646522b
    { 5795662, { 6 }}, // -> ETH, 0x1429c28066e3e41073e7abece864e5ca9b0dfcef28bec90a83e6ed04d91997ac
    { 5818087, { 7 }}, // -> ETH, 0xe606358c10f59dfbdb7ad823826881ee3915e06320f1019187af92e96201e7ed
    { 5819543, { 8 }}, // -> ETH, 0x597595bdf79ec29e8a7079fecddd741a40471bbd8fd92e11cdfc0d78d973cb16
    { 6104163, { 9 }}, // -> ETH, 0xe87d76e5a47600f70ee11816ba8d1756b9295eca12487cbe1223a80e3a603d44
    { UINT64_MAX, { 9 }}
};
static size_t mapCount = sizeof (map) / sizeof (struct BlockStateMap);

static BREthereumAccountState
hackFakeAccountStateLESProofs (uint64_t number) {
    for (int i = 0; i < mapCount; i++)
        if (number < map[i].number)
            return map[i - 1].state;
    assert (0);
}

