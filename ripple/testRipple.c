//
//  BRRipple.h
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include "BRRipple.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

static void
testRippleTransaction (void /* ... */) {
    BRRippleTransaction transaction;
    
    uint8_t expected_output[] = {
        0x12, 0x00, 0x00, // transaction type - 16-bit
        0x24, 0x00, 0x00, 0x00, 0x01, // Sequence - 32-bit
        0x61, 0x40, 0x00, 0x00, 0x00, 0x00, 0xF4, 0x42, 0x40, // Amount 1,000,000 - 64-bit
        0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, // Fee 12 - 64-bit
        0x81, 0x14, 0xB5, 0xF7, 0x62, 0x79, 0x8A, 0x53, 0xD5, // Source Address - type 8
        0x43, 0xA0, 0x14, 0xCA, 0xF8, 0xB2, 0x97, 0xCF, 0xF8, 0xF2, 0xF9, 0x37, 0xE8,
        0x83, 0x14, 0xB6, 0xF8, 0x63, 0x80, 0x8B, 0x54, 0xD6, // Target Address - type 8
        0x43, 0xA0, 0x14, 0xCA, 0xF8, 0xB2, 0x97, 0xCF, 0xF8, 0xF2, 0xF9, 0x37, 0xE8
    };

    // Here are the 20 byte addresses
    uint8_t sourceBytes[] = { 0xB5, 0xF7, 0x62, 0x79, 0x8A, 0x53, 0xD5,
        0x43, 0xA0, 0x14, 0xCA, 0xF8, 0xB2, 0x97, 0xCF, 0xF8, 0xF2, 0xF9, 0x37, 0xE8 };
    uint8_t destBytes[] = { 0xB6, 0xF8, 0x63, 0x80, 0x8B, 0x54, 0xD6,
        0x43, 0xA0, 0x14, 0xCA, 0xF8, 0xB2, 0x97, 0xCF, 0xF8, 0xF2, 0xF9, 0x37, 0xE8 };
    BRRippleAddress sourceAddress;
    BRRippleAddress targetAddress;
    memcpy(sourceAddress.bytes, sourceBytes, 20);
    memcpy(targetAddress.bytes, destBytes, 20);
    transaction = rippleTransactionCreate(sourceAddress, targetAddress, PAYMENT, 1000000, 1, 12);
    assert(transaction);

    // Now serialize the transaction
    BRRippleSerializedTransaction s = rippleTransactionSerialize(transaction);
    assert(s);
    int size = getSerializedSize(s);
    uint8_t* bytes = getSerializedBytes(s);

    // Compare the size of the input and output
    assert(size == sizeof(expected_output));
    assert(memcmp(expected_output, bytes, size));
}

extern void
runRippleTest (void /* ... */) {
    testRippleTransaction ();
}
