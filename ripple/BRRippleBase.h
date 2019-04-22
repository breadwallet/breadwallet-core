//
//  BRRippleBase.h
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRipple_base_h
#define BRRipple_base_h

#define ADDRESS_BYTES   (20)

// A Ripple Address - 20 bytes
typedef struct {
    uint8_t bytes[ADDRESS_BYTES];
} BRRippleAddress;

typedef enum {
    PAYMENT
} BRRippleTransactionType ;

typedef struct _ripple_field {
    int typeCode;
    int fieldCode;
    union _data {
        uint8_t i8;
        uint16_t i16;
        uint32_t i32;
        uint64_t i64;
        BRRippleAddress address;
    } data;
} BRRippleField;

#endif
