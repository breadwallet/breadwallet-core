/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.brd;

import com.google.common.base.Optional;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class EthTransaction {

    public static Optional<EthTransaction> asTransaction(JSONObject json) {
        try {
            String hash = json.getString("hash");
            String sourceAddr = json.getString("from");
            String targetAddr = json.getString("to");
            String contractAddr = json.getString("contractAddress");
            String amount = json.getString("value");
            String gasLimit = json.getString("gas");
            String gasPrice = json.getString("gasPrice");
            String data = json.getString("input");
            String nonce = json.getString("nonce");
            String gasUsed = json.getString("gasUsed");
            String blockNumber = json.getString("blockNumber");
            String blockHash = json.getString("blockHash");
            String blockConfirmations = json.getString("confirmations");
            String blockTransacionIndex = json.getString("transactionIndex");
            String blockTimestamp = json.getString("timeStamp");
            String isError = json.getString("isError");

            return Optional.of(new EthTransaction(
                    hash, sourceAddr, targetAddr, contractAddr, amount, gasLimit, gasPrice,
                    data, nonce, gasUsed, blockNumber, blockHash, blockConfirmations, blockTransacionIndex,
                    blockTimestamp, isError
            ));
        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<EthTransaction>> asTransactions(JSONArray json) {
        List<EthTransaction> objs = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject obj = json.optJSONObject(i);
            if (obj == null) {
                return Optional.absent();
            }

            Optional<EthTransaction> opt = EthTransaction.asTransaction(obj);
            if (!opt.isPresent()) {
                return Optional.absent();
            }

            objs.add(opt.get());
        }
        return Optional.of(objs);
    }

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
    private final String blockTransacionIndex;
    private final String blockTimestamp;
    private final String isError;

    private EthTransaction(String hash, String sourceAddr, String targetAddr, String contractAddr, String amount,
                           String gasLimit, String gasPrice, String data, String nonce, String gasUsed,
                           String blockNumber, String blockHash, String blockConfirmations,
                           String blockTransacionIndex, String blockTimestamp, String isError) {
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
        this.blockTransacionIndex = blockTransacionIndex;
        this.blockTimestamp = blockTimestamp;
        this.isError = isError;
    }

    public String getHash() {
        return hash;
    }

    public String getSourceAddr() {
        return sourceAddr;
    }

    public String getTargetAddr() {
        return targetAddr;
    }

    public String getContractAddr() {
        return contractAddr;
    }

    public String getAmount() {
        return amount;
    }

    public String getGasLimit() {
        return gasLimit;
    }

    public String getGasPrice() {
        return gasPrice;
    }

    public String getData() {
        return data;
    }

    public String getNonce() {
        return nonce;
    }

    public String getGasUsed() {
        return gasUsed;
    }

    public String getBlockNumber() {
        return blockNumber;
    }

    public String getBlockHash() {
        return blockHash;
    }

    public String getBlockConfirmations() {
        return blockConfirmations;
    }

    public String getBlockTransacionIndex() {
        return blockTransacionIndex;
    }

    public String getBlockTimestamp() {
        return blockTimestamp;
    }

    public String getIsError() {
        return isError;
    }
}
