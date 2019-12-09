//
//  BRCryptoSync.h
//  BRCore
//
//  Created by Ed Gamble on 11/27/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoSync_h
#define BRCryptoSync_h

#ifdef __cplusplus
extern "C" {
#endif

    /// MARK: Sync Stopped Reason

    typedef enum {
        CRYPTO_SYNC_STOPPED_REASON_COMPLETE,
        CRYPTO_SYNC_STOPPED_REASON_REQUESTED,
        CRYPTO_SYNC_STOPPED_REASON_UNKNOWN,
        CRYPTO_SYNC_STOPPED_REASON_POSIX
    } BRCryptoSyncStoppedReasonType;

    typedef struct {
        BRCryptoSyncStoppedReasonType type;
        union {
            struct {
                int errnum;
            } posix;
        } u;
    } BRCryptoSyncStoppedReason;

    extern BRCryptoSyncStoppedReason
    cryptoSyncStoppedReasonComplete(void);

    extern BRCryptoSyncStoppedReason
    cryptoSyncStoppedReasonRequested(void);

    extern BRCryptoSyncStoppedReason
    cryptoSyncStoppedReasonUnknown(void);

    extern BRCryptoSyncStoppedReason
    cryptoSyncStoppedReasonPosix(int errnum);

    /**
     * Return a descriptive message as to why the sync stopped.
     *
     *@return the detailed reason as a string or NULL
     */
    extern char *
    cryptoSyncStoppedReasonGetMessage(BRCryptoSyncStoppedReason *reason);

    /// MARK: Sync Mode

    ///
    /// The modes supported for syncing of a wallet's transactions.  These are the supported modes
    /// but they are not necessarily available for any individual WalletKit execution.
    /// Specifically, the API_ONLY mode may be supported but if the backend services are not
    /// accessible, such as if a device is in 'airplane mode', then API_ONLY will not be available.
    ///
    typedef enum {
        /**
         * Use the BRD backend for all Core blockchain state.  The BRD backend includes a 'submit
         * transaction' interface.
         */
        CRYPTO_SYNC_MODE_API_ONLY,

        /**
         * Use the BRD backend for everything other than 'submit transaction'
         */
        CRYPTO_SYNC_MODE_API_WITH_P2P_SEND,

        /**
         * Use the BRD backend for an initial sync and then, once complete, use P2P.  If a sync
         * has not occurred in a while, use the BRD backend again before using P2P (so as to catch-up
         * quickly)
         */
        CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC,

        /**
         * Use acomplete block chain sync, even starting at block zero (but usually from a block
         * derived from the accounts `earliestStartTime` (or the BIP-39 introduction block).
         */
        CRYPTO_SYNC_MODE_P2P_ONLY
    } BRCryptoSyncMode;

#define NUMBER_OF_SYNC_MODES    (1 + CRYPTO_SYNC_MODE_P2P_ONLY)

    extern const char *
    cryptoSyncModeString (BRCryptoSyncMode m);

    /// MARK: Sync Depth

    typedef enum {
        /**
         * Sync from the block height of the last confirmed send transaction.
         */
        CRYPTO_SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND,

        /**
         * Sync from the block height of the last trusted block; this is dependent on the
         * blockchain and mode as to how it determines trust.
         */
        CRYPTO_SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK,

        /**
         * Sync from the block height of the point in time when the account was created.
         */
        CRYPTO_SYNC_DEPTH_FROM_CREATION
    } BRCryptoSyncDepth;

    /// The Percent Complete (0...100.0) derived from the last block processed relative to the
    /// full block range in a sync.
    typedef float BRCryptoSyncPercentComplete;

#define AS_CRYPTO_SYNC_PERCENT_COMPLETE(number)    ((BRCryptoSyncPercentComplete) (number))

    /// The Timestamp (in the Unix epoch) of the last block processed in a sync.
    typedef uint32_t BRCryptoSyncTimestamp;

#define AS_CRYPTO_SYNC_TIMESTAMP(unixSeconds)      ((BRCryptoSyncTimestamp) (unixSeconds))
#define NO_CRYPTO_SYNC_TIMESTAMP                   (AS_CRYPTO_SYNC_TIMESTAMP (0))

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoSync_h */
