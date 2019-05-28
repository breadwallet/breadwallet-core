package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.breadwallet.crypto.blockchaindb.models.Utilities;
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
            new Blockchain("bitcoin-mainnet", "Bitcoin", "mainnet", true, "btc", 654321, ImmutableList.of()),
            new Blockchain("bitcash-mainnet", "Bitcash", "mainnet", true, "bch", 1000000, ImmutableList.of()),
            new Blockchain("ethereum-mainnet", "Ethereum", "mainnet", true, "eth", 8000000, ImmutableList.of()),

            // Testnet
            new Blockchain("bitcoin-testnet", "Bitcoin", "testnet", false, "btc", 900000, ImmutableList.of()),
            new Blockchain("bitcash-testnet", "Bitcash", "testnet", false, "bch", 1200000, ImmutableList.of()),
            new Blockchain("ethereum-testnet", "Ethereum", "testnet", false, "eth", 1000000, ImmutableList.of()),
            new Blockchain("ethereum-rinkeby", "Ethereum", "rinkeby", false, "eth", 2000000, ImmutableList.of())
    );

    public static Optional<Blockchain> asBlockchain(JSONObject json) {
        try {
            String id = json.getString("id");
            String name = json.getString("name");
            String network = json.getString("network");
            boolean isMainnet = json.getBoolean("is_mainnet");
            String currency = json.getString("native_currency_id");
            long blockHeight = Utilities.getLongFromString(json, "block_height");

            JSONArray feeEstimatesJson = json.getJSONArray("fee_estimates");
            Optional<List<BlockchainFee>> feeEstimatesOption = BlockchainFee.asBlockchainFees(feeEstimatesJson);
            if (!feeEstimatesOption.isPresent()) return Optional.absent();
            List<BlockchainFee> feeEstimates = feeEstimatesOption.get();

            return Optional.of(new Blockchain(id, name, network, isMainnet, currency, Math.max(blockHeight, 575020),
                    feeEstimates));

        } catch (JSONException | NumberFormatException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<Blockchain>> asBlockchains(JSONArray json) {
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
    private final List<BlockchainFee> feeEstimates;

    public Blockchain(String id, String name, String network, boolean isMainnet, String currency, long blockHeight,
                      List<BlockchainFee> feeEstimates) {
        this.id = id;
        this.name = name;
        this.network = network;
        this.isMainnet = isMainnet;
        this.currency = currency;
        this.blockHeight = blockHeight;
        this.feeEstimates = feeEstimates;
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
