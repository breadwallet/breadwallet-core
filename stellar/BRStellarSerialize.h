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
#include "BRArray.h"

#ifdef __cplusplus
extern "C" {
#endif

const int ST_XDR_SUCCESS = 0;
const int ST_XDR_FAILURE = -1;
const int ST_XDR_UNSUPPORTED_OPERATION = -2;
    
extern size_t stellarSerializeTransaction(BRStellarAccountID *accountID,
                                       BRStellarFee fee,
                                       BRStellarSequence sequence,
                                       BRStellarTimeBounds *timeBounds,
                                       int numTimeBounds,
                                       BRStellarMemo *memo,
                                       BRArrayOf(BRStellarOperation) operations,
                                       uint32_t version,
                                       uint8_t *signature,
                                       int signatureLength,
                                       uint8_t **buffer);

bool stellarDeserializeTransaction(BRStellarAccountID *accountID,
                                                   BRStellarFee *fee,
                                                   BRStellarSequence *sequence,
                                                   BRStellarTimeBounds **timeBounds,
                                                   uint32_t *numTimeBounds,
                                                   BRStellarMemo **memo,
                                                   BRArrayOf(BRStellarOperation) *ops,
                                                   int32_t *version,
                                                   uint8_t **signature,
                                                   uint32_t *signatureLength,
                                                   uint8_t *buffer, size_t bufferLength);

int stellarDeserializeResultXDR(uint8_t * result_xdr, size_t result_length, BRArrayOf(BRStellarOperation) *ops,
                                 BRStellarTransactionResult * txResult);

#ifdef __cplusplus
}
#endif

#endif // BRStellar_serialize_h
