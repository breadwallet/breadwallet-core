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
#include "BRCryptoBase.h"

// Even though we only support the Payment type - plan for
// the future
typedef enum {
    RIPPLE_TX_TYPE_UNKNOWN = -1,
    RIPPLE_TX_TYPE_PAYMENT = 0,
    RIPPLE_TX_TYPE_ESCROW_CREATE = 1,
    RIPPLE_TX_TYPE_ESCROW_FINISH     = 2,
    RIPPLE_TX_TYPE_ACCOUNT_SET       = 3,
    RIPPLE_TX_TYPE_ESCROW_CANCEL     = 4,
    RIPPLE_TX_TYPE_REGULAR_KEY_SET   = 5,
    RIPPLE_TX_TYPE_NICKNAME_SET      = 6, // open
    RIPPLE_TX_TYPE_OFFER_CREATE      = 7,
    RIPPLE_TX_TYPE_OFFER_CANCEL      = 8,
    no_longer_used      = 9,
    RIPPLE_TX_TYPE_TICKET_CREATE     = 10,
    RIPPLE_TX_TYPE_TICKET_CANCEL     = 11,
    RIPPLE_TX_TYPE_SIGNER_LIST_SET   = 12,
    RIPPLE_TX_TYPE_PAYCHAN_CREATE    = 13,
    RIPPLE_TX_TYPE_PAYCHAN_FUND      = 14,
    RIPPLE_TX_TYPE_PAYCHAN_CLAIM     = 15,
    RIPPLE_TX_TYPE_CHECK_CREATE      = 16,
    RIPPLE_TX_TYPE_CHECK_CASH        = 17,
    RIPPLE_TX_TYPE_CHECK_CANCEL      = 18,
    RIPPLE_TX_TYPE_DEPOSIT_PREAUTH   = 19,
    RIPPLE_TX_TYPE_TRUST_SET         = 20,
    RIPPLE_TX_TYPE_AMENDMENT         = 100,
    RIPPLE_TX_TYPE_FEE               = 101,
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

#define RIPPLE_XRP_TO_DROPS(xrp)   (1000000 * (xrp))   // 1e6 drops/xrp
#endif
