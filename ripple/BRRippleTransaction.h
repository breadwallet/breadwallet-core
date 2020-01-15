//
//  BRRippleTransaction.h
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRipple_transaction_h
#define BRRipple_transaction_h

#include "BRRippleBase.h"
#include "BRRippleFeeBasis.h"
#include "BRKey.h"
#include "support/BRSet.h"
#include "BRRippleAddress.h"

typedef struct BRRippleTransactionRecord *BRRippleTransaction;

/**
 * Create a Ripple transaction
 *
 * NOTE: for this first iteration the only transaction type supported
 * is a Payment, in XRP only. The payment only supports the required fields.
 *
 * @param  sourceAddress  Ripple address of owner account
 * @param  targetAddress  Ripple address of recieving account
 * @param  amount         XRP drop amount to be sent
 * @param  fee            XRP fee in drops
 * @param  publicKey      source account's public key
 *
 * @return transaction    a ripple transaction
 */
extern BRRippleTransaction /* caller must free - rippleTransactionFree */
rippleTransactionCreate(BRRippleAddress sourceAddress,
                        BRRippleAddress targetAddress,
                        BRRippleUnitDrops amount, // For now assume XRP drops.
                        BRRippleFeeBasis feeBasis);

/**
 * Create a Ripple transaction
 *
 * NOTE: for this first iteration the only transaction type supported
 * is a Payment, in XRP only. The payment only supports the required fields.
 *
 * @param  bytes    serialized bytes from the server
 * @param  length   length of above bytes
 *
 * @return transaction    a ripple transaction
 */
extern BRRippleTransaction /* caller must free - rippleTransactionFree */
rippleTransactionCreateFromBytes(uint8_t *bytes, int length);

extern BRRippleTransaction
rippleTransactionClone (BRRippleTransaction transaction);

/**
 * Clean up any memory for this transaction
 *
 * @param transaction  BRRippleTransaction
 */
extern void rippleTransactionFree(BRRippleTransaction transaction);

/**
 * Get a copy of the serialized/signed bytes for a transaction
 *
 * @param  transaction   handle to a serialized/signed transaction
 * @param size           the number of bytes for the newly allocated buffer
 *
 * @return buffer        pointer to buffer (caller must free) with serialized/signed bytes
 *                       or NULL if the transaction has not yet been serialized.
 */
extern uint8_t * // Caller MUST free these bytes
rippleTransactionSerialize(BRRippleTransaction transaction, size_t * bufferSize);

/**
 * Get the hash of a ripple transaction
 *
 * @param  transaction   a valid ripple transaction
 * @return hash          a BRRippleTransactionHash object
 */
extern BRRippleTransactionHash rippleTransactionGetHash(BRRippleTransaction transaction);

/**
 * Get the Account Txn ID hash (if applicable)
 *
 * @param  transaction   a valid ripple transaction
 * @return hash          a BRRippleTransactionHash object
 */
extern BRRippleTransactionHash rippleTransactionGetAccountTxnId(BRRippleTransaction transaction);

/**
 * Get the Fee basis for this transaction.
 *
 * @param  transaction   a valid ripple transaction
 * @return feeBasis      the base fee that will be used when serializing the transaction
 *                       NOTE: the actual tx fee might be higher depending on the tx type
 */
extern BRRippleFeeBasis rippleTransactionGetFeeBasis(BRRippleTransaction transaction);

/**
 * Various getter methods for the transaction
 */
extern BRRippleTransactionType rippleTransactionGetType(BRRippleTransaction transaction);
extern BRRippleUnitDrops rippleTransactionGetFee(BRRippleTransaction transaction);
extern BRRippleUnitDrops rippleTransactionGetAmount(BRRippleTransaction transaction);
extern BRRippleSequence rippleTransactionGetSequence(BRRippleTransaction transaction);
extern BRRippleFlags rippleTransactionGetFlags(BRRippleTransaction transaction);
extern BRRippleLastLedgerSequence rippleTransactionGetLastLedgerSequence(BRRippleTransaction transaction);

extern BRRippleAddress // caller owns object, must free with rippleAddressFree
rippleTransactionGetSource(BRRippleTransaction transaction);
extern BRRippleAddress // caller owns object, must free with rippleAddressFree
rippleTransactionGetTarget(BRRippleTransaction transaction);

extern BRKey rippleTransactionGetPublicKey(BRRippleTransaction transaction);

extern UInt256 rippleTransactionGetInvoiceID(BRRippleTransaction transaction);
extern BRRippleSourceTag rippleTransactionGetSourceTag(BRRippleTransaction transaction);
extern BRRippleDestinationTag rippleTransactionGetDestinationTag(BRRippleTransaction transaction);

extern BRRippleAmount rippleTransactionGetAmountRaw(BRRippleTransaction transaction, BRRippleAmountType amountType);

extern BRSetOf(BRRippleTransaction) rippleTransactionSetCreate (size_t initialSize);

/// Transaction Attribute
extern size_t rippleTransactionFieldRequiredCount;
extern const char **rippleTransactionFieldRequiredNames;

extern size_t rippleTransactionFieldOptionalCount;
extern const char *rippleTransactionFieldOptionalNames[];

extern void rippleTransactionSetDestinationTag (BRRippleTransaction transaction, BRRippleDestinationTag tag);
extern void rippleTransactionSetInvoiceID (BRRippleTransaction transaction, UInt256 invoiceId);
#endif
