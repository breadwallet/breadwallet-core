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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ADDRESS_BYTES   (20)

// A Ripple Address - 20 bytes
typedef struct {
    uint8_t bytes[ADDRESS_BYTES];
} BRRippleAddress;

// Even though we only support the Payment type - plan for
// the future
typedef enum {
    RIPPLE_TX_TYPE_UNKNOWN = -1,
    RIPPLE_TX_TYPE_PAYMENT = 0
} BRRippleTransactionType ;

// A Ripple Transaction Hash
typedef struct {
    uint8_t bytes[32];
} BRRippleTransactionHash;

typedef struct {
    int currencyType; // 0 - ripple, 1 - other, -1 unknown/invalid
    uint8_t currencyCode[20];
    uint8_t issuerId[20];
    union _amount {
        uint64_t u64Amount; // XRP drops
        double   dAmount;   // for other currencies
    } amount;
} BRRippleAmount;

typedef enum {
    RIPPLE_AMOUNT_TYPE_AMOUNT,
    RIPPLE_AMOUNT_TYPE_FEE,
    RIPPLE_AMOUNT_TYPE_SENDMAX,
    RIPPLE_AMOUNT_TYPE_DELIVERMIN
} BRRippleAmountType;

// Stucture to hold the calculated signature
typedef struct {
    uint8_t signature[256];
    int sig_length;
} BRRippleSignatureRecord;
typedef BRRippleSignatureRecord *BRRippleSignature;

typedef uint64_t BRRippleUnitDrops;
typedef uint32_t BRRippleSequence;
typedef uint32_t BRRippleFlags;
typedef uint32_t BRRippleLastLedgerSequence;
typedef uint32_t BRRippleSourceTag;
typedef uint32_t BRRippleDestinationTag;

#endif
