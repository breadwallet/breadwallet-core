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
#include "BRStellarOperation.h"
#include <stdlib.h>

extern BRStellarOperation
stellarOperationCreatePayment(BRStellarAccountID *destination, BRStellarAsset asset, BRStellarAmount amount)
{
    BRStellarOperation op;
    memset(&op, 0x00, sizeof(BRStellarOperation));
    op.type = ST_OP_PAYMENT;
    op.operation.payment.destination = *destination;
    op.operation.payment.amount = amount;
    op.operation.payment.asset = asset;
    return op;
}

extern BRStellarAsset stellarAssetCreateAsset(const char* assetCode, BRStellarAccountID *issuer)
{
    BRStellarAsset asset;
    memset(&asset, 0x00, sizeof(BRStellarAsset));

    if (0 == strcmp("XML", assetCode)) {
        asset.type = ASSET_TYPE_NATIVE;
    } else {
        if (strlen(assetCode) <= 4) {
            asset.type = ASSET_TYPE_CREDIT_ALPHANUM4;
            strncpy(asset.assetCode, assetCode, strlen(assetCode));
            // We need an issuer in this case
            asset.issuer = *issuer;
        }
        else {
            // Make sure that we cover the case where someone passes in invalid code
            size_t amountToCopy = strlen(assetCode) <= 12 ? strlen(assetCode) : 12;
            asset.type = ASSET_TYPE_CREDIT_ALPHANUM12;
            strncpy(asset.assetCode, assetCode, amountToCopy);
            asset.issuer = *issuer;
        }
    }
    return asset;
}

