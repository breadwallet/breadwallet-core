//
//  BRCryptoPayment.h
//  BRCore
//
//  Created by Michael Carrara on 8/27/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

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

    typedef enum {
        CRYPTO_PAYMENT_PROTOCOL_ERROR_NONE,
        CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_MISSING,
        CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_NOT_TRUSTED,
        CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_TYPE_NOT_SUPPORTED,
        CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_VERIFICATION_FAILED,
        CRYPTO_PAYMENT_PROTOCOL_ERROR_EXPIRED
    } BRCryptoPaymentProtocolError;

    typedef void * BRCryptoPayProtReqContext;

    typedef char * (*BRCryptoPayProtReqBitPayAndBip70CommonNameExtractor) (BRCryptoPaymentProtocolRequest request,
                                                                           BRCryptoPayProtReqContext context,
                                                                           const char *pkiType,
                                                                           uint8_t *certBytes[],
                                                                           size_t certLengths[],
                                                                           size_t certCount);

    typedef BRCryptoPaymentProtocolError (*BRCryptoPayProtReqBitPayAndBip70Validator) (BRCryptoPaymentProtocolRequest request,
                                                                                       BRCryptoPayProtReqContext context,
                                                                                       const char *pkiType,
                                                                                       uint64_t expires,
                                                                                       uint8_t *certBytes[],
                                                                                       size_t certLengths[],
                                                                                       size_t certCount,
                                                                                       const uint8_t *digest,
                                                                                       size_t digestLength,
                                                                                       const uint8_t *signature,
                                                                                       size_t signatureLength);


    typedef struct {
        BRCryptoPayProtReqContext context;
        BRCryptoPayProtReqBitPayAndBip70Validator validator;
        BRCryptoPayProtReqBitPayAndBip70CommonNameExtractor nameExtractor;
    } BRCryptoPayProtReqBitPayAndBip70Callbacks;

    typedef BRCryptoPayProtReqBitPayAndBip70Callbacks BRCryptoPayProtReqBitPayCallbacks;
    typedef BRCryptoPayProtReqBitPayAndBip70Callbacks BRCryptoPayProtReqBip70Callbacks;

    /// Mark: BitPay Payment Protocol Request Builder

    extern BRCryptoPaymentProtocolRequestBitPayBuilder
    cryptoPaymentProtocolRequestBitPayBuilderCreate (BRCryptoNetwork cryptoNetwork,
                                                     BRCryptoCurrency cryptoCurrency,
                                                     BRCryptoPayProtReqBitPayCallbacks callbacks,
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
                                                BRCryptoPayProtReqBip70Callbacks callbacks,
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

    extern BRCryptoNetworkFee
    cryptoPaymentProtocolRequestGetRequiredNetworkFee (BRCryptoPaymentProtocolRequest protoReq);

    extern BRCryptoAddress
    cryptoPaymentProtocolRequestGetPrimaryTargetAddress (BRCryptoPaymentProtocolRequest protoReq);

    // If the return value is not NULL, it must be deallocated using free()
    extern char *
    cryptoPaymentProtocolRequestGetCommonName (BRCryptoPaymentProtocolRequest protoReq);

    extern BRCryptoPaymentProtocolError
    cryptoPaymentProtocolRequestIsValid(BRCryptoPaymentProtocolRequest protoReq);

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

    extern const char *
    cryptoPaymentProtocolPaymentACKGetMemo (BRCryptoPaymentProtocolPaymentACK protoAck);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoPayment_h */
