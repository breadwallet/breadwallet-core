//
//  BBRUtil.h
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/16/2018.
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

#ifndef BR_Util_H
#define BR_Util_H

#include "BRUtilHex.h"
#include "BRUtilMath.h"

#define eth_log(topic, formatter, ...)   _eth_log("ETH: %s: " formatter "\n", (topic), __VA_ARGS__)

#if defined(TARGET_OS_MAC)
#  include <Foundation/Foundation.h>
#  define _eth_log(...) NSLog(__VA_ARGS__)
#elif defined(__ANDROID__)
#  include <android/log.h>
#  define _eth_log(...) __android_log_print(ANDROID_LOG_INFO, "", __VA_ARGS__)
#else
#  include <stdio.h>
#  define _eth_log(...) printf(__VA_ARGS__)
#endif

#endif // BR_Util_H
