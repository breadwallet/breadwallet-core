//
//  BREthereumContract
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/5/18.
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
#include <stdarg.h>
#include <memory.h>
#include <assert.h>
#include "BREthereumContract.h"
#include "BREthereumBase.h"
#include "BRUtil.h"

/* Forward Declarations */
static void
encodeReverseBytes (uint8_t *t, const uint8_t *s, size_t slen);

// https://medium.com/@jgm.orinoco/understanding-erc-20-token-contracts-a809a7310aa5
// https://github.com/ethereum/wiki/wiki/Ethereum-Contract-ABI
// https://ethereumbuilders.gitbooks.io/guide/content/en/solidity_features.html
// http://solidity.readthedocs.io/en/develop/abi-spec.html#function-selector

// Contracts are not defined dynamically - so we can adjust this if need be.  Notably our primary
// concern is the ERC20 contract - which as less than ten functions
#define DEFAULT_CONTRACT_FUNCTION_LIMIT   10

/**
 * Define a function type to encode an argument into 64 characters, per the Ethereum Contract ABI.
 * NO CHECK is performed on the size of `chars`.
 *
 */
typedef void (*ArgumentEncodeFunc) (uint8_t *bytes, size_t bytesCount, uint8_t *target, size_t targetCount);

// Encode an Ethereum Address
static void argumentEncodeAddress (uint8_t *bytes, size_t bytesCount, uint8_t *target, size_t targetCount);
// Encode an Ethereum Number
static void argumentEncodeUInt256 (uint8_t *bytes, size_t bytesCount, uint8_t *target, size_t targetCount);

/**
 *
 *
 */
struct BREthereumFunctionRecord {
  char *interface;
  char *signature;
  char *selector;
  unsigned int argumentCount;
  ArgumentEncodeFunc argumentEncoders[5];
};

/**
 *
 *
 */
struct BREthereumContractRecord {
  unsigned int functionsCount;
  struct BREthereumFunctionRecord functions[DEFAULT_CONTRACT_FUNCTION_LIMIT];
};

//
// ERC20 Token Contract
//
// https://github.com/ethereum/EIPs/blob/master/EIPS/eip-20.md
// https://medium.com/hellogold/erc20-coins-and-the-multisig-wallet-acc3b43e2137
// 06fdde03 name()
// 95d89b41 symbol()
// 313ce567 decimals()
// 18160ddd totalSupply()
// 70a08231 balanceOf(address)
// a9059cbb transfer(address,uint256)
// 23b872dd transferFrom(address,address,uint256)
// 095ea7b3 approve(address,uint256)
// dd62ed3e allowance(address,address)
// 54fd4d50 version()

static struct BREthereumContractRecord contractRecordERC20 = {
  6,
  {
    // function name() constant returns (string name)
    // function symbol() constant returns (string symbol)
    // function decimals() constant returns (uint8 decimals)
    // function version() constant returns (string version)

    { "function totalSupply() constant returns (uint256 totalSupply)",
      "totalSupply()",
      "0x18160ddd",
      0
    },

    { "function balanceOf(address _owner) constant returns (uint256 balance)",
      "balanceOf(address)",
      "0x70a08231",
      1,
      argumentEncodeAddress
    },

    { "function transfer(address _to, uint256 _value) returns (bool success)",
      "transfer(address,uint256)",
      "0xa9059cbb",
      2,
      argumentEncodeAddress,
      argumentEncodeUInt256
    },

    { "function transferFrom(address _from, address _to, uint256 _value) returns (bool success)",
      "transferFrom(address,address,uint256)",
      "0x23b872dd",
      3,
      argumentEncodeAddress,
      argumentEncodeAddress,
      argumentEncodeUInt256
    },

    { "function approve(address _spender, uint256 _value) returns (bool success)",
      "approve(address,uint256)",
      "0x095ea7b3",
      2,
      argumentEncodeAddress,
      argumentEncodeUInt256
    },

    { "function allowance(address _owner, address _spender) constant returns (uint256 remaining)",
      "allowance(address,address)",
      "0xdd62ed3e",  // dd62ed3e
      2,
      argumentEncodeAddress,
      argumentEncodeAddress
    }

    // Events
  }
};
BREthereumContract contractERC20 = &contractRecordERC20;
BREthereumFunction functionERC20Transfer = &contractRecordERC20.functions[2];


static int
functionIsEncodedInData (BREthereumFunction function, const char *data) {
  return NULL != data &&
         0 == strncmp (function->selector, data, strlen(function->selector));
}

extern BREthereumFunction
contractLookupFunctionForEncoding (BREthereumContract contract, const char *encoding) {
  for (int i = 0; i < contract->functionsCount; i++)
    if (functionIsEncodedInData(&contract->functions[i], encoding))
      return &contract->functions[i];
  return NULL;
}

private_extern UInt256
functionERC20TransferDecodeAmount (BREthereumFunction function,
                                   const char *data,
                                   BRCoreParseStatus *status) {
  assert (function == functionERC20Transfer);
  // Second argument - skip selector + 1st argument
  data = &data [strlen(function->selector) + 1 * 64];
  return createUInt256Parse(data, 16, status);
}

private_extern char *
functionERC20TransferDecodeAddress (BREthereumFunction function,
                                    const char *data) {
  assert (function == functionERC20Transfer);
  // Second argument - skip selector + 1st argument
  char result[2 + 40 + 1];
  result[0] = '0';
  result[1] = 'x';
  memcpy (&result[2], &data[strlen(function->selector) + 24], 40);  // 24 -> skip '0000...<addr>'
  result[42] = '\0';
  return strdup (result);
}

/**
*/
extern const char *
contractEncode (BREthereumContract contract, BREthereumFunction function, ...) {
  // We'll require VAR ARGS
  unsigned int argsCount = function->argumentCount;

  // The encoding result is the function selector plus 64 chars for each argument plus '\0'
  char *encoding = malloc (strlen(function->selector) - 2 + argsCount * 64 + 1);
  size_t encodingIndex = 0;

  // Copy the selector
  memcpy (&encoding[encodingIndex], &function->selector[2], strlen(function->selector) - 2);
  encodingIndex += strlen(function->selector) - 2;

  va_list args;
  va_start (args, function);
  for (int i = 0; i < argsCount; i++) {
    uint8_t targetBytes[32];

    // Encode each argument into an `encoding` offset
    uint8_t *bytes = va_arg (args, uint8_t*);
    assert (NULL != bytes);

    size_t  bytesCount = va_arg (args, size_t);
    (*function->argumentEncoders[i]) (bytes, bytesCount, targetBytes, 32);

    encodeHex(&encoding[encodingIndex], 65, targetBytes, 32);
    encodingIndex += 64;
  }
  encoding[encodingIndex] = '\0';
  va_end(args);

  return encoding;
}

/**
 *  An address is what: bytes, "0x..." or "..." ?
 */
static void argumentEncodeAddress (uint8_t *bytes, size_t bytesCount, uint8_t *target, size_t targetCount) {
  memset (target, 0, targetCount);

  uint8_t decodedBytes[bytesCount/2];
  decodeHex(decodedBytes, bytesCount/2, (char *) bytes, bytesCount);

  memcpy (&target[targetCount - bytesCount/2], decodedBytes, bytesCount/2);
}

/**
 *
 */
static void argumentEncodeUInt256 (uint8_t *bytes, size_t bytesCount, uint8_t *target, size_t targetCount) {
  assert (32 == bytesCount && 32 == targetCount);
  // TODO: ENDIAN
  encodeReverseBytes(target, bytes, bytesCount);
}

//
// Support
//
static void
encodeReverseBytes (uint8_t *t, const uint8_t *s, size_t slen) {
  for (int i = 0; i < slen; i++)
    t[slen - i - 1] = s[i];
}


/* ERC20
o TotalSupply [Get the total token supply]
o BalanceOf (address _owner) constant returns (uint256 balance) [Get the account balance of another account with address _owner]
o transfer(address _to, uint256 _value) returns (bool success) [Send _value amount of tokens to address _to]
o transferFrom(address _from, address _to, uint256 _value) returns (bool success)[Send _value amount of tokens from address _from to address _to]
o approve(address _spender, uint256 _value) returns (bool success) [Allow _spender to withdraw from your account, multiple times, up to the _value amount. If this function is called again it overwrites the current allowance with _value]
o allowance (address *_owner*, address *_spender*) constant returns (uint256 remaining) [Returns the amount which _spender is still allowed to withdraw from _owner]
*/

/*
 Recv Address: Contract 0x8713d26637cf49e1b6b4a7ce57106aabc9325343
 Input Data:
  Function: transfer(address _to, uint256 _value)

  MethodID: 0xa9059cbb
  [0]:  000000000000000000000000c46a1ea7dba082a8d593c549f1b2dd7354f25692      // address
  [1]:  000000000000000000000000000000000000000000000006f05b59d3b2000000      // amount
*/

/*
 > web3.sha3('balanceOf(address)')
 "0x70a08231b98ef4ca268c9cc3f6b4590e4bfec28280db06bb5d45e689f2a360be"

 > funcSelector = web3.sha3('balanceOf(address)').slice(0,10)
 "0x70a08231"  // first 4 bytes, 8 characters
*/

/*
 Raw Hash:
 0xf8ad83067642850ba43b74008301246a94558ec3152e2eb2174905cd19aea4e34a23de9ad68
   // RLP Header - 'data'
   0b844
      a9059cbb
      000000000000000000000000932a27e1bc84f5b74c29af3d888926b1307f4a5c
      0000000000000000000000000000000000000000000001439152d319e84d0000
   // v, r, s signature
   25, a0a352fe7973fe554d3d5d21effb82667b3a17cc7b259eec482baf41a5ac80e155a0772ba32bfe32ccf7c4b764db155cd3e39b66c3b10abaa44ce27bc3013dd9ae7b

 Input Data:
     Function: transfer(address _to, uint256 _value) ***

     MethodID: 0xa9059cbb
     [0]:  000000000000000000000000932a27e1bc84f5b74c29af3d888926b1307f4a5c
     [1]:  0000000000000000000000000000000000000000000001439152d319e84d0000

 Implication ->
 */
