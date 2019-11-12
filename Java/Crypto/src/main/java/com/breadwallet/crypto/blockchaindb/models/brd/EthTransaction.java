/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.brd;

import com.google.gson.annotations.SerializedName;

import static com.google.common.base.Preconditions.checkNotNull;

public class EthTransaction {

    public String hash;

    @SerializedName("from")
    private String sourceAddr;

    @SerializedName("to")
    private String targetAddr;

    @SerializedName("contractAddress")
    private String contractAddr;

    @SerializedName("value")
    private String amount;

    @SerializedName("gas")
    private String gasLimit;

    private String gasPrice;

    @SerializedName("input")
    private String data;

    private String nonce;

    private String gasUsed;

    private String blockNumber;

    private String blockHash;

    @SerializedName("confirmations")
    private String blockConfirmations;

    @SerializedName("transactionIndex")
    private String blockTransactionIndex;

    @SerializedName("timeStamp")
    private String blockTimestamp;

    private String isError;

    public String getHash() {
        checkNotNull(hash);
        return hash;
    }

    public String getSourceAddr() {
        checkNotNull(sourceAddr);
        return sourceAddr;
    }

    public String getTargetAddr() {
        checkNotNull(targetAddr);
        return targetAddr;
    }

    public String getContractAddr() {
        checkNotNull(contractAddr);
        return contractAddr;
    }

    public String getAmount() {
        checkNotNull(amount);
        return amount;
    }

    public String getGasLimit() {
        checkNotNull(gasLimit);
        return gasLimit;
    }

    public String getGasPrice() {
        checkNotNull(gasPrice);
        return gasPrice;
    }

    public String getData() {
        checkNotNull(data);
        return data;
    }

    public String getNonce() {
        checkNotNull(nonce);
        return nonce;
    }

    public String getGasUsed() {
        checkNotNull(gasUsed);
        return gasUsed;
    }

    public String getBlockNumber() {
        checkNotNull(blockNumber);
        return blockNumber;
    }

    public String getBlockHash() {
        checkNotNull(blockHash);
        return blockHash;
    }

    public String getBlockConfirmations() {
        checkNotNull(blockConfirmations);
        return blockConfirmations;
    }

    public String getBlockTransactionIndex() {
        checkNotNull(blockTransactionIndex);
        return blockTransactionIndex;
    }

    public String getBlockTimestamp() {
        checkNotNull(blockTimestamp);
        return blockTimestamp;
    }

    public String getIsError() {
        checkNotNull(isError);
        return isError;
    }
}
