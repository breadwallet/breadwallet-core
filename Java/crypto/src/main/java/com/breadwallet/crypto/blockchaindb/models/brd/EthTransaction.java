/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.brd;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

import static com.google.common.base.Preconditions.checkNotNull;

public class EthTransaction {

    // creators

    @JsonCreator
    public static EthTransaction create(@JsonProperty("hash") String hash,
                                        @JsonProperty("from") String sourceAddr,
                                        @JsonProperty("to") String targetAddr,
                                        @JsonProperty("contractAddress") String contractAddr,
                                        @JsonProperty("value") String amount,
                                        @JsonProperty("gas") String gasLimit,
                                        @JsonProperty("gasPrice") String gasPrice,
                                        @JsonProperty("input") String data,
                                        @JsonProperty("nonce") String nonce,
                                        @JsonProperty("gasUsed") String gasUsed,
                                        @JsonProperty("blockNumber") String blockNumber,
                                        @JsonProperty("blockHash") String blockHash,
                                        @JsonProperty("confirmations") String blockConfirmations,
                                        @JsonProperty("transactionIndex") String blockTransactionIndex,
                                        @JsonProperty("timeStamp") String blockTimestamp,
                                        @JsonProperty("isError") String isError) {
        return new EthTransaction(
                checkNotNull(hash),
                checkNotNull(sourceAddr),
                checkNotNull(targetAddr),
                checkNotNull(contractAddr),
                checkNotNull(amount),
                checkNotNull(gasLimit),
                checkNotNull(gasPrice),
                checkNotNull(data),
                checkNotNull(nonce),
                checkNotNull(gasUsed),
                checkNotNull(blockNumber),
                checkNotNull(blockHash),
                checkNotNull(blockConfirmations),
                checkNotNull(blockTransactionIndex),
                checkNotNull(blockTimestamp),
                checkNotNull(isError)
        );
    }

    // fields

    private final String hash;
    private final String sourceAddr;
    private final String targetAddr;
    private final String contractAddr;
    private final String amount;
    private final String gasLimit;
    private final String gasPrice;
    private final String data;
    private final String nonce;
    private final String gasUsed;
    private final String blockNumber;
    private final String blockHash;
    private final String blockConfirmations;
    private final String blockTransactionIndex;
    private final String blockTimestamp;
    private final String isError;

    private EthTransaction(String hash,
                           String sourceAddr,
                           String targetAddr,
                           String contractAddr,
                           String amount,
                           String gasLimit,
                           String gasPrice,
                           String data,
                           String nonce,
                           String gasUsed,
                           String blockNumber,
                           String blockHash,
                           String blockConfirmations,
                           String blockTransactionIndex,
                           String blockTimestamp,
                           String isError) {
        this.hash = hash;
        this.sourceAddr = sourceAddr;
        this.targetAddr = targetAddr;
        this.contractAddr = contractAddr;
        this.amount = amount;
        this.gasLimit = gasLimit;
        this.gasPrice = gasPrice;
        this.data = data;
        this.nonce = nonce;
        this.gasUsed = gasUsed;
        this.blockNumber = blockNumber;
        this.blockHash = blockHash;
        this.blockConfirmations = blockConfirmations;
        this.blockTransactionIndex = blockTransactionIndex;
        this.blockTimestamp = blockTimestamp;
        this.isError = isError;
    }

    // getters

    @JsonProperty("hash")
    public String getHash() {
        return hash;
    }

    @JsonProperty("from")
    public String getSourceAddr() {
        return sourceAddr;
    }

    @JsonProperty("to")
    public String getTargetAddr() {
        return targetAddr;
    }

    @JsonProperty("contractAddress")
    public String getContractAddr() {
        return contractAddr;
    }

    @JsonProperty("value")
    public String getAmount() {
        return amount;
    }

    @JsonProperty("gas")
    public String getGasLimit() {
        return gasLimit;
    }

    @JsonProperty("gasPrice")
    public String getGasPrice() {
        return gasPrice;
    }

    @JsonProperty("input")
    public String getData() {
        return data;
    }

    @JsonProperty("nonce")
    public String getNonce() {
        return nonce;
    }

    @JsonProperty("gasUsed")
    public String getGasUsed() {
        return gasUsed;
    }

    @JsonProperty("blockNumber")
    public String getBlockNumber() {
        return blockNumber;
    }

    @JsonProperty("blockHash")
    public String getBlockHash() {
        return blockHash;
    }

    @JsonProperty("confirmations")
    public String getBlockConfirmations() {
        return blockConfirmations;
    }

    @JsonProperty("transactionIndex")
    public String getBlockTransactionIndex() {
        return blockTransactionIndex;
    }

    @JsonProperty("timeStamp")
    public String getBlockTimestamp() {
        return blockTimestamp;
    }

    @JsonProperty("isError")
    public String getIsError() {
        return isError;
    }
}
