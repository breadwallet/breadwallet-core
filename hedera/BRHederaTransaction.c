/**
*/

#include "BRHederaTransaction.h"
#include "BRHederaCrypto.h"
#include "BRHederaSerialize.h"
#include "vendor/ed25519/ed25519.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>

struct BRHederaTransactionRecord {
    BRHederaAddress source;
    BRHederaAddress target;
    BRHederaUnitTinyBar amount;
    char * transactionId;
    BRHederaUnitTinyBar fee;
    uint8_t * serializedBytes;
    size_t serializedSize;
    BRHederaTransactionHash hash;
    BRHederaFeeBasis feeBasis;
    BRHederaAddress nodeAddress;
    BRHederaTimeStamp timeStamp;
    uint64_t blockHeight;
};

char * createTransactionID(BRHederaAddress address, BRHederaTimeStamp timeStamp)
{
    char buffer[128] = {0};
    const char * hederaAddress = hederaAddressAsString(address);
    sprintf(buffer, "%s-%lld-%d", hederaAddress, timeStamp.seconds, timeStamp.nano);
    char * result = calloc(1, strlen(buffer) + 1);
    strncpy(result, buffer, strlen(buffer));
    return result;
}

extern BRHederaTransaction hederaTransactionCreateNew (BRHederaAddress source,
                                                       BRHederaAddress target,
                                                       BRHederaUnitTinyBar amount,
                                                       BRHederaFeeBasis feeBasis,
                                                       BRHederaAddress nodeAddress,
                                                       BRHederaTimeStamp *timeStamp)
{
    BRHederaTransaction transaction = calloc (1, sizeof(struct BRHederaTransactionRecord));
    transaction->source = hederaAddressClone (source);
    transaction->target = hederaAddressClone (target);
    transaction->amount = amount;
    transaction->feeBasis = feeBasis;
    transaction->fee = hederaFeeBasisGetFee (&transaction->feeBasis);
    transaction->nodeAddress = hederaAddressClone (nodeAddress);
    if (timeStamp != NULL) {
        transaction->timeStamp = *timeStamp;
    } else {
        // Generate a timestamp - hedera uses the timestamp in the transaction ID
        // and includes seconds and nano-seconds
        transaction->timeStamp = hederaGenerateTimeStamp();
    }
    transaction->transactionId = createTransactionID(source, transaction->timeStamp);
    transaction->blockHeight = 0;
    return transaction;
}

extern BRHederaTransaction hederaTransactionCreate (BRHederaAddress source,
                                                    BRHederaAddress target,
                                                    BRHederaUnitTinyBar amount,
                                                    BRHederaUnitTinyBar fee,
                                                    const char * txID,
                                                    BRHederaTransactionHash hash,
                                                    uint64_t timestamp, uint64_t blockHeight,
                                                    int error)
{
    // This is an existing transaction - it must have a transaction ID
    assert(source);
    assert(target);
    BRHederaTransaction transaction = calloc (1, sizeof(struct BRHederaTransactionRecord));
    transaction->source = hederaAddressClone (source);
    transaction->target = hederaAddressClone (target);
    transaction->amount = amount;
    transaction->fee = fee;
    transaction->feeBasis.pricePerCostFactor = fee;
    transaction->feeBasis.costFactor = 1;

    // Parse the transactionID
    if (txID && strlen(txID) > 1) {
        transaction->transactionId = (char*) calloc(1, strlen(txID) + 1);
        strcpy(transaction->transactionId, txID);
        // Get the timestamp from the transaction id
        transaction->timeStamp = hederaParseTimeStamp(txID);
    } else {
        // It would great to be able to get the timestamp from the txID - but I guess
        // we just have to use whatever came from blockset
        transaction->timeStamp.seconds = timestamp;
    }

    transaction->hash = hash;
    transaction->blockHeight = blockHeight;

    return transaction;
}

extern BRHederaTransaction
hederaTransactionClone (BRHederaTransaction transaction)
{
    assert(transaction);
    BRHederaTransaction newTransaction = calloc (1, sizeof(struct BRHederaTransactionRecord));
    newTransaction->source = hederaTransactionGetSource(transaction);
    newTransaction->target = hederaTransactionGetTarget(transaction);
    newTransaction->amount = hederaTransactionGetAmount(transaction);
    newTransaction->fee = transaction->fee;
    newTransaction->feeBasis = transaction->feeBasis;

    if (transaction->transactionId && strlen(transaction->transactionId) > 1) {
        newTransaction->transactionId = (char*) calloc(1, strlen(transaction->transactionId) + 1);
        strcpy(newTransaction->transactionId, transaction->transactionId);
    }

    newTransaction->hash = transaction->hash;
    newTransaction->blockHeight = transaction->blockHeight;
    newTransaction->timeStamp = transaction->timeStamp;

    if (transaction->serializedSize > 0 && transaction->serializedBytes) {
        newTransaction->serializedSize = transaction->serializedSize;
        newTransaction->serializedBytes = calloc(1, newTransaction->serializedSize);
        memcpy(newTransaction->serializedBytes, transaction->serializedBytes, newTransaction->serializedSize);
    }

    if (transaction->nodeAddress) {
        newTransaction->nodeAddress = hederaAddressClone(transaction->nodeAddress);
    }

    return newTransaction;
}

extern void hederaTransactionFree (BRHederaTransaction transaction)
{
    assert (transaction);
    if (transaction->serializedBytes) free (transaction->serializedBytes);
    if (transaction->transactionId) free (transaction->transactionId);
    if (transaction->source) hederaAddressFree (transaction->source);
    if (transaction->target) hederaAddressFree (transaction->target);
    if (transaction->nodeAddress) hederaAddressFree (transaction->nodeAddress);
    free (transaction);
}

extern size_t
hederaTransactionSignTransaction (BRHederaTransaction transaction,
                                  BRKey publicKey,
                                  UInt512 seed)
{
    assert (transaction);

    // If previously signed - delete and resign
    if (transaction->serializedBytes) {
        free (transaction->serializedBytes);
        transaction->serializedSize = 0;
    }

    BRHederaUnitTinyBar fee = hederaFeeBasisGetFee(&transaction->feeBasis);

    // Generate the private key from the seed
    BRKey key = hederaKeyCreate (seed);
    unsigned char privateKey[64] = {0};
    unsigned char temp[32] = {0}; // Use the public key that is sent in instead
    ed25519_create_keypair (temp, privateKey, key.secret.u8);

    // First we need to serialize the body since it is the thing we sign
    size_t bodySize;
    uint8_t * body = hederaTransactionBodyPack (transaction->source,
                                                transaction->target,
                                                transaction->nodeAddress,
                                                transaction->amount,
                                                transaction->timeStamp,
                                                fee,
                                                NULL,
                                                &bodySize);

    // Create signature from the body bytes
    unsigned char signature[64];
    memset (signature, 0x00, 64);
    ed25519_sign(signature, body, bodySize, publicKey.pubKey, privateKey);

    // Serialize the full transaction including signature and public key
    uint8_t * serializedBytes = hederaTransactionPack (signature, 64,
                                                       publicKey.pubKey, 32,
                                                       body, bodySize,
                                                       &transaction->serializedSize);

    // We are now done with the body - it was copied to the serialized bytes so we
    // must clean up it now.
    free (body);

    // Due to how the Hedera API works the "node account id" of the server we will submit to
    // is included in the signing data so we MUST get the server to use the correct node.
    // The BDB server implementation requires that we add in the node account id along with
    // the serialized bytes.
    uint8_t nodeAccountId[HEDERA_ADDRESS_SERIALIZED_SIZE];
    hederaAddressSerialize (transaction->nodeAddress, nodeAccountId, HEDERA_ADDRESS_SERIALIZED_SIZE);

    // The buffer has to hold the nodeAccountId and the serialized bytes
    transaction->serializedBytes = calloc(1, transaction->serializedSize + HEDERA_ADDRESS_SERIALIZED_SIZE);

    // First copy the nodeAccountId,
    memcpy(transaction->serializedBytes, nodeAccountId, HEDERA_ADDRESS_SERIALIZED_SIZE);

    // Now copy the serialized transaction bytes after the node account id
    memcpy(transaction->serializedBytes + HEDERA_ADDRESS_SERIALIZED_SIZE, serializedBytes, transaction->serializedSize);

    // Cleanup temporary buffers
    free (serializedBytes);

    transaction->serializedSize += HEDERA_ADDRESS_SERIALIZED_SIZE; // This will be our new size of serialized bytes
    return transaction->serializedSize;
}

extern uint8_t * hederaTransactionSerialize (BRHederaTransaction transaction, size_t *size)
{
    if (transaction->serializedBytes) {
        *size = transaction->serializedSize;
        return transaction->serializedBytes;
    } else {
        *size = 0;
        return NULL;
    }
}

extern BRHederaTransactionHash hederaTransactionGetHash(BRHederaTransaction transaction)
{
    assert(transaction);
    return transaction->hash;
}

extern char * hederaTransactionGetTransactionId(BRHederaTransaction transaction)
{
    assert(transaction);
    if (transaction->transactionId) {
        return strdup(transaction->transactionId);
    }
    return NULL;
}

extern BRHederaUnitTinyBar hederaTransactionGetFee(BRHederaTransaction transaction)
{
    assert(transaction);
    return transaction->fee;
}

extern BRHederaUnitTinyBar hederaTransactionGetAmount(BRHederaTransaction transaction)
{
    assert(transaction);
    return transaction->amount;
}

extern BRHederaAddress hederaTransactionGetSource(BRHederaTransaction transaction)
{
    assert(transaction);
    return hederaAddressClone (transaction->source);
}

extern BRHederaAddress hederaTransactionGetTarget(BRHederaTransaction transaction)
{
    assert(transaction);
    return hederaAddressClone (transaction->target);
}

extern bool hederaTransactionEqual (BRHederaTransaction t1, BRHederaTransaction t2)
{
    assert(t1);
    assert(t2);
    // Equal means the same transaction id, source, target
    bool result = false;
    // First check the transaction ID
    if (t1->transactionId && t2->transactionId) {
        if (0 == strcmp(t1->transactionId, t2->transactionId)) {
            result = true;
        }
    } else {
        // Transaction IDs are not available - use the hash
        BRHederaTransactionHash hash1 = hederaTransactionGetHash(t1);
        BRHederaTransactionHash hash2 = hederaTransactionGetHash(t2);
        if (memcmp(hash1.bytes, hash2.bytes, sizeof(hash1.bytes)) == 0) {
            // Hash is the same - compare the source
            BRHederaAddress source1 = hederaTransactionGetSource(t1);
            BRHederaAddress source2 = hederaTransactionGetSource(t2);
            if (1 == hederaAddressEqual(source1, source2)) {
                // OK - compare the target
                BRHederaAddress target1 = hederaTransactionGetTarget(t1);
                BRHederaAddress target2 = hederaTransactionGetTarget(t2);
                if (1 == hederaAddressEqual(target1, target2)) {
                    result = true;
                }
                hederaAddressFree(target1);
                hederaAddressFree(target2);
            }
            hederaAddressFree (source1);
            hederaAddressFree (source2);
        }
    }
    return result;
}

extern BRHederaTimeStamp hederaGenerateTimeStamp(void)
{
    BRHederaTimeStamp ts;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts.seconds = tv.tv_sec;
    ts.nano = tv.tv_usec;
    return ts;
}

BRHederaTimeStamp hederaParseTimeStamp(const char* transactionID)
{
    BRHederaTimeStamp ts;

    // The transaction ID looks like this:
    // block-chain:shard.realm.account-seconds-nanos-index

    // Copy the transactionID and then use strtok to parse
    char * txID = strdup(transactionID);
    const char * searchTokens = ":.";
    strtok(txID, searchTokens); // blockchain-id
    strtok(NULL, searchTokens); // shard
    strtok(NULL, searchTokens); // realm
    char * accountAndTS = strtok(NULL, searchTokens); // account plus rest of the string

    // Now start over and search for the "-" the string now looks like this:
    // account-seconds-nanos-index-whatever
    strtok(accountAndTS, "-"); // Account number
    char * secondsStr = strtok(NULL, "-"); // Seconds
    char * nanosStr = strtok(NULL, "-"); // Nanos

    //sscanf(transactionID, "%s:%lld.%lld.%lld-%lld-%d-", blockchain, &shard, &realm, &account, &seconds, &nanos);
    sscanf(secondsStr, "%lld", &ts.seconds);
    sscanf(nanosStr, "%d", &ts.nano);

    return ts;
}
