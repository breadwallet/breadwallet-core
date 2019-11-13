/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.brd;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.JsonProcessingException;

import java.util.List;

import static com.google.common.base.Preconditions.checkNotNull;

public class EthLog {

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

    private static List<String> trimLast(List<String> strings) {
        if (!strings.isEmpty()) {
            int index = strings.size() - 1;
            if (strings.get(index).isEmpty()) {
                strings.remove(index);
            }
        }
        return strings;
    }

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

    public String getHash() {
        return hash;
    }

    public String getContract() {
        return contract;
    }

    public List<String> getTopics() {
        return trimLast(topics);
    }

    public String getData() {
        return data;
    }

    public String getGasPrice() {
        return gasPrice;
    }

    public String getGasUsed() {
        return gasUsed;
    }

    public String getLogIndex() {
        return logIndex;
    }

    public String getBlockNumber() {
        return blockNumber;
    }

    public String getBlockTransactionIndex() {
        return blockTransactionIndex;
    }

    public String getBlockTimestamp() {
        return blockTimestamp;
    }
}
