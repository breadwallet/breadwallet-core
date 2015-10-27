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
    
    while ((b & 0x80) && *off < len) {
        b = buf[(*off)++];
        varInt += (uint64_t)(b & 0x7f) << 7*i++;
    }
    
    return (b & 0x80) ? 0 : varInt;
}

static const uint8_t *ProtoBufLenDelim(const uint8_t *buf, size_t *len, size_t *off)
{
    const uint8_t *data = NULL;
    size_t dataLen = ProtoBufVarInt(buf, *len, off);
    
    if (*off + dataLen <= *len) data = buf + *off;
    *off += dataLen;
    *len = dataLen;
    return data;
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

static inline void ProtoBufString(char **str, const void *data, size_t len)
{
    if (data) {
        if (! *str) array_new(*str, len + 1);
        array_clear(*str);
        array_add_array(*str, data, len);
        array_add(*str, '\0');
    }
}

static inline size_t ProtoBufBytes(uint8_t **bytes, const void *data, size_t len)
{
    if (data) {
        if (! *bytes) array_new(*bytes, len);
        array_clear(*bytes);
        array_add_array(*bytes, data, len);
    }
    
    return (*bytes) ? array_count(*bytes) : 0;
}

#define proto_buf_set_default(array, key) do {\
    if ((array_capacity(array) - array_count(array))*sizeof(*(array)) <= (key))\
        array_set_capacity((array), array_count(array) + 1 + ((key) + 1)/sizeof(*(array)));\
    ((uint8_t *)&(array)[array_count(array)])[(key)] = 1;\
} while(0)

#define proto_buf_is_default(array, key) (\
    ((key) < (array_capacity(array) - array_count(array))*sizeof(*(array))) ?\
    ((uint8_t *)&(array)[array_count(array)])[(key)] : 0\
)

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

// buf must contain a serialized details struct, result must be freed by calling BRPaymentProtocolDetailsFree()
BRPaymentProtocolDetails *BRPaymentProtocolDetailsParse(const uint8_t *buf, size_t len)
{
    BRPaymentProtocolDetails *details = calloc(1, sizeof(BRPaymentProtocolDetails));
    BRTxOutput *output;
    size_t off = 0;

    array_new(details->outputs, 1);
    
    while (off < len) {
        uint64_t i = 0, amount = UINT64_MAX;
        const uint8_t *d = NULL, *script = NULL;
        size_t o = 0, dlen = len, slen = 0;

        switch (ProtoBufField(&i, &d, buf, &dlen, &off)) {
            case details_network: ProtoBufString(&details->network, d, dlen); break;
            case details_outputs: while (o < dlen) slen = dlen, ProtoBufField(&amount, &script, d, &slen, &o); break;
            case details_time: details->time = i; break;
            case details_expires: details->expires = i; break;
            case details_memo: ProtoBufString(&details->memo, d, dlen); break;
            case details_payment_url: ProtoBufString(&details->paymentURL, d, dlen); break;
            case details_merchant_data: details->merchDataLen = ProtoBufBytes(&details->merchantData, d, dlen); break;
            default: break;
        }

        if (script) {
            array_set_count(details->outputs, details->outputsCount + 1);
            output = &details->outputs[details->outputsCount];
            BRTxOutputSetScript(output, script, slen);
            output->amount = (amount == UINT64_MAX) ? 0 : amount;
            if (amount == UINT64_MAX) proto_buf_set_default(output->script, output_amount);
            details->outputsCount++;
        }
    }
    
    if (! details->network) {
        ProtoBufString(&details->network, "main", 4);
        proto_buf_set_default(details->network, details_network);
    }
    
    if (array_count(details->outputs) == 0) { // one or more outputs required
        BRPaymentProtocolDetailsFree(details);
        details = NULL;
    }
    
    return details;
}

// writes serialized details struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolDetailsSerialize(BRPaymentProtocolDetails *details, uint8_t *buf, size_t len)
{
    return 0;
}

// frees memory allocated for details struct
void BRPaymentProtocolDetailsFree(BRPaymentProtocolDetails *details)
{
    if (details->network) free(details->network);

    for (size_t i = 0; i < array_count(details->outputs); i++) {
        BRTxOutputSetScript(&details->outputs[i], NULL, 0);
    }
    
    array_free(details->outputs);
    if (details->memo) free(details->memo);
    if (details->paymentURL) free(details->paymentURL);
    if (details->merchantData) free(details->merchantData);
    free(details);
}

// buf must contain a serialized request struct, result must be freed by calling BRPaymentProtocolRequestFree()
BRPaymentProtocolRequest *BRPaymentProtocolRequestParse(const uint8_t *buf, size_t len)
{
    return NULL;
}

// writes serialized request struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolRrequestSerialize(BRPaymentProtocolRequest *request, uint8_t *buf, size_t len)
{
    return 0;
}

// writes the DER encoded certificate corresponding to index to cert, returns the number of bytes written to cert, or
// the total certLen needed if cert is NULL, returns 0 if index of out-of-bounds
size_t BRPaymentProtocolRequestCert(BRPaymentProtocolRequest *request, uint8_t *cert, size_t certLen, size_t index)
{
    return 0;
}

// writes the hash of the request to md needed to sign or verify the request, returns the number of bytes written, or
// the total bytes needed if md is NULL
size_t BRPaymentProtocolRequestDigest(BRPaymentProtocolRequest *request, uint8_t *md, size_t mdLen)
{
    return 0;
}

// frees memory allocated for request struct
void BRPaymentProtocolRequestFree(BRPaymentProtocolRequest *request)
{
}

// buf must contain a serialized payment struct, result must be freed by calling BRPaymentProtocolPaymentFree()
BRPaymentProtocolPayment *BRPaymentProtocolPaymentParse(const uint8_t *buf, size_t len)
{
    return NULL;
}

// writes serialized payment struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolPaymentSerialize(BRPaymentProtocolPayment *payment, uint8_t *buf, size_t len)
{
    return 0;
}

// frees memory allocated for payment struct
void BRPaymentProtocolPaymentFree(BRPaymentProtocolPayment *payment)
{
}

// buf must contain a serialized ACK struct, result must be freed by calling BRPaymentProtocolACKFree()
BRPaymentProtocolPayment *BRPaymentProtocolACKParse(const uint8_t *buf, size_t len)
{
    return NULL;
}

// writes serialized ACK struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolACKSerialize(BRPaymentProtocolPayment *payment, uint8_t *buf, size_t len)
{
    return 0;
}

// frees memory allocated for ACK struct
void BRPaymentProtocolACKFree(BRPaymentProtocolPayment *payment)
{
}
