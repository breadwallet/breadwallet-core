//
//  BRAssert.h
//  BRCore
//
//  Created by Ed Gamble on 2/4/19.
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

#ifndef BRAssert_h
#define BRAssert_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * BRAssert() is meant to replace assert() so as to provide a meaningful 'release' assert. Normally
 * assert() is disabled except in -DDEBUG modes; however, in Core code a failed assertion is truely
 * fatal.  BRAssert() handle the condition failure in all build types.
 *
 * @param condition if false, assert.
 */
#define BRAssert(condition)   \
    do { if (!(condition)) { BRFail(); } } while (0)

/**
 * BRFail(), which is essentially `BRAssert(0)`, fails Core.
 */
extern void
BRFail (void) __attribute__((__noreturn__));

typedef void *BRAssertInfo;
typedef void (*BRAssertHandler) (BRAssertInfo info);

/**
 * Install a handler for a BRAssert failure.  If `BRAssert(0)` occurs, then a Unix signal for
 * `signum` will be raised and caught.  When caught, the provided handler will be invokded as
 * `handler(info)`.  The handler runs in a pthread (not in a Unix signal context) and thus it can
 * do anything.
 *
 * Invocation of the handler implies that Core has failed.  The appropriate response is to
 * delete/release all Core resources (Bitcoin/Ethereum Wallets/Transfer) and to restart Core w/
 * a FULL-SYNC for all blockchains.
 *
 * BRAssert uses the provided `signal`; generally you'd want this to be SIGUSR1/2 and not used
 * elsewhere in your code base.  (TBD truth)
 *
 * BRAssertInstall() should be called before Core is used.  Thus before wallets, peer manager,
 * wallet managers, etc are created.
 *
 *
 * @param signum the Unix signal to use for BRAssert signaling.
 * @param info some handler context
 * @param handler some handler
 */
extern void
BRAssertInstall (int signum, BRAssertInfo info, BRAssertHandler handler);

/**
 * Connect BRAssert.  Will start a pthread to wait on and handle BRAssert failures.
 */
extern void
BRAssertConnect (void);

/**
 * Disconnect BRAssert.  Will kill the pthread waiting on BRAssert failures.
 */
extern void
BRAssertDisconnect (void);

/**
 * Return true (1) if BRAssert is connected, false (0) otherwise.

 @return true if connected
 */
extern int
BRAassertIsConnected (void);

/// MARK: Private-ish

// Function to specify a recovery, e.g.: recovery BRPeerManager with BRPeerDisconnect.
typedef void *BRAssertRecoveryInfo;
typedef void (*BRAssertRecoveryHandler) (BRAssertRecoveryInfo info);

/**
 * On a BRAssert signal, define a recovery handler
 *
 * @param info some handler context.
 * @param handler some handler
 */
extern void
BRAssertDefineRecovery (BRAssertRecoveryInfo info,
                        BRAssertRecoveryHandler handler);

#ifdef __cplusplus
}
#endif

#include <stdio.h>

#endif /* BRAssert_h */
