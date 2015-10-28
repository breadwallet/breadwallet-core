//
//  BRPaymentProtocol.c
//
//  Created by Aaron Voisine on 9/7/15.
//  Copyright (c) 2015 breadwallet LLC
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

#include "BRPaymentProtocol.h"
#include "BRHash.h"
#include <string.h>

// BIP70 payment protocol: https://github.com/bitcoin/bips/blob/master/bip-0070.mediawiki

#define PROTOBUF_VARINT   0 // int32, int64, uint32, uint64, sint32, sint64, bool, enum
#define PROTOBUF_64BIT    1 // fixed64, sfixed64, double
#define PROTOBUF_LENDELIM 2 // string, bytes, embedded messages, packed repeated fields
#define PROTOBUF_32BIT    5 // fixed32, sfixed32, float

static uint64_t ProtoBufVarInt(const uint8_t *buf, size_t len, size_t *off)
{
    uint64_t varInt = 0;
    uint8_t b = 0x80;
    size_t i = 0;
    
    while ((b & 0x80) && buf && *off < len) {
        b = buf[(*off)++];
        varInt += (uint64_t)(b & 0x7f) << 7*i++;
    }
    
    return (b & 0x80) ? 0 : varInt;
}

static void ProtoBufSetVarInt(uint8_t *buf, size_t len, uint64_t i, size_t *off)
{
    uint8_t b;
    
    do {
        b = i & 0x7f;
        i >>= 7;
        if (i > 0) b |= 0x80;
        if (buf && *off + 1 <= len) buf[*off] = b;
        (*off)++;
    } while (i > 0);
}

static const uint8_t *ProtoBufLenDelim(const uint8_t *buf, size_t *len, size_t *off)
{
    const uint8_t *data = NULL;
    size_t dataLen = ProtoBufVarInt(buf, *len, off);
    
    if (buf && *off + dataLen <= *len) data = buf + *off;
    *off += dataLen;
    *len = dataLen;
    return data;
}

static void ProtoBufSetLenDelim(uint8_t *buf, size_t len, const void *data, size_t dataLen, size_t *off)
{
    ProtoBufSetVarInt(buf, len, dataLen, off);
    if (buf && *off + dataLen <= len) memcpy(buf + *off, data, dataLen);
    *off += dataLen;
}

// sets either i or data depending on field type, and returns field key
static uint64_t ProtoBufField(uint64_t *i, const uint8_t **data, const uint8_t *buf, size_t *len, size_t *off)
{
    uint64_t key = ProtoBufVarInt(buf, *len, off);
    uint64_t varInt = 0;
    const uint8_t *lenDelim = NULL;
    
    switch (key & 0x07) {
        case PROTOBUF_VARINT: varInt = ProtoBufVarInt(buf, *len, off); if (i) *i = varInt; break;
        case PROTOBUF_64BIT: *off += sizeof(uint64_t); break; // not used by payment protocol
        case PROTOBUF_LENDELIM: lenDelim = ProtoBufLenDelim(buf, len, off); if (data) *data = lenDelim; break;
        case PROTOBUF_32BIT: *off += sizeof(uint32_t); break; // not used by payment protocol
        default: break;
    }
    
    return key >> 3;
}

static void ProtoBufString(char **str, const void *data, size_t len)
{
    if (data) {
        if (! *str) array_new(*str, len + 1);
        array_clear(*str);
        array_add_array(*str, data, len);
        array_add(*str, '\0');
    }
}

static void ProtoBufSetString(uint8_t *buf, size_t len, const char *str, uint64_t key, size_t *off)
{
    size_t strLen = strlen(str);
    
    ProtoBufSetVarInt(buf, len, (key << 3) | PROTOBUF_LENDELIM, off);
    ProtoBufSetLenDelim(buf, len, str, strLen, off);
}

static size_t ProtoBufBytes(uint8_t **bytes, const void *data, size_t len)
{
    if (data) {
        if (! *bytes) array_new(*bytes, len);
        array_clear(*bytes);
        array_add_array(*bytes, data, len);
    }
    
    return (*bytes) ? array_count(*bytes) : 0;
}

static void ProtoBufSetBytes(uint8_t *buf, size_t len, const uint8_t *bytes, size_t bytesLen, uint64_t key, size_t *off)
{
    ProtoBufSetVarInt(buf, len, (key << 3) | PROTOBUF_LENDELIM, off);
    ProtoBufSetLenDelim(buf, len, bytes, bytesLen, off);
}

static void ProtoBufSetInt(uint8_t *buf, size_t len, uint64_t i, uint64_t key, size_t *off)
{
    ProtoBufSetVarInt(buf, len, (key << 3) | PROTOBUF_VARINT, off);
    ProtoBufSetVarInt(buf, len, i, off);
}

#define proto_buf_is_default(array, key) (\
    ((key) < (array_capacity(array) - array_count(array))*sizeof(*(array))) ?\
        ((uint8_t *)&(array)[array_count(array)])[(key)] : 0\
)

#define proto_buf_set_default(array, key) do {\
    if ((array_capacity(array) - array_count(array))*sizeof(*(array)) <= (key))\
        array_set_capacity((array), array_count(array) + 1 + ((key) + 1)/sizeof(*(array)));\
    ((uint8_t *)&(array)[array_count(array)])[(key)] = 1;\
} while(0)

typedef enum {
    output_amount = 1,
    output_script = 2
} output_key;

typedef enum {
    details_network = 1,
    details_outputs = 2,
    details_time = 3,
    details_expires = 4,
    details_memo = 5,
    details_payment_url = 6,
    details_merchant_data = 7
} details_key;

typedef enum {
    request_version = 1,
    request_pki_type = 2,
    request_pki_data = 3,
    request_details = 4,
    request_signature = 5
} request_key;

typedef enum {
    certificates_cert = 1
} certificates_key;

typedef enum {
    payment_merchant_data = 1,
    payment_transactions = 2,
    payment_refund_to = 3,
    payment_memo = 4
} payment_key;

typedef enum {
    ack_payment = 1,
    ack_memo = 2
} ack_key;

static void BRPaymentProtocolOutputParse(BRTxOutput *output, const uint8_t *buf, size_t len)
{
    size_t off = 0;
    
    output->amount = UINT64_MAX;
    
    while (off < len) {
        uint64_t i = 0;
        const uint8_t *data = NULL;
        size_t dataLen = len;
        
        switch (ProtoBufField(&i, &data, buf, &dataLen, &off)) {
            case output_amount: output->amount = i; break;
            case output_script: BRTxOutputSetScript(output, data, dataLen); break;
            default: break;
        }
    }
    
    if (! output->script) {
        array_new(output->script, 0);
        proto_buf_set_default(output->script, output_script);
    }
    
    if (output->amount == UINT64_MAX) {
        output->amount = 0;
        proto_buf_set_default(output->script, output_amount);
    }
}

static size_t BRPaymentProtocolOutputSerialize(BRTxOutput *output, uint8_t *buf, size_t len)
{
    size_t off = 0;
    
    if (output->script && ! proto_buf_is_default(output->script, output_amount)) {
        ProtoBufSetInt(buf, len, output->amount, output_amount, &off);
    }
    
    if (output->script && ! proto_buf_is_default(output->script, output_script)) {
        ProtoBufSetBytes(buf, len, output->script, output->scriptLen, output_script, &off);
    }
    
    return (! buf || off <= len) ? off : 0;
}

// buf must contain a serialized details struct, result must be freed by calling BRPaymentProtocolDetailsFree()
BRPaymentProtocolDetails *BRPaymentProtocolDetailsParse(const uint8_t *buf, size_t len)
{
    BRPaymentProtocolDetails *details = calloc(1, sizeof(BRPaymentProtocolDetails));
    size_t off = 0;

    array_new(details->outputs, 1);
    
    while (off < len) {
        BRTxOutput output = { "", 0, NULL, 0 };
        uint64_t i = 0;
        const uint8_t *data = NULL;
        size_t dataLen = len;

        switch (ProtoBufField(&i, &data, buf, &dataLen, &off)) {
            case details_network: ProtoBufString(&details->network, data, dataLen); break;
            case details_outputs: BRPaymentProtocolOutputParse(&output, data, dataLen); break;
            case details_time: details->time = i; break;
            case details_expires: details->expires = i; break;
            case details_memo: ProtoBufString(&details->memo, data, dataLen); break;
            case details_payment_url: ProtoBufString(&details->paymentURL, data, dataLen); break;
            case details_merchant_data: details->merchDataLen = ProtoBufBytes(&details->merchantData, data, dataLen);
            default: break;
        }

        if (output.script) array_add(details->outputs, output);
    }
    
    if (! details->network) {
        ProtoBufString(&details->network, "main", strlen("main"));
        proto_buf_set_default(details->network, details_network);
    }
    
    details->outputsCount = array_count(details->outputs);
    
    if (details->outputsCount == 0) { // one or more outputs required
        BRPaymentProtocolDetailsFree(details);
        details = NULL;
    }
    
    return details;
}

// writes serialized details struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolDetailsSerialize(BRPaymentProtocolDetails *details, uint8_t *buf, size_t len)
{
    size_t off = 0;
    
    if (details->network && ! proto_buf_is_default(details->network, details_network)) {
        ProtoBufSetString(buf, len, details->network, details_network, &off);
    }
    
    for (size_t i = 0; i < details->outputsCount; i++) {
        uint8_t outputBuf[BRPaymentProtocolOutputSerialize(&details->outputs[i], NULL, 0)];
        size_t outputLen = BRPaymentProtocolOutputSerialize(&details->outputs[i], outputBuf, sizeof(outputBuf));

        ProtoBufSetBytes(buf, len, outputBuf, outputLen, details_outputs, &off);
    }

    if (details->time > 0) ProtoBufSetInt(buf, len, details->time, details_time, &off);
    if (details->expires > 0) ProtoBufSetInt(buf, len, details->expires, details_expires, &off);
    if (details->memo) ProtoBufSetString(buf, len, details->memo, details_memo, &off);
    if (details->paymentURL) ProtoBufSetString(buf, len, details->paymentURL, details_payment_url, &off);
    if (details->merchantData) ProtoBufSetBytes(buf, len, details->merchantData, details->merchDataLen,
                                                details_merchant_data, &off);

    return (! buf || off <= len) ? off : 0;
}

// frees memory allocated for details struct
void BRPaymentProtocolDetailsFree(BRPaymentProtocolDetails *details)
{
    if (details->network) free(details->network);

    for (size_t i = 0; i < details->outputsCount; i++) {
        BRTxOutputSetScript(&details->outputs[i], NULL, 0);
    }
    
    if (details->outputs) array_free(details->outputs);
    if (details->memo) free(details->memo);
    if (details->paymentURL) free(details->paymentURL);
    if (details->merchantData) free(details->merchantData);
    free(details);
}

// buf must contain a serialized request struct, result must be freed by calling BRPaymentProtocolRequestFree()
BRPaymentProtocolRequest *BRPaymentProtocolRequestParse(const uint8_t *buf, size_t len)
{
    BRPaymentProtocolRequest *request = calloc(1, sizeof(BRPaymentProtocolRequest));
    size_t off = 0;
    
    request->version = UINT32_MAX;
    
    while (off < len) {
        uint64_t i = 0;
        const uint8_t *data = NULL;
        size_t dataLen = len;
        
        switch (ProtoBufField(&i, &data, buf, &dataLen, &off)) {
            case request_version: request->version = (uint32_t)i; break;
            case request_pki_type: ProtoBufString(&request->pkiType, data, dataLen); break;
            case request_pki_data: request->pkiLen = ProtoBufBytes(&request->pkiData, data, dataLen); break;
            case request_details: request->details = BRPaymentProtocolDetailsParse(data, dataLen); break;
            case request_signature: request->sigLen = ProtoBufBytes(&request->signature, data, dataLen); break;
            default: break;
        }
    }
    
    if (! request->pkiType) {
        ProtoBufString(&request->pkiType, "none", strlen("none"));
        proto_buf_set_default(request->pkiType, request_pki_type);
    }
    
    if (request->version == UINT32_MAX) {
        request->version = 1;
        proto_buf_set_default(request->pkiType, request_version);
    }
    
    if (! request->details) { // required
        BRPaymentProtocolRequestFree(request);
        request = NULL;
    }

    return request;
}

// writes serialized request struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolRequestSerialize(BRPaymentProtocolRequest *request, uint8_t *buf, size_t len)
{
    size_t off = 0;

    if (request->pkiType && ! proto_buf_is_default(request->pkiType, request_version)) {
        ProtoBufSetInt(buf, len, request->version, request_version, &off);
    }
    
    if (request->pkiType && ! proto_buf_is_default(request->pkiType, request_pki_type)) {
        ProtoBufSetString(buf, len, request->pkiType, request_pki_type, &off);
    }
    
    if (request->pkiData) ProtoBufSetBytes(buf, len, request->pkiData, request->pkiLen, request_pki_data, &off);

    if (request->details) {
        uint8_t detailsBuf[BRPaymentProtocolDetailsSerialize(request->details, NULL, 0)];
        size_t detailsLen = BRPaymentProtocolDetailsSerialize(request->details, detailsBuf, sizeof(detailsBuf));

        ProtoBufSetBytes(buf, len, detailsBuf, detailsLen, request_details, &off);
    }
    
    if (request->signature) ProtoBufSetBytes(buf, len, request->signature, request->sigLen, request_signature, &off);

    return (! buf || off <= len) ? off : 0;
}

// writes the DER encoded certificate corresponding to index to cert, returns the number of bytes written to cert, or
// the total certLen needed if cert is NULL, returns 0 if index of out-of-bounds
size_t BRPaymentProtocolRequestCert(BRPaymentProtocolRequest *request, uint8_t *cert, size_t certLen, size_t index)
{
    size_t off = 0;
    
    while (request->pkiData && off < request->pkiLen) {
        uint64_t i = 0;
        const uint8_t *data = NULL;
        size_t dataLen = request->pkiLen;
        
        if (ProtoBufField(&i, &data, request->pkiData, &dataLen, &off) == certificates_cert && data) {
            if (index == 0) {
                if (cert && dataLen <= certLen) memcpy(cert, data, dataLen);
                break;
            }
            else index--;
        }
    }
    
    return (! cert || off <= certLen) ? off : 0;
}

// writes the hash of the request to md needed to sign or verify the request, returns the number of bytes written, or
// the total bytes needed if md is NULL
size_t BRPaymentProtocolRequestDigest(BRPaymentProtocolRequest *request, uint8_t *md, size_t mdLen)
{
    request->sigLen = 0; // set signature to 0 bytes, a signature can't sign itself
    
    uint8_t buf[BRPaymentProtocolRequestSerialize(request, NULL, 0)];
    size_t len = BRPaymentProtocolRequestSerialize(request, buf, sizeof(buf));
    
    if (request->pkiType && strncmp(request->pkiType, "x509+sha256", strlen("x509+sha256") + 1) == 0) {
        if (256/8 <= mdLen) BRSHA256(md, buf, len);
        len = 256/8;
    }
    else if (request->pkiType && strncmp(request->pkiType, "x509+sha1", strlen("x509+sha1") + 1) == 0) {
        if (160/8 <= mdLen) BRSHA1(md, buf, len);
        len = 160/8;
    }
    else len = 0;
    
    if (request->signature) request->sigLen = array_count(request->signature);
    return (! md || len <= mdLen) ? len : 0;
}

// frees memory allocated for request struct
void BRPaymentProtocolRequestFree(BRPaymentProtocolRequest *request)
{
    if (request->pkiType) array_free(request->pkiType);
    if (request->pkiData) array_free(request->pkiData);
    if (request->details) BRPaymentProtocolDetailsFree(request->details);
    if (request->signature) array_free(request->signature);
    free(request);
}

// returns a newly allocated BRPaymentProtocolPayment struct that must be freed with BRPaymentProtocolPaymentFree()
BRPaymentProtocolPayment *BRPaymentProtocolPaymentNew(const uint8_t *merchantData, size_t merchDataLen,
                                                      const BRTransaction *transactions[], size_t txCount,
                                                      const uint64_t refundToAmounts[],
                                                      const BRAddress refundToAddresses[], size_t refundToCount,
                                                      const char *memo)
{
    BRPaymentProtocolPayment *payment = calloc(1, sizeof(BRPaymentProtocolPayment));
    
    if (merchantData) {
        array_new(payment->merchantData, merchDataLen);
        array_add_array(payment->merchantData, merchantData, merchDataLen);
        payment->merchDataLen = merchDataLen;
    }
    
    if (transactions) {
        array_new(payment->transactions, txCount);
        array_add_array(payment->transactions, transactions, txCount);
        payment->txCount = txCount;
    }
    
    if (refundToAddresses) {
        array_new(payment->refundTo, refundToCount);
        
        for (size_t i = 0; i < refundToCount; i++) {
            BRTxOutput output = { "", 0, NULL, 0 };
            
            BRTxOutputSetAddress(&output, refundToAddresses[i].s);
            if (refundToAmounts) output.amount = refundToAmounts[i];
            array_add(payment->refundTo, output);
        }
        
        payment->refundToCount = refundToCount;
    }
    
    if (memo) {
        array_new(payment->memo, strlen(memo) + 1);
        array_add_array(payment->memo, memo, strlen(memo) + 1);
    }
    
    return payment;
}

// buf must contain a serialized payment struct, result must be freed by calling BRPaymentProtocolPaymentFree()
BRPaymentProtocolPayment *BRPaymentProtocolPaymentParse(const uint8_t *buf, size_t len)
{
    BRPaymentProtocolPayment *payment = calloc(1, sizeof(BRPaymentProtocolPayment));
    size_t off = 0;
    
    array_new(payment->transactions, 1);
    array_new(payment->refundTo, 1);
    
    while (off < len) {
        BRTransaction *tx = NULL;
        BRTxOutput output = { "", 0, NULL, 0 };
        uint64_t i = 0;
        const uint8_t *data = NULL;
        size_t dataLen = len;
        
        switch (ProtoBufField(&i, &data, buf, &dataLen, &off)) {
            case payment_transactions: tx = BRTransactionParse(data, dataLen); break;
            case payment_refund_to: BRPaymentProtocolOutputParse(&output, data, dataLen); break;
            case payment_memo: ProtoBufString(&payment->memo, data, dataLen); break;
            case payment_merchant_data: payment->merchDataLen = ProtoBufBytes(&payment->merchantData, data, dataLen);
            default: break;
        }
        
        if (tx) array_add(payment->transactions, tx);
        if (output.script) array_add(payment->refundTo, output);
    }
    
    payment->txCount = array_count(payment->transactions);
    payment->refundToCount = array_count(payment->refundTo);
    
    return payment;
}

// writes serialized payment struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolPaymentSerialize(BRPaymentProtocolPayment *payment, uint8_t *buf, size_t len)
{
    size_t off = 0;

    if (payment->merchantData) {
        ProtoBufSetBytes(buf, len, payment->merchantData, payment->merchDataLen, payment_merchant_data, &off);
    }

    for (size_t i = 0; i < payment->txCount; i++) {
        uint8_t txBuf[BRTransactionSerialize(payment->transactions[i], NULL, 0)];
        size_t txLen = BRTransactionSerialize(payment->transactions[i], txBuf, sizeof(txBuf));

        ProtoBufSetBytes(buf, len, txBuf, txLen, payment_transactions, &off);
    }

    for (size_t i = 0; i < payment->refundToCount; i++) {
        uint8_t outputBuf[BRPaymentProtocolOutputSerialize(&payment->refundTo[i], NULL, 0)];
        size_t outputLen = BRPaymentProtocolOutputSerialize(&payment->refundTo[i], outputBuf, sizeof(outputBuf));
        
        ProtoBufSetBytes(buf, len, outputBuf, outputLen, payment_refund_to, &off);
    }

    if (payment->memo) ProtoBufSetString(buf, len, payment->memo, payment_memo, &off);

    return (! buf || off <= len) ? off : 0;
}

// frees memory allocated for payment struct (does not call BRTransactionFree() on transactions)
void BRPaymentProtocolPaymentFree(BRPaymentProtocolPayment *payment)
{
    if (payment->merchantData) array_free(payment->merchantData);
    if (payment->transactions) array_free(payment->transactions);
}

// buf must contain a serialized ACK struct, result must be freed by calling BRPaymentProtocolACKFree()
BRPaymentProtocolACK *BRPaymentProtocolACKParse(const uint8_t *buf, size_t len)
{
    BRPaymentProtocolACK *ack = calloc(1, sizeof(BRPaymentProtocolACK));
    size_t off = 0;
    
    while (off < len) {
        uint64_t i = 0;
        const uint8_t *data = NULL;
        size_t dataLen = len;
        
        switch (ProtoBufField(&i, &data, buf, &dataLen, &off)) {
            case ack_payment: ack->payment = BRPaymentProtocolPaymentParse(data, dataLen); break;
            case ack_memo: ProtoBufString(&ack->memo, data, dataLen); break;
            default: break;
        }
    }
    
    if (! ack->payment) { // required
        BRPaymentProtocolACKFree(ack);
        ack = NULL;
    }
    
    return ack;
}

// writes serialized ACK struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolACKSerialize(BRPaymentProtocolACK *ack, uint8_t *buf, size_t len)
{
    size_t off = 0;
    
    if (ack->payment) {
        uint8_t paymentBuf[BRPaymentProtocolPaymentSerialize(ack->payment, NULL, 0)];
        size_t paymentLen = BRPaymentProtocolPaymentSerialize(ack->payment, paymentBuf, sizeof(paymentBuf));
        
        ProtoBufSetBytes(buf, len, paymentBuf, paymentLen, ack_payment, &off);
    }
    
    if (ack->memo) ProtoBufSetString(buf, len, ack->memo, ack_memo, &off);
    
    return (! buf || off <= len) ? off : 0;
}

// frees memory allocated for ACK struct
void BRPaymentProtocolACKFree(BRPaymentProtocolACK *ack)
{
    if (ack->payment) BRPaymentProtocolPaymentFree(ack->payment);
    if (ack->memo) array_free(ack->memo);
    free(ack);
}
