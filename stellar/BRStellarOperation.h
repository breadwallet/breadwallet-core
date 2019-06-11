//
//  BRStellarOperation.h
//  Core
//
//  Created by Carl Cherry on 6/12/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRStellar_operation_h
#define BRStellar_operation_h

#include "BRStellarBase.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Create a Stellar operation object
 *
 * @param destination    Destination accountID for the payment
 * @param asset          asset for the payment
 * @param amount         amount
 *
 * @return operation     an operation of type Payment
 */
extern BRStellarOperation
stellarOperationCreatePayment(BRStellarAccountID *destination, BRStellarAsset asset, BRStellarAmount amount);

/*
 * Create an Asset object
 *
 * @param assetCode      Code identifying the asset ("XML" is the native asset for stellar)
 *                       https://www.stellar.org/developers/guides/concepts/assets.html
 *                       See the section on Alphanumeric 4-character and 12-character supported asset codes
 * @param issuer         AccountID of issuer IF asset type is NOT native
 *
 * @return asset
 */
extern BRStellarAsset stellarAssetCreateAsset(const char* assetCode, BRStellarAccountID *issuer);

#ifdef __cplusplus
}
#endif

#endif // BRStellar_operation_h
