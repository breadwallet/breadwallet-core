//
//  com_breadwallet_core_ethereum_BREthereumToken
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/20/18.
//  Copyright (c) 2018 breadwallet LLC
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

#include <stdlib.h>
#include <malloc.h>
#include <BREthereumWallet.h>
#include <BRCoreJni.h>
#include <BREthereum.h>
#include "com_breadwallet_core_ethereum_BREthereumToken.h"

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumWallet
 * Method:    getBalance
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumWallet_getBalance
        (JNIEnv *env, jobject thisObject) {
    BREthereumWallet wallet = (BREthereumWallet) getJNIReference(env, thisObject);
    BREthereumAmount balance = walletGetBalance(wallet);

    char *number = (AMOUNT_ETHER == amountGetType(balance)
                    ? etherGetValueString(balance.u.ether, WEI)
                    : tokenQuantityGetValueString(balance.u.tokenQuantity,
                                                  TOKEN_QUANTITY_TYPE_DECIMAL));

    jstring result = (*env)->NewStringUTF (env, number);
    free (number);
    return result;
}

