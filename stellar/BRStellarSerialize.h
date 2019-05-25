//
//  BRStellarSerialize.h
//  Core
//
//  Created by Carl Cherry on 5/21/2019
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRStellar_serialize_h
#define BRStellar_serialize_h

#include "BRStellarBase.h"
#include "BRStellarPrivateStructs.h"
#include "BRStellarTransaction.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int stellarSerializeTransaction(BRStellarAccountID accountID,
                                       BRStellarFee fee,
                                           BRStellarSequence sequence,
                                           BRStellarTimeBounds *timeBounds,
                                           int numTimeBounds,
                                           BRStellarMemo *memo,
                                           BRStellarOperation * operations,
                                           int numOperations,
                                           uint8_t *signature,
                                           size_t signatureLength,
                                           uint8_t *buffer, size_t bufferSize);


#ifdef __cplusplus
}
#endif
#endif // BRStellar_serialize_h
