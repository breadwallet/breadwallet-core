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
#include <stdbool.h>
#include "support/BRArray.h"
#include "BRStellarResultCodes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STELLAR_ADDRESS_BYTES   (56)

typedef enum st_network_type {
    STELLAR_NETWORK_PUBLIC = 0,
    STELLAR_NETWORK_TESTNET = 1
} BRStellarNetworkType;

typedef enum st_crypto_key_type
{
    KEY_TYPE_ED25519 = 0,
    KEY_TYPE_PRE_AUTH_TX = 1,
    KEY_TYPE_HASH_X = 2
} BRStellarCryptoKeyType;

typedef enum st_public_key_type
{
    // The only key/pair type supported by Stellar
    PUBLIC_KEY_TYPE_ED25519 = KEY_TYPE_ED25519
} BRStellarPublicKeyType;

typedef enum st_memo_type
{
    MEMO_NONE = 0,
    MEMO_TEXT = 1,
    MEMO_ID = 2,
    MEMO_HASH = 3,
    MEMO_RETURN = 4
} BRStellarMemoType;

typedef enum st_asset_type
{
    ASSET_TYPE_NATIVE = 0,
    ASSET_TYPE_CREDIT_ALPHANUM4 = 1,
    ASSET_TYPE_CREDIT_ALPHANUM12 = 2
} BRStellarAssetType;

typedef enum st_key_type
{
    SIGNER_KEY_TYPE_ED25519 = KEY_TYPE_ED25519,
    SIGNER_KEY_TYPE_PRE_AUTH_TX = KEY_TYPE_PRE_AUTH_TX,
    SIGNER_KEY_TYPE_HASH_X = KEY_TYPE_HASH_X
} BRStellarSignerKeyType;

typedef enum st_operation_type
{
    ST_OP_CREATE_ACCOUNT = 0,
    ST_OP_PAYMENT = 1,
    ST_OP_PATH_PAYMENT = 2,
    ST_OP_MANAGE_SELL_OFFER = 3,
    ST_OP_CREATE_PASSIVE_SELL_OFFER = 4,
    ST_OP_SET_OPTIONS = 5,
    ST_OP_CHANGE_TRUST = 6,
    ST_OP_ALLOW_TRUST = 7,
    ST_OP_ACCOUNT_MERGE = 8,
    ST_OP_INFLATION = 9,
    ST_OP_MANAGE_DATA = 10,
    ST_OP_BUMP_SEQUENCE = 11,
    ST_OP_MANAGE_BUY_OFFER = 12
} BRStellarOperationType;

typedef enum st_manage_offer_type
{
    ST_MANAGE_OFFER_CREATED = 0,
    ST_MANAGE_OFFER_UPDATED = 1,
    ST_MANAGE_OFFER_DELETED = 2
} BRStellarManageOfferType;

typedef uint32_t BRStellarFee;
typedef int64_t  BRStellarSequence;
typedef uint64_t TimePoint;
typedef double   BRStellarAmount;

// A Stellar Address - 56 bytes
typedef struct {
    char bytes[STELLAR_ADDRESS_BYTES + 1]; // NULL terminated string
} BRStellarAddress;

// A Stellar Transaction Hash
typedef struct {
    uint8_t bytes[32];
} BRStellarTransactionHash;

// Stucture to hold the decorated signature
typedef struct {
    uint8_t signature[68];
} BRStellarSignatureRecord;
typedef BRStellarSignatureRecord *BRStellarSignature;

typedef struct st_time_bounds {
    TimePoint minTime;
    TimePoint maxTime;
} BRStellarTimeBounds;

typedef struct st_memo {
    BRStellarMemoType memoType;
    uint64_t id;
    char text[28];
    uint8_t hash[32]; // the hash of what to pull from the content server
    uint8_t retHash[32]; // the hash of the tx you are rejecting
} BRStellarMemo;

typedef struct st_account_id {
    BRStellarPublicKeyType accountType;
    uint8_t                accountID[32];
} BRStellarAccountID;

typedef struct st_asset
{
    BRStellarAssetType type;
    char assetCode[12]; // This will either be 4 or 12 bytes value, right padded with 0's
    BRStellarAccountID issuer;
} BRStellarAsset;

typedef struct st_price
{
    int32_t n; // numerator
    int32_t d; // denominator
} BRStellarPrice;

typedef struct st_payment_op {
    BRStellarAccountID       destination;
    BRStellarAsset           asset;         // what they end up with
    BRStellarAmount          amount;        // amount they end up with
} BRStellarPaymentOp;

typedef struct st_path_payment_op
{
    BRStellarAsset           sendAsset;   // asset we pay with
    BRStellarAmount          sendMax;     // the maximum amount of sendAsset to
                                          // send (excluding fees).
                                          // The operation will fail if can't be met
    BRStellarAccountID       destination; // recipient of the payment
    BRStellarAsset           destAsset;   // what they end up with
    BRStellarAmount          destAmount;  // amount they end up with
    
    BRStellarAsset           path[5];     // additional hops it must go through to get there
    uint32_t                 numPaths;    // how many actual paths are there
} BRStellarPathPaymentOp;

typedef struct _stellar_create_account_op_ {
    BRStellarAccountID       account;
    BRStellarAmount          startingBalance; // amount account is created with
} BRStellarCreateAccountOp;

typedef struct st_claim_offer_atom
{
    BRStellarAccountID sellerID; // Account that owns the offer
    uint64_t           offerID;

    // amount and asset taken from the owner
    BRStellarAsset assetSold;
    double         amountSold;

    // amount and asset sent to the owner
    BRStellarAsset assetBought;
    double         amountBought;
} BRStellarClaimOfferAtom;

typedef struct st_offer_entry
{
    BRStellarAccountID sellerID;
    uint64_t           offerID;
    BRStellarAsset     selling; // A
    BRStellarAsset     buying;  // B
    double             amount;  // amount of A

    /* price for this offer:
     price of A in terms of B
     price=AmountB/AmountA=priceNumerator/priceDenominator
     price is after fees
     */
    BRStellarPrice price;
    uint32_t       flags; // see OfferEntryFlags

    // OfferEntry object also has a reserved int32 at the
    // end of the structure.
} BRStellarOfferEntry;

typedef struct st_manage_offer_success_result
{
    BRArrayOf(BRStellarClaimOfferAtom) claimOfferAtom;
    uint32_t                           offerType;
    BRStellarOfferEntry                offer;
} BRStellarManageOfferResult;

typedef struct st_manage_sell_offer
{
    BRStellarAsset           selling;
    BRStellarAsset           buying;
    BRStellarAmount          amount; // amount being sold. if set to 0, delete the offer
    BRStellarPrice           price;  // price of thing being sold in terms of what you are buying
    int64_t                  offerID; // 0=create a new offer, otherwise edit an existing offer
    BRStellarManageOfferResult offerResult;
} BRStellarManageSellOfferOp;

typedef struct st_manage_buy_offer
{
    BRStellarAsset           selling;
    BRStellarAsset           buying;
    BRStellarAmount          amount; // amount being sold. if set to 0, delete the offer
    BRStellarPrice           price;  // price of thing being sold in terms of what you are buying
    int64_t                  offerID; // 0=create a new offer, otherwise edit an existing offer
    BRStellarManageOfferResult offerResult;
} BRStellarManageBuyOfferOp;

typedef struct st_passive_sell_offer_op
{
    BRStellarAsset           selling; // A
    BRStellarAsset           buying;  // B
    BRStellarAmount          amount;  // amount taker gets. if set to 0, delete the offer
    BRStellarPrice           price;   // cost of A in terms of B
    BRStellarManageOfferResult offerResult;
} BRStellarPassiveSellOfferOp;

typedef struct st_change_trust
{
    BRStellarAsset  line;
    uint64_t        limit;
} BRStellarChangeTrustOp;

typedef struct st_bump_sequence
{
    uint64_t  sequence; // The number to bump to
} BRStellarBumpSequenceOp;

typedef struct st_allow_trust
{
    BRStellarAccountID  trustor;
    uint32_t            assetType;
    char                assetCode[12];
    bool                authorize;
} BRStellarAllowTrustOp;

typedef struct st_signer_key_
{
    BRStellarSignerKeyType keyType;
    uint8_t key[32];
} BRStellerSignerKey;

typedef struct st_signer
{
    BRStellerSignerKey key;
    uint32_t weight; // really only need 1 byte
} BRStellarSigner;

typedef struct st_set_options_op
{
    // This one is a real pain to do in C - but these fields are
    // all optional and I don't know what the values are so we can't
    // use 0 as a way knowing if a field is present - so the two choices are
    // use all pointer and then have to deal with allocating/deleting or have
    // flags for all the setting to determine if they are set.  Going with
    // the latter
    BRStellarAccountID inflationDest; // sets the inflation destination

    uint32_t clearFlags; // which flags to clear
    uint32_t setFlags;   // which flags to set

    // account threshold manipulation
    uint32_t masterWeight; // weight of the master account
    uint32_t lowThreshold;
    uint32_t medThreshold;
    uint32_t highThreshold;

    char homeDomain[32]; // sets the home domain, not sure what you do with 32 chars

    // Add, update or remove a signer for the account
    // signer is deleted if the weight is 0
    BRStellarSigner signer;

    // Now an array of flags to tell us which settings are valid
    // They will be in the order that they appear above. i.e.
    // if homeDomain is set then settings[7] will be set to 1
    uint8_t settings[9];
} BRStellarSetOptionsOp;

typedef struct st_account_merge
{
    BRStellarAccountMergeResultCode resultCode;
    BRStellarAccountID              destination;
    BRStellarAmount                 balance;
} BRStellarAccountMergeOp;

typedef struct st_manage_data
{
    char                      key[64];
    char                      value[64];
    int32_t                   resultCode;
} BRStellarManageDataOp;

typedef struct st_inflation_payout
{
    BRStellarAccountID  destination;
    double              amount;
} BRStellarInflationPayout;

typedef struct st_inflation {
    BRArrayOf(BRStellarInflationPayout) payouts;
} BRStellarInflationOp;

typedef struct st_operation {
    BRStellarOperationType  type;
    BRStellarAccountID      source;     // Optional
    int32_t                 resultCode; // If we parse result_xdr this tells us
                                        // about the success/error
    union _op_ {
        BRStellarPaymentOp          payment;
        BRStellarManageSellOfferOp  manageSellOffer;
        BRStellarCreateAccountOp    createAccount;
        BRStellarPathPaymentOp      pathPayment;
        BRStellarPassiveSellOfferOp passiveSellOffer;
        BRStellarSetOptionsOp       options;
        BRStellarChangeTrustOp      changeTrust;
        BRStellarAllowTrustOp       allowTrust;
        BRStellarAccountMergeOp     accountMerge;
        BRStellarManageDataOp       manageData;
        BRStellarBumpSequenceOp     bumpSequence;
        BRStellarManageBuyOfferOp   manageBuyOffer;
        BRStellarInflationOp        inflation;
    } operation;
} BRStellarOperation;

typedef struct st_tx_result {
    int32_t               resultParsed; // 0 - not parse, 1 - parsed
    BRStellarTXResultCode resultCode;
    uint64_t              fee; // Not sure why this is 64 bits???
                               // When we send a payment it is only 32 bits.
} BRStellarTransactionResult;


#ifdef __cplusplus
}
#endif

#endif // BRStellar_base_h
