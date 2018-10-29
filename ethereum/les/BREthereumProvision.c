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

extern BREthereumLESMessageIdentifier
provisionGetMessageLESIdentifier (BREthereumProvisionType type) {
    switch (type) {
        case PROVISION_BLOCK_HEADERS:        return LES_MESSAGE_GET_BLOCK_HEADERS;
        case PROVISION_BLOCK_PROOFS:         return -1;
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
        case PROVISION_BLOCK_PROOFS:         return PIP_REQUEST_HEADER_PROOF;
        case PROVISION_BLOCK_BODIES:         return PIP_REQUEST_BLOCK_BODY;
        case PROVISION_TRANSACTION_RECEIPTS: return PIP_REQUEST_BLOCK_RECEIPTS;
        case PROVISION_ACCOUNTS:             return PIP_REQUEST_ACCOUNT;
        case PROVISION_TRANSACTION_STATUSES: return PIP_REQUEST_TRANSACTION_INDEX;
        case PROVISION_SUBMIT_TRANSACTION:   assert (0); return -1;
    }
}

/// MARK: - LES

static BREthereumMessage
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

        case PROVISION_BLOCK_PROOFS: {
            assert (0);
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
            size_t hashesCount = array_count(hashes);

            if (NULL == provision->accounts) {
                array_new (provision->accounts, hashesCount);
                array_set_count (provision->accounts, hashesCount);
            }

            size_t hashesOffset = index * messageContentLimit;

            BRArrayOf(BREthereumLESMessageGetProofsSpec) specs;
            array_new (specs, hashesCount);

            for (size_t i = 0; i < minimum (messageContentLimit, hashesCount - hashesOffset); i++) {
                BREthereumLESMessageGetProofsSpec spec = {
                    hashes[index],
                    address,
                    0,
                };
                array_add (specs, spec);
            }

            return (BREthereumMessage) {
                MESSAGE_LES,
                { .les = {
                    LES_MESSAGE_GET_PROOFS,
                    { .getProofs = { messageId, specs }}}}
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

static void
provisionHandleMessageLES (BREthereumProvision *provisionMulti,
                           OwnershipGiven BREthereumLESMessage message,
                           size_t messageContentLimit,
                           uint64_t messageIdBase) {
    switch (provisionMulti->type) {
        case PROVISION_BLOCK_HEADERS: {
            assert (LES_MESSAGE_BLOCK_HEADERS == message.identifier);

            BREthereumProvisionHeaders *provision = &provisionMulti->u.headers;
            BRArrayOf(BREthereumBlockHeader) provisionHeaders = provision->headers;

            BREthereumProvisionIdentifier identifier = messageLESGetRequestId (&message);

            BRArrayOf(BREthereumBlockHeader) messageHeaders;
            messageLESBlockHeadersConsume(&message.u.blockHeaders, &messageHeaders);

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messageHeaders); index++)
                provisionHeaders[offset + index] = messageHeaders[index];

            array_free (messageHeaders);
            break;
        }

        case PROVISION_BLOCK_PROOFS: {
            assert (0);
        }

        case PROVISION_BLOCK_BODIES: {
            assert (LES_MESSAGE_BLOCK_BODIES == message.identifier);

            BREthereumProvisionBodies *provision = &provisionMulti->u.bodies;
            BRArrayOf(BREthereumBlockBodyPair) provisionPairs = provision->pairs;

            BREthereumProvisionIdentifier identifier = messageLESGetRequestId (&message);

            BRArrayOf(BREthereumBlockBodyPair) messagePairs;
            messageLESBlockBodiesConsume (&message.u.blockBodies, &messagePairs);

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messagePairs); index++)
                provisionPairs[offset + index] = messagePairs[index];

            array_free (messagePairs);
            break;
        }

        case PROVISION_TRANSACTION_RECEIPTS: {
            assert (LES_MESSAGE_RECEIPTS == message.identifier);

            BREthereumProvisionReceipts *provision = &provisionMulti->u.receipts;
            BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) provisionPairs = provision->receipts;

            BREthereumProvisionIdentifier identifier = messageLESGetRequestId (&message);

            BRArrayOf(BREthereumLESMessageReceiptsArray) messagePairs;
            messageLESReceiptsConsume (&message.u.receipts, &messagePairs);

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messagePairs); index++)
                provisionPairs[offset + index] = messagePairs[index].receipts;

            array_free (messagePairs);
            break;
        }

        case PROVISION_ACCOUNTS: {
            assert (LES_MESSAGE_PROOFS == message.identifier);
            BREthereumProvisionAccounts *provision = &provisionMulti->u.accounts;
            BREthereumHash hash = addressGetHash(provision->address);
            BREthereumData key  = { sizeof(BREthereumHash), hash.bytes };

            // We'll fill this - at the proper index if a multiple provision.
            BRArrayOf(BREthereumAccountState) provisionAccounts = provision->accounts;

            BREthereumProvisionIdentifier identifier = messageLESGetRequestId (&message);

            BRArrayOf(BREthereumMPTNodePath) messagePaths;
            messageLESProofsConsume(&message.u.proofs, &messagePaths);

            // We need a coder to RLP decode the proof's RLP data into an AccountState.  We could,
            // and probably should, pass the coder for LES all the way down here.  It is a long
            // way down... so we'll create one.  In fact, this is insufficient, because the LES
            // coder has network and perhaps other context - although that is not needed here.
            //
            // We could add a coder to the BREthereumProvisionAccounts... yes, probably should.
            BRRlpCoder coder = rlpCoderCreate();

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messagePaths); index++) {
                // We expect, require, one path for each index.  A common 'GetProofs' error
                // is be have an empty array for messagePaths - that is, no proofs and no
                // non-proofs.  That is surely an error (boot the node), but...
                BREthereumMPTNodePath path = messagePaths[index];
                BREthereumBoolean foundValue = ETHEREUM_BOOLEAN_FALSE;
                BRRlpData data = mptNodePathGetValue (path, key, &foundValue);
                if (ETHEREUM_BOOLEAN_IS_TRUE(foundValue)) {
                    BRRlpItem item = rlpGetItem (coder, data);
                    provisionAccounts[offset + index] = accountStateRlpDecode (item, coder);
                    rlpReleaseItem (coder, item);
                }
                else provisionAccounts[offset + index] = accountStateCreateEmpty();
                rlpDataRelease(data);
            }
            rlpCoderRelease(coder);

            mptNodePathsRelease(messagePaths);
            break;
        }

        case PROVISION_TRANSACTION_STATUSES: {
            assert (LES_MESSAGE_TX_STATUS == message.identifier);

            BREthereumProvisionStatuses *provision = &provisionMulti->u.statuses;
            BRArrayOf(BREthereumTransactionStatus) provisionPairs = provision->statuses;

            BREthereumProvisionIdentifier identifier = messageLESGetRequestId (&message);

            BRArrayOf(BREthereumTransactionStatus) messagePairs = message.u.txStatus.stati;
            messageLESTxStatusConsume (&message.u.txStatus, &messagePairs);

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messagePairs); index++)
                provisionPairs[offset + index] = messagePairs[index];

            array_free (messagePairs);
            break;
        }
        case PROVISION_SUBMIT_TRANSACTION: {
            assert (LES_MESSAGE_TX_STATUS == message.identifier);

            BREthereumProvisionSubmission *provision = &provisionMulti->u.submission;
            provision->status = message.u.txStatus.stati[0];
            break;
          }
    }
    messageLESRelease(&message);
}

/// MARK: PIP

static BREthereumMessage
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

        case PROVISION_BLOCK_PROOFS: {
            BREthereumProvisionProofs *provision = &provisionMulti->u.proofs;

            BRArrayOf(uint64_t) numbers = provision->numbers;
            size_t numbersCount = array_count(numbers);

            if (NULL == provision->proofs) {
                array_new (provision->proofs, numbersCount);
                array_set_count (provision->proofs, numbersCount);
            }

            size_t numbersOffset = index * messageContentLimit;

            BRArrayOf(BREthereumPIPRequestInput) inputs;
            array_new (inputs, messageContentLimit);

            for (size_t i = 0; i < minimum (messageContentLimit, numbersCount - numbersOffset); i++) {
                BREthereumPIPRequestInput input = {
                    PIP_REQUEST_HEADER_PROOF,
                    { .headerProof = { numbers[numbersOffset + i]}}
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

        case PROVISION_BLOCK_BODIES: {
            BREthereumProvisionBodies *provision = &provisionMulti->u.bodies;

            BRArrayOf(BREthereumHash) hashes = provision->hashes;
            size_t hashesCount = array_count(hashes);

            if (NULL == provision->pairs) {
                array_new (provision->pairs, hashesCount);
                array_set_count (provision->pairs, hashesCount);
            }

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

            // We have two messages to submit a transaction, but only one response.  The response
            // needs the proper messageIdentifier in order to be pair with this provision.
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

static void
provisionHandleMessagePIP (BREthereumProvision *provisionMulti,
                           OwnershipGiven BREthereumPIPMessage message,
                           size_t messageContentLimit,
                           uint64_t messageIdBase) {
    switch (provisionMulti->type) {
        case PROVISION_BLOCK_HEADERS: {
            assert (PIP_MESSAGE_RESPONSE == message.type);

            BREthereumProvisionHeaders *provision = &provisionMulti->u.headers;
            BRArrayOf(BREthereumBlockHeader) provisionHeaders = provision->headers;

            BREthereumProvisionIdentifier identifier = messagePIPGetRequestId(&message);

            BRArrayOf(BREthereumPIPRequestOutput) outputs;
            messagePIPResponseConsume(&message.u.response, &outputs);

            // TODO: Not likely.
            assert (1 == array_count(outputs));
            BREthereumPIPRequestOutput output = outputs[0];

            assert (PIP_REQUEST_HEADERS == output.identifier);
            BRArrayOf(BREthereumBlockHeader) messageHeaders = output.u.headers.headers;
            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(messageHeaders); index++)
                provisionHeaders[offset + index] = messageHeaders[index];

            array_free (messageHeaders);
            array_free (outputs);
            break;
        }

        case PROVISION_BLOCK_PROOFS: {
            BREthereumProvisionProofs *provision = &provisionMulti->u.proofs;
            BRArrayOf(BREthereumBlockHeaderProof) provisionProofs = provision->proofs;

            BREthereumProvisionIdentifier identifier = messagePIPGetRequestId(&message);

            BRArrayOf(BREthereumPIPRequestOutput) outputs = NULL;
            messagePIPResponseConsume (&message.u.response, &outputs);

            BRRlpCoder coder = rlpCoderCreate();

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(outputs); index++) {
                assert (PIP_REQUEST_HEADER_PROOF == outputs[index].identifier);

                // The MPT 'key' is the RLP encoding of the block number
                BRRlpItem item = rlpEncodeUInt64(coder, provision->numbers[index], 0);
                BRRlpData data = rlpGetDataSharedDontRelease (coder, item);
                BREthereumData key = { data.bytesCount, data.bytes };

                BREthereumMPTNodePath path; // = outputs[index].u.headerProof.path;

                messagePIPRequestHeaderProofOutputConsume (&outputs[index].u.headerProof, &path);

                if (ETHEREUM_BOOLEAN_IS_TRUE (mptNodePathIsValid (path, key))) {
                    provisionProofs[offset + index].hash = outputs[index].u.headerProof.blockHash;
                    provisionProofs[offset + index].totalDifficulty = outputs[index].u.headerProof.blockTotalDifficulty;
                }
                else {
                    provisionProofs[offset + index].hash = (BREthereumHash) EMPTY_HASH_INIT;
                    provisionProofs[offset + index].totalDifficulty = UINT256_ZERO;
                }

                rlpReleaseItem (coder, item);
                mptNodePathRelease (path);
            }
            array_free (outputs);
            rlpCoderRelease(coder);
            break;
        }

        case PROVISION_BLOCK_BODIES: {
            BREthereumProvisionBodies *provision = &provisionMulti->u.bodies;
            BRArrayOf(BREthereumBlockBodyPair) provisionPairs = provision->pairs;

            BREthereumProvisionIdentifier identifier = messagePIPGetRequestId(&message);

            BRArrayOf(BREthereumPIPRequestOutput) outputs = NULL;
            messagePIPResponseConsume(&message.u.response, &outputs);

            //assert (array_count(provisionPairs) == array_count(messageOutputs));

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(outputs); index++) {
                assert (PIP_REQUEST_BLOCK_BODY == outputs[index].identifier);
                // This 'consumes' outputs[index] by taking {transactions, headers}
                provisionPairs[offset + index].transactions = outputs[index].u.blockBody.transactions;
                provisionPairs[offset + index].uncles = outputs[index].u.blockBody.headers;
            }

            array_free (outputs);
            break;
        }

        case PROVISION_TRANSACTION_RECEIPTS: {
            BREthereumProvisionReceipts *provision = &provisionMulti->u.receipts;
            BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) provisionReceiptsArray = provision->receipts;

            BREthereumProvisionIdentifier identifier = messagePIPGetRequestId(&message);

            BRArrayOf(BREthereumPIPRequestOutput) outputs = NULL;
            messagePIPResponseConsume(&message.u.response, &outputs);

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(outputs); index++) {
                assert (PIP_REQUEST_BLOCK_RECEIPTS == outputs[index].identifier);
                provisionReceiptsArray[offset + index] = outputs[index].u.blockReceipt.receipts;
            }

            array_free (outputs);
            break;
        }

        case PROVISION_ACCOUNTS: {
            BREthereumProvisionAccounts *provision = &provisionMulti->u.accounts;
            BRArrayOf(BREthereumAccountState) provisionAccounts= provision->accounts;

            BREthereumProvisionIdentifier identifier = messagePIPGetRequestId(&message);

            BRArrayOf(BREthereumPIPRequestOutput) outputs = NULL;
            messagePIPResponseConsume(&message.u.response, &outputs);

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(outputs); index++) {
                assert (PIP_REQUEST_ACCOUNT == outputs[index].identifier);
                provisionAccounts[offset + index] =
                accountStateCreate (outputs[index].u.account.nonce,
                                    etherCreate(outputs[index].u.account.balance),
                                    outputs[index].u.account.storageRootHash,
                                    outputs[index].u.account.codeHash);
            }

            array_free (outputs);
            break;
        }

        case PROVISION_TRANSACTION_STATUSES: {
            BREthereumProvisionStatuses *provision = &provisionMulti->u.statuses;
            BRArrayOf(BREthereumTransactionStatus) provisionStatuses= provision->statuses;

            BREthereumProvisionIdentifier identifier = messagePIPGetRequestId(&message);

            BRArrayOf(BREthereumPIPRequestOutput) outputs = NULL;
            messagePIPResponseConsume(&message.u.response, &outputs);

            size_t offset = messageContentLimit * (identifier - messageIdBase);
            for (size_t index = 0; index < array_count(outputs); index++) {
                assert (PIP_REQUEST_TRANSACTION_INDEX == outputs[index].identifier);
               provisionStatuses[offset + index] =
                transactionStatusCreateIncluded (gasCreate(0),
                                                 outputs[index].u.transactionIndex.blockHash,
                                                 outputs[index].u.transactionIndex.blockNumber,
                                                 outputs[index].u.transactionIndex.transactionIndex);
            }

            array_free (outputs);
            break;
        }

        case PROVISION_SUBMIT_TRANSACTION: {
            BREthereumProvisionSubmission *provision = &provisionMulti->u.submission;

            BRArrayOf(BREthereumPIPRequestOutput) outputs = NULL;
            messagePIPResponseConsume(&message.u.response, &outputs);

            switch (array_count(outputs)) {
                case 0:
                    // TODO: probably 'unknown'
                    provision->status = transactionStatusCreate (TRANSACTION_STATUS_QUEUED);
                    break;

                case 1:
                    provision->status =
                    transactionStatusCreateIncluded (gasCreate(0),
                                                     outputs[0].u.transactionIndex.blockHash,
                                                     outputs[0].u.transactionIndex.blockNumber,
                                                     outputs[0].u.transactionIndex.transactionIndex);
                default:
                    assert(0);
            }

            array_free (outputs);
            break;
        }
    }
    messagePIPRelease(&message);
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
provisionRelease (BREthereumProvision *provision,
                  BREthereumBoolean releaseResults) {
    switch (provision->type) {

        case PROVISION_BLOCK_HEADERS:
            if (ETHEREUM_BOOLEAN_IS_TRUE(releaseResults) && NULL != provision->u.headers.headers)
                // Sometimes the headers will be NULL - because we preallocated the response.
                blockHeadersRelease(provision->u.headers.headers);
            break;

        case PROVISION_BLOCK_PROOFS:
            if (NULL != provision->u.proofs.numbers)
                array_free (provision->u.proofs.numbers);
            if (ETHEREUM_BOOLEAN_IS_TRUE(releaseResults) && NULL != provision->u.proofs.proofs)
                array_free (provision->u.proofs.proofs);
            break;
            
        case PROVISION_BLOCK_BODIES:
            if (NULL != provision->u.bodies.hashes)
                array_free (provision->u.bodies.hashes);
            if (ETHEREUM_BOOLEAN_IS_TRUE(releaseResults) && NULL != provision->u.bodies.pairs)
                blockBodyPairsRelease(provision->u.bodies.pairs);
            break;

        case PROVISION_TRANSACTION_RECEIPTS:
            if (NULL != provision->u.receipts.hashes)
                array_free (provision->u.receipts.hashes);
            if (ETHEREUM_BOOLEAN_IS_TRUE(releaseResults) && NULL != provision->u.receipts.receipts) {
                size_t count = array_count(provision->u.receipts.receipts);
                for (size_t index = 0; index < count; index++)
                    transactionReceiptsRelease (provision->u.receipts.receipts[index]);
                array_free (provision->u.receipts.receipts);
            }
            break;

        case PROVISION_ACCOUNTS:
            if (NULL != provision->u.accounts.hashes)
                array_free (provision->u.accounts.hashes);
            if (ETHEREUM_BOOLEAN_IS_TRUE(releaseResults) && NULL != provision->u.accounts.accounts)
                array_free (provision->u.accounts.accounts);
            break;

        case PROVISION_TRANSACTION_STATUSES:
            if (NULL != provision->u.statuses.hashes)
                array_free (provision->u.statuses.hashes);
            if (ETHEREUM_BOOLEAN_IS_TRUE(releaseResults) && NULL != provision->u.statuses.statuses)
                array_free (provision->u.statuses.statuses);
            break;

        case PROVISION_SUBMIT_TRANSACTION:
            if (NULL != provision->u.submission.transaction)
                transactionRelease (provision->u.submission.transaction);
            break;
    }
}

extern void
provisionHeadersConsume (BREthereumProvisionHeaders *provision,
                          BRArrayOf(BREthereumBlockHeader) *headers) {
    if (NULL != headers) {
        *headers = provision->headers;
        provision->headers = NULL;
    }
}

extern void
provisionProofsConsume (BREthereumProvisionProofs *provision,
                        BRArrayOf(uint64_t) *numbers,
                        BRArrayOf(BREthereumBlockHeaderProof) *proofs) {
    if (NULL != numbers) { *numbers = provision->numbers; provision->numbers = NULL; }
    if (NULL != proofs ) { *proofs  = provision->proofs ; provision->proofs  = NULL; }
}

extern void
provisionBodiesConsume (BREthereumProvisionBodies *provision,
                        BRArrayOf(BREthereumHash) *hashes,
                        BRArrayOf(BREthereumBlockBodyPair) *pairs) {
    if (NULL != hashes) { *hashes = provision->hashes; provision->hashes = NULL; }
    if (NULL != pairs ) { *pairs  = provision->pairs;  provision->pairs  = NULL; }
}

extern void
provisionReceiptsConsume (BREthereumProvisionReceipts *provision,
                          BRArrayOf(BREthereumHash) *hashes,
                          BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) *receipts) {
    if (NULL != hashes)   { *hashes   = provision->hashes;   provision->hashes   = NULL; }
    if (NULL != receipts) { *receipts = provision->receipts; provision->receipts = NULL; }
}

extern void
provisionAccountsConsume (BREthereumProvisionAccounts *provision,
                          BRArrayOf(BREthereumHash) *hashes,
                          BRArrayOf(BREthereumAccountState) *accounts) {
    if (NULL != hashes)   { *hashes   = provision->hashes;   provision->hashes   = NULL; }
    if (NULL != accounts) { *accounts = provision->accounts; provision->accounts = NULL; }
}


extern void
provisionStatusesConsume (BREthereumProvisionStatuses *provision,
                          BRArrayOf(BREthereumHash) *hashes,
                          BRArrayOf(BREthereumTransactionStatus) *statuses) {
    if (NULL != hashes)   { *hashes   = provision->hashes;   provision->hashes   = NULL; }
    if (NULL != statuses) { *statuses = provision->statuses; provision->statuses = NULL; }
}

extern void
provisionSubmissionConsume (BREthereumProvisionSubmission *provision,
                            BREthereumTransaction *transaction,
                            BREthereumTransactionStatus *status) {
    if (NULL != transaction) { *transaction = provision->transaction; provision->transaction = NULL; }
    if (NULL != status)      { *status      = provision->status; }
}

extern void
provisionHandleMessage (BREthereumProvision *provision,
                        OwnershipGiven BREthereumMessage message,
                        size_t messageContentLimit,
                        uint64_t messageIdBase) {
    switch (message.identifier) {
        case MESSAGE_P2P:
        case MESSAGE_DIS:
        case MESSAGE_ETH:
            messageRelease(&message);
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

extern void
provisionResultRelease (BREthereumProvisionResult *result) {
    switch (result->status) {
        case PROVISION_SUCCESS:
            provisionRelease (&result->u.success.provision, ETHEREUM_BOOLEAN_TRUE);
            break;

        case PROVISION_ERROR:
             break;
    }
}
