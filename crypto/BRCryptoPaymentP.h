//
//  BRCryptoPaymentP.h
//  BRCore
//
//  Created by Ed Gamble on 12/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoPaymentP_h
#define BRCryptoPaymentP_h

#include "BRCryptoPayment.h"

#ifdef __cplusplus
extern "C" {
#endif

private_extern BRArrayOf(BRTxOutput)
cryptoPaymentProtocolRequestGetOutputsAsBTC (BRCryptoPaymentProtocolRequest request);

#ifdef __cplusplus
}
#endif



#endif /* BRCryptoPaymentP_h */
