//
//  BRPeerManager.h
//
//  Created by Aaron Voisine on 9/2/15.
//  Copyright (c) 2015 breadwallet LLC.
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

#ifndef BRPeerManager_h
#define BRPeerManager_h

#include "BRWallet.h"
#include "BRTransaction.h"
#include <stddef.h>

typedef enum {
    BRPeerManagerErrorNone = 0,
    BRPeerManagerErrorTxNotSigned,
    BRPeerManagerErrorNotConnected,
    BRPeerManagerErrorTimedOut = 1001,
    BRPeerManagerErrorDoubleSpend
} BRPeerManagerError;

typedef struct {
    int connected;
    struct BRPeerManagerContext *context;
} BRPeerManager;

// returns a newly allocated BRPeerManager struct that must be freed by calling BRPeerManagerFree()
BRPeerManager *BRPeerManagerCreate(void *(*alloc)(size_t), BRWallet *wallet);

// connect to bitcoin peer-to-peer network
void BRPeerManagerConnect(BRPeerManager *manager);

// rescan blockchain for potentially missing transactions
void BRPeerManagerRescan(BRPeerManager *manager);

// current proof-of-work verified best block height
uint32_t BRPeerManagerLastBlockHeight(BRPeerManager *manager);

// the (unverified) best block height reported by connected peers
uint32_t BRPeerManagerEstimatedBlockHeight(BRPeerManager *manager);

// current network sync progress from 0 to 1
double BRPeerManagerSyncProgress(BRPeerManager *manager);

// returns the number of currently connected peers
size_t BRPeerManagerPeerCount(BRPeerManager *manager);

// publishes tx to bitcoin network
void BRPeerManagerPublishTx(BRTransaction *tx, void (*callback)(BRPeerManagerError error, void *info), void *info);

// number of connected peers that have relayed the transaction
size_t BRPeerMangaerRelayCount(UInt256 txHash);

// frees memory allocated for manager
void BRPeerManagerFree(BRPeerManager *manager, void (*free)(void *));

#endif // BRPeerManager_h
