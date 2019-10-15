//
//  BRGenericHandlers.h
//  Core
//
//  Created by Ed Gamble on 6/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRGenericHandlers_h
#define BRGenericHandlers_h

#include "support/BRInt.h" // UInt256
#include "support/BRArray.h"
#include "support/BRKey.h"
#include "support/BRFileService.h"
#include "BRGenericBase.h"

#ifdef __cplusplus
extern "C" {
#endif

     // MARK: - Generic Network

     typedef BRGenericAddress (*BRGenericNetworkAddressCreate) (const char * address);
     typedef void (*BRGenericNetworkAddressFree) (BRGenericAddress address);

     typedef struct {
         BRGenericNetworkAddressCreate networkAddressCreate;
         BRGenericNetworkAddressFree networkAddressFree;
     } BRGenericNetworkHandlers;


     // MARK: - Generic Account

    typedef BRGenericAccount (*BRGenericAccountCreate) (const char *type, UInt512 seed);
    typedef BRGenericAccount (*BRGenericAccountCreateWithPublicKey) (const char *type, BRKey key);
    typedef BRGenericAccount (*BRGenericAccountCreateWithSerialization) (const char *type, uint8_t *bytes, size_t bytesCount);
    typedef void (*BRGenericAccountFree) (BRGenericAccount account);
    typedef BRGenericAddress (*BRGenericAccountGetAddress) (BRGenericAccount account);
    typedef uint8_t * (*BRGenericAccountGetSerialization) (BRGenericAccount account, size_t *bytesCount);
    typedef void (*BRGenericAccountSignTransferWithSeed) (BRGenericAccount account, BRGenericTransfer transfer, UInt512 seed);
    typedef void (*BRGenericAccountSignTransferWithKey) (BRGenericAccount account, BRGenericTransfer transfer, BRKey *key);

    typedef struct {
        BRGenericAccountCreate create;
        BRGenericAccountCreateWithPublicKey createWithPublicKey;
        BRGenericAccountCreateWithSerialization createWithSerialization;
        BRGenericAccountFree free;
        BRGenericAccountGetAddress getAddress;
        BRGenericAccountGetSerialization getSerialization;
        BRGenericAccountSignTransferWithSeed signTransferWithSeed;
        BRGenericAccountSignTransferWithKey signTransferWithKey;
    } BRGenericAccountHandlers;

    // MARK: - Generic Address

    typedef char * (*BRGenericAddressAsString) (BRGenericAddress address);
    typedef int (*BRGenericAddressEqual) (BRGenericAddress address1,
                                          BRGenericAddress address2);

    typedef struct {
        BRGenericAddressAsString asString;
        BRGenericAddressEqual equal;
    } BRGenericAddressHandlers;

    // MARK: - Generic Transfer

    typedef BRGenericTransfer (*BRGenericTransferCreate) (BRGenericAddress source,
                                                          BRGenericAddress target,
                                                          UInt256 amount);
    typedef void (*BRGenericTransferFree) (BRGenericTransfer transfer);
    typedef BRGenericAddress (*BRGenericTransferGetSourceAddress) (BRGenericTransfer transfer);
    typedef BRGenericAddress (*BRGenericTransferGetTargetAddress) (BRGenericTransfer transfer);
    typedef UInt256 (*BRGenericTransferGetAmount) (BRGenericTransfer transfer);
    typedef UInt256 (*BRGenericTransferGetFee) (BRGenericTransfer transfer);
    typedef BRGenericFeeBasis (*BRGenericTransferGetFeeBasis) (BRGenericTransfer transfer);
    typedef BRGenericTransferDirection (*BRGenericTransferGetDirection) (BRGenericTransfer transfer);
    typedef BRGenericHash (*BRGenericTransferGetHash) (BRGenericTransfer transfer);
    typedef uint8_t * (*BRGenericTransferGetSerialization) (BRGenericTransfer transfer, size_t *bytesCount);

    typedef struct {
        BRGenericTransferCreate create;
        BRGenericTransferFree   free;
        BRGenericTransferGetSourceAddress sourceAddress;
        BRGenericTransferGetTargetAddress targetAddress;
        BRGenericTransferGetAmount amount;
        BRGenericTransferGetFee fee;
        BRGenericTransferGetFeeBasis feeBasis;
        BRGenericTransferGetDirection direction;
        BRGenericTransferGetHash hash;
        BRGenericTransferGetSerialization getSerialization;
    } BRGenericTransferHandlers;

    // MARK: - Generic Wallet

    typedef BRGenericWallet (*BRGenericWalletCreate) (BRGenericAccount account);
    typedef void (*BRGenericWalletFree) (BRGenericWallet wallet);
    typedef UInt256 (*BRGenericWalletGetBalance) (BRGenericWallet wallet);
    typedef BRGenericTransfer (*BRGenericWalletCreateTransfer) (BRGenericWallet wallet,
                                                                BRGenericAddress address,
                                                                UInt256 amount,
                                                                BRGenericFeeBasis estimatedFeeBasis);

    typedef struct {
        BRGenericWalletCreate create;
        BRGenericWalletFree free;
        BRGenericWalletGetBalance balance;
        // set balance
        // ...
        BRGenericWalletCreateTransfer createTransfer;
    } BRGenericWalletHandlers;

    // MARK: - Generic (Wallet) Manager

    // Create a transfer from the
    typedef BRGenericTransfer (*BRGenericWalletManagerRecoverTransfer) (const char *hash,
                                                                        const char *from,
                                                                        const char *to,
                                                                        const char *amount,
                                                                        const char *currency,
                                                                        uint64_t timestamp,
                                                                        uint64_t blockHeight);

    typedef BRArrayOf(BRGenericTransfer) (*BRGenericWalletManagerRecoverTransfersFromRawTransaction) (uint8_t *bytes,
                                                                                                      size_t   bytesCount);

    typedef void (*BRGenericWalletManagerInitializeFileService) (BRFileServiceContext context,
                                                                 BRFileService fileService);

    typedef BRArrayOf(BRGenericTransfer) (*BRGenericWalletManagerLoadTransfers) (BRFileServiceContext context,
                                                                                 BRFileService fileService);

    typedef BRGenericAPISyncType (*BRGenericWalletManagerGetAPISyncType) (void);

    typedef struct {
        BRGenericWalletManagerRecoverTransfer transferRecover;
        BRGenericWalletManagerRecoverTransfersFromRawTransaction transfersRecoverFromRawTransaction;
        BRGenericWalletManagerInitializeFileService fileServiceInit;
        BRGenericWalletManagerLoadTransfers fileServiceLoadTransfers;
        BRGenericWalletManagerGetAPISyncType apiSyncType;
    } BRGenericManagerHandlers;

    // MARK: - Generic Handlers

    typedef struct BRGenericHandersRecord {
        const char *type;
        BRGenericNetworkHandlers network;
        BRGenericAccountHandlers account;
        BRGenericAddressHandlers address;
        BRGenericTransferHandlers transfer;
        BRGenericWalletHandlers wallet;
        BRGenericManagerHandlers manager;
    } *BRGenericHandlers;

    extern void
    genHandlersInstall (const BRGenericHandlers handlers);

    extern const BRGenericHandlers
    genHandlerLookup (const char *symbol);

#ifdef __cplusplus
}
#endif

#endif /* BRGenericHandlers_h */
