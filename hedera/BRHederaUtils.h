//
//  BRHederaUtils.h
//  Core
//
//  Created by Carl Cherry on Oct. 16, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRHederaUtils_h
#define BRHederaUtils_h

#include "BRHederaBase.h"

#ifdef __cplusplus
extern "C" {
#endif

int hederaAddressGetString(BRHederaAddress address, char * addressString, int length);

BRHederaTransactionId hederaParseTransactionId (const char * txId);

#ifdef __cplusplus
}
#endif

#endif // BRHederaUtils_h
