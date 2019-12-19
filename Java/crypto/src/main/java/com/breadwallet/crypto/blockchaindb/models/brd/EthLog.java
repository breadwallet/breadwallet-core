/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.brd;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

import java.util.List;

import static com.google.common.base.Preconditions.checkNotNull;

public class EthLog {

    // creators

    @JsonCreator
    public static EthLog create(@JsonProperty("transactionHash") String hash,
                                @JsonProperty("address") String contract,
                                @JsonProperty("topics") List<String> topics,
                                @JsonProperty("data") String data,
                                @JsonProperty("gasPrice") String gasPrice,
                                @JsonProperty("gasUsed") String gasUsed,
                                @JsonProperty("logIndex") String logIndex,
                                @JsonProperty("blockNumber") String blockNumber,
                                @JsonProperty("transactionIndex") String blockTransactionIndex,
                                @JsonProperty("timeStamp") String blockTimestamp) {
        return new EthLog(
                checkNotNull(hash),
                checkNotNull(contract),
                checkNotNull(topics),
                checkNotNull(data),
                checkNotNull(gasPrice),
                checkNotNull(gasUsed),
                checkNotNull(logIndex),
                checkNotNull(blockNumber),
                checkNotNull(blockTransactionIndex),
                checkNotNull(blockTimestamp)
        );
    }

    // fields

    private final String hash;
    private final String contract;
    private final List<String> topics;
    private final String data;
    private final String gasPrice;
    private final String gasUsed;
    private final String logIndex;
    private final String blockNumber;
    private final String blockTransactionIndex;
    private final String blockTimestamp;

    private EthLog(String hash,
                   String contract,
                   List<String> topics,
                   String data,
                   String gasPrice,
                   String gasUsed,
                   String logIndex,
                   String blockNumber,
                   String blockTransactionIndex,
                   String blockTimestamp) {
        this.hash = hash;
        this.contract = contract;
        this.topics = topics;
        this.data = data;
        this.gasPrice = gasPrice;
        this.gasUsed= gasUsed;
        this.logIndex = logIndex;
        this.blockNumber = blockNumber;
        this.blockTransactionIndex= blockTransactionIndex;
        this.blockTimestamp = blockTimestamp;
    }

    // getters

    @JsonProperty("transactionHash")
    public String getHash() {
        return hash;
    }

    @JsonProperty("address")
    public String getContract() {
        return contract;
    }

    @JsonProperty("topics")
    public List<String> getTopicsValue() {
        return topics;
    }

    @JsonIgnore
    public List<String> getTopics() {
        return trimLast(topics);
    }

    @JsonProperty("data")
    public String getData() {
        return data;
    }

    @JsonProperty("gasPrice")
    public String getGasPrice() {
        return gasPrice;
    }

    @JsonProperty("gasUsed")
    public String getGasUsed() {
        return gasUsed;
    }

    @JsonProperty("logIndex")
    public String getLogIndex() {
        return logIndex;
    }

    @JsonProperty("blockNumber")
    public String getBlockNumber() {
        return blockNumber;
    }

    @JsonProperty("transactionIndex")
    public String getBlockTransactionIndex() {
        return blockTransactionIndex;
    }

    @JsonProperty("timeStamp")
    public String getBlockTimestamp() {
        return blockTimestamp;
    }

    // internals

    private static List<String> trimLast(List<String> strings) {
        if (!strings.isEmpty()) {
            int index = strings.size() - 1;
            if (strings.get(index).isEmpty()) {
                strings.remove(index);
            }
        }
        return strings;
    }
}
