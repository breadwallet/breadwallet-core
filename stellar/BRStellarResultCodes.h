//
//  BRStellarResultCodes.h
//  Core
//
//  Created by Carl Cherry on 6/11/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRStellar_result_codes_h
#define BRStellar_result_codes_h

#ifdef __cplusplus
extern "C" {
#endif

// Transaction Result code - top level. A transaction with multiple
// operations acts as an atomic unit. I can only assume that if some
// operations succeed but one fails then at the high level it would
// report to be ST_TX_FAILED
typedef enum st_result_code
{
    ST_TX_SUCCESS = 0,
    ST_TX_FAILED = -1,
    ST_TX_TOO_EARLY = -2,
    ST_TX_TOO_LATE = -3,
    ST_TX_MISSING_OPERATION = -4,
    ST_TX_BAD_SEQ = -5,
    ST_TX_BAD_AUTH = -6,
    ST_TX_INSUFFICIENT_BALANCE = -7,
    ST_TX_NO_ACCOUNT = -8,
    ST_TX_INSUFFICIENT_FEE = -9,
    ST_TX_BAD_AUTH_EXTRA = -10,
    ST_TX_INTERNAL_ERROR = -11,
} BRStellarTXResultCode;

// For some reason there is an extra level of status
// for operations. i.e.
// array of ops
//    [0] ST_OP_RESULT_CODE_INNER
//      [tr] ST_OP_PAYMENT
//         [paymentResult] ST_PAYMENT_SUCCESS
//    [1] ST_OP_RESULT_CODE_INNER
//      [tr] ST_OP_PAYMENT
//         [paymentResult] ST_PAYMENT_SUCCESS
typedef enum st_operation_result_code {
    ST_OP_RESULT_CODE_INNER = 0,
    ST_OP_RESULT_CODE_BAD_AUTH = -1,
    ST_OP_RESULT_CODE_NO_ACCOUNT = -2,
    ST_OP_RESULT_CODE_NOT_SUPPORTED = -3,
    ST_OP_RESULT_CODE_TOO_MANY_SUBENTRIES = -4,
    ST_OP_RESULT_CODE_EXCEEDED_WORK_LIMIT = -5
} BRStellarOperationResultCode;

typedef enum st_payment_result_code
{
    // codes considered as "success" for the operation
    ST_PAYMENT_SUCCESS = 0, // payment successfuly completed
    
    // codes considered as "failure" for the operation
    ST_PAYMENT_MALFORMED = -1,          // bad input
    ST_PAYMENT_UNDERFUNDED = -2,        // not enough funds in source account
    ST_PAYMENT_SRC_NO_TRUST = -3,       // no trust line on source account
    ST_PAYMENT_SRC_NOT_AUTHORIZED = -4, // source not authorized to transfer
    ST_PAYMENT_NO_DESTINATION = -5,     // destination account does not exist
    ST_PAYMENT_NO_TRUST = -6,       // destination missing a trust line for asset
    ST_PAYMENT_NOT_AUTHORIZED = -7, // destination not authorized to hold asset
    ST_PAYMENT_LINE_FULL = -8,      // destination would go above their limit
    ST_PAYMENT_NO_ISSUER = -9       // missing issuer on asset
} BRStellarPaymentResultCode;

typedef enum _account_merge_result_code
{
    // codes considered as "success" for the operation
    ST_ACCOUNT_MERGE_SUCCESS = 0,
    // codes considered as "failure" for the operation
    ST_ACCOUNT_MERGE_MALFORMED = -1,       // can't merge onto itself
    ST_ACCOUNT_MERGE_NO_ACCOUNT = -2,      // destination does not exist
    ST_ACCOUNT_MERGE_IMMUTABLE_SET = -3,   // source account has AUTH_IMMUTABLE set
    ST_ACCOUNT_MERGE_HAS_SUB_ENTRIES = -4, // account has trust lines/offers
    ST_ACCOUNT_MERGE_SEQNUM_TOO_FAR = -5,  // sequence number is over max allowed
    ST_ACCOUNT_MERGE_DEST_FULL = -6        // can't add source balance to
    // destination balance
} BRStellarAccountMergeResultCode;

typedef enum st_account_create_result_code
{
    ST_CREATE_ACCOUNT_SUCCESS = 0,
    ST_CREATE_ACCOUNT_MALFORMED = -1,
    ST_CREATE_ACCOUNT_UNDERFUNDED = -2,
    ST_CREATE_ACCOUNT_LOW_RESERVE = -3,
    ST_CREATE_ACCOUNT_ALREADY_EXIST = -4
} BRStellarCreateAccountResultCodes;

typedef enum st_path_payment_result_code
{
    ST_PATH_PAYMENT_SUCCESS = 0,
    ST_PATH_PAYMENT_MALFORMED = -1,
    ST_PATH_PAYMENT_UNDERFUNDED = -2,
    ST_PATH_PAYMENT_SRC_NO_TRUST = -3,
    ST_PATH_PAYMENT_SRC_NOT_AUTHORIZED = -4,
    ST_PATH_PAYMENT_NO_DESTINATION = -5,
    ST_PATH_PAYMENT_NO_TRUST = -6,
    ST_PATH_PAYMENT_NOT_AUTHORIZED = -7,
    ST_PATH_PAYMENT_LINE_FULL = -8,
    ST_PATH_PAYMENT_NO_ISSUER = -9,
    ST_PATH_PAYMENT_TOO_FEW_OFFERS = -10,
    ST_PATH_PAYMENT_OFFER_CROSS_SELF = -11,
    ST_PATH_PAYMENT_OVER_SENDMAX = -12
} BRStellarPathPaymentResultCodes;

typedef enum st_set_options_result_code
{
    ST_SET_OPTIONS_SUCCESS = 0,
    ST_SET_OPTIONS_LOW_RESERVE = -1,
    ST_SET_OPTIONS_TOO_MANY_SIGNERS = -2,
    ST_SET_OPTIONS_BAD_FLAGS = -3,
    ST_SET_OPTIONS_INVALID_INFLATION = -4,
    ST_SET_OPTIONS_CANT_CHANGE = -5,
    ST_SET_OPTIONS_UNKNOWN_FLAG = -6,
    ST_SET_OPTIONS_THRESHOLD_OUT_OF_RANGE = -7,
    ST_SET_OPTIONS_BAD_SIGNER = -8,
    ST_SET_OPTIONS_INVALID_HOME_DOMAIN = -9
} BRStellarSetOptionsResultCodes;


#ifdef __cplusplus
}
#endif

#endif // BRStellar_result_codes_h
