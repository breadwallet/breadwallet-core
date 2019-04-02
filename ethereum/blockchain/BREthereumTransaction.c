//
//  BBREthereumTransaction.c
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/21/2018.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "BREthereumTransaction.h"

// #define TRANSACTION_LOG_ALLOC_COUNT

#define MAX(a,b) ((a) > (b) ? (a) : (b))

#if defined (TRANSACTION_LOG_ALLOC_COUNT)
static unsigned int transactionAllocCount = 0;
#endif

//
// Transaction
//
struct BREthereumTransactionRecord {
    // THIS MUST BE FIRST to support BRSet operations.

    /**
     * The transaction's hash.   This will be 'empty' until the transaction is submitted.
     */
    BREthereumHash hash;

    /**
     * The source address - sends 'amount'
     */
    BREthereumAddress sourceAddress;

    /**
     * The target address - recvs 'amount'
     */
    BREthereumAddress targetAddress;

    /**
     * The amount transferred from sourceAddress to targetAddress.  Note that this is not
     * necessarily the 'amount' RLP encoded in the 'raw transaction'.  Specifically if the `amount`
     * is for TOKEN, then the RLP encoded amount is 0 and the RLP encoded data for the ERC20
     * transfer function encodes the amount.
     */
    BREthereumEther amount;

    /**
     * The gas Price
     */
    BREthereumGasPrice gasPrice;

    /**
     * The gas limit
     */
    BREthereumGas gasLimit;

    /**
     * The gas estimate
     */
    BREthereumGas gasEstimate;

    /**
     * The nonce
     */
    uint64_t nonce;

    /**
     * The chainId
     */
    BREthereumChainId chainId;   // EIP-135 - chainId - "Since EIP-155 use chainId for v"

    /**
     * The data
     */
    char *data;

    /**
     * The signature, if signed (signer is not NULL).  This is a 'VRS' signature.
     */
    BREthereumSignature signature;

    /**
     * The status
     */
    BREthereumTransactionStatus status;
};

extern BREthereumTransaction
transactionCreate(BREthereumAddress sourceAddress,
                  BREthereumAddress targetAddress,
                  BREthereumEther amount,
                  BREthereumGasPrice gasPrice,
                  BREthereumGas gasLimit,
                  const char *data,
                  uint64_t nonce) {
    BREthereumTransaction transaction = calloc (1, sizeof (struct BREthereumTransactionRecord));

    transactionSetStatus(transaction, transactionStatusCreate (TRANSACTION_STATUS_UNKNOWN));
    transaction->sourceAddress = sourceAddress;
    transaction->targetAddress = targetAddress;
    transaction->amount = amount;
    transaction->gasPrice = gasPrice;
    transaction->gasLimit = gasLimit;           // Must not be changed.
    transaction->data = (NULL == data ? NULL : strdup (data));
    transaction->nonce = nonce;
    transaction->chainId = 0;
    transaction->hash = hashCreateEmpty();
    transaction->gasEstimate = gasLimit;

    // Ensure that `transactionIsSigned()` returns FALSE.
    signatureClear (&transaction->signature, SIGNATURE_TYPE_RECOVERABLE_VRS_EIP);

#if defined (TRANSACTION_LOG_ALLOC_COUNT)
    eth_log ("MEM", "TX Create - Count: %d", ++transactionAllocCount);
#endif
    return transaction;
}

extern BREthereumTransaction
transactionCopy (BREthereumTransaction transaction) {
    BREthereumTransaction copy = calloc (1, sizeof (struct BREthereumTransactionRecord));
    memcpy (copy, transaction, sizeof (struct BREthereumTransactionRecord));
    copy->data = (NULL == transaction->data ? NULL : strdup(transaction->data));

#if defined (TRANSACTION_LOG_ALLOC_COUNT)
    eth_log ("MEM", "TX Copy - Count: %d", ++transactionAllocCount);
#endif

    return copy;
}

extern void
transactionRelease (BREthereumTransaction transaction) {
    if (NULL != transaction) {
        if (NULL != transaction->data) free (transaction->data);
#if defined (TRANSACTION_LOG_ALLOC_COUNT)
        eth_log ("MEM", "TX Release - Count: %d", --transactionAllocCount);
#endif
        free (transaction);
    }
}

extern void
transactionReleaseForSet (void *ignore, void *item) {
    transactionRelease((BREthereumTransaction) item);
}

extern BREthereumAddress
transactionGetSourceAddress(BREthereumTransaction transaction) {
    return transaction->sourceAddress;
}

extern BREthereumAddress
transactionGetTargetAddress(BREthereumTransaction transaction) {
    return transaction->targetAddress;
}

extern BREthereumBoolean
transactionHasAddress (BREthereumTransaction transaction,
                       BREthereumAddress address) {
    return (ETHEREUM_BOOLEAN_IS_TRUE(addressEqual(address, transaction->sourceAddress))
            || ETHEREUM_BOOLEAN_IS_TRUE(addressEqual(address, transaction->targetAddress))
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern BREthereumEther
transactionGetAmount(BREthereumTransaction transaction) {
    return transaction->amount;
}

extern BREthereumEther
transactionGetFee (BREthereumTransaction transaction, int *overflow) {
    return etherCreate
    (mulUInt256_Overflow(transaction->gasPrice.etherPerGas.valueInWEI,
                         createUInt256 (ETHEREUM_BOOLEAN_IS_TRUE(transactionIsConfirmed(transaction))
                                        ? transaction->status.u.included.gasUsed.amountOfGas
                                        : transaction->gasLimit.amountOfGas),
                         overflow));
}

extern BREthereumEther
transactionGetFeeLimit (BREthereumTransaction transaction, int *overflow) {
    return etherCreate
    (mulUInt256_Overflow(transaction->gasPrice.etherPerGas.valueInWEI,
                         createUInt256 (transaction->gasLimit.amountOfGas),
                         overflow));
}

extern BREthereumGasPrice
transactionGetGasPrice (BREthereumTransaction transaction) {
    return transaction->gasPrice;
}

extern BREthereumGas
transactionGetGasLimit (BREthereumTransaction transaction) {
    return transaction->gasLimit;
}

static BREthereumGas
gasLimitApplyMargin (BREthereumGas gas) {
    return gasCreate(((100 + GAS_LIMIT_MARGIN_PERCENT) * gas.amountOfGas) / 100);
}

extern BREthereumGas
transactionGetGasEstimate (BREthereumTransaction transaction) {
    return transaction->gasEstimate;
}

extern void
transactionSetGasEstimate (BREthereumTransaction transaction,
                           BREthereumGas gasEstimate) {
    transaction->gasEstimate = gasEstimate;

    // Ensure that the gasLimit is at least 20% more than gasEstimate
    // unless the gasEstimate is 21000 (special case for ETH transfers).
    BREthereumGas gasLimitWithMargin = (21000 != gasEstimate.amountOfGas
                                        ? gasLimitApplyMargin (gasEstimate)
                                        : gasCreate(21000));
    if (gasLimitWithMargin.amountOfGas > transaction->gasLimit.amountOfGas)
        transaction->gasLimit = gasLimitWithMargin;
}

extern uint64_t
transactionGetNonce (BREthereumTransaction transaction) {
    return transaction->nonce;
}

private_extern void
transactionSetNonce (BREthereumTransaction transaction,
                     uint64_t nonce) {
    transaction->nonce = nonce;
}

extern size_t
transactionHashValue (const void *t)
{
    return hashSetValue(&((BREthereumTransaction) t)->hash);
}

extern int
transactionHashEqual (const void *t1, const void *t2) {
    return t1 == t2 || hashSetEqual (&((BREthereumTransaction) t1)->hash,
                                     &((BREthereumTransaction) t2)->hash);
}

//
// Data
//
extern const char *
transactionGetData (BREthereumTransaction transaction) {
    return transaction->data;
}

//
// Sign
//
extern void
transactionSign(BREthereumTransaction transaction,
                BREthereumSignature signature) {
    transactionSetStatus(transaction, transactionStatusCreate (TRANSACTION_STATUS_UNKNOWN));
    transaction->signature = signature;

    // The signature algorithm does not account for EIP-155 and thus the chainID.  We are signing
    // transactions according to EIP-155.  Thus v = CHAIN_ID * 2 + 35 or v = CHAIN_ID * 2 + 36
    // whereas the non-EIP-155 value of v is { 27, 28 }
    assert (SIGNATURE_TYPE_RECOVERABLE_VRS_EIP == signature.type);
    assert (27 == signature.sig.vrs.v || 28 == signature.sig.vrs.v);
}

extern BREthereumBoolean
transactionIsSigned (BREthereumTransaction transaction) {
    switch (transaction->signature.type) {
        case SIGNATURE_TYPE_RECOVERABLE_VRS_EIP:
            return AS_ETHEREUM_BOOLEAN (transaction->signature.sig.vrs.v != 0);
        case SIGNATURE_TYPE_RECOVERABLE_RSV:
            return AS_ETHEREUM_BOOLEAN (transaction->signature.sig.rsv.v != 0);
    }
}

extern const BREthereumHash
transactionGetHash (BREthereumTransaction transaction) {
    return transaction->hash;
}

extern void
transactionSetHash (BREthereumTransaction transaction,
                    BREthereumHash hash) {
    transaction->hash = hash;
}

extern BREthereumSignature
transactionGetSignature (BREthereumTransaction transaction) {
    return transaction->signature;
}

extern BREthereumAddress
transactionExtractAddress(BREthereumTransaction transaction,
                          BREthereumNetwork network,
                          BRRlpCoder coder) {
    if (ETHEREUM_BOOLEAN_IS_FALSE (transactionIsSigned(transaction))) {
        BREthereumAddress emptyAddress = EMPTY_ADDRESS_INIT;
        return emptyAddress;
    }

    int success = 1;

    BRRlpItem item = transactionRlpEncode (transaction, network, RLP_TYPE_TRANSACTION_UNSIGNED, coder);
    BRRlpData data = rlpGetData(coder, item);

    BREthereumAddress address = signatureExtractAddress(transaction->signature,
                                   data.bytes,
                                   data.bytesCount,
                                   &success);
    
    rlpDataRelease(data);
    rlpReleaseItem(coder, item);
    return address;
}

//
// Tranaction RLP Encode
//
extern BRRlpItem
transactionRlpEncode(BREthereumTransaction transaction,
                     BREthereumNetwork network,
                     BREthereumRlpType type,
                     BRRlpCoder coder) {
    BRRlpItem items[13]; // more than enough
    size_t itemsCount = 0;

    items[0] = rlpEncodeUInt64(coder, transaction->nonce, 1);
    items[1] = gasPriceRlpEncode(transaction->gasPrice, coder);
    items[2] = gasRlpEncode(transaction->gasLimit, coder);
    items[3] = addressRlpEncode(transaction->targetAddress, coder);
    items[4] = etherRlpEncode(transaction->amount, coder);
    items[5] = rlpEncodeHexString(coder, transaction->data);
    itemsCount = 6;

    // EIP-155:
    // If block.number >= FORK_BLKNUM and v = CHAIN_ID * 2 + 35 or v = CHAIN_ID * 2 + 36, then when
    // computing the hash of a transaction for purposes of signing or recovering, instead of hashing
    // only the first six elements (i.e. nonce, gasprice, startgas, to, value, data), hash nine
    // elements, with v replaced by CHAIN_ID, r = 0 and s = 0. The currently existing signature
    // scheme using v = 27 and v = 28 remains valid and continues to operate under the same rules
    // as it does now.

    transaction->chainId = networkGetChainId(network);

    switch (type) {
        case RLP_TYPE_TRANSACTION_UNSIGNED:
            // For EIP-155, encode { v, r, s } with v as the chainId and both r and s as empty.
            items[6] = rlpEncodeUInt64(coder, transaction->chainId, 1);
            items[7] = rlpEncodeString(coder, "");
            items[8] = rlpEncodeString(coder, "");
            itemsCount += 3;
            break;

        case RLP_TYPE_TRANSACTION_SIGNED: // aka NETWORK
        case RLP_TYPE_ARCHIVE:
            // For EIP-155, encode v with the chainID.
            items[6] = rlpEncodeUInt64(coder, transaction->signature.sig.vrs.v + 8 +
                                       2 * transaction->chainId, 1);

            items[7] = rlpEncodeBytesPurgeLeadingZeros (coder,
                                                        transaction->signature.sig.vrs.r,
                                                        sizeof (transaction->signature.sig.vrs.r));
            
            items[8] = rlpEncodeBytesPurgeLeadingZeros (coder,
                                                        transaction->signature.sig.vrs.s,
                                                        sizeof (transaction->signature.sig.vrs.s));
            itemsCount += 3;

            // For ARCHIVE add in a few things beyond 'SIGNED / NETWORK'
            if (RLP_TYPE_ARCHIVE == type) {
                items[ 9] = addressRlpEncode(transaction->sourceAddress, coder);
                items[10] = hashRlpEncode(transaction->hash, coder);
                items[11] = transactionStatusRLPEncode(transaction->status, coder);
                itemsCount += 3;
            }
            break;
    }
    
    BRRlpItem result = rlpEncodeListItems(coder, items, itemsCount);

    if (RLP_TYPE_TRANSACTION_SIGNED == type) {
        BRRlpData data = rlpGetDataSharedDontRelease(coder, result);
        transaction->hash = hashCreateFromData(data);
    }

    return result;
}

//
// Tranaction RLP Decode
//
extern BREthereumTransaction
transactionRlpDecode (BRRlpItem item,
                      BREthereumNetwork network,
                      BREthereumRlpType type,
                      BRRlpCoder coder) {
    
    BREthereumTransaction transaction = calloc (1, sizeof(struct BREthereumTransactionRecord));
    
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert (( 9 == itemsCount && (RLP_TYPE_TRANSACTION_SIGNED == type || RLP_TYPE_TRANSACTION_UNSIGNED == type)) ||
            (12 == itemsCount && RLP_TYPE_ARCHIVE == type));
    
    // Encoded as:
    //    items[0] = transactionEncodeNonce(transaction, transaction->nonce, coder);
    //    items[1] = gasPriceRlpEncode(transaction->gasPrice, coder);
    //    items[2] = gasRlpEncode(transaction->gasLimit, coder);
    //    items[3] = transactionEncodeAddressForHolding(transaction, transaction->amount, coder);
    //    items[4] = amountRlpEncode(transaction->amount, coder);
    //    items[5] = transactionEncodeDataForHolding(transaction, transaction->amount, coder);
    
    transaction->nonce = rlpDecodeUInt64(coder, items[0], 1);
    transaction->gasPrice = gasPriceRlpDecode(items[1], coder);
    transaction->gasLimit = gasRlpDecode(items[2], coder);
    
    transaction->targetAddress = addressRlpDecode(items[3], coder);
    transaction->amount = etherRlpDecode(items[4], coder);
    transaction->data = rlpDecodeHexString (coder, items[5], "0x");
    
    transaction->chainId = networkGetChainId(network);
    
    uint64_t eipChainId = rlpDecodeUInt64(coder, items[6], 1);
    
    // By default, ensure `transacdtionIsSigned()` returns FALSE.
    signatureClear (&transaction->signature, SIGNATURE_TYPE_RECOVERABLE_VRS_EIP);
    
    // We have a signature - is this the proper logic?
    if (eipChainId != transaction->chainId) {
        // RLP_TYPE_TRANSACTION_SIGNED
        transaction->signature.type = SIGNATURE_TYPE_RECOVERABLE_VRS_EIP;
        
        // If we are RLP decoding a transactino prior to EIP-xxx, then the eipChainId will
        // not be encoded with the chainId.  In that case, just use the eipChainId
        transaction->signature.sig.vrs.v = (eipChainId > 30
                                            ? eipChainId - 8 - 2 * transaction->chainId
                                            : eipChainId);
        
        BRRlpData rData = rlpDecodeBytesSharedDontRelease (coder, items[7]);
        assert (32 >= rData.bytesCount);
        memcpy (&transaction->signature.sig.vrs.r[32 - rData.bytesCount],
                rData.bytes, rData.bytesCount);
        
        BRRlpData sData = rlpDecodeBytesSharedDontRelease (coder, items[8]);
        assert (32 >= sData.bytesCount);
        memcpy (&transaction->signature.sig.vrs.s[32 - sData.bytesCount],
                sData.bytes, sData.bytesCount);
        
    }
    
    switch (type) {
        case RLP_TYPE_ARCHIVE:
            // Extract the archive-specific data
            transaction->sourceAddress = addressRlpDecode(items[9], coder);
            transaction->hash = hashRlpDecode(items[10], coder);
            transaction->status = transactionStatusRLPDecode(items[11], NULL, coder);
            break;

        case RLP_TYPE_TRANSACTION_SIGNED: {
            // With a SIGNED RLP encoding, we can extract the source address and compute the hash.
            BRRlpData result = rlpGetDataSharedDontRelease(coder, item);
            transaction->hash = hashCreateFromData(result);

            // :fingers-crossed:
            transaction->sourceAddress = transactionExtractAddress (transaction, network, coder);
            break;
        }

        default: break;
    }

#if defined (TRANSACTION_LOG_ALLOC_COUNT)
    eth_log ("MEM", "TX RLPDecode - Count: %d", ++transactionAllocCount);
#endif
    return transaction;
}

extern char *
transactionGetRlpHexEncoded (BREthereumTransaction transaction,
                             BREthereumNetwork network,
                             BREthereumRlpType type,
                             const char *prefix) {
    if (NULL == prefix) prefix = "";

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = transactionRlpEncode (transaction, network, type, coder);
    BRRlpData data = rlpGetDataSharedDontRelease(coder, item);

    char *result;

    if (0 == data.bytesCount)
        result = strdup (prefix);
    else {
        size_t resultLen = strlen(prefix) + 2 * data.bytesCount + 1;
        result = malloc (resultLen);
        strcpy (result, prefix);
        encodeHex(&result[strlen(prefix)], 2 * data.bytesCount + 1, data.bytes, data.bytesCount);
    }

    rlpReleaseItem(coder, item);
    rlpCoderRelease(coder);
    return result;
}

extern BREthereumTransactionStatus
transactionGetStatus (BREthereumTransaction transaction) {
    return transaction->status;
}

extern void
transactionSetStatus (BREthereumTransaction transaction,
                      BREthereumTransactionStatus status) {
    transaction->status = status;
}

extern BREthereumBoolean
transactionIsConfirmed (BREthereumTransaction transaction) {
    return AS_ETHEREUM_BOOLEAN (TRANSACTION_STATUS_INCLUDED == transaction->status.type);
}

extern BREthereumBoolean
transactionIsSubmitted (BREthereumTransaction transaction) {
    return AS_ETHEREUM_BOOLEAN(TRANSACTION_STATUS_UNKNOWN != transaction->status.type);
}

extern BREthereumBoolean
transactionIsErrored (BREthereumTransaction transaction) {
    return AS_ETHEREUM_BOOLEAN(TRANSACTION_STATUS_ERRORED == transaction->status.type);
}

static int
transactionHasStatus(BREthereumTransaction transaction,
                     BREthereumTransactionStatusType type) {
    return type == transaction->status.type;
}


/**
 * Compare two transactions based on their block, or if not blocked, their nonce.
 *
 * @param t1
 * @param t2
 =
 * @return a BREthereumComparison result.
 */
extern BREthereumComparison
transactionCompare(BREthereumTransaction t1,
                   BREthereumTransaction t2) {

    if (  t1 == t2) return ETHEREUM_COMPARISON_EQ;
    if (NULL == t2) return ETHEREUM_COMPARISON_LT;
    if (NULL == t1) return ETHEREUM_COMPARISON_GT;

    int t1Blocked = transactionHasStatus(t1, TRANSACTION_STATUS_INCLUDED);
    int t2Blocked = transactionHasStatus(t2, TRANSACTION_STATUS_INCLUDED);

    if (t1Blocked && t2Blocked)
        return (t1->status.u.included.blockNumber < t2->status.u.included.blockNumber
                ? ETHEREUM_COMPARISON_LT
                : (t1->status.u.included.blockNumber > t2->status.u.included.blockNumber
                   ? ETHEREUM_COMPARISON_GT
                   : (t1->status.u.included.transactionIndex < t2->status.u.included.transactionIndex
                      ? ETHEREUM_COMPARISON_LT
                      : (t1->status.u.included.transactionIndex > t2->status.u.included.transactionIndex
                         ? ETHEREUM_COMPARISON_GT
                         : ETHEREUM_COMPARISON_EQ))));

    else if (!t1Blocked && t2Blocked)
        return ETHEREUM_COMPARISON_GT;

    else if (t1Blocked && !t2Blocked)
        return ETHEREUM_COMPARISON_LT;
    
    else
        return (t1->nonce < t2->nonce
                ? ETHEREUM_COMPARISON_LT
                : (t1->nonce > t2->nonce
                   ? ETHEREUM_COMPARISON_GT
                   : ETHEREUM_COMPARISON_EQ));
}

extern void
transactionShow (BREthereumTransaction transaction, const char *topic) {
    int overflow;

    char *hash = hashAsString (transaction->hash);
    char *source = addressGetEncodedString(transaction->sourceAddress, 1);
    char *target = addressGetEncodedString(transactionGetTargetAddress(transaction), 1);
    char *amount = etherGetValueString (transactionGetAmount(transaction), ETHER);
    char *gasP   = etherGetValueString (transactionGetGasPrice(transaction).etherPerGas, GWEI);
    char *fee    = etherGetValueString (transactionGetFee(transaction, &overflow), ETHER);

    BREthereumEther totalEth = etherCreate(addUInt256_Overflow(transaction->amount.valueInWEI, transactionGetFee(transaction, &overflow).valueInWEI, &overflow));
    char *total  = etherGetValueString (totalEth, ETHER);
    char *totalWEI = etherGetValueString (totalEth, WEI);

    eth_log (topic, "=== Transaction%s", "");
    eth_log (topic, "    Hash  : %s", hash);
    eth_log (topic, "    Nonce : %" PRIu64, transactionGetNonce(transaction));
    eth_log (topic, "    Source: %s", source);
    eth_log (topic, "    Target: %s", target);
    eth_log (topic, "    Amount: %s ETHER", amount);
    eth_log (topic, "    GasPrc: %s GWEI", gasP);
    eth_log (topic, "    GasLmt: %" PRIu64, transactionGetGasLimit(transaction).amountOfGas);
    eth_log (topic, "    Fee   : %s ETHER", fee);
    eth_log (topic, "    Total : %s ETHER", total);
    eth_log (topic, "    Total : %s WEI", totalWEI);
    eth_log (topic, "    Data  : %s", transaction->data);

    BREthereumContractFunction function = contractLookupFunctionForEncoding (contractERC20, transaction->data);
    if (NULL != function && functionERC20Transfer == function) {
        BRCoreParseStatus status;
        UInt256 funcAmount = functionERC20TransferDecodeAmount(function, transaction->data, &status);
        char *funcAddr   = functionERC20TransferDecodeAddress (function, transaction->data);
        char *funcAmt    = coerceString(funcAmount, 10);

        BREthereumToken token = tokenLookup(target);

        eth_log (topic, "    Token : %s", (NULL == token ? "???" : tokenGetSymbol(token)));
        eth_log (topic, "    TokFnc: %s", "erc20 transfer");
        eth_log (topic, "    TokAmt: %s", funcAmt);
        eth_log (topic, "    TokAdr: %s", funcAddr);

        free (funcAmt); free (funcAddr);

    }
    free (totalWEI); free (total); free (fee); free (gasP);
    free (amount); free (target); free (source); free (hash);

}

extern void
transactionsRelease (BRArrayOf(BREthereumTransaction) transactions) {
    if (NULL != transactions) {
        size_t count = array_count(transactions);
        for (size_t index = 0; index < count; index++)
            transactionRelease(transactions[index]);
        array_free (transactions);
    }
}

/*
     https://github.com/ethereum/pyethereum/blob/develop/ethereum/transactions.py#L22
     https://github.com/ethereum/pyrlp/blob/develop/rlp/sedes/lists.py#L135

     A transaction is stored as:
    [nonce, gasprice, startgas, to, value, data, v, r, s]
    nonce is the number of transactions already sent by that account, encoded
    in binary form (eg.  0 -> '', 7 -> '\x07', 1000 -> '\x03\xd8').
    (v,r,s) is the raw Electrum-style signature of the transaction without the
    signature made with the private key corresponding to the sending account,
    with 0 <= v <= 3. From an Electrum-style signature (65 bytes) it is
    possible to extract the public key, and thereby the address, directly.
    A valid transaction is one where:
    (i) the signature is well-formed (ie. 0 <= v <= 3, 0 <= r < P, 0 <= s < N,
        0 <= r < P - N if v >= 2), and
    (ii) the sending account has enough funds to pay the fee and the value.
    """

    fields = [
        ('nonce', big_endian_int),
        ('gasprice', big_endian_int),
        ('startgas', big_endian_int),
        ('to', utils.address),
        ('value', big_endian_int),
        ('data', binary),
        ('v', big_endian_int),
        ('r', big_endian_int),
        ('s', big_endian_int),
    ]

 */


/*
 $ curl -X POST -H  "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_getTransactionByHash","params":["0x3104b0ee2aba4197f4da656d6144e5978c0b7bcb08890ed7bd6228bc9dbe745e"],"id":1}' http://localhost:8545
    {"jsonrpc":"2.0","id":1,
    "result":{"blockHash":"0xbf197f8ce876514b8922af10824efba5b4ce3fc7ab9ef97443ef9c56bd0cae32",
        "blockNumber":"0x1b930a",
        "from":"0x888197521cfe05ff89960c50012252008819b2cb",
        "gas":"0x1d8a8",
        "gasPrice":"0x4a817c800",
        "hash":"0x3104b0ee2aba4197f4da656d6144e5978c0b7bcb08890ed7bd6228bc9dbe745e",
        "input":"0x",
        "nonce":"0x0",
        "to":"0xf8e60edd24bc15f32bb4260ec2cea7c54cced121",
        "transactionIndex":"0x3",
        "value":"0xde0b6b3a7640000",
        "v":"0x2b",
        "r":"0xa571650cb08199d808b6646f634a8f7431cfd103a243654263faf2518e3efd40",
        "s":"0x4d2774147ccb90d1e7ad9358eb895c5f5d24db26b9d3e880bcee4fa06e5b3e1b"}}
 */

/*
 Signing

 https://bitcoin.stackexchange.com/questions/38351/ecdsa-v-r-s-what-is-v

 > msgSha = web3.sha3('Now it the time')
"0x8b3942af68acfd875239181babe9ce093c420ca78d15b178fb63cf839dcf0971"

 > personal.unlockAccount(eth.accounts[<n>], "password", 3600)

 $ curl -X POST -H  "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_sign","
    params":["0xf8e60edd24bc15f32bb4260ec2cea7c54cced121", "0x8b3942af68acfd875239181babe9ce093c420ca78d15b178fb63cf839dcf0971"],
    "id":1}'
    http://localhost:8545

 {"jsonrpc":"2.0","id":1,
    "result":"0xe79ba93e981e8ee50b8d07b0be7ae4526bc4d9bf7dcffe88dff62c502b2a126d7f772e2374869b41b0f5c0061d6d828348c96a7021f0c3227e73431d8ebbf1331b"}
 */

/*
 * r, s, v

  signature = signature.substr(2); //remove 0x
  const r = '0x' + signature.slice(0, 64)
  const s = '0x' + signature.slice(64, 128)
  const v = '0x' + signature.slice(128, 130)
  const v_decimal = web3.toDecimal(v)

 > web3.eth.sign ("0xf8e60edd24bc15f32bb4260ec2cea7c54cced121", "0x8b3942af68acfd875239181babe9ce093c420ca78d15b178fb63cf839dcf0971")
"0xe79ba93e981e8ee50b8d07b0be7ae4526bc4d9bf7dcffe88dff62c502b2a126d7f772e2374869b41b0f5c0061d6d828348c96a7021f0c3227e73431d8ebbf1331b"
 */

/*
 > msg = 'Try again'
 > msgSha = web3.sha3(msg)
 > sig = web3.eth.sign ("0xf8e60edd24bc15f32bb4260ec2cea7c54cced121", msgSha) // account, sha-ed msg
 > personal.ecRecover (msgSha, sig)
 "0xf8e60edd24bc15f32bb4260ec2cea7c54cced121"
 > eth.accounts[1]
 "0xf8e60edd24bc15f32bb4260ec2cea7c54cced121"
 */


/*

 ===== RLP Encode - =====

 All this method use some form of rlp.encode(<transaction>, ...)
   sigHash = utils.sha3 (rlp.encode (self, UnsignedTransaction)

     @property
    def sender(self):
        if not self._sender:
            # Determine sender
            if self.r == 0 and self.s == 0:
                self._sender = null_address
            else:
                if self.v in (27, 28):
                    vee = self.v
                    sighash = utils.sha3(rlp.encode(self, UnsignedTransaction))
                elif self.v >= 37:
                    vee = self.v - self.network_id * 2 - 8
                    assert vee in (27, 28)
                    rlpdata = rlp.encode(rlp.infer_sedes(self).serialize(self)[
                                         :-3] + [self.network_id, '', ''])
                    sighash = utils.sha3(rlpdata)
                else:
                    raise InvalidTransaction("Invalid V value")
                if self.r >= secpk1n or self.s >= secpk1n or self.r == 0 or self.s == 0:
                    raise InvalidTransaction("Invalid signature values!")
                pub = ecrecover_to_pub(sighash, vee, self.r, self.s)
                if pub == b'\x00' * 64:
                    raise InvalidTransaction(
                        "Invalid signature (zero privkey cannot sign)")
                self._sender = utils.sha3(pub)[-20:]
        return self._sender

    @property
    def network_id(self):
        if self.r == 0 and self.s == 0:
            return self.v
        elif self.v in (27, 28):
            return None
        else:
            return ((self.v - 1) // 2) - 17

    @sender.setter
    def sender(self, value):
        self._sender = value

    def sign(self, key, network_id=None):
        """Sign this transaction with a private key.
        A potentially already existing signature would be overridden.
        """
        if network_id is None:
            rawhash = utils.sha3(rlp.encode(self, UnsignedTransaction))
        else:
            assert 1 <= network_id < 2**63 - 18
            rlpdata = rlp.encode(rlp.infer_sedes(self).serialize(self)[
                                 :-3] + [network_id, b'', b''])
            rawhash = utils.sha3(rlpdata)

        key = normalize_key(key)

        self.v, self.r, self.s = ecsign(rawhash, key)
        if network_id is not None:
            self.v += 8 + network_id * 2

        self._sender = utils.privtoaddr(key)
        return self

    @property
    def hash(self):
        return utils.sha3(rlp.encode(self))

    def to_dict(self):
        d = {}
        for name, _ in self.__class__.fields:
            d[name] = getattr(self, name)
            if name in ('to', 'data'):
                d[name] = '0x' + encode_hex(d[name])
        d['sender'] = '0x' + encode_hex(self.sender)
        d['hash'] = '0x' + encode_hex(self.hash)
        return d


 */


/*

 ##
    # Sign this transaction with a private key.
    #
    # A potentially already existing signature would be override.
    #
    def sign(key)
      raise InvalidTransaction, "Zero privkey cannot sign" if [0, '', Constant::PRIVKEY_ZERO, Constant::PRIVKEY_ZERO_HEX].include?(key)

      rawhash = Utils.keccak256 signing_data(:sign)
      key = PrivateKey.new(key).encode(:bin)

      vrs = Secp256k1.recoverable_sign rawhash, key
      self.v = encode_v(vrs[0])
      self.r = vrs[1]
      self.s = vrs[2]

      self.sender = PrivateKey.new(key).to_address

      self
    end


     def signing_data(mode)
      case mode
      when :sign
        if v == 0 # use encoding rules before EIP155
          RLP.encode(self, sedes: UnsignedTransaction)
        else
          raise InvalidTransaction, "invalid signature"
        end
      when :verify
        if v == V_ZERO || v == V_ONE # encoded v before EIP155
          RLP.encode(self, sedes: UnsignedTransaction)
        end
      else
        raise InvalidTransaction, "invalid signature"
      end
    end
  end


 def encode(obj, sedes=None, infer_serializer=True, cache=False):
    """Encode a Python object in RLP format.
    By default, the object is serialized in a suitable way first (using :func:`rlp.infer_sedes`)
    and then encoded. Serialization can be explicitly suppressed by setting `infer_serializer` to
    ``False`` and not passing an alternative as `sedes`.
    If `obj` has an attribute :attr:`_cached_rlp` (as, notably, :class:`rlp.Serializable`) and its
    value is not `None`, this value is returned bypassing serialization and encoding, unless
    `sedes` is given (as the cache is assumed to refer to the standard serialization which can be
    replaced by specifying `sedes`).
    If `obj` is a :class:`rlp.Serializable` and `cache` is true, the result of the encoding will be
    stored in :attr:`_cached_rlp` if it is empty and :meth:`rlp.Serializable.make_immutable` will
    be invoked on `obj`.
    :param sedes: an object implementing a function ``serialize(obj)`` which will be used to
                  serialize ``obj`` before encoding, or ``None`` to use the infered one (if any)
    :param infer_serializer: if ``True`` an appropriate serializer will be selected using
                             :func:`rlp.infer_sedes` to serialize `obj` before encoding
    :param cache: cache the return value in `obj._cached_rlp` if possible and make `obj` immutable
                  (default `False`)
    :returns: the RLP encoded item
    :raises: :exc:`rlp.EncodingError` in the rather unlikely case that the item is too big to
             encode (will not happen)
    :raises: :exc:`rlp.SerializationError` if the serialization fails
    """


 https://github.com/ethereum/pyrlp/blob/develop/rlp/sedes/lists.py
 *
 */



/*
     public byte[] getEncoded() {

        if (rlpEncoded != null) return rlpEncoded;

        // parse null as 0 for nonce
        byte[] nonce = null;
        if (this.nonce == null || this.nonce.length == 1 && this.nonce[0] == 0) {
            nonce = RLP.encodeElement(null);
        } else {
            nonce = RLP.encodeElement(this.nonce);
        }
        byte[] gasPrice = RLP.encodeElement(this.gasPrice);
        byte[] gasLimit = RLP.encodeElement(this.gasLimit);
        byte[] receiveAddress = RLP.encodeElement(this.receiveAddress);
        byte[] value = RLP.encodeElement(this.value);
        byte[] data = RLP.encodeElement(this.data);

        byte[] v, r, s;

        if (signature != null) {
            int encodeV;
            if (chainId == null) {
                encodeV = signature.v;
            } else {
                encodeV = signature.v - LOWER_REAL_V;
                encodeV += chainId * 2 + CHAIN_ID_INC;
            }
            v = RLP.encodeInt(encodeV);
            r = RLP.encodeElement(BigIntegers.asUnsignedByteArray(signature.r));
            s = RLP.encodeElement(BigIntegers.asUnsignedByteArray(signature.s));
        } else {
            // Since EIP-155 use chainId for v
            v = chainId == null ? RLP.encodeElement(EMPTY_BYTE_ARRAY) : RLP.encodeInt(chainId);
            r = RLP.encodeElement(EMPTY_BYTE_ARRAY);
            s = RLP.encodeElement(EMPTY_BYTE_ARRAY);
        }

        this.rlpEncoded = RLP.encodeList(nonce, gasPrice, gasLimit,
                receiveAddress, value, data, v, r, s);

        this.hash = this.getHash();

        return rlpEncoded;
    }


     public synchronized void rlpParse() {
        if (parsed) return;
        try {
            RLPList decodedTxList = RLP.decode2(rlpEncoded);
            RLPList transaction = (RLPList) decodedTxList.get(0);

            // Basic verification
            if (transaction.size() > 9 ) throw new RuntimeException("Too many RLP elements");
            for (RLPElement rlpElement : transaction) {
                if (!(rlpElement instanceof RLPItem))
                    throw new RuntimeException("Transaction RLP elements shouldn't be lists");
            }

            this.nonce = transaction.get(0).getRLPData();
            this.gasPrice = transaction.get(1).getRLPData();
            this.gasLimit = transaction.get(2).getRLPData();
            this.receiveAddress = transaction.get(3).getRLPData();
            this.value = transaction.get(4).getRLPData();
            this.data = transaction.get(5).getRLPData();
            // only parse signature in case tx is signed
            if (transaction.get(6).getRLPData() != null) {
                byte[] vData =  transaction.get(6).getRLPData();
                BigInteger v = ByteUtil.bytesToBigInteger(vData);
                this.chainId = extractChainIdFromV(v);
                byte[] r = transaction.get(7).getRLPData();
                byte[] s = transaction.get(8).getRLPData();
                this.signature = ECDSASignature.fromComponents(r, s, getRealV(v));
            } else {
                logger.debug("RLP encoded tx is not signed!");
            }
            this.parsed = true;
            this.hash = getHash();
        } catch (Exception e) {
            throw new RuntimeException("Error on parsing RLP", e);
        }
    }

 */
