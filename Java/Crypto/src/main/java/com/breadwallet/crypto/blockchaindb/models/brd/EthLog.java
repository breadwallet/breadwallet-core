/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.brd;

import com.breadwallet.crypto.blockchaindb.models.Utilities;
import com.google.gson.annotations.SerializedName;

import java.util.List;

import static com.google.common.base.Preconditions.checkNotNull;

public class EthLog {

    private static List<String> trimLast(List<String> strings) {
        if (!strings.isEmpty()) {
            int index = strings.size() - 1;
            if (strings.get(index).isEmpty()) {
                strings.remove(index);
            }
        }
        return strings;
    }

    @SerializedName("transactionHash")
    private String hash;

    @SerializedName("address")
    private String contract;

    private List<String> topics;

    private String data;

    private String gasPrice;

    private String gasUsed;

    private String logIndex;

    private String blockNumber;

    @SerializedName("transactionIndex")
    private String blockTransactionIndex;

    @SerializedName("timeStamp")
    private String blockTimestamp;

    public String getHash() {
        checkNotNull(hash);
        return hash;
    }

    public String getContract() {
        checkNotNull(contract);
        return contract;
    }

    public List<String> getTopics() {
        checkNotNull(topics);
        return trimLast(topics);
    }

    public String getData() {
        checkNotNull(data);
        return data;
    }

    public String getGasPrice() {
        checkNotNull(gasPrice);
        return gasPrice;
    }

    public String getGasUsed() {
        checkNotNull(gasUsed);
        return gasUsed;
    }

    public String getLogIndex() {
        checkNotNull(logIndex);
        return logIndex;
    }

    public String getBlockNumber() {
        checkNotNull(blockNumber);
        return blockNumber;
    }

    public String getBlockTransactionIndex() {
        checkNotNull(blockTransactionIndex);
        return blockTransactionIndex;
    }

    public String getBlockTimestamp() {
        checkNotNull(blockTimestamp);
        return blockTimestamp;
    }
}
