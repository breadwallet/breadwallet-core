package com.breadwallet.crypto.blockchaindb;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.apis.BlockJsonApi;
import com.breadwallet.crypto.blockchaindb.apis.BlockchainJsonApi;
import com.breadwallet.crypto.blockchaindb.apis.CurrencyJsonApi;
import com.breadwallet.crypto.blockchaindb.apis.JsonApiClient;
import com.breadwallet.crypto.blockchaindb.apis.SubscriptionJsonApi;
import com.breadwallet.crypto.blockchaindb.apis.TransactionJsonApi;
import com.breadwallet.crypto.blockchaindb.apis.TransferJsonApi;
import com.breadwallet.crypto.blockchaindb.apis.WalletJsonApi;
import com.breadwallet.crypto.blockchaindb.models.Block;
import com.breadwallet.crypto.blockchaindb.models.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.Currency;
import com.breadwallet.crypto.blockchaindb.models.Subscription;
import com.breadwallet.crypto.blockchaindb.models.Transaction;
import com.breadwallet.crypto.blockchaindb.models.Transfer;
import com.breadwallet.crypto.blockchaindb.models.Wallet;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import okhttp3.OkHttpClient;

public class BlockchainDb {

    private final BlockJsonApi blockApi;
    private final BlockchainJsonApi blockchainApi;
    private final CurrencyJsonApi currencyApi;
    private final SubscriptionJsonApi subscriptionApi;
    private final TransferJsonApi transferApi;
    private final TransactionJsonApi transactionApi;
    private final WalletJsonApi walletApi;

    public BlockchainDb(OkHttpClient client, String bdbBaseURL, @Nullable BlockchainDataTask bdbDataTask,
                        String apiBaseURL, @Nullable BlockchainDataTask apiDataTask) {
        BlockchainDataTask defaultDataTask = (cli, request, callback) -> cli.newCall(request).enqueue(callback);
        bdbDataTask = bdbDataTask == null ? defaultDataTask : bdbDataTask;

        ExecutorService executorService = Executors.newCachedThreadPool();

        JsonApiClient bdbClient = new JsonApiClient(client, bdbBaseURL, bdbDataTask);

        this.blockApi = new BlockJsonApi(bdbClient, executorService);
        this.blockchainApi = new BlockchainJsonApi(bdbClient);
        this.currencyApi = new CurrencyJsonApi(bdbClient);
        this.subscriptionApi = new SubscriptionJsonApi(bdbClient);
        this.transferApi = new TransferJsonApi(bdbClient);
        this.transactionApi = new TransactionJsonApi(bdbClient, executorService);
        this.walletApi = new WalletJsonApi(bdbClient);

    }

    // Blockchain

    public void getBlockchains(BlockchainCompletionHandler<List<Blockchain>> handler) {
        blockchainApi.getBlockchains(handler);
    }

    public void getBlockchains(boolean ismainnet, BlockchainCompletionHandler<List<Blockchain>> handler) {
        blockchainApi.getBlockchains(ismainnet, handler);
    }

    public void getBlockchain(String id, BlockchainCompletionHandler<Blockchain> handler) {
        blockchainApi.getBlockchain(id, handler);
    }

    // Currency

    public void getCurrencies(BlockchainCompletionHandler<List<Currency>> handler) {
        currencyApi.getCurrencies(handler);
    }

    public void getCurrencies(@Nullable String id, BlockchainCompletionHandler<List<Currency>> handler) {
        currencyApi.getCurrencies(id, handler);
    }

    public void getCurrency(String id, BlockchainCompletionHandler<Currency> handler) {
        currencyApi.getCurrency(id, handler);
    }

    // Subscription

    public void getOrCreateSubscription(Subscription subscription, BlockchainCompletionHandler<Subscription> handler) {
        subscriptionApi.getOrCreateSubscription(subscription, handler);
    }

    public void getSubscription(String id, BlockchainCompletionHandler<Subscription> handler) {
        subscriptionApi.getSubscription(id, handler);
    }

    public void createSubscription(Subscription subscription, BlockchainCompletionHandler<Subscription> handler) {
        subscriptionApi.createSubscription(subscription, handler);
    }

    public void updateSubscription(Subscription subscription, BlockchainCompletionHandler<Subscription> handler) {
        subscriptionApi.updateSubscription(subscription, handler);
    }

    public void deleteSubscription(String id, BlockchainCompletionHandler<Subscription> handler) {
        subscriptionApi.deleteSubscription(id, handler);
    }

    // Transfer

    public void getTransfers(String id, List<String> addresses, BlockchainCompletionHandler<List<Transfer>> handler) {
        transferApi.getTransfers(id, addresses, handler);
    }

    public void getTransfer(String id, BlockchainCompletionHandler<Transfer> handler) {
        transferApi.getTransfer(id, handler);
    }

    // Wallet

    public void getOrCreateWallet(Wallet wallet, BlockchainCompletionHandler<Wallet> handler) {
        walletApi.getOrCreateWallet(wallet, handler);
    }

    public void getWallet(String id, BlockchainCompletionHandler<Wallet> handler) {
        walletApi.getWallet(id, handler);
    }

    public void createWallet(Wallet wallet, BlockchainCompletionHandler<Wallet> handler) {
        walletApi.createWallet(wallet, handler);
    }

    public void updateWallet(Wallet wallet, BlockchainCompletionHandler<Wallet> handler) {
        walletApi.updateWallet(wallet, handler);
    }

    public void deleteWallet(String id, BlockchainCompletionHandler<Wallet> handler) {
        walletApi.deleteWallet(id, handler);
    }

    // Transactions

    public void getTransactions(String id, List<String> addresses, long beginBlockNumber, long endBlockNumber,
                                boolean includeRaw, boolean includeProof,
                                BlockchainCompletionHandler<List<Transaction>> handler) {
        transactionApi.getTransactions(id, addresses, beginBlockNumber, endBlockNumber, includeRaw, includeProof, handler);
    }

    public void getTransaction(String id, boolean includeRaw, boolean includeProof,
                               BlockchainCompletionHandler<Transaction> handler) {
        transactionApi.getTransaction(id, includeRaw, includeProof, handler);
    }

    // TODO: Add putTransaction()

    // Blocks

    public void getBlocks(String id, long beginBlockNumber, long endBlockNumber, boolean includeRaw,
                          boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                          BlockchainCompletionHandler<List<Block>> handler) {
        blockApi.getBlocks(id, beginBlockNumber, endBlockNumber, includeRaw, includeTx, includeTxRaw, includeTxProof, handler);
    }

    public void getBlock(String id, boolean includeRaw,
                         boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                         BlockchainCompletionHandler<Block> handler) {
        blockApi.getBlock(id, includeRaw, includeTx, includeTxRaw, includeTxProof, handler);
    }
}
