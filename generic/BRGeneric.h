//
//  BRGeneric.h
//  Core
//
//  Created by Ed Gamble on 6/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRGeneric_h
#define BRGeneric_h

#include "support/BRInt.h" // UInt256
#include "support/BRSet.h" // BRSet
#include "support/BRKey.h" // BRKey
#include "support/BRArray.h"
#include "BRGenericBase.h"
#include "BRGenericClient.h"

#ifdef __cplusplus
extern "C" {
#endif

    // MARK: - Network

    extern BRGenericNetwork
    gwmNetworkCreate (const char * type);

    extern void
    gwmNetworkRelease (BRGenericNetwork network);

    extern BRGenericAddress
    gwmNetworkAddressCreate (BRGenericNetwork network, const char * address);

    extern void
    gwmNetworkAddressRelease (BRGenericNetwork network, BRGenericAddress address);

    // MARK: - Account

    extern BRGenericAccount
    gwmAccountCreate (const char *type,
                      UInt512 seed);

    extern BRGenericAccount
    gwmAccountCreateWithPublicKey (const char *type,
                                   BRKey publicKey);

    extern BRGenericAccount
    gwmAccountCreateWithSerialization (const char *type,
                                       uint8_t *bytes,
                                       size_t   bytesCount);
    
    extern void
    gwmAccountRelease (BRGenericAccount account);

    extern const char *
    gwmAccountGetType (BRGenericAccount account);

    extern int
    gwmAccountHasType (BRGenericAccount account,
                       const char *type);

    extern BRGenericAddress
    gwmAccountGetAddress (BRGenericAccount account);

    extern uint8_t *
    gwmAccountGetSerialization (BRGenericAccount account, size_t *bytesCount);

     // Address

    extern char *
    gwmAddressAsString (BRGenericNetwork nid,
                        BRGenericAddress aid);

    extern int
    gwmAddressEqual (BRGenericNetwork nid,
                     BRGenericAddress aid1,
                     BRGenericAddress aid2);

// Fee Basis
    // MARK: Fee Basis

extern UInt256 gwmGetFeeBasisPricePerCostFactor (BRGenericFeeBasis feeBasis);
extern double gwmGetFeeBasisCostFactor (BRGenericFeeBasis feeBasis);
extern uint32_t gwmGetFeeBasisIsEqual (BRGenericFeeBasis fb1, BRGenericFeeBasis fb2);
extern void gwmFeeBasisRelease (BRGenericFeeBasis feeBasis);


    // Transfer

    extern void
    gwmTransferRelease (BRGenericTransfer transfer);

    extern BRGenericAddress
    gwmTransferGetSourceAddress (BRGenericTransfer transfer);

    extern BRGenericAddress
    gwmTransferGetTargetAddress (BRGenericTransfer transfer);

    extern UInt256
    gwmTransferGetAmount (BRGenericTransfer transfer);

    extern UInt256
    gwmTransferGetFee (BRGenericTransfer transfer);

    extern BRGenericFeeBasis
    gwmTransferGetFeeBasis (BRGenericTransfer transfer);

    extern BRGenericHash
    gwmTransferGetHash (BRGenericTransfer transfer);

    // Wallet


    /**
     * Create the PrimaryWallet for `gwm`
     */
    extern BRGenericWallet
    gwmWalletCreate (BRGenericAccount account);

    extern void
    gwmWalletRelease (BRGenericWallet wallet);

    extern UInt256
    gwmWalletGetBalance (BRGenericWallet wallet);

    extern BRGenericAddress
    gwmWalletGetAddress (BRGenericWallet wid);

    extern BRGenericFeeBasis
    gwmWalletGetDefaultFeeBasis (BRGenericWallet wid);

    extern void
    gwmWalletSetDefaultFeeBasis (BRGenericWallet wid,
                                 BRGenericFeeBasis bid);

    extern BRGenericTransfer
    gwmWalletCreateTransfer (BRGenericWallet wid,
                             BRGenericAddress target,
                             UInt256 amount);
                             // ...

    extern UInt256
    gwmWalletEstimateTransferFee (BRGenericWallet wid,
                                  UInt256 amount,
                                  BRGenericFeeBasis feeBasis,
                                  int *overflow);

    extern void
    gwmWalletSubmitTransfer (BRGenericWallet wid,
                             BRGenericTransfer transfer,
                             UInt512 seed);

// MARK: Generic (Wallet) Manager

    extern BRGenericManager
    gwmCreate (BRGenericClient client,
               const char *type,
               BRGenericNetwork network,
               BRGenericAccount account,
               uint64_t accountTimestamp,
               const char *storagePath,
               uint32_t syncPeriodInSeconds,
               uint64_t blockHeight);

    extern void
    gwmRelease (BRGenericManager gwm);

    extern void
    gwmStop (BRGenericManager gwm);

    extern void
    gwmConnect (BRGenericManager gwm);

    extern void
    gwmDisconnect (BRGenericManager gwm);

    extern void
    gwmSync (BRGenericManager gwm);

#if 0
    extern BRArrayOf (BRGenericTransfer) // BRSetOf
    gwmRestorePersistentTransfers (BRGenericManager gwm);

    extern void
    gwmPersistTransfer (BRGenericManager gwm,
                        BRGenericTransfer  tid);
#endif
    extern BRGenericAddress
    gwmGetAccountAddress (BRGenericManager gwm);

    extern BRGenericWallet
    gwmCreatePrimaryWallet (BRGenericManager gwm);

    extern BRGenericAccount
    gwmGetAccount (BRGenericManager gwm);

    extern BRGenericNetwork
    gwmGetNetwork (BRGenericManager gwm);

    extern BRGenericClient
    gwmGetClient (BRGenericManager gwm);

    extern BRGenericTransfer
    gwmRecoverTransfer (BRGenericManager gwm, BRGenericWallet wallet,
                        const char *hash,
                        const char *from,
                        const char *to,
                        const char *amount,
                        const char *currency,
                        uint64_t timestamp,
                        uint64_t blockHeight);

    extern BRArrayOf(BRGenericTransfer)
    gwmRecoverTransfersFromRawTransaction (BRGenericManager gwm,
                                           uint8_t *bytes,
                                           size_t   bytesCount);

extern BRArrayOf(BRGenericTransfer)
gwmLoadTransfers (BRGenericManager gwm);


#endif /* BRGeneric_h */
