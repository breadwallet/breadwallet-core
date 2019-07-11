/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
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
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;

import okhttp3.OkHttpClient;

public class BlockchainDb {

    private final AtomicInteger ridGenerator;

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

    public BlockchainDb(OkHttpClient client, String bdbBaseURL, @Nullable DataTask bdbDataTask,
                        String apiBaseURL, @Nullable DataTask apiDataTask) {
        DataTask defaultDataTask = (cli, request, callback) -> cli.newCall(request).enqueue(callback);
        bdbDataTask = bdbDataTask == null ? defaultDataTask : bdbDataTask;
        apiDataTask = apiDataTask == null ? defaultDataTask : apiDataTask;

        BdbApiClient bdbClient = new BdbApiClient(client, bdbBaseURL, bdbDataTask);
        BrdApiClient brdClient = new BrdApiClient(client, apiBaseURL, apiDataTask);

        ExecutorService executorService = Executors.newCachedThreadPool();

        this.ridGenerator = new AtomicInteger(0);

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

    public void getBlockchains(CompletionHandler<List<Blockchain>> handler) {
        blockchainApi.getBlockchains(true, handler);
    }

    public void getBlockchains(boolean isMainnet, CompletionHandler<List<Blockchain>> handler) {
        blockchainApi.getBlockchains(isMainnet, handler);
    }

    public void getBlockchain(String id, CompletionHandler<Blockchain> handler) {
        blockchainApi.getBlockchain(id, handler);
    }

    // Currency

    public void getCurrencies(CompletionHandler<List<Currency>> handler) {
        currencyApi.getCurrencies(handler);
    }

    public void getCurrencies(@Nullable String id, CompletionHandler<List<Currency>> handler) {
        currencyApi.getCurrencies(id, handler);
    }

    public void getCurrency(String id, CompletionHandler<Currency> handler) {
        currencyApi.getCurrency(id, handler);
    }

    // Subscription

    public void getOrCreateSubscription(Subscription subscription, CompletionHandler<Subscription> handler) {
        subscriptionApi.getOrCreateSubscription(subscription, handler);
    }

    public void getSubscription(String id, CompletionHandler<Subscription> handler) {
        subscriptionApi.getSubscription(id, handler);
    }

    public void createSubscription(Subscription subscription, CompletionHandler<Subscription> handler) {
        subscriptionApi.createSubscription(subscription, handler);
    }

    public void updateSubscription(Subscription subscription, CompletionHandler<Subscription> handler) {
        subscriptionApi.updateSubscription(subscription, handler);
    }

    public void deleteSubscription(String id, CompletionHandler<Subscription> handler) {
        subscriptionApi.deleteSubscription(id, handler);
    }

    // Transfer

    public void getTransfers(String id, List<String> addresses, CompletionHandler<List<Transfer>> handler) {
        transferApi.getTransfers(id, addresses, handler);
    }

    public void getTransfer(String id, CompletionHandler<Transfer> handler) {
        transferApi.getTransfer(id, handler);
    }

    // Wallet

    public void getOrCreateWallet(Wallet wallet, CompletionHandler<Wallet> handler) {
        walletApi.getOrCreateWallet(wallet, handler);
    }

    public void getWallet(String id, CompletionHandler<Wallet> handler) {
        walletApi.getWallet(id, handler);
    }

    public void createWallet(Wallet wallet, CompletionHandler<Wallet> handler) {
        walletApi.createWallet(wallet, handler);
    }

    public void updateWallet(Wallet wallet, CompletionHandler<Wallet> handler) {
        walletApi.updateWallet(wallet, handler);
    }

    public void deleteWallet(String id, CompletionHandler<Wallet> handler) {
        walletApi.deleteWallet(id, handler);
    }

    // Transactions

    public void getTransactions(String id, List<String> addresses, UnsignedLong beginBlockNumber,
                                UnsignedLong endBlockNumber,
                                boolean includeRaw, boolean includeProof,
                                CompletionHandler<List<Transaction>> handler) {
        transactionApi.getTransactions(id, addresses, beginBlockNumber, endBlockNumber, includeRaw, includeProof,
                handler);
    }

    public void getTransaction(String id, boolean includeRaw, boolean includeProof,
                               CompletionHandler<Transaction> handler) {
        transactionApi.getTransaction(id, includeRaw, includeProof, handler);
    }

    public void createTransaction(String id, byte[] txHash, byte[] tx, CompletionHandler<Transaction> handler) {
        transactionApi.createTransaction(id, txHash, tx, handler);
    }

    // Blocks

    public void getBlocks(String id, UnsignedLong beginBlockNumber, UnsignedLong endBlockNumber, boolean includeRaw,
                          boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                          CompletionHandler<List<Block>> handler) {
        blockApi.getBlocks(id, beginBlockNumber, endBlockNumber, includeRaw, includeTx, includeTxRaw, includeTxProof,
                handler);
    }

    public void getBlock(String id, boolean includeRaw,
                         boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                         CompletionHandler<Block> handler) {
        blockApi.getBlock(id, includeRaw, includeTx, includeTxRaw, includeTxProof, handler);
    }

    // ETH Balance

    public void getBalanceAsEth(String networkName, String address, CompletionHandler<String> handler) {
        ethBalanceApi.getBalanceAsEth(networkName, address, ridGenerator.getAndIncrement(), handler);
    }

    public void getBalanceAsTok(String networkName, String address, String tokenAddress,
                                CompletionHandler<String> handler) {
        ethBalanceApi.getBalanceAsTok(networkName, address, tokenAddress, ridGenerator.getAndIncrement(), handler);
    }

    // ETH Gas

    public void getGasPriceAsEth(String networkName, CompletionHandler<String> handler) {
        ethGasApi.getGasPriceAsEth(networkName, ridGenerator.getAndIncrement(), handler);
    }

    public void getGasEstimateAsEth(String networkName, String from, String to, String amount, String data,
                                    CompletionHandler<String> handler) {
        ethGasApi.getGasEstimateAsEth(networkName, from, to, amount, data, ridGenerator.getAndIncrement(), handler);
    }

    // ETH Token

    public void getTokensAsEth(CompletionHandler<List<EthToken>> handler) {
        ethTokenApi.getTokensAsEth(ridGenerator.getAndIncrement(), handler);
    }

    // ETH Block

    public void getBlockNumberAsEth(String networkName, CompletionHandler<String> handler) {
        ethBlockApi.getBlockNumberAsEth(networkName, ridGenerator.getAndIncrement(), handler);
    }

    // ETH Transfer

    public void submitTransactionAsEth(String networkName, String transaction,
                                       CompletionHandler<String> handler) {
        ethTransferApi.submitTransactionAsEth(networkName, transaction, ridGenerator.getAndIncrement(), handler);
    }

    public void getTransactionsAsEth(String networkName, String address,
                                     UnsignedLong begBlockNumber, UnsignedLong endBlockNumber,
                                     CompletionHandler<List<EthTransaction>> handler) {
        ethTransferApi.getTransactionsAsEth(networkName, address, begBlockNumber, endBlockNumber,
                ridGenerator.getAndIncrement(), handler);
    }

    public void getNonceAsEth(String networkName, String address, CompletionHandler<String> handler) {
        ethTransferApi.getNonceAsEth(networkName, address, ridGenerator.getAndIncrement(), handler);
    }

    public void getLogsAsEth(String networkName, @Nullable String contract, String address, String event,
                             UnsignedLong begBlockNumber, UnsignedLong endBlockNumber,
                             CompletionHandler<List<EthLog>> handler) {
        ethTransferApi.getLogsAsEth(networkName, contract, address, event, begBlockNumber, endBlockNumber,
                ridGenerator.getAndIncrement(), handler);
    }

    public void getBlocksAsEth(String networkName, String address, UnsignedInteger interests,
                               UnsignedLong blockStart, UnsignedLong blockEnd,
                               CompletionHandler<List<UnsignedLong>> handler) {
        ethTransferApi.getBlocksAsEth(networkName, address, interests, blockStart, blockEnd,
                ridGenerator.getAndIncrement(), handler);
    }
}
