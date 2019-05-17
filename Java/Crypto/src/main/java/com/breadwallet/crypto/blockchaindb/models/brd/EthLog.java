package com.breadwallet.crypto.blockchaindb.models.brd;

import com.breadwallet.crypto.blockchaindb.models.Utilities;
import com.google.common.base.Optional;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class EthLog {

    public static Optional<EthLog> asLog(JSONObject json, int rid) {
        try {
            String hash = json.getString("transactionHash");
            String contract = json.getString("address");
            List<String> topics = trimLast(Utilities.getStringList(json, "topics"));
            String data = json.getString("data");
            String gasPrice = json.getString("gasPrice");
            String gasUsed = json.getString("gasUsed");
            String logIndex = json.getString("logIndex");
            String blockNumber = json.getString("blockNumber");
            String blockTransactionIndex = json.getString("transactionIndex");
            String blockTimestamp = json.getString("timeStamp");

            return Optional.of(new EthLog(
                    hash, contract, topics, data, gasPrice, gasUsed, logIndex, blockNumber,
                    blockTransactionIndex, blockTimestamp, rid
            ));
        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<EthLog>> asLogs(JSONArray json, int rid) {
        List<EthLog> objs = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject obj = json.optJSONObject(i);
            if (obj == null) {
                return Optional.absent();
            }

            Optional<EthLog> opt = EthLog.asLog(obj, rid);
            if (!opt.isPresent()) {
                return Optional.absent();
            }

            objs.add(opt.get());
        }
        return Optional.of(objs);
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
    private final int rid;

    private EthLog(String hash, String contract, List<String> topics, String data, String gasPrice, String gasUsed,
                  String logIndex, String blockNumber, String blockTransactionIndex, String blockTimestamp, int rid) {
        this.hash = hash;
        this.contract = contract;
        this.topics = topics;
        this.data = data;
        this.gasPrice = gasPrice;
        this.gasUsed = gasUsed;
        this.logIndex = logIndex;
        this.blockNumber = blockNumber;
        this.blockTransactionIndex = blockTransactionIndex;
        this.blockTimestamp = blockTimestamp;
        this.rid = rid;
    }

    public String getHash() {
        return hash;
    }

    public String getContract() {
        return contract;
    }

    public List<String> getTopics() {
        return topics;
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

    public int getRid() {
        return rid;
    }
}
