//
//  BREthereumBase.h
//  BRCore
//
//  Created by Ed Gamble on 11/19/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//

#ifndef BREthereumBase_h
#define BREthereumBase_h

#include "ethereum/base/BREthereumBase.h"

/**
 * An Ethereum Account holds the public key data associated with a User's 'BIP-39 Paper Key'.
 *
 * The account provides the ethereum address (in both string and byte form) and the nonce.  (The
 * account nonce represent the number of ethereum transactions originated by the account; it is
 * monotonically increasing and is used to guard against double spends.) As per the Ethereum
 * specification and unlike bitcoin, the account has a single address.
 */
typedef struct BREthereumAccountRecord *BREthereumAccount;

/**
 * An Ethereum Transfer represents an exchange of a asset between a source and a target address.
 *
 * The ethereum specification does not define a 'transfer'; however, the specificaion does define
 * `transaction` and `log` and depending on the 'Smart Contract' a `log` can represent an
 * exchange (such as for ERC20).  An Ethereum Transfer is thus an abstraction over a 'transaction'
 * and a 'log' (for such an exchange)
 *
 * An Ethereum Transfer has both a source and a target address at least one of which will be the
 * address for the User's account. A transfer includes an amount.  (The amount is a value in a
 * specfiic currency - ETH, ERC20 token).  A transfer includes a `feeBasis` and a `gasEstimate`
 * where are Ethereum specific concepts used to estimate/compute the fee for the transfers.
 *
 * If the Ethereum Transfer is created by the User, then it has an `originatingTransaction`.  This
 * transaction is the submission the the Ethereum P2P network needed to accomplish the transfer.
 *
 * Once the transfer has been submitted it will have a 'Transfer Basis'.  The basis is represents
 * the outcome of the submission - it will be either a transaction or a log with an appropriate
 * status (such as 'included' or 'errored')
 *
 * A transfer includes a status.
 */
typedef struct BREthereumTransferRecord *BREthereumTransfer;

/**
 * An Ethereum Wallet holds a balance and the transfers applicable to a specific currency in a
 * User's account.  A User likely has multiple wallets - such as for ETH and BRD - all of which are
 * associated with a single account.
 *
 * A wallet references the User's account and the account's primary address.
 *
 * A wallet has an optional Ethereum Token'.  If present, the wallet holds transfers for an ERC20
 * (and perhaps other Smart Contract) tokens.  If not present, the wallet holds ETH transfers.
 *
 * A wallet is associated with a network.  Thus, a single account might have different wallets for
 * different networks - such as 'foundation', 'ropsten' and 'rinkeby' (which is, in fact, only a
 * testing feature).
 *
 * A wallet holds a default gas price and a default gas limit.  These are used when creating a
 * transfer.  Different assets (ETH, BRC, etc) require different amounts of gas to execute on the
 * Ethereum P2P network.  Different assets might demand a different gas price (essentially the
 * speed at which the P2P network includes the transfer).  These defaults can be set based on the
 * needs of the wallet's asset.
 *
 * A wallet holds all the transfers - both with User being the source and/or the target.  Depending
 * on the asset the transfers could be based on a transaction or a log.  It should be the case that
 * the balance is the sum total of the transfers; however, there is an important caveat - if the
 * wallet represents a token asset, then the fees for a transfer apply to the ETH wallet.
 */
typedef struct BREthereumWalletRecord *BREthereumWallet;

/**
 * An Ethereum Wallet Manager (EWM) manages all the wallets associated with a single account on a
 * specific Ethereum P2P network.  An EWM is the sole/primary interface between IOS and Android
 * applications with Ethereum wallets.
 *
 * An EWM defines a 'client' with a set of callback interfaces that must be implemented by the
 * IOS and Android applications.  These callbacks privide functionality that is architecturally
 * inappropriate to include in Core code - such as HTTP queries which use libraries that are not
 * and will not be part of Core code.
 *
 * An EWM is associated with a particular account and a network.  There may be multiple EWMs with
 * one per network - such as one EWM for 'foundation', 'ropsten' and 'rinkeby' (essentially a
 * testing feature).  All EWMs are expected to share a single account - although the interface
 * allows a per EWM account.
 *
 * An EWM holds multipe wallets with one ETH wallet and zero or more ERC20 (or other smart
 * contract) token wallets.  Generally wallets for tokens are created as needed - specifically while
 * scanning the block chain for logs applicable to the account; any log representing an ERC20
 * transfer for a known token will produce a new wallet.  The client interface includes a callback
 * to announce new wallets.
 *
 * An EWM stores persistent data, such as for blocks, peers, transactions and logs using a
 * BRFileService.
 *
 * An EWM runs in its own pthread.  Interactions with the EWM are either asynchronous or blocking
 * and protected by a mutex in the EWM.  Generally blocking functions are those that query EWM state
 * whereas asynchronous functions are those computing or updating something.  An asynchronous
 * function always leads to a client callback of some kind to announce the result of the
 * computation.
 *
 * An EWM has a specific mode.  The mode determines how the EWM scans the block chain.  If the EWM
 * uses the P2P network (as opposed to a fully 'BRD Service Assisted' mode), then the EWM references
 * a 'BCS' (block chain slice) object through with all P2P interactions proceed.
 */
typedef struct BREthereumEWMRecord *BREthereumEWM;

typedef enum {
    FEE_BASIS_NONE,
    FEE_BASIS_GAS
} BREthereumFeeBasisType;

typedef struct {
    BREthereumFeeBasisType type;
    union {
        struct {
            BREthereumGas limit;
            BREthereumGasPrice price;
        } gas;
    } u;
} BREthereumFeeBasis;

extern BREthereumFeeBasis
feeBasisCreate (BREthereumGas limit,
                BREthereumGasPrice price);

//
// Errors - Right Up Front - 'The Emperor Has No Clothes' ??
//
typedef enum {
    SUCCESS,

    // Reference access
    ERROR_UNKNOWN_NODE,
    ERROR_UNKNOWN_TRANSACTION,
    ERROR_UNKNOWN_ACCOUNT,
    ERROR_UNKNOWN_WALLET,
    ERROR_UNKNOWN_BLOCK,
    ERROR_UNKNOWN_LISTENER,

    // Node
    ERROR_NODE_NOT_CONNECTED,

    // Transfer
    ERROR_TRANSACTION_HASH_MISMATCH,
    ERROR_TRANSACTION_SUBMISSION,

    // Acount
    // Wallet
    // Block
    // Listener

    // Numeric
    ERROR_NUMERIC_PARSE,

} BREthereumStatus;


#endif /* BREthereumBase_h */
