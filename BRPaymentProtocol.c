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
#include "BRArray.h"
#include <string.h>
#include <stdint.h>

// BIP70 payment protocol: https://github.com/bitcoin/bips/blob/master/bip-0070.mediawiki

#define PROTOBUF_VARINT   0 // int32, int64, uint32, uint64, sint32, sint64, bool, enum
#define PROTOBUF_64BIT    1 // fixed64, sfixed64, double
#define PROTOBUF_LENDELIM 2 // string, bytes, embedded messages, packed repeated fields
#define PROTOBUF_32BIT    5 // fixed32, sfixed32, float

static uint64_t _ProtoBufVarInt(const uint8_t *buf, size_t bufLen, size_t *off)
{
    uint64_t varInt = 0;
    uint8_t b = 0x80;
    size_t i = 0;
    
    while ((b & 0x80) && buf && *off < bufLen) {
        b = buf[(*off)++];
        varInt += (uint64_t)(b & 0x7f) << 7*i++;
    }
    
    return (b & 0x80) ? 0 : varInt;
}

static void _ProtoBufSetVarInt(uint8_t *buf, size_t bufLen, uint64_t i, size_t *off)
{
    uint8_t b;
    
    do {
        b = i & 0x7f;
        i >>= 7;
        if (i > 0) b |= 0x80;
        if (buf && *off + 1 <= bufLen) buf[*off] = b;
        (*off)++;
    } while (i > 0);
}

static const uint8_t *_ProtoBufLenDelim(const uint8_t *buf, size_t *len, size_t *off)
{
    const uint8_t *data = NULL;
    size_t dataLen = _ProtoBufVarInt(buf, *len, off);
    
    if (buf && *off + dataLen <= *len) data = &buf[*off];
    *off += dataLen;
    *len = dataLen;
    return data;
}

static void _ProtoBufSetLenDelim(uint8_t *buf, size_t bufLen, const void *data, size_t dataLen, size_t *off)
{
    if (data) {
        _ProtoBufSetVarInt(buf, bufLen, dataLen, off);
        if (buf && *off + dataLen <= bufLen) memcpy(&buf[*off], data, dataLen);
        *off += dataLen;
    }
}

// the following fixed int functions are not used by payment protocol, and only work for parsing/serializing unknown
// fields - the values returned or set are unconverted raw byte values
static uint64_t _ProtoBufFixed(const uint8_t *buf, size_t bufLen, size_t *off, size_t size)
{
    uint64_t i = 0;
    
    if (buf && *off + size <= bufLen && size <= sizeof(i)) memcpy(&i, &buf[*off], size);
    *off += size;
    return i;
}

static void _ProtoBufSetFixed(uint8_t *buf, size_t bufLen, uint64_t i, size_t *off, size_t size)
{
    if (buf && *off + size <= bufLen && size <= sizeof(i)) memcpy(&buf[*off], &i, size);
    *off += size;
}

// sets either i or data depending on field type, and returns field key
static uint64_t _ProtoBufField(uint64_t *i, const uint8_t **data, const uint8_t *buf, size_t *len, size_t *off)
{
    uint64_t varInt = 0, fixedInt = 0, key = _ProtoBufVarInt(buf, *len, off);
    const uint8_t *lenDelim = NULL;
    
    switch (key & 0x07) {
        case PROTOBUF_VARINT: varInt = _ProtoBufVarInt(buf, *len, off); if (i) *i = varInt; break;
        case PROTOBUF_64BIT: fixedInt = _ProtoBufFixed(buf, *len, off, sizeof(uint64_t)); if (i) *i = fixedInt; break;
        case PROTOBUF_LENDELIM: lenDelim = _ProtoBufLenDelim(buf, len, off); if (data) *data = lenDelim; break;
        case PROTOBUF_32BIT: fixedInt = _ProtoBufFixed(buf, *len, off, sizeof(uint32_t)); if (i) *i = fixedInt; break;
        default: break;
    }
    
    return key;
}

static void _ProtoBufString(char **str, const void *data, size_t dataLen)
{
    if (data) {
        if (! *str) array_new(*str, dataLen + 1);
        array_clear(*str);
        array_add_array(*str, (const char *)data, dataLen);
        array_add(*str, '\0');
    }
}

static void _ProtoBufSetString(uint8_t *buf, size_t bufLen, const char *str, uint64_t key, size_t *off)
{
    size_t strLen = (str) ? strlen(str) : 0;
    
    _ProtoBufSetVarInt(buf, bufLen, (key << 3) | PROTOBUF_LENDELIM, off);
    _ProtoBufSetLenDelim(buf, bufLen, str, strLen, off);
}

static size_t _ProtoBufBytes(uint8_t **bytes, const void *data, size_t dataLen)
{
    if (data) {
        if (! *bytes) array_new(*bytes, dataLen);
        array_clear(*bytes);
        array_add_array(*bytes, (const uint8_t *)data, dataLen);
    }
    
    return (*bytes) ? array_count(*bytes) : 0;
}

static void _ProtoBufSetBytes(uint8_t *buf, size_t bufLen, const uint8_t *bytes, size_t bytesLen, uint64_t key,
                              size_t *off)
{
    _ProtoBufSetVarInt(buf, bufLen, (key << 3) | PROTOBUF_LENDELIM, off);
    _ProtoBufSetLenDelim(buf, bufLen, bytes, bytesLen, off);
}

static void _ProtoBufSetInt(uint8_t *buf, size_t bufLen, uint64_t i, uint64_t key, size_t *off)
{
    _ProtoBufSetVarInt(buf, bufLen, (key << 3) | PROTOBUF_VARINT, off);
    _ProtoBufSetVarInt(buf, bufLen, i, off);
}

static void _ProtoBufUnknown(uint8_t **unknown, uint64_t key, uint64_t i, const void *data, size_t dataLen)
{
    size_t bufLen = 10 + ((key & 0x07) == PROTOBUF_LENDELIM ? dataLen : 0);
    uint8_t _buf[(bufLen <= MAX_STACK) ? bufLen : 0], *buf = (bufLen <= MAX_STACK) ? _buf : malloc(bufLen);
    size_t off = 0, o = 0, l;
    uint64_t k;
    
    _ProtoBufSetVarInt(buf, bufLen, key, &off);
    
    switch (key & 0x07) {
        case PROTOBUF_VARINT: _ProtoBufSetVarInt(buf, bufLen, i, &off); break;
        case PROTOBUF_64BIT: _ProtoBufSetFixed(buf, bufLen, i, &off, sizeof(uint64_t)); break;
        case PROTOBUF_LENDELIM: _ProtoBufSetLenDelim(buf, bufLen, data, dataLen, &off); break;
        case PROTOBUF_32BIT: _ProtoBufSetFixed(buf, bufLen, i, &off, sizeof(uint32_t)); break;
        default: break;
    }
    
    if (off < bufLen) bufLen = off;
    if (! *unknown) array_new(*unknown, bufLen);
    off = 0;
    
    while (off < array_count(*unknown)) {
        l = array_count(*unknown);
        o = off;
        k = _ProtoBufField(NULL, NULL, *unknown, &l, &off);
        if (k == key) array_rm_range(*unknown, o, off - o);
        if (k >= key) break;
    }
    
    array_insert_array(*unknown, o, buf, bufLen);
    if (buf != _buf) free(buf);
}

#define _protobuf_is_default(array, key) (\
    ((key) < (array_capacity(array) - array_count(array))*sizeof(*(array))) ?\
        ((uint8_t *)&(array)[array_count(array)])[(key)] : 0\
)

#define _protobuf_set_default(array, key) do {\
    if ((array_capacity(array) - array_count(array))*sizeof(*(array)) <= (key))\
        array_set_capacity((array), array_count(array) + 1 + (key)/sizeof(*(array)));\
    ((uint8_t *)&(array)[array_count(array)])[(key)] = 1;\
} while(0)

#define _protobuf_unknown(array, key) (\
    (array_capacity(array)*sizeof(*(array)) <\
     ((array_count(array)*sizeof(*(array)) + (key) + 0xf) & ~0xf) + sizeof(uint8_t *)) ? NULL :\
    *(uint8_t **)(((uint8_t *)(array)) + ((array_count(array)*sizeof(*(array)) + (key) + 0xf) & ~0xf))\
)

#define _protobuf_set_unknown(array, key, unknown) do {\
    size_t _protobuf_idx = (array_count(array)*sizeof(*(array)) + (key) + 0xf) & ~0xf;\
    if (array_capacity(array)*sizeof(*(array)) < _protobuf_idx + sizeof(uint8_t *))\
        array_set_capacity((array), (_protobuf_idx + sizeof(uint8_t *))/sizeof(*(array)) + 1);\
    *(uint8_t **)(((uint8_t *)(array)) + _protobuf_idx) = (unknown);\
} while(0)

typedef enum {
    output_amount = 1,
    output_script = 2,
    output_unknown = 3,
} output_key;

typedef enum {
    details_network = 1,
    details_outputs = 2,
    details_time = 3,
    details_expires = 4,
    details_memo = 5,
    details_payment_url = 6,
    details_merch_data = 7,
    details_unknown = 8,
} details_key;

typedef enum {
    request_version = 1,
    request_pki_type = 2,
    request_pki_data = 3,
    request_details = 4,
    request_signature = 5,
    request_unknown = 6
} request_key;

typedef enum {
    certificates_cert = 1,
    certificates_unknown = 2
} certificates_key;

typedef enum {
    payment_merch_data = 1,
    payment_transactions = 2,
    payment_refund_to = 3,
    payment_memo = 4,
    payment_unknown = 5
} payment_key;

typedef enum {
    ack_payment = 1,
    ack_memo = 2,
    ack_unknown = 3
} ack_key;

static void _BRPaymentProtocolOutputParse(BRTxOutput *output, const uint8_t *buf, size_t bufLen)
{
    size_t off = 0;
    uint8_t *unknown = NULL;
    
    output->amount = UINT64_MAX;
    
    while (off < bufLen) {
        const uint8_t *data = NULL;
        size_t dataLen = bufLen;
        uint64_t i = 0, key = _ProtoBufField(&i, &data, buf, &dataLen, &off);
        
        switch (key >> 3) {
            case output_amount: output->amount = i; break;
            case output_script: BRTxOutputSetScript(output, data, dataLen); break;
            default: _ProtoBufUnknown(&unknown, key, i, data, dataLen); break;
        }
    }
    
    if (! output->script) {
        array_new(output->script, 0);
        _protobuf_set_default(output->script, output_script);
    }
    
    if (output->amount == UINT64_MAX) {
        output->amount = 0;
        _protobuf_set_default(output->script, output_amount);
    }
    
    if (unknown) _protobuf_set_unknown(output->script, output_unknown, unknown);
}

static size_t _BRPaymentProtocolOutputSerialize(BRTxOutput *output, uint8_t *buf, size_t bufLen)
{
    size_t off = 0;
    uint8_t *unknown = NULL;
    
    if (output->script && ! _protobuf_is_default(output->script, output_amount)) {
        _ProtoBufSetInt(buf, bufLen, output->amount, output_amount, &off);
    }
    
    if (output->script && ! _protobuf_is_default(output->script, output_script)) {
        _ProtoBufSetBytes(buf, bufLen, output->script, output->scriptLen, output_script, &off);
    }
    
    if (output->script) unknown = _protobuf_unknown(output->script, output_unknown);
    if (unknown && buf && off + array_count(unknown) <= bufLen) memcpy(&buf[off], unknown, array_count(unknown));
    if (unknown) off += array_count(unknown);
    
    return (! buf || off <= bufLen) ? off : 0;
}

// buf must contain a serialized details struct
// returns a details struct that must be freed by calling BRPaymentProtocolDetailsFree()
BRPaymentProtocolDetails *BRPaymentProtocolDetailsParse(const uint8_t *buf, size_t bufLen)
{
    BRPaymentProtocolDetails *details = calloc(1, sizeof(BRPaymentProtocolDetails));
    size_t off = 0;
    uint8_t *unknown = NULL;

    array_new(details->outputs, 1);
    
    while (off < bufLen) {
        BRTxOutput output = BR_TX_OUTPUT_NONE;
        const uint8_t *data = NULL;
        size_t dLen = bufLen;
        uint64_t i = 0, key = _ProtoBufField(&i, &data, buf, &dLen, &off);

        switch (key >> 3) {
            case details_network: _ProtoBufString(&details->network, data, dLen); break;
            case details_outputs: _BRPaymentProtocolOutputParse(&output, data, dLen); break;
            case details_time: details->time = i; break;
            case details_expires: details->expires = i; break;
            case details_memo: _ProtoBufString(&details->memo, data, dLen); break;
            case details_payment_url: _ProtoBufString(&details->paymentURL, data, dLen); break;
            case details_merch_data: details->merchDataLen = _ProtoBufBytes(&details->merchantData, data, dLen); break;
            default: _ProtoBufUnknown(&unknown, key, i, data, dLen); break;
        }

        if (output.script) array_add(details->outputs, output);
    }
    
    if (! details->network) {
        _ProtoBufString(&details->network, "main", strlen("main"));
        _protobuf_set_default(details->network, details_network);
    }
    
    details->outputsCount = array_count(details->outputs);
    
    if (unknown) _protobuf_set_unknown(details->network, details_unknown, unknown);
    
    if (details->outputsCount == 0) { // one or more outputs required
        BRPaymentProtocolDetailsFree(details);
        details = NULL;
    }
    
    return details;
}

// writes serialized details struct to buf and returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolDetailsSerialize(const BRPaymentProtocolDetails *details, uint8_t *buf, size_t bufLen)
{
    size_t i, off = 0, outputLen = 0x100, l;
    uint8_t *unknown = NULL, *outputBuf = malloc(outputLen);
    
    if (details->network && ! _protobuf_is_default(details->network, details_network)) {
        _ProtoBufSetString(buf, bufLen, details->network, details_network, &off);
    }
    
    for (i = 0; i < details->outputsCount; i++) {
        l = _BRPaymentProtocolOutputSerialize(&details->outputs[i], NULL, 0);
        if (l > outputLen) outputBuf = realloc(outputBuf, (outputLen = l));
        l = _BRPaymentProtocolOutputSerialize(&details->outputs[i], outputBuf, outputLen);
        _ProtoBufSetBytes(buf, bufLen, outputBuf, l, details_outputs, &off);
    }

    free(outputBuf);
    if (details->time > 0) _ProtoBufSetInt(buf, bufLen, details->time, details_time, &off);
    if (details->expires > 0) _ProtoBufSetInt(buf, bufLen, details->expires, details_expires, &off);
    if (details->memo) _ProtoBufSetString(buf, bufLen, details->memo, details_memo, &off);
    if (details->paymentURL) _ProtoBufSetString(buf, bufLen, details->paymentURL, details_payment_url, &off);
    if (details->merchantData) _ProtoBufSetBytes(buf, bufLen, details->merchantData, details->merchDataLen,
                                                 details_merch_data, &off);

    if (details->network) unknown = _protobuf_unknown(details->network, details_unknown);
    if (unknown && buf && off + array_count(unknown) <= bufLen) memcpy(&buf[off], unknown, array_count(unknown));
    if (unknown) off += array_count(unknown);
    
    return (! buf || off <= bufLen) ? off : 0;
}

// frees memory allocated for details struct
void BRPaymentProtocolDetailsFree(BRPaymentProtocolDetails *details)
{
    if (details->network && _protobuf_unknown(details->network, details_unknown)) {
        array_free(_protobuf_unknown(details->network, details_unknown));
    }

    if (details->network) array_free(details->network);

    for (size_t i = 0; i < details->outputsCount; i++) {
        if (details->outputs[i].script && _protobuf_unknown(details->outputs[i].script, output_unknown)) {
            array_free(_protobuf_unknown(details->outputs[i].script, output_unknown));
        }
        
        BRTxOutputSetScript(&details->outputs[i], NULL, 0);
    }
    
    if (details->outputs) array_free(details->outputs);
    if (details->memo) array_free(details->memo);
    if (details->paymentURL) array_free(details->paymentURL);
    if (details->merchantData) array_free(details->merchantData);
    free(details);
}

// buf must contain a serialized request struct
// returns a request struct that must be freed by calling BRPaymentProtocolRequestFree()
BRPaymentProtocolRequest *BRPaymentProtocolRequestParse(const uint8_t *buf, size_t bufLen)
{
    BRPaymentProtocolRequest *request = calloc(1, sizeof(BRPaymentProtocolRequest));
    size_t off = 0;
    uint8_t *unknown = NULL;
    
    request->version = UINT32_MAX;
    
    while (off < bufLen) {
        const uint8_t *data = NULL;
        size_t dataLen = bufLen;
        uint64_t i = 0, key = _ProtoBufField(&i, &data, buf, &dataLen, &off);
        
        switch (key >> 3) {
            case request_version: request->version = (uint32_t)i; break;
            case request_pki_type: _ProtoBufString(&request->pkiType, data, dataLen); break;
            case request_pki_data: request->pkiLen = _ProtoBufBytes(&request->pkiData, data, dataLen); break;
            case request_details: request->details = BRPaymentProtocolDetailsParse(data, dataLen); break;
            case request_signature: request->sigLen = _ProtoBufBytes(&request->signature, data, dataLen); break;
            default: _ProtoBufUnknown(&unknown, key, i, data, dataLen); break;
        }
    }
    
    if (! request->pkiType) {
        _ProtoBufString(&request->pkiType, "none", strlen("none"));
        _protobuf_set_default(request->pkiType, request_pki_type);
    }
    
    if (request->version == UINT32_MAX) {
        request->version = 1;
        _protobuf_set_default(request->pkiType, request_version);
    }
    
    if (unknown) _protobuf_set_unknown(request->pkiType, request_unknown, unknown);
    
    if (! request->details) { // required
        BRPaymentProtocolRequestFree(request);
        request = NULL;
    }

    return request;
}

// writes serialized request struct to buf and returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolRequestSerialize(const BRPaymentProtocolRequest *request, uint8_t *buf, size_t bufLen)
{
    size_t off = 0;
    uint8_t *unknown = NULL;

    if (request->pkiType && ! _protobuf_is_default(request->pkiType, request_version)) {
        _ProtoBufSetInt(buf, bufLen, request->version, request_version, &off);
    }
    
    if (request->pkiType && ! _protobuf_is_default(request->pkiType, request_pki_type)) {
        _ProtoBufSetString(buf, bufLen, request->pkiType, request_pki_type, &off);
    }
    
    if (request->pkiData) _ProtoBufSetBytes(buf, bufLen, request->pkiData, request->pkiLen, request_pki_data, &off);

    if (request->details) {
        size_t detailsLen = BRPaymentProtocolDetailsSerialize(request->details, NULL, 0);
        uint8_t *detailsBuf = malloc(detailsLen);

        detailsLen = BRPaymentProtocolDetailsSerialize(request->details, detailsBuf, detailsLen);
        _ProtoBufSetBytes(buf, bufLen, detailsBuf, detailsLen, request_details, &off);
        free(detailsBuf);
    }
    
    if (request->signature) _ProtoBufSetBytes(buf, bufLen, request->signature, request->sigLen, request_signature,&off);

    if (request->pkiType) unknown = _protobuf_unknown(request->pkiType, request_unknown);
    if (unknown && buf && off + array_count(unknown) <= bufLen) memcpy(&buf[off], unknown, array_count(unknown));
    if (unknown) off += array_count(unknown);

    return (! buf || off <= bufLen) ? off : 0;
}

// writes the DER encoded certificate corresponding to index to cert
// returns the number of bytes written to cert, or the total certLen needed if cert is NULL
// returns 0 if index is out-of-bounds
size_t BRPaymentProtocolRequestCert(const BRPaymentProtocolRequest *request, uint8_t *cert, size_t certLen, size_t idx)
{
    size_t off = 0, len = 0;
    
    while (request->pkiData && off < request->pkiLen) {
        const uint8_t *data = NULL;
        size_t dataLen = request->pkiLen;
        uint64_t i = 0, key = _ProtoBufField(&i, &data, request->pkiData, &dataLen, &off);
        
        if ((key >> 3) == certificates_cert && data) {
            if (idx == 0) {
                len = dataLen;
                if (cert && len <= certLen) memcpy(cert, data, len);
                break;
            }
            else idx--;
        }
    }
    
    return (idx == 0 && (! cert || len <= certLen)) ? len : 0;
}

// writes the hash of the request to md needed to sign or verify the request
// returns the number of bytes written, or the total bytes needed if md is NULL
size_t BRPaymentProtocolRequestDigest(BRPaymentProtocolRequest *request, uint8_t *md, size_t mdLen)
{
    request->sigLen = 0; // set signature to 0 bytes, a signature can't sign itself
    
    size_t len = BRPaymentProtocolRequestSerialize(request, NULL, 0);
    uint8_t *buf = malloc(len);
    
    len = BRPaymentProtocolRequestSerialize(request, buf, len);
    
    if (request->pkiType && strncmp(request->pkiType, "x509+sha256", strlen("x509+sha256") + 1) == 0) {
        if (256/8 <= mdLen) BRSHA256(md, buf, len);
        len = 256/8;
    }
    else if (request->pkiType && strncmp(request->pkiType, "x509+sha1", strlen("x509+sha1") + 1) == 0) {
        if (160/8 <= mdLen) BRSHA1(md, buf, len);
        len = 160/8;
    }
    else len = 0;
    
    free(buf);
    if (request->signature) request->sigLen = array_count(request->signature);
    return (! md || len <= mdLen) ? len : 0;
}

// frees memory allocated for request struct
void BRPaymentProtocolRequestFree(BRPaymentProtocolRequest *request)
{
    if (request->pkiType && _protobuf_unknown(request->pkiType, request_unknown)) {
        array_free(_protobuf_unknown(request->pkiType, request_unknown));
    }

    if (request->pkiType) array_free(request->pkiType);
    if (request->pkiData) array_free(request->pkiData);
    if (request->details) BRPaymentProtocolDetailsFree(request->details);
    if (request->signature) array_free(request->signature);
    free(request);
}

// returns a newly allocated payment struct that must be freed with BRPaymentProtocolPaymentFree()
BRPaymentProtocolPayment *BRPaymentProtocolPaymentNew(const uint8_t *merchantData, size_t merchDataLen,
                                                      BRTransaction *transactions[], size_t txCount,
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
            BRTxOutput output = BR_TX_OUTPUT_NONE;
            
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

// buf must contain a serialized payment struct
// returns a payment struct that must be freed by calling BRPaymentProtocolPaymentFree()
BRPaymentProtocolPayment *BRPaymentProtocolPaymentParse(const uint8_t *buf, size_t bufLen)
{
    BRPaymentProtocolPayment *payment = calloc(1, sizeof(BRPaymentProtocolPayment));
    size_t off = 0;
    uint8_t *unknown = NULL;
    
    array_new(payment->transactions, 1);
    array_new(payment->refundTo, 1);
    
    while (off < bufLen) {
        BRTransaction *tx = NULL;
        BRTxOutput output = BR_TX_OUTPUT_NONE;
        const uint8_t *data = NULL;
        size_t dLen = bufLen;
        uint64_t i = 0, key = _ProtoBufField(&i, &data, buf, &dLen, &off);
        
        switch (key >> 3) {
            case payment_transactions: tx = BRTransactionParse(data, dLen); break;
            case payment_refund_to: _BRPaymentProtocolOutputParse(&output, data, dLen); break;
            case payment_memo: _ProtoBufString(&payment->memo, data, dLen); break;
            case payment_merch_data: payment->merchDataLen = _ProtoBufBytes(&payment->merchantData, data, dLen); break;
            default: _ProtoBufUnknown(&unknown, key, i, data, dLen); break;
        }
        
        if (tx) array_add(payment->transactions, tx);
        if (output.script) array_add(payment->refundTo, output);
    }
    
    payment->txCount = array_count(payment->transactions);
    payment->refundToCount = array_count(payment->refundTo);
    
    if (unknown) _protobuf_set_unknown(payment->transactions, payment_unknown, unknown);
    
    return payment;
}

// writes serialized payment struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolPaymentSerialize(const BRPaymentProtocolPayment *payment, uint8_t *buf, size_t bufLen)
{
    size_t off = 0, sLen = 0x100, l;
    uint8_t *unknown = NULL, *sBuf = malloc(sLen);

    if (payment->merchantData) {
        _ProtoBufSetBytes(buf, bufLen, payment->merchantData, payment->merchDataLen, payment_merch_data, &off);
    }

    for (size_t i = 0; i < payment->txCount; i++) {
        l = BRTransactionSerialize(payment->transactions[i], NULL, 0);
        if (l > sLen) sBuf = realloc(sBuf, (sLen = l));
        l = BRTransactionSerialize(payment->transactions[i], sBuf, sLen);
        _ProtoBufSetBytes(buf, bufLen, sBuf, l, payment_transactions, &off);
    }

    for (size_t i = 0; i < payment->refundToCount; i++) {
        l = _BRPaymentProtocolOutputSerialize(&payment->refundTo[i], NULL, 0);
        if (l > sLen) sBuf = realloc(sBuf, (sLen = l));
        l = _BRPaymentProtocolOutputSerialize(&payment->refundTo[i], sBuf, l);
        _ProtoBufSetBytes(buf, bufLen, sBuf, l, payment_refund_to, &off);
    }

    free(sBuf);
    if (payment->memo) _ProtoBufSetString(buf, bufLen, payment->memo, payment_memo, &off);

    if (payment->transactions) unknown = _protobuf_unknown(payment->transactions, payment_unknown);
    if (unknown && buf && off + array_count(unknown) <= bufLen) memcpy(&buf[off], unknown, array_count(unknown));
    if (unknown) off += array_count(unknown);
    
    return (! buf || off <= bufLen) ? off : 0;
}

// frees memory allocated for payment struct (does not call BRTransactionFree() on transactions)
void BRPaymentProtocolPaymentFree(BRPaymentProtocolPayment *payment)
{
    if (payment->merchantData) array_free(payment->merchantData);
    
    if (payment->transactions && _protobuf_unknown(payment->transactions, payment_unknown)) {
        array_free(_protobuf_unknown(payment->transactions, payment_unknown));
    }

    if (payment->transactions) array_free(payment->transactions);
    
    for (size_t i = 0; i < payment->refundToCount; i++) {
        if (payment->refundTo[i].script && _protobuf_unknown(payment->refundTo[i].script, output_unknown)) {
            array_free(_protobuf_unknown(payment->refundTo[i].script, output_unknown));
        }
        
        BRTxOutputSetScript(&payment->refundTo[i], NULL, 0);
    }

    if (payment->refundTo) array_free(payment->refundTo);
    if (payment->memo) array_free(payment->memo);
}

// buf must contain a serialized ACK struct
// returns a ACK struct that must be freed by calling BRPaymentProtocolACKFree()
BRPaymentProtocolACK *BRPaymentProtocolACKParse(const uint8_t *buf, size_t bufLen)
{
    BRPaymentProtocolACK *ack = calloc(1, sizeof(BRPaymentProtocolACK) + sizeof(uint8_t *));
    size_t off = 0;
    uint8_t *unknown = NULL;
    
    while (off < bufLen) {
        const uint8_t *data = NULL;
        size_t dataLen = bufLen;
        uint64_t i = 0, key = _ProtoBufField(&i, &data, buf, &dataLen, &off);
        
        switch (key >> 3) {
            case ack_payment: ack->payment = BRPaymentProtocolPaymentParse(data, dataLen); break;
            case ack_memo: _ProtoBufString(&ack->memo, data, dataLen); break;
            default: _ProtoBufUnknown(&unknown, key, i, data, dataLen); break;
        }
    }
    
    if (unknown) *(uint8_t **)(ack + 1) = unknown;
    
    if (! ack->payment) { // required
        BRPaymentProtocolACKFree(ack);
        ack = NULL;
    }
    
    return ack;
}

// writes serialized ACK struct to buf and returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolACKSerialize(const BRPaymentProtocolACK *ack, uint8_t *buf, size_t bufLen)
{
    size_t off = 0;
    uint8_t *unknown = *(uint8_t **)(ack + 1);
    
    if (ack->payment) {
        size_t paymentLen = BRPaymentProtocolPaymentSerialize(ack->payment, NULL, 0);
        uint8_t *paymentBuf = malloc(paymentLen);
        
        paymentLen = BRPaymentProtocolPaymentSerialize(ack->payment, paymentBuf, paymentLen);
        _ProtoBufSetBytes(buf, bufLen, paymentBuf, paymentLen, ack_payment, &off);
        free(paymentBuf);
    }
    
    if (ack->memo) _ProtoBufSetString(buf, bufLen, ack->memo, ack_memo, &off);

    if (unknown && buf && off + array_count(unknown) <= bufLen) memcpy(&buf[off], unknown, array_count(unknown));
    if (unknown) off += array_count(unknown);
    
    return (! buf || off <= bufLen) ? off : 0;
}

// frees memory allocated for ACK struct
void BRPaymentProtocolACKFree(BRPaymentProtocolACK *ack)
{
    uint8_t *unknown = *(uint8_t **)(ack + 1);
    
    if (ack->payment) BRPaymentProtocolPaymentFree(ack->payment);
    if (ack->memo) array_free(ack->memo);
    if (unknown) array_free(unknown);
    free(ack);
}
