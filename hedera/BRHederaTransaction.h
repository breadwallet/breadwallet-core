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
#include "BRHederaAddress.h"
#include "BRHederaBase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BRHederaTransactionRecord *BRHederaTransaction;

/**
 * Create a new Hedera transaction for sending a transfer
 *
 * @param source - account sending (or that sent) the amount
 * @param target - account receiving the amount
 * @param amount - amount that was (or will be) transferred
 *
 * @return transaction
 */
extern BRHederaTransaction /* caller owns memory and must call "hederaTransactionFree" function */
hederaTransactionCreateNew(BRHederaAddress source, BRHederaAddress target, BRHederaUnitTinyBar amount);

/**
 * Create a Hedera transaction recovered from the blockset server
 *
 * @param source    - account that sent the amount
 * @param target    - account received the amount
 * @param amount    - amount that was transferred
 * @param txID      - transaction ID for this transfer
 *
 * @return transaction
 */
extern BRHederaTransaction /* caller must free - hederaTransactionFree */
hederaTransactionCreate(BRHederaAddress source, BRHederaAddress target,
                        BRHederaUnitTinyBar amount,
                        const char *txID,
                        BRHederaTransactionHash hash);

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
                                  BRHederaAddress nodeAddress,
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

// Getters for the various transaction fields
extern BRHederaTransactionHash hederaTransactionGetHash(BRHederaTransaction transaction);
extern char * // Caller owns memory and must free calling "free"
hederaTransactionGetTransactionId(BRHederaTransaction transaction);
extern BRHederaUnitTinyBar hederaTransactionGetFee(BRHederaTransaction transaction);
extern BRHederaUnitTinyBar hederaTransactionGetAmount(BRHederaTransaction transaction);
extern BRHederaAddress hederaTransactionGetSource(BRHederaTransaction transaction);
extern BRHederaAddress hederaTransactionGetTarget(BRHederaTransaction transaction);

#ifdef __cplusplus
}
#endif

#endif // BRHederaTransaction_h
