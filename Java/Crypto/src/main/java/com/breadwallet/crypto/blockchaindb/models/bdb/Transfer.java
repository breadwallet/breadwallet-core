/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonProperty;

import java.util.Map;

public class Transfer {

    // fields

    @JsonProperty("transfer_id")
    private String transferId;

    @JsonProperty("blockchain_id")
    private String blockchainId;

    @JsonProperty("from_address")
    private String fromAddress;

    @JsonProperty("to_address")
    private String toAddress;

    @JsonProperty("transaction_id")
    private String transactionId;

    private Long index;

    private Amount amount;

    private Map<String, String> meta;

    private Long acknowledgements;

    // getters

    public String getId() {
        return transferId;
    }

    public String getBlockchainId() {
        return blockchainId;
    }

    public String getFromAddress() {
        return fromAddress;
    }

    public String getToAddress() {
        return toAddress;
    }

    public String getTransactionId() {
        return transactionId;
    }

    public Long getIndex() {
        return index;
    }

    public Amount getAmount() {
        return amount;
    }

    public Map<String, String> getMeta() {
        return meta;
    }

    public Long getAcknowledgements() {
        return acknowledgements;
    }
}
