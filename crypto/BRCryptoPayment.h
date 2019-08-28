//
//  BRCryptoPayment.h
//  BRCore
//
//  Created by Michael Carrara on 8/27/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BRCryptoPayment_h
#define BRCryptoPayment_h

#include "BRCryptoBase.h"
#include "BRCryptoAddress.h"
#include "BRCryptoAmount.h"
#include "BRCryptoCurrency.h"
#include "BRCryptoNetwork.h"
#include "BRCryptoTransfer.h"

#ifdef __cplusplus
extern "C" {
#endif

    /// Mark: Forward Declarations

    typedef struct BRCryptoPaymentProtocolRequestBitPayBuilderRecord *BRCryptoPaymentProtocolRequestBitPayBuilder;

    typedef struct BRCryptoPaymentProtocolRequestRecord *BRCryptoPaymentProtocolRequest;

    typedef struct BRCryptoPaymentProtocolPaymentRecord *BRCryptoPaymentProtocolPayment;

    typedef struct BRCryptoPaymentProtocolPaymentACKRecord *BRCryptoPaymentProtocolPaymentACK;

    typedef enum {
        CRYPTO_PAYMENT_PROTOCOL_TYPE_BITPAY,
        CRYPTO_PAYMENT_PROTOCOL_TYPE_BIP70,
    } BRCryptoPaymentProtocolType;

    /// Mark: BitPay Payment Protocol Request Builder

    extern BRCryptoPaymentProtocolRequestBitPayBuilder
    cryptoPaymentProtocolRequestBitPayBuilderCreate (BRCryptoNetwork cryptoNetwork,
                                                     BRCryptoCurrency cryptoCurrency,
                                                     const char *network,
                                                     uint64_t time,
                                                     uint64_t expires,
                                                     double feePerByte,
                                                     const char *memo,
                                                     const char *paymentURL,
                                                     const uint8_t *merchantData,
                                                     size_t merchDataLen);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoPaymentProtocolRequestBitPayBuilder, cryptoPaymentProtocolRequestBitPayBuilder);

    extern void
    cryptoPaymentProtocolRequestBitPayBuilderAddOutput(BRCryptoPaymentProtocolRequestBitPayBuilder builder,
                                                       const char *address,
                                                       uint64_t amount);

    extern BRCryptoPaymentProtocolRequest
    cryptoPaymentProtocolRequestBitPayBuilderBuild(BRCryptoPaymentProtocolRequestBitPayBuilder builder);

    /// Mark: Payment Protocol Request

    extern BRCryptoBoolean
    cryptoPaymentProtocolRequestValidateSupported (BRCryptoPaymentProtocolType type,
                                                   BRCryptoNetwork network,
                                                   BRCryptoCurrency currency,
                                                   BRCryptoWallet wallet);

    extern BRCryptoPaymentProtocolRequest
    cryptoPaymentProtocolRequestCreateForBip70 (BRCryptoNetwork cryptoNetwork,
                                                BRCryptoCurrency cryptoCurrency,
                                                uint8_t *serialization,
                                                size_t serializationLen);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoPaymentProtocolRequest, cryptoPaymentProtocolRequest);

    extern BRCryptoPaymentProtocolType
    cryptoPaymentProtocolRequestGetType (BRCryptoPaymentProtocolRequest protoReq);

    extern BRCryptoBoolean
    cryptoPaymentProtocolRequestIsSecure (BRCryptoPaymentProtocolRequest protoReq);

    extern const char *
    cryptoPaymentProtocolRequestGetMemo (BRCryptoPaymentProtocolRequest protoReq);

    extern const char *
    cryptoPaymentProtocolRequestGetPaymentURL (BRCryptoPaymentProtocolRequest protoReq);

    extern BRCryptoAmount
    cryptoPaymentProtocolRequestGetTotalAmount (BRCryptoPaymentProtocolRequest protoReq);

    extern BRCryptoAddress
    cryptoPaymentProtocolRequestGetPrimaryTargetAddress (BRCryptoPaymentProtocolRequest protoReq);

    extern BRCryptoNetworkFee
    cryptoPaymentProtocolRequestGetRequiredNetworkFee (BRCryptoPaymentProtocolRequest protoReq);

    /// Mark: Payment Protocol Payment

    extern BRCryptoPaymentProtocolPayment
    cryptoPaymentProtocolPaymentCreate (BRCryptoPaymentProtocolRequest protoReq,
                                        BRCryptoTransfer transfer,
                                        BRCryptoAddress refundAddress);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoPaymentProtocolPayment, cryptoPaymentProtocolPayment);

    extern uint8_t *
    cryptoPaymentProtocolPaymentEncode(BRCryptoPaymentProtocolPayment protoPay,
                                       size_t *encodedLen);

    /// Mark: Payment Protocol ACK

    extern BRCryptoPaymentProtocolPaymentACK
    cryptoPaymentProtocolPaymentACKCreateForBip70 (uint8_t *serialization,
                                                   size_t serializationLen);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoPaymentProtocolPaymentACK, cryptoPaymentProtocolPaymentACK);

    extern char *
    cryptoPaymentProtocolPaymentACKGetMemo (BRCryptoPaymentProtocolPaymentACK protoAck);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoPayment_h */
