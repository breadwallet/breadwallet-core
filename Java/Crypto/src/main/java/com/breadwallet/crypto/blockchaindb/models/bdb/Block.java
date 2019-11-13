/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.primitives.UnsignedLong;

import java.util.Date;
import java.util.List;

public class Block {

    // fields

    @JsonProperty("block_id")
    private String blockId;

    private String hash;

    @JsonProperty("blockchain_id")
    private String blockchainId;

    @JsonProperty("prev_hash")
    private String prevHash;

    @JsonProperty("next_hash")
    private String nextHash;

    private UnsignedLong height;

    private Date mined;

    private TransactionList transactionList;

    @JsonProperty("transaction_ids")
    private List<String> transactionIds;

    private UnsignedLong size;

    @JsonProperty("total_fees")
    private Amount totalFees;

    private String header;

    private String raw;

    private UnsignedLong acknowledgements;

    @JsonProperty("is_active_chain")
    private boolean isActiveChain;

    // getters

    public String getId() {
        return blockId;
    }

    public String getHash() {
        return hash;
    }

    public String getBlockchainId() {
        return blockchainId;
    }

    public String getPrevHash() {
        return prevHash;
    }

    public String getNextHash() {
        return nextHash;
    }

    public UnsignedLong getHeight() {
        return height;
    }

    public Date getMined() {
        return mined;
    }

    public TransactionList getTransactionList() {
        return transactionList;
    }

    public List<String> getTransactionIds() {
        return transactionIds;
    }

    public UnsignedLong getSize() {
        return size;
    }

    public Amount getTotalFees() {
        return totalFees;
    }

    public String getHeader() {
        return header;
    }

    public String getRaw() {
        return raw;
    }

    public UnsignedLong getAcknowledgements() {
        return acknowledgements;
    }

    public boolean isActiveChain() {
        return isActiveChain;
    }
}
