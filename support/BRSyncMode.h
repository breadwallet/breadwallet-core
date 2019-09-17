//
//  BRSyncMode.h
//  BRCore
//
//  Created by Ed Gamble on 3/18/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//

#ifndef BRSyncMode_h
#define BRSyncMode_h

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYNC_STOPPED_REASON_COMPLETE,
    SYNC_STOPPED_REASON_REQUESTED,
    SYNC_STOPPED_REASON_UNKNOWN,
    SYNC_STOPPED_REASON_POSIX
} BRSyncStoppedReasonType;

typedef struct {
    BRSyncStoppedReasonType type;
    union {
        struct {
            int errnum;
        } posix;
    } u;
} BRSyncStoppedReason;

extern BRSyncStoppedReason
BRSyncStoppedReasonComplete(void);

extern BRSyncStoppedReason
BRSyncStoppedReasonRequested(void);

extern BRSyncStoppedReason
BRSyncStoppedReasonUnknown(void);

extern BRSyncStoppedReason
BRSyncStoppedReasonPosix(int errnum);

extern const char *
BRSyncStoppedReasonPosixGetMessage(BRSyncStoppedReason *reason);

typedef enum {
    DISCONNECT_REASON_REQUESTED,
    DISCONNECT_REASON_UNKNOWN,
    DISCONNECT_REASON_POSIX
} BRDisconnectReasonType;

typedef struct {
    BRDisconnectReasonType type;
    union {
        struct {
            int errnum;
        } posix;
    } u;
} BRDisconnectReason;

extern BRDisconnectReason
BRDisconnectReasonRequested(void);

extern BRDisconnectReason
BRDisconnectReasonUnknown(void);

extern BRDisconnectReason
BRDisconnectReasonPosix(int errnum);

extern const char *
BRDisconnectReasonPosixGetMessage(BRDisconnectReason *reason);

typedef enum {
    /**
     * Use the BRD backend for all Core blockchain state.  The BRD backend includes a 'submit
     * transaction' interface.
     */
    SYNC_MODE_BRD_ONLY,

    /**
     * Use the BRD backend for everything other than 'submit transaction'
     */
    SYNC_MODE_BRD_WITH_P2P_SEND,

    /**
     * Use the BRD backend for an initial sync and then, once complete, use P2P.  If a sync
     * has not occurred in a while, use the BRD backend again before using P2P (so as to catch-up
     * quickly)
     */
    SYNC_MODE_P2P_WITH_BRD_SYNC,

    /**
     * Use acomplete block chain sync, even starting at block zero (but usually from a block
     * derived from the accounts `earliestStartTime` (or the BIP-39 introduction block).
     */
    SYNC_MODE_P2P_ONLY
} BRSyncMode;

extern const char *
BRSyncModeString (BRSyncMode m);

typedef enum {
    /**
     * Sync from the block height of the last confirmed send transaction.
     */
    SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND,

    /**
     * Sync from the block height of the last trusted block; this is dependent on the
     * blockchain and mode as to how it determines trust.
     */
    SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK,

    /**
     * Sync from the block height of the point in time when the account was created.
     */
    SYNC_DEPTH_FROM_CREATION
} BRSyncDepth;

/// The Percent Complete (0...100.0) derived from the last block processed relative to the
/// full block range in a sync.
typedef float BRSyncPercentComplete;

#define AS_SYNC_PERCENT_COMPLETE(number)    ((BRSyncPercentComplete) (number))

/// The Timestamp (in the Unix epoch) of the last block processed in a sync.
typedef uint32_t BRSyncTimestamp;

#define AS_SYNC_TIMESTAMP(unixSeconds)      ((BRSyncTimestamp) (unixSeconds))
#define NO_SYNC_TIMESTAMP                   (AS_SYNC_TIMESTAMP (0))

#ifdef __cplusplus
}
#endif

#endif /* BRSyncMode_h */
