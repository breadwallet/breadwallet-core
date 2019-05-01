//
//  BRRippleSerialize.c
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "BRRipple.h"
#include "BRRippleBase.h"

struct BRRippleSerializedTransactionRecord {
    int size;
    uint8_t *buffer;
}  ;

/**
 * Compare 2 Ripple fields
 *
 * @param field1  A ripple field
 * @param field2  A ripple field
 * @return <0  field1 goes before field2
 *          0  the fields are the same
 *         >0  field2 goes before field1
 */
int compare_function(const void *field1, const void *field2) {
    BRRippleField *first = (BRRippleField *)field1;
    BRRippleField *second = (BRRippleField *)field2;
    // The sort order if first by type and then by field - so
    // just combine the values into a single integer and sort.
    int fieldid1 = first->typeCode << 16 | first->fieldCode;
    int fieldid2 = second->typeCode << 16 | second->fieldCode;
    return(fieldid1 - fieldid2);
}

/**
 * Calculate the buffer size needed to store a serialized transaction
 *
 * @param fields      An array of fields
 * @param num_fields  The size of the array
 * @return the size of the buffer needed
 */
int calculate_buffer_size(BRRippleField *fields, int num_fields)
{
    // The fieldid is stored as 2-4 bytes. Since a few extra bytes won't hurt
    // just allocate 4 bytes per field to be safe.
    int size = num_fields * 4;
    // Figure out the approximate size of the buffer we need to make
    for(int i = 0; i < num_fields; i++) {
        // Add on the potential size of the fieldid
        switch (fields[i].typeCode) {
            case 1:
                size += 2; // 16-bit integers
                break;
            case 2:
                size += 4; // 32-bit integers
                break;
            case 6:
                size += 8; // 64-bit integers
                break;
            case 7:
                if (fields[i].fieldCode == 4) {
                    size += sizeof(fields[i].data.signature.signature);
                    size += 4; // max needed to store length
                } else if (fields[i].fieldCode == 3) {
                    size += sizeof(fields[i].data.publicKey.pubKey);
                    size += 4; // max needed to store length
                }
                break;
            case 8:
                // Accounts - which is really the address of 20 bytes plus
                // 1 extra byte for the size.
                if (fields[i].fieldCode == 1 || fields[i].fieldCode == 3) {
                    size += 21; // 20 bytes for the address, 1 for the length
                }
                break;
        }
    }
    return size;
}

int add_fieldid(int type, int code, uint8_t *buffer)
{
    if (type < 16) {
        if (code < 16) {
            unsigned char fieldid = (unsigned char)((type << 4) | code);
            buffer[0] = fieldid;
            return 1;
        } else {
            buffer[0] = type << 4;
            buffer[1] = code;
            return 2;
        }
    }
    else if (code < 16)
    {
        // uncommon type, common name
        buffer[0] = code;
        buffer[1] = type;
        return 2;
    }
    else
    {
        // uncommon type, uncommon name
        buffer[0] = 0;
        buffer[1] = type;
        buffer[2] = code;
        return 3;
    }
    return 0;
}

int add_uint16(uint16_t i, uint8_t* buffer) {
    /* Copy the 2 bytes to the buffer */
    buffer[0] = (char)(i >> 8);
    buffer[1] = i & 0XFF;
    return 2;
}

int add_uint32(uint32_t i, uint8_t* buffer) {
    /* Copy 4 bytes to the buffer */
    buffer[0] = i >> 24;
    buffer[1] = ((i >> 16) & 0xff);
    buffer[2] = ((i >> 8) & 0xff);
    buffer[3] = i & 0xff;
    return 4;
}

int add_uint64(uint64_t i, uint8_t* buffer)
{
    buffer[0] = (i >> 56);
    buffer[1] = (i >> 48) & 0xff;
    buffer[2] = (i >> 40) & 0xff;
    buffer[3] = (i >> 32) & 0xff;
    buffer[4] = (i >> 24) & 0xff;
    buffer[5] = (i >> 16) & 0xff;
    buffer[6] = (i >> 8) & 0xff;
    buffer[7] = (i & 0xff);
    return 8;
}

int add_amount(uint64_t amount, uint8_t* buffer)
{
    // The only amount type we support at the moment is XRP.
    // the most significant bit is always 0 to indicate that it's XRP
    // and the second-most-significant bit is 1 to indicate that it is positive
    amount = amount | 0x4000000000000000;
    return add_uint64(amount, buffer);
}

int add_raw(uint8_t* raw, int length, uint8_t* buffer)
{
    memcpy(buffer, raw, length);
    return length;
}

int add_length(int length, uint8_t* buffer)
{
    if (length <= 192)
    {
        buffer[0] = (unsigned char)length;
        return 1;
    }
    else if (length <= 12480)
    {
        length -= 193;
        buffer[0] = 193 + (unsigned char)(length >> 8);
        buffer[1] = (unsigned char)(length & 0xff);
        return 2;
    }
    else if (length <= 918744)
    {
        length -= 12481;
        buffer[0] = 241 + (unsigned char)(length >> 16);
        buffer[1] = (unsigned char)((length >> 8) & 0xff);
        buffer[2] = (unsigned char)(length & 0xff);
        return 3;
    }
    else {
        return 0;
    }
}

int add_blob(BRRippleField *field, uint8_t *buffer)
{
    // Variable length blob
    if (field->fieldCode == 3) {
        // SigningPubKey - according to the docs this is a
        int length = add_length(33, buffer);
        length += add_raw(field->data.publicKey.pubKey, 33, &buffer[length]);
        return length;
    } else if (field->fieldCode == 4){
        // TxnSignature
        int length = add_length(field->data.signature.sig_length, buffer);
        length += add_raw(field->data.signature.signature,
                          field->data.signature.sig_length,
                          &buffer[length]);
        return length;
    } else {
        return 0;
    }
}

int add_content(BRRippleField *field, uint8_t *buffer)
{
    switch(field->typeCode) {
        case 1:
            return(add_uint16(field->data.i16, buffer));
            break;
        case 2:
            return(add_uint32(field->data.i32, buffer));
            break;
        case 6:
            return(add_amount(field->data.i64, buffer));
            break;
        case 7:
            // Variable length blob
            return(add_blob(field,buffer));
            break;
        case 8:
            assert(field->fieldCode == 1 || field->fieldCode == 3);
            // As of now there is only 2 fields that are of type 8 that are supported
            // Both are Ripple addresses
            add_length(sizeof(field->data.address), buffer);
            return(1 + add_raw(field->data.address.bytes, sizeof(field->data.address.bytes), &buffer[1]));
            break;
        default:
            return 0;
    }
}

extern BRRippleSerializedTransaction serialize (BRRippleField *fields, int num_fields)
{
    // Create the stucture to hold the results
    BRRippleSerializedTransaction result = calloc(1, sizeof(struct BRRippleSerializedTransactionRecord));
    result->size = calculate_buffer_size(fields, num_fields);
    result->buffer = calloc(1, result->size);
    
    // The values (fields) in the Ripple transaction are sorted by
    // type code and field code (asc)
    qsort(fields, num_fields, sizeof(BRRippleField), compare_function);

    // serialize all the fields adding the fieldis and content to the buffer
    int buffer_index = 0;
    for (int i = 0; i < num_fields; i++) {
        buffer_index += add_fieldid(fields[i].typeCode, fields[i].fieldCode, &result->buffer[buffer_index]);
        buffer_index += add_content( &fields[i], &result->buffer[buffer_index]);
    }

    result->size = buffer_index;

    return result;
}

extern uint32_t getSerializedSize(BRRippleSerializedTransaction s)
{
    return s->size;
}
extern uint8_t* getSerializedBytes(BRRippleSerializedTransaction s)
{
    return s->buffer;
}

extern void deleteSerializedBytes(BRRippleSerializedTransaction sTransaction)
{
    assert(sTransaction);
    if (sTransaction->buffer) {
        free(sTransaction->buffer);
    }
    free(sTransaction);
}
