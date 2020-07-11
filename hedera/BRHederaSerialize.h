//
//  BRHederaSerialize.h
//
//  Created by Carl Cherry on Oct. 17, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRHederaSerialize_h
#define BRHederaSerialize_h

#include "BRHederaBase.h"
#include "BRHederaAddress.h"
#include "support/BRKey.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t * hederaTransactionBodyPack (BRHederaAddress source,
                                     BRHederaAddress target,
                                          BRHederaAddress nodeAddress,
                                          BRHederaUnitTinyBar amount,
                                          BRHederaTimeStamp timeStamp,
                                          BRHederaUnitTinyBar fee,
                                          const char * memo,
                                          size_t *size);

uint8_t * hederaTransactionPack (uint8_t * signature, size_t signatureSize,
                                      uint8_t * publicKey, size_t publicKeySize,
                                      uint8_t * body, size_t bodySize,
                                      size_t * serializedSize);

#ifdef __cplusplus
}
#endif

#endif // BRHederaSerialize_h
