package com.breadwallet.crypto.blockchaindb;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.apis.bdb.BlockApi;
import com.breadwallet.crypto.blockchaindb.apis.bdb.BlockchainApi;
import com.breadwallet.crypto.blockchaindb.apis.bdb.CurrencyApi;
import com.breadwallet.crypto.blockchaindb.apis.bdb.BdbApiClient;
import com.breadwallet.crypto.blockchaindb.apis.bdb.SubscriptionApi;
import com.breadwallet.crypto.blockchaindb.apis.bdb.TransactionApi;
import com.breadwallet.crypto.blockchaindb.apis.bdb.TransferApi;
import com.breadwallet.crypto.blockchaindb.apis.bdb.WalletApi;
import com.breadwallet.crypto.blockchaindb.apis.brd.EthBalanceApi;
import com.breadwallet.crypto.blockchaindb.apis.brd.BrdApiClient;
import com.breadwallet.crypto.blockchaindb.apis.brd.EthGasApi;
import com.breadwallet.crypto.blockchaindb.apis.brd.EthTokenApi;
import com.breadwallet.crypto.blockchaindb.apis.brd.EthBlockApi;
import com.breadwallet.crypto.blockchaindb.apis.brd.EthTransferApi;
import com.breadwallet.crypto.blockchaindb.models.bdb.Block;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.Currency;
import com.breadwallet.crypto.blockchaindb.models.bdb.Subscription;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transfer;
import com.breadwallet.crypto.blockchaindb.models.bdb.Wallet;
import com.breadwallet.crypto.blockchaindb.models.brd.EthLog;
import com.breadwallet.crypto.blockchaindb.models.brd.EthToken;
import com.breadwallet.crypto.blockchaindb.models.brd.EthTransaction;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import okhttp3.OkHttpClient;

// TODO: Lets discuss rids; do we need these in the callback?

public class BlockchainDb {

    private final BlockApi blockApi;
    private final BlockchainApi blockchainApi;
    private final CurrencyApi currencyApi;
    private final SubscriptionApi subscriptionApi;
    private final TransferApi transferApi;
    private final TransactionApi transactionApi;
    private final WalletApi walletApi;

    private final EthBalanceApi ethBalanceApi;
    private final EthBlockApi ethBlockApi;
    private final EthGasApi ethGasApi;
    private final EthTokenApi ethTokenApi;
    private final EthTransferApi ethTransferApi;

    public BlockchainDb(OkHttpClient client, String bdbBaseURL, String apiBaseURL) {
        this(client, bdbBaseURL, null, apiBaseURL, null);
    }

    public BlockchainDb(OkHttpClient client, String bdbBaseURL, @Nullable BlockchainDataTask bdbDataTask,
                        String apiBaseURL, @Nullable BlockchainDataTask apiDataTask) {
        BlockchainDataTask defaultDataTask = (cli, request, callback) -> cli.newCall(request).enqueue(callback);
        bdbDataTask = bdbDataTask == null ? defaultDataTask : bdbDataTask;


        BdbApiClient bdbClient = new BdbApiClient(client, bdbBaseURL, bdbDataTask);
        BrdApiClient brdClient = new BrdApiClient(client, apiBaseURL, apiDataTask);

        ExecutorService executorService = Executors.newCachedThreadPool();

        this.blockApi = new BlockApi(bdbClient, executorService);
        this.blockchainApi = new BlockchainApi(bdbClient);
        this.currencyApi = new CurrencyApi(bdbClient);
        this.subscriptionApi = new SubscriptionApi(bdbClient);
        this.transferApi = new TransferApi(bdbClient);
        this.transactionApi = new TransactionApi(bdbClient, executorService);
        this.walletApi = new WalletApi(bdbClient);

        this.ethBalanceApi = new EthBalanceApi(brdClient);
        this.ethBlockApi = new EthBlockApi(brdClient);
        this.ethGasApi = new EthGasApi(brdClient);
        this.ethTokenApi = new EthTokenApi(brdClient);
        this.ethTransferApi = new EthTransferApi(brdClient, executorService);
    }

    // Blockchain

    public void getBlockchains(BlockchainCompletionHandler<List<Blockchain>> handler) {
        blockchainApi.getBlockchains(true, handler);
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
        transactionApi.getTransactions(id, addresses, beginBlockNumber, endBlockNumber, includeRaw, includeProof,
                handler);
    }

    public void getTransaction(String id, boolean includeRaw, boolean includeProof,
                               BlockchainCompletionHandler<Transaction> handler) {
        transactionApi.getTransaction(id, includeRaw, includeProof, handler);
    }

    public void putTransaction(String id, byte[] data, BlockchainCompletionHandler<Transaction> handler) {
        transactionApi.putTransaction(id, data, handler);
    }

    // Blocks

    public void getBlocks(String id, long beginBlockNumber, long endBlockNumber, boolean includeRaw,
                          boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                          BlockchainCompletionHandler<List<Block>> handler) {
        blockApi.getBlocks(id, beginBlockNumber, endBlockNumber, includeRaw, includeTx, includeTxRaw, includeTxProof,
                handler);
    }

    public void getBlock(String id, boolean includeRaw,
                         boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                         BlockchainCompletionHandler<Block> handler) {
        blockApi.getBlock(id, includeRaw, includeTx, includeTxRaw, includeTxProof, handler);
    }

    // ETH Balance

    public void getBalanceAsEth(String networkName, String address, int rid,
                                BlockchainCompletionHandler<String> handler) {
        ethBalanceApi.getBalanceAsEth(networkName, address, rid, handler);
    }

    public void getBalanceAsTok(String networkName, String address, String tokenAddress, int rid,
                                BlockchainCompletionHandler<String> handler) {
        ethBalanceApi.getBalanceAsTok(networkName, address, tokenAddress, rid, handler);
    }

    // ETH Gas

    public void getGasPriceAsEth(String networkName, int rid, BlockchainCompletionHandler<String> handler) {
        ethGasApi.getGasPriceAsEth(networkName, rid, handler);
    }

    public void getGasEstimateAsEth(String networkName, String from, String to, String amount, String data, int rid,
                                    BlockchainCompletionHandler<String> handler) {
        ethGasApi.getGasEstimateAsEth(networkName, from, to, amount, data, rid, handler);
    }

    // ETH Token

    public void getTokensAsEth(int rid, BlockchainCompletionHandler<List<EthToken>> handler) {
        ethTokenApi.getTokensAsEth(rid, handler);
    }

    // ETH Block

    public void getBlockNumberAsEth(String networkName, int rid, BlockchainCompletionHandler<String> handler) {
        ethBlockApi.getBlockNumberAsEth(networkName, rid, handler);
    }

    // ETH Transfer

    public void submitTransactionAsEth(String networkName, String transaction, int rid,
                                       BlockchainCompletionHandler<String> handler) {
        ethTransferApi.submitTransactionAsEth(networkName, transaction, rid, handler);
    }

    public void getTransactionsAsEth(String networkName, String address, long begBlockNumber, long endBlockNumber,
                                     int rid, BlockchainCompletionHandler<List<EthTransaction>> handler) {
        ethTransferApi.getTransactionsAsEth(networkName, address, begBlockNumber, endBlockNumber, rid, handler);
    }

    public void getNonceAsEth(String networkName, String address, int rid, BlockchainCompletionHandler<String> handler) {
        ethTransferApi.getNonceAsEth(networkName, address, rid, handler);
    }

    public void getLogsAsEth(String networkName, @Nullable String contract, String address, String event,
                             long begBlockNumber, long endBlockNumber, int rid, BlockchainCompletionHandler<List<EthLog>> handler) {
        ethTransferApi.getLogsAsEth(networkName, contract, address, event, begBlockNumber, endBlockNumber, rid, handler);
    }

    public void getBlocksAsEth(String networkName, String address, int interests, long blockStart, long blockEnd,
                               int rid, BlockchainCompletionHandler<List<Long>> handler) {
        ethTransferApi.getBlocksAsEth(networkName, address, interests, blockStart, blockEnd, rid, handler);
    }
}
