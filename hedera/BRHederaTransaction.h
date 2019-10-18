//
//  BRHederaTransaction.h
//  Core
//
//  Created by Carl Cherry on Oct. 16, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRHederaTransaction_h
#define BRHederaTransaction_h

#include "support/BRInt.h"
#include "support/BRKey.h"
#include "BRHederaAccount.h"
#include "BRHederaBase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BRHederaTransactionRecord *BRHederaTransaction;

/**
 * Create a Hedera transaction
 *
 * @param source - account sending (or that sent) the amount
 * @param target - account receiving the amount
 * @param amount - amount that was (or will be) transferred
 *
 * @return transaction
 */
extern BRHederaTransaction /* caller owns memory and must call "hederaTransactionFree" function */
hederaTransactionCreate(BRHederaAccountID source, BRHederaAccountID target, BRHederaUnitTinyBar amount);

/**
 * Free (destroy) a Hedera transaction object
 *
 * @param transaction
 *
 * @return void
 */
extern void hederaTransactionFree (BRHederaTransaction transaction);

/**
 * Sign a Hedera transaction
 *
 * @param transaction
 * @param public key      - of the source account
 * @param node account ID - for some reason we need include information about
 *                          the node in the request, part of the signing data
 * @param timeStamp       - used to create the transaction id - just use current
 *                          time
 * @param fee             - max number of tinybars the caller is willing to pay
 * @param seed            - seed for this account, used to create private key
 *
 * @return size           - number of bytes in the signed transaction
 */
extern size_t
hederaTransactionSignTransaction (BRHederaTransaction transaction,
                                  BRKey publicKey,
                                  BRHederaAccountID nodeAccountID,
                                  BRHederaTimeStamp timeStamp,
                                  BRHederaUnitTinyBar fee,
                                  UInt512 seed);

/**
 * Get serialiezd bytes for the specified transaction
 *
 * @param transaction
 * @param size         - pointer to variable to hold the size
 *
 * @return bytes       - pointer to serialized bytes
 */
extern uint8_t * /* caller owns and must free using normal "free" function */
hederaTransactionSerialize (BRHederaTransaction transaction, size_t *size);

#ifdef __cplusplus
}
#endif

#endif // BRHederaTransaction_h
