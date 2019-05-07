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

typedef struct BRRippleTransactionRecord *BRRippleTransaction;
typedef struct BRRippleSerializedTransactionRecord *BRRippleSerializedTransaction;

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
extern BRRippleTransaction
rippleTransactionCreate(BRRippleAddress sourceAddress,
                        BRRippleAddress targetAddress,
                        uint64_t amount, // For now assume XRP drops.
                        uint64_t fee);

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
extern BRRippleTransaction
rippleTransactionCreateFromBytes(uint8_t *bytes, int length);

/**
 * Delete a Ripple transaction
 *
 * @param transaction  BRRippleTransaction
 */
extern void deleteRippleTransaction(BRRippleTransaction transaction);

/**
 * Get the size of a serialized transaction
 *
 * @param  s     serialized transaction
 * @return size
 */
extern uint32_t getSerializedSize(BRRippleSerializedTransaction s);

/**
 * Get the raw bytes of a serialized transaction
 *
 * @param  s     serialized transaction
 * @return bytes uint8_t
 */
extern uint8_t* getSerializedBytes(BRRippleSerializedTransaction s);

/**
 * Get the hash of a ripple transaction
 *
 * @param  transaction   a valid ripple transaction
 * @return hash          a BRRippleTransactionHash object
 */
extern BRRippleTransactionHash rippleTransactionGetHash(BRRippleTransaction transaction);

/**
 * Various getter methods for the transaction
 *
 */
extern uint16_t rippleTransactionGetType(BRRippleTransaction transaction);
extern uint64_t rippleTransactionGetFee(BRRippleTransaction transaction);
extern uint64_t rippleTransactionGetAmount(BRRippleTransaction transaction);
extern uint32_t rippleTransactionGetSequence(BRRippleTransaction transaction);
extern uint32_t rippleTransactionGetFlags(BRRippleTransaction transaction);
extern BRRippleAddress rippleTransactionGetSource(BRRippleTransaction transaction);
extern BRRippleAddress rippleTransactionGetTarget(BRRippleTransaction transaction);
extern BRKey rippleTransactionGetPublicKey(BRRippleTransaction transaction);

#endif
