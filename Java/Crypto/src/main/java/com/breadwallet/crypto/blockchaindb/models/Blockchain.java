package com.breadwallet.crypto.blockchaindb.models;

import com.breadwallet.crypto.blockchaindb.JsonUtilities;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableList;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class Blockchain {

    public static List<Blockchain> DEFAULT_BLOCKCHAINS = ImmutableList.of(
            // Mainnet
            new Blockchain("bitcash-mainnet", "Bitcash", "mainnet", true, "bch", 1000000),
            new Blockchain("ethereum-mainnet", "Ethereum", "mainnet", true, "eth", 8000000),

            // Testnet
            new Blockchain("bitcoin-testnet", "Bitcoin", "testnet", false, "btc", 900000),
            new Blockchain("bitcash-testnet", "Bitcash", "testnet", false, "bch", 1200000),
            new Blockchain("ethereum-testnet", "Ethereum", "testnet", false, "eth", 1000000),
            new Blockchain("ethereum-rinkeby", "Ethereum", "rinkeby", false, "eth", 2000000)
    );

    public static Optional<Blockchain> asBlockchain(JSONObject json) {
        try {
            String id = json.getString("id");
            String name = json.getString("name");
            String network = json.getString("network");
            boolean isMainnet = json.getBoolean("is_mainnet");
            String currency = json.getString("native_currency_id");
            long blockHeight = JsonUtilities.getLongFromString(json,"block_height");
            return Optional.of(new Blockchain(id, name, network, isMainnet, currency, Math.max(blockHeight, 575020)));

        } catch (JSONException | NumberFormatException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<Blockchain>> asBlockchains(JSONArray json){
        List<Blockchain> blockchains = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject blockchainsObject = json.optJSONObject(i);
            if (blockchainsObject == null) {
                return Optional.absent();
            }

            Optional<Blockchain> optionalBlockchains = Blockchain.asBlockchain(blockchainsObject);
            if (!optionalBlockchains.isPresent()) {
                return Optional.absent();
            }

            blockchains.add(optionalBlockchains.get());
        }
        return Optional.of(blockchains);
    }

    private final String id;
    private final String name;
    private final String network;
    private final boolean isMainnet;
    private final String currency;
    private final long blockHeight;

    public Blockchain(String id, String name, String network, boolean isMainnet, String currency, long blockHeight) {
        this.id = id;
        this.name = name;
        this.network = network;
        this.isMainnet = isMainnet;
        this.currency = currency;
        this.blockHeight = blockHeight;
    }

    public String getId() {
        return id;
    }

    public String getName() {
        return name;
    }

    public String getNetwork() {
        return network;
    }

    public boolean isMainnet() {
        return isMainnet;
    }

    public String getCurrency() {
        return currency;
    }

    public long getBlockHeight() {
        return blockHeight;
    }
}
