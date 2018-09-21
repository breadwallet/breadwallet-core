//
//  BREthereumTransferStuff.c
//  BRCore
//
//  Created by Ed Gamble on 7/26/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

#include <stdio.h>

#if 0 // defined (TRANSACTION_ENCODE_TOKEN)
char *strData = rlpDecodeItemHexString (coder, items[5], "0x");
assert (NULL != strData);
if ('\0' == strData[0] || 0 == strcmp (strData, "0x")) {
    // This is a ETHER transfer
    transaction->targetAddress = addressRlpDecode(items[3], coder);
    transaction->amount = amountRlpDecodeAsEther(items[4], coder);
    transaction->data = strData;
}
else {
    // This is a TOKEN transfer.

    BREthereumAddress contractAddr = addressRlpDecode(items[3], coder);
    BREthereumToken token = tokenLookupByAddress(contractAddr);

    // Confirm `strData` encodes functionERC20Transfer
    BREthereumContractFunction function = contractLookupFunctionForEncoding(contractERC20, strData);
    if (NULL == token || function != functionERC20Transfer) {
        free (transaction);
        return NULL;
    }

    BRCoreParseStatus status = CORE_PARSE_OK;
    UInt256 amount = functionERC20TransferDecodeAmount (function, strData, &status);
    char *recvAddr = functionERC20TransferDecodeAddress(function, strData);

    if (CORE_PARSE_OK != status) {
        free (transaction);
        return NULL;
    }

    transaction->amount = amountCreateToken(createTokenQuantity(token, amount));
    transaction->targetAddress = addressCreate(recvAddr);

    free (recvAddr);
}


/////
static BRRlpItem
transactionEncodeDataForHolding (BREthereumTransaction transaction,
                                 BREthereumAmount holding,
                                 BRRlpCoder coder) {
    return (NULL == transaction->data || 0 == strlen(transaction->data)
            ? rlpEncodeItemString(coder, "")
            : rlpEncodeItemHexString(coder, transaction->data));
}

static BRRlpItem
transactionEncodeAddressForHolding (BREthereumTransaction transaction,
                                    BREthereumAmount holding,
                                    BRRlpCoder coder) {
    switch (amountGetType(holding)) {
        case AMOUNT_ETHER:
            return addressRlpEncode(transaction->targetAddress, coder);
        case AMOUNT_TOKEN: {
            BREthereumToken token = tokenQuantityGetToken (amountGetTokenQuantity(holding));
            BREthereumAddress contractAddress = tokenGetAddressRaw(token);
            return addressRlpEncode(contractAddress, coder);
        }
    }
}


//  ===========

extern void
transactionSetGasEstimate (BREthereumTransaction transaction,
                           BREthereumGas gasEstimate) {
    transaction->gasEstimate = gasEstimate;
    // Ensure that the gasLimit is at least X% more than gasEstimate.
    BREthereumGas gasLimitWithMargin = gasApplyLmitMargin(gasEstimate);
    if (gasLimitWithMargin.amountOfGas > transaction->gasLimit.amountOfGas)
        transaction->gasLimit = gasLimitWithMargin;
}

extern BREthereumEther
transactionGetFee (BREthereumTransaction transaction, int *overflow) {
    return etherCreate
    (mulUInt256_Overflow(transaction->gasPrice.etherPerGas.valueInWEI,
                         createUInt256 (ETHEREUM_BOOLEAN_IS_TRUE(transactionIsConfirmed(transaction))
                                        ? transaction->status.u.included.gasUsed.amountOfGas
                                        : transaction->gasEstimate.amountOfGas),
                         overflow));
}

extern BREthereumEther
transactionGetFeeLimit (BREthereumTransaction transaction, int *overflow) {
    return etherCreate
    (mulUInt256_Overflow(transaction->gasPrice.etherPerGas.valueInWEI,
                         createUInt256 (transaction->gasLimit.amountOfGas),
                         overflow));
}

/**
 * Return the fee (in Ether) for transaction.  If the transaction is confirmed (aka blocked) then
 * the value returned is the actual fee paid (as gasUsed * gasPrice); if the transaction is not
 * confirmed then an estimated fee is returned (as gasEstimate * gasPrice).
 */
extern BREthereumEther
transactionGetFee (BREthereumTransaction transaction, int *overflow);

/**
 * Return the maximum fee (in Ether) for transaction (as gasLimit * gasPrice).
 */
extern BREthereumEther
transactionGetFeeLimit (BREthereumTransaction transaction, int *overflow);


#endif










#if defined TRANSACTION_ENCODE_TOKEN
char *strData = rlpDecodeItemHexString (coder, items[5], "0x");
assert (NULL != strData);
if ('\0' == strData[0] || 0 == strcmp (strData, "0x")) {
    // This is a ETHER transfer
    transaction->targetAddress = addressRlpDecode(items[3], coder);
    transaction->amount = amountRlpDecodeAsEther(items[4], coder);
    transaction->data = strData;
}
else {
    // This is a TOKEN transfer.

    BREthereumAddress contractAddr = addressRlpDecode(items[3], coder);
    BREthereumToken token = tokenLookupByAddress(contractAddr);

    // Confirm `strData` encodes functionERC20Transfer
    BREthereumContractFunction function = contractLookupFunctionForEncoding(contractERC20, strData);
    if (NULL == token || function != functionERC20Transfer) {
        free (transaction);
        return NULL;
    }

    BRCoreParseStatus status = CORE_PARSE_OK;
    UInt256 amount = functionERC20TransferDecodeAmount (function, strData, &status);
    char *recvAddr = functionERC20TransferDecodeAddress(function, strData);

    if (CORE_PARSE_OK != status) {
        free (transaction);
        return NULL;
    }

    transaction->amount = amountCreateToken(createTokenQuantity(token, amount));
    transaction->targetAddress = addressCreate(recvAddr);

    free (recvAddr);
}



extern BREthereumGas
transactionGetGasEstimate (BREthereumTransaction transaction) {
    return transaction->gasEstimate;
}

extern void
transactionSetGasEstimate (BREthereumTransaction transaction,
                           BREthereumGas gasEstimate) {
    transaction->gasEstimate = gasEstimate;
    // Ensure that the gasLimit is at least X% more than gasEstimate.
    BREthereumGas gasLimitWithMargin = gasApplyLmitMargin(gasEstimate);
    if (gasLimitWithMargin.amountOfGas > transaction->gasLimit.amountOfGas)
        transaction->gasLimit = gasLimitWithMargin;
}


#endif
