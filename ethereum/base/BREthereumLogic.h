//
//  BREthereumLogic
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/22/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Logic_H
#define BR_Ethereum_Logic_H

#ifdef __cplusplus
extern "C" {
#endif

#define private_extern  extern
    
//
// Etherum Boolean
//
typedef enum {
  ETHEREUM_BOOLEAN_TRUE = 0,               // INTENTIONALLY 'backwards'
  ETHEREUM_BOOLEAN_FALSE = 1
} BREthereumBoolean;

#define ETHEREUM_BOOLEAN_IS_TRUE(x)  ((x) == ETHEREUM_BOOLEAN_TRUE)
#define ETHEREUM_BOOLEAN_IS_FALSE(x) ((x) == ETHEREUM_BOOLEAN_FALSE)

#define AS_ETHEREUM_BOOLEAN(x)    ((x) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE)

//
// Ethereum Comparison
//
typedef enum {
  ETHEREUM_COMPARISON_LT = -1,
  ETHEREUM_COMPARISON_EQ =  0,
  ETHEREUM_COMPARISON_GT = +1
} BREthereumComparison;

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Logic_H */
