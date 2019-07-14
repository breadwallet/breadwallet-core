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

    typedef struct BRGenericHandersRecord *BRGenericHandlers;

    typedef struct BRGenericAccountWithTypeRecord {
        BRGenericHandlers handlers;
        BRGenericAccount base;
    } *BRGenericAccountWithType;;

    // MARK: - Generic Account

    typedef void * (*BRGenericAccountCreate) (const char *type, UInt512 seed);
    typedef void * (*BRGenericAccountCreateWithPublicKey) (const char *type, BRKey key);
    typedef void * (*BRGenericAccountCreateWithSerialization) (const char *type, uint8_t *bytes, size_t bytesCount);
    typedef void (*BRGenericAccountFree) (BRGenericAccount account);
    typedef BRGenericAddress (*BRGenericAccountGetAddress) (BRGenericAccount account);
    typedef uint8_t * (*BRGenericAccountGetSerialization) (BRGenericAccount account, size_t *bytesCount);

    // MARK: - Generic Address
    typedef char * (*BRGenericAddressAsString) (BRGenericAddress address);
    typedef int (*BRGenericAddressEqual) (BRGenericAddress address1,
                                          BRGenericAddress address2);

    // MARK: - Generic Treansfer
    typedef BRGenericAddress (*BRGenericTransferGetSourceAddress) (BRGenericTransfer transfer);
    typedef BRGenericAddress (*BRGenericTransferGetTargetAddress) (BRGenericTransfer transfer);
    typedef UInt256 (*BRGenericTransferGetAmount) (BRGenericTransfer transfer);
    typedef UInt256 (*BRGenericTransferGetFee) (BRGenericTransfer transfer);
    typedef BRGenericFeeBasis (*BRGenericTransferGetFeeBasis) (BRGenericTransfer transfer);
    typedef BRGenericHash (*BRGenericTransferGetHash) (BRGenericTransfer transfer);

    // MARK: - Generic Wallet
    typedef BRGenericWallet (*BRGenericWalletCreate) (BRGenericAccount account);
    typedef void (*BRGenericWalletFree) (BRGenericWallet wallet);
    typedef UInt256 (*BRGenericWalletGetBalance) (BRGenericWallet wallet);

    // MAEK: - Generic Wallet Manager

    typedef BRGenericTransfer (*BRGenericWalletManagerRecoverTransfer) (uint8_t *bytes,
                                                                        size_t   bytesCount);

    typedef void (*BRGenericWalletManagerInitializeFileService) (BRFileServiceContext context,
                                                                 BRFileService fileService);

    typedef BRArrayOf(BRGenericTransfer) (*BRGenericWalletManagerLoadTransfers) (BRFileServiceContext context,
                                                                                 BRFileService fileService);

    // MARK: - Generic Handlers

    struct BRGenericHandersRecord {
        const char *type;
        struct {
            BRGenericAccountCreate create;
            BRGenericAccountCreateWithPublicKey createWithPublicKey;
            BRGenericAccountCreateWithSerialization createWithSerialization;
            BRGenericAccountFree free;
            BRGenericAccountGetAddress getAddress;
            BRGenericAccountGetSerialization getSerialization;
        } account;

        struct {
            BRGenericAddressAsString asString;
            BRGenericAddressEqual equal;
        } address;
        
        struct {
            // create
            // free
            // ...
            BRGenericTransferGetSourceAddress sourceAddress;
            BRGenericTransferGetTargetAddress targetAddress;
            BRGenericTransferGetAmount amount;
            BRGenericTransferGetFee fee;
            BRGenericTransferGetFeeBasis feeBasis;
            BRGenericTransferGetHash hash;
        } transfer;

        struct {
            BRGenericWalletCreate create;
            BRGenericWalletFree free;
            BRGenericWalletGetBalance balance;
            // set balance

            // ...
        } wallet;

        struct {
            BRGenericWalletManagerRecoverTransfer transferRecover;

            BRGenericWalletManagerInitializeFileService fileServiceInit;
            BRGenericWalletManagerLoadTransfers fileServiceLoadTransfers;
        } manager;
    };

    extern void
    genericHandlersInstall (const BRGenericHandlers handlers);

    extern const BRGenericHandlers
    genericHandlerLookup (const char *symbol);

#ifdef __cplusplus
}
#endif

#endif /* BRGenericHandlers_h */
