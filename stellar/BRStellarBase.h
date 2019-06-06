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

// Stucture to hold the decorated signature
typedef struct {
    uint8_t signature[68];
} BRStellarSignatureRecord;
typedef BRStellarSignatureRecord *BRStellarSignature;

typedef uint32_t BRStellarFee;
typedef int64_t  BRStellarSequence;
typedef uint64_t TimePoint;

typedef struct __time_bounds {
    TimePoint minTime;
    TimePoint maxTime;
} BRStellarTimeBounds;

typedef enum _stellar_crypto_key_type
{
    KEY_TYPE_ED25519 = 0,
    KEY_TYPE_PRE_AUTH_TX = 1,
    KEY_TYPE_HASH_X = 2
} BRStellarCryptoKeyType;

typedef enum _stellar_public_key_type
{
    PUBLIC_KEY_TYPE_ED25519 = KEY_TYPE_ED25519
} BRStellarPublicKeyType;

typedef enum _stellar_memo_type
{
    MEMO_NONE = 0,
    MEMO_TEXT = 1,
    MEMO_ID = 2,
    MEMO_HASH = 3,
    MEMO_RETURN = 4
} BRStellarMemoType;

typedef struct __stellar_memo {
    BRStellarMemoType memoType;
    uint64_t id;
    char text[28];
    uint8_t hash[32]; // the hash of what to pull from the content server
    uint8_t retHash[32]; // the hash of the tx you are rejecting
} BRStellarMemo;

typedef struct _br_account_id {
    BRStellarPublicKeyType accountType;
    uint8_t                accountID[32];
} BRStellarAccountID;

typedef enum _stellar_asset_type
{
    ASSET_TYPE_NATIVE = 0,
    ASSET_TYPE_CREDIT_ALPHANUM4 = 1,
    ASSET_TYPE_CREDIT_ALPHANUM12 = 2
} BRStellarAssetType;

typedef struct _stellar_asset_
{
    BRStellarAssetType type;
    char assetCode[12]; // This will either be 4 or 12 bytes value, right padded with 0's
    BRStellarAccountID issuer;
} BRStellarAsset;

typedef struct _stellar_price
{
    int32_t n; // numerator
    int32_t d; // denominator
} BRStellarPrice;

typedef struct _stellar_payment_op_ {
    BRStellarAccountID destination;
    BRStellarAsset     asset;         // what they end up with
    double             amount;        // amount they end up with
} BRStellarPaymentOp;

typedef struct _ManageSellOfferOp
{
    BRStellarAsset selling;
    BRStellarAsset buying;
    int64_t amount; // amount being sold. if set to 0, delete the offer
    BRStellarPrice price;  // price of thing being sold in terms of what you are buying
    
    // 0=create a new offer, otherwise edit an existing offer
    int64_t offerID;
} BRStellarManageSellOfferOp;

typedef enum __stellar_operation_type
{
    CREATE_ACCOUNT = 0,
    PAYMENT = 1,
    PATH_PAYMENT = 2,
    MANAGE_SELL_OFFER = 3,
    CREATE_PASSIVE_SELL_OFFER = 4,
    SET_OPTIONS = 5,
    CHANGE_TRUST = 6,
    ALLOW_TRUST = 7,
    ACCOUNT_MERGE = 8,
    INFLATION = 9,
    MANAGE_DATA = 10,
    BUMP_SEQUENCE = 11,
    MANAGE_BUY_OFFER = 12
} BRStellarOperationType;

typedef struct _stellar_operation_ {
    BRStellarOperationType type;
    BRStellarAccountID source; // Optional
    union _op_ {
        BRStellarPaymentOp         payment;
        BRStellarManageSellOfferOp mangeSellOffer;
    } operation;
} BRStellarOperation;

typedef enum __stellar_network_type {
    STELLAR_NETWORK_PUBLIC = 0,
    STELLAR_NETWORK_TESTNET = 1
} BRStellarNetworkType;

#endif
