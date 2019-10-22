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
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include "BRRipple.h"
#include "BRRippleBase.h"
#include "BRRipplePrivateStructs.h"
#include "BRRippleAddress.h"
#include "BRArray.h"

// Forward declarations
static int get_content(uint8_t *buffer, BRRippleField *field);

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
uint32_t calculate_buffer_size(BRRippleField *fields, uint32_t num_fields)
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
            // Common type and name
            unsigned char fieldid = (unsigned char)((type << 4) | code);
            buffer[0] = fieldid;
            return 1;
        } else {
            // Common type, uncommon name
            buffer[0] = type << 4;
            buffer[1] = code;
            return 2;
        }
    }
    else if (code < 16)
    {
        // uncommon type, common name, reverse the byte order
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
        {
            assert(field->fieldCode == 1 || field->fieldCode == 3);
            // As of now there is only 2 fields that are of type 8 that are supported
            // Both are Ripple addresses
            int address_length = rippleAddressGetRawSize(field->data.address);
            add_length(address_length, buffer);

            uint8_t address_bytes[address_length];
            rippleAddressGetRawBytes (field->data.address, address_bytes, address_length);
            return(1 + add_raw(address_bytes, address_length, &buffer[1]));
            break;
        }
        default:
            return 0;
    }
}

extern uint32_t rippleSerialize (BRRippleField *fields, uint32_t num_fields, uint8_t * buffer, uint32_t bufferSize)
{
    // Create the stucture to hold the results
    uint32_t size = calculate_buffer_size(fields, num_fields);
    
    if (bufferSize < size) {
        return size;
    }

    // The values (fields) in the Ripple transaction are sorted by
    // type code and field code (asc)
    qsort(fields, num_fields, sizeof(BRRippleField), compare_function);

    // serialize all the fields adding the field IDs and content to the buffer
    int buffer_index = 0;
    for (int i = 0; i < num_fields; i++) {
        buffer_index += add_fieldid(fields[i].typeCode, fields[i].fieldCode, &buffer[buffer_index]);
        buffer_index += add_content( &fields[i], &buffer[buffer_index]);
    }

    return buffer_index;
}

/*
 * The following are helper functions for the de-serialization process
 *
 */

// Parse out the type and field codes
int get_fieldcode(uint8_t * buffer, BRRippleField *field)
{
    assert(field);
    assert(buffer);

    if (buffer[0] == 0) {
        // This is an uncommon type with 3 bytes
        // both type and field >= 16
        field->typeCode = buffer[1];
        field->fieldCode = buffer[2];
        return 3; // 3-bytes to store the code
    } else if ((buffer[0] & 0x0F) == 0) {
        // lower 4 bits of first byte are all off
        // 2 byte code where type < 16, field >= 16
        field->typeCode = buffer[0] >> 4 & 0x0F;
        field->fieldCode = buffer[1];
        return 2; // 2 bytes to store the code
    } else if ((buffer[0] & 0xF0) == 0) {
        // high 4 bits of first byte are all off
        // 2-byte code where type >= 16, field < 16
        // reverse byte order
        field->typeCode = buffer[1];
        field->fieldCode = buffer[0];
        return 2;
    } else {
        // This is the most common case - type and field are in the same byte
        // both type and code < 16
        field->fieldCode = buffer[0] & 0x0F;
        field->typeCode = buffer[0] >> 4 & 0x0F;
        return 1; // 1 byte to store the code
    }
}

// For variable length fields - the length of the field
// is also stored as a variable length integer.
int get_length(uint8_t* buffer, int * length)
{
    // Get the value for the first bytes
    int lengthLength = (int)buffer[0];
    if (lengthLength <= 192) {
        // We are done
        *length = lengthLength;
        return 1; // We are using 1 byte for length
    } else if (lengthLength <= 240) {
        // This is a 2-byte length
        *length = 193
                  + ((buffer[0] - 193) * 256)
                  + buffer[1];
        return 2; // We are using 2 bytes for length
    } else if (lengthLength <= 254) {
        // 3-byte string
        *length = 12481 +
                  ((buffer[0] - 241) * 65536)
                  + (buffer[1] * 256)
                  + buffer[2];
        return 3; // We are using 3 bytes for length
    } else {
        // Invalid length
        return -1;
    }
}

// 16-bit unsigned integer
int get_u16(uint8_t * buffer, uint16_t * value)
{
    *value = (buffer[0] << 8) + (buffer[1] & 0xFF);
    return 2;
}

// 32-bit unsigned integer
int get_u32(uint8_t * buffer, uint32_t * value)
{
    *value = (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3];
    return 4;
}

// 64-bit unsigned integer
int get_u64(uint8_t * buffer, uint64_t * value)
{
    *value = ((uint64_t)buffer[0] << 56) +
        ((uint64_t)buffer[1] << 48) +
        ((uint64_t)buffer[2] << 40) +
        ((uint64_t)buffer[3] << 32) +
        (buffer[4] << 24) +
        (buffer[5] << 16) +
        (buffer[6] << 8) +
        buffer[7];
    return 8;
}

// Binary data
int get_bytes(uint8_t *buffer, uint8_t *output, int length)
{
    memcpy(output, buffer, length);
    return length;
}

// Amount fields - either XRP or something else
int get_amount(uint8_t * buffer, BRRippleAmount * amount)
{
    // If bit-0 is 0 then this is XRP
    bool isXRP = (buffer[0] & 0x80) == 0x00;
    if (isXRP) {
        // XRP currency
        amount->currencyType = 0; // XRP
        get_u64(buffer, &amount->amount.u64Amount);
        // Get rid of the sign bit - FYI, XRP is always possitive
        amount->amount.u64Amount = amount->amount.u64Amount & 0xBFFFFFFFFFFFFFFF;
        return 8; // always 8 bytes
    } else {
        // Some other currency
        amount->currencyType = 1; // NOT XRPs
        // <xrpbit><signbit><8-bit exponent><54-bit matissa>
        // if sign bit is 0 number is negative for some reason
        int sign = (buffer[0] & 0x40) ? 1 : -1;
        int8_t exponent = ((buffer[0] & 0x3F) << 2) + ((buffer[1] & 0xC0) >> 6) - 97;
        uint64_t mantissa = ((uint64_t)(buffer[1] & 0x3F) << 48) +
            ((uint64_t)(buffer[2]) << 40) +
            ((uint64_t)(buffer[3]) << 32) +
            ((uint64_t)(buffer[4]) << 24) +
            ((uint64_t)(buffer[5]) << 16) +
            ((uint64_t)(buffer[6]) << 8) +
            (buffer[7] & 0xFF);
        amount->amount.dAmount = (double)mantissa * pow(10, exponent) * sign;
        memcpy(amount->currencyCode, &buffer[8], 20);
        memcpy(amount->issuerId, &buffer[28], 20);
        return (384/8);
    }
}

// Get the Variable length content
int get_VLContent(uint8_t *buffer, BRRippleField *field)
{
    int content_length = 0;
    // Figure out how many bytes were used to store the length and also
    // get the content length
    int lengthLength = get_length(buffer, &content_length);
    if (field->typeCode == 7) {
        if (field->fieldCode == 3) { // public key
            memcpy(field->data.publicKey.pubKey, &buffer[lengthLength], content_length);
        } else if (field->fieldCode == 4) { // signature
            memcpy(field->data.signature.signature, &buffer[lengthLength], content_length);
            field->data.signature.sig_length = content_length;
        }
    } else {
        if (field->fieldCode == 1 || field->fieldCode == 3) { // address
            field->data.address = rippleAddressCreateFromBytes(&buffer[1], 20);
        }
    }
    return (lengthLength + content_length);
}

// STObject - not supported
int get_STObject(uint8_t *buffer, BRRippleField *field)
{
    // TODO - support embedded objects
    //printf("STObject\n");
    return 0;
}

int get_MemoField(uint8_t *buffer, BRRippleMemoNode *memoNode)
{
    int bytesRead = 0;

    BRRippleField field;
    bytesRead += get_fieldcode(&buffer[bytesRead], &field);
    if (7 == field.typeCode &&
        (field.fieldCode >= 12 && field.fieldCode <= 14)) {
        // Memo field - get the field type and content
        int content_length = 0;
        int lengthLength = get_length(&buffer[bytesRead], &content_length);
        bytesRead += lengthLength;
        VLBytes * content = createVLBytes(content_length);
        memcpy(content->value, &buffer[bytesRead], content_length);
        bytesRead += content_length;
        switch(field.fieldCode) {
            case 12:
                memoNode->memo.memoType = content;
                break;
            case 13:
                memoNode->memo.memoData = content;
                break;
            case 14:
                memoNode->memo.memoFormat = content;
                break;
        }
        return (bytesRead);
    } else {
        // Not a memo field
        return 0;
    }
}

int get_memo_object(uint8_t *buffer, BRRippleMemoNode * memoNode)
{
    int bytesRead = 0;
    // Peek ahead at the next byte
    uint8_t code = buffer[bytesRead];
    while (code != 0xE1) { // 0xE1 indicates end of object
        // Get the memo fields
        bytesRead += get_MemoField(&buffer[bytesRead], memoNode);
        code = buffer[bytesRead]; // Peek ahead at the next byte
    }
    bytesRead += 1; // the E1 bytes
    return bytesRead;
}

// STArray
int get_STArray(uint8_t *buffer, BRRippleField *field)
{
    // Keep track of where we are in the buffer
    int bytesRead = 0;

    // Peek ahead at the next byte
    uint8_t code = buffer[bytesRead];
    while (code != 0xF1) { // 0xF1 indicates end of array
        // The array can contain multiple objects - get the next one
        BRRippleField stObject;
        bytesRead += get_fieldcode(&buffer[bytesRead], &stObject);
        if (10 == stObject.fieldCode) {
            // This is a memo - fetch a new memo list node and attach to field
            BRRippleMemoNode * memoNode = memoListAdd(field);
            // Get the memo content
            bytesRead += get_memo_object(&buffer[bytesRead], memoNode);
        } else {
            // Don't support anything other that memos currently.
            return 0; // indicate we can't parse this
        }
        code = buffer[bytesRead]; // peek ahead to see if we are done
    }
    return bytesRead + 1;  // Add in the F1 byte
}

// PathSet - not supported
int get_PathSet(uint8_t *buffer, BRRippleField *field)
{
    // TODO - support path sets
    //printf("PathSet\n");
    return 0;
}

// Get the content for the specified type and field codes
static int get_content(uint8_t *buffer, BRRippleField *field)
{
    // According to the ripple docs this is the FULL list of
    // field types that can appear in a transaction
    // https://developers.ripple.com/serialization.html - see "Type List"
    // Currenly we cannot support types 14, 15, and 18.
    switch(field->typeCode) {
        case 1: // 2-byte integer
            return get_u16(buffer, &field->data.i16);
        case 2: // 4-byte integer
            return get_u32(buffer, &field->data.i32);
        case 4: // Hash128
            return get_bytes(buffer, field->data.hash, 16);
        case 5: // Hash256
            return get_bytes(buffer, field->data.hash, 32);
        case 6: // Amount
            return get_amount(buffer, &field->data.amount);
        case 7:
        case 8: // 7 & 8 are variable length fields
            return get_VLContent(buffer, field);
        case 14: // STObject
            return get_STObject(buffer, field);
        case 15: // STArray
            return get_STArray(buffer, field);
        case 16:
            field->data.i8 = buffer[0];
            return 1;
        case 17: // Hash160
            return get_bytes(buffer, field->data.hash, 20);
        case 18: // PathSet
            return get_PathSet(buffer, field);
        default:
            printf("Unknown\n");
            return 0;
    }
}

// Thise are the fields that we can parse and have the content
bool addFieldToList(BRRippleField * field)
{
    switch(field->typeCode) {
        case 1:
        case 2:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 15:
        case 16:
        case 17:
            return true;
        default:
            return false;
            break;
    }
}

extern int rippleDeserialize(uint8_t *buffer, int bufferSize, BRArrayOf(BRRippleField) *fields)
{
    assert(buffer);
    assert(fields);

    int index = 0;

    while (index < bufferSize - 1) {
        // Get the code and field
        BRRippleField field;
        memset(&field, 0x00, sizeof(BRRippleField));

        index += get_fieldcode(&buffer[index], &field);

        int content_length = get_content(&buffer[index], &field);
        if (0 == content_length) {
            // We were unable to parse this field - so quit now
            // The caller is expected to check the number of bytes
            // to see if serialization was complete.
            if (field.memos != 0) {
                memoListFree(field.memos);
            }
            return index;
        }
        index += content_length;

        // If we care about this field - add it to our array
        if (addFieldToList(&field)) {
            array_add(*fields, field);
        }
    }
    return index;
}
