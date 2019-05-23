//
//  BRStellarBase.h
//  Core
//
//  Created by Carl Cherry on 5/21/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRStellar_base_h
#define BRStellar_base_h
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define STELLAR_ADDRESS_BYTES   (56)

// A Stellar Address - 56 bytes
typedef struct {
    char bytes[STELLAR_ADDRESS_BYTES + 1]; // NULL terminated string
} BRStellarAddress;

// Even though we only support the Payment type - plan for
// the future
typedef enum {
    STELLAR_TX_TYPE_UNKNOWN = -1,
    STELLAR_TX_TYPE_PAYMENT = 0,
} BRStellarTransactionType ;

// A Stellar Transaction Hash
typedef struct {
    uint8_t bytes[32];
} BRStellarTransactionHash;

// Stucture to hold the calculated signature
typedef struct {
    uint8_t signature[256];
    int sig_length;
} BRStellarSignatureRecord;
typedef BRStellarSignatureRecord *BRStellarSignature;

#endif
