/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
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
import com.breadwallet.crypto.blockchaindb.apis.brd.EthBalanceApi;
import com.breadwallet.crypto.blockchaindb.apis.brd.BrdApiClient;
import com.breadwallet.crypto.blockchaindb.apis.brd.EthGasApi;
import com.breadwallet.crypto.blockchaindb.apis.brd.EthTokenApi;
import com.breadwallet.crypto.blockchaindb.apis.brd.EthBlockApi;
import com.breadwallet.crypto.blockchaindb.apis.brd.EthTransferApi;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Block;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.Currency;
import com.breadwallet.crypto.blockchaindb.models.bdb.Subscription;
import com.breadwallet.crypto.blockchaindb.models.bdb.SubscriptionCurrency;
import com.breadwallet.crypto.blockchaindb.models.bdb.SubscriptionEndpoint;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transfer;
import com.breadwallet.crypto.blockchaindb.models.brd.EthLog;
import com.breadwallet.crypto.blockchaindb.models.brd.EthToken;
import com.breadwallet.crypto.blockchaindb.models.brd.EthTransaction;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;

import okhttp3.OkHttpClient;
import okhttp3.Request;

public class BlockchainDb {

    private static final String DEFAULT_BDB_BASE_URL = "https://api.blockset.com";
    private static final String DEFAULT_API_BASE_URL = "https://api.breadwallet.com";
    private static final DataTask DEFAULT_DATA_TASK = (cli, request, callback) -> cli.newCall(request).enqueue(callback);

    private final AtomicInteger ridGenerator;

    private final BlockApi blockApi;
    private final BlockchainApi blockchainApi;
    private final CurrencyApi currencyApi;
    private final SubscriptionApi subscriptionApi;
    private final TransferApi transferApi;
    private final TransactionApi transactionApi;

    private final EthBalanceApi ethBalanceApi;
    private final EthBlockApi ethBlockApi;
    private final EthGasApi ethGasApi;
    private final EthTokenApi ethTokenApi;
    private final EthTransferApi ethTransferApi;

    public BlockchainDb(OkHttpClient client) {
        this(client, null, null, null, null);
    }

    public BlockchainDb(OkHttpClient client, String bdbBaseURL, String apiBaseURL) {
        this(client, bdbBaseURL, null, apiBaseURL, null);
    }

    public BlockchainDb(OkHttpClient client,
                        @Nullable String bdbBaseURL,
                        @Nullable DataTask bdbDataTask,
                        @Nullable String apiBaseURL,
                        @Nullable DataTask apiDataTask) {
        bdbBaseURL = bdbBaseURL == null ? DEFAULT_BDB_BASE_URL : bdbBaseURL;
        apiBaseURL = apiBaseURL == null ? DEFAULT_API_BASE_URL : apiBaseURL;

        bdbDataTask = bdbDataTask == null ? DEFAULT_DATA_TASK : bdbDataTask;
        apiDataTask = apiDataTask == null ? DEFAULT_DATA_TASK : apiDataTask;

        ObjectCoder coder = ObjectCoder.createObjectCoderWithFailOnUnknownProperties();
        BdbApiClient bdbClient = new BdbApiClient(client, bdbBaseURL, bdbDataTask, coder);
        BrdApiClient brdClient = new BrdApiClient(client, apiBaseURL, apiDataTask, coder);

        ExecutorService executorService = Executors.newCachedThreadPool();

        this.ridGenerator = new AtomicInteger(0);

        this.blockApi = new BlockApi(bdbClient, executorService);
        this.blockchainApi = new BlockchainApi(bdbClient);
        this.currencyApi = new CurrencyApi(bdbClient);
        this.subscriptionApi = new SubscriptionApi(bdbClient);
        this.transferApi = new TransferApi(bdbClient, executorService);
        this.transactionApi = new TransactionApi(bdbClient, executorService);

        this.ethBalanceApi = new EthBalanceApi(brdClient);
        this.ethBlockApi = new EthBlockApi(brdClient);
        this.ethGasApi = new EthGasApi(brdClient);
        this.ethTokenApi = new EthTokenApi(brdClient);
        this.ethTransferApi = new EthTransferApi(brdClient);
    }

    public static BlockchainDb createForTest (OkHttpClient client,
                                              String bdbAuthToken) {
        return createForTest(client, bdbAuthToken, null, null);
    }

    public static BlockchainDb createForTest (OkHttpClient client,
                                              String bdbAuthToken,
                                              @Nullable String bdbBaseURL,
                                              @Nullable String apiBaseURL) {
        DataTask brdDataTask = (cli, request, callback) -> {
            Request decoratedRequest = request.newBuilder()
                    .header("Authorization", "Bearer " + bdbAuthToken)
                    .build();
            cli.newCall(decoratedRequest).enqueue(callback);
        };
        return new BlockchainDb (client, bdbBaseURL, brdDataTask, apiBaseURL, null);
    }

    // Blockchain

    public void getBlockchains(CompletionHandler<List<Blockchain>, QueryError> handler) {
        blockchainApi.getBlockchains(
                true,
                handler
        );
    }

    public void getBlockchains(boolean isMainnet,
                               CompletionHandler<List<Blockchain>, QueryError> handler) {
        blockchainApi.getBlockchains(
                isMainnet,
                handler
        );
    }

    public void getBlockchain(String id,
                              CompletionHandler<Blockchain, QueryError> handler) {
        blockchainApi.getBlockchain(
                id,
                handler
        );
    }

    // Currency

    public void getCurrencies(CompletionHandler<List<Currency>, QueryError> handler) {
        currencyApi.getCurrencies(
                handler
        );
    }

    public void getCurrencies(@Nullable String id,
                              CompletionHandler<List<Currency>, QueryError> handler) {
        currencyApi.getCurrencies(
                id,
                handler
        );
    }

    public void getCurrency(String id,
                            CompletionHandler<Currency, QueryError> handler) {
        currencyApi.getCurrency(
                id,
                handler
        );
    }

    // Subscription

    public void getOrCreateSubscription(Subscription subscription,
                                        CompletionHandler<Subscription, QueryError> handler) {
        subscriptionApi.getOrCreateSubscription(
                subscription,
                handler
        );
    }

    public void getSubscription(String id,
                                CompletionHandler<Subscription, QueryError> handler) {
        subscriptionApi.getSubscription(
                id,
                handler
        );
    }

    public void getSubscriptions(CompletionHandler<List<Subscription>, QueryError> handler) {
        subscriptionApi.getSubscriptions(
                handler
        );
    }

    public void createSubscription(String deviceId,
                                   SubscriptionEndpoint endpoint,
                                   List<SubscriptionCurrency> currencies,
                                   CompletionHandler<Subscription, QueryError> handler) {
        subscriptionApi.createSubscription(
                deviceId,
                endpoint,
                currencies,
                handler
        );
    }

    public void updateSubscription(Subscription subscription,
                                   CompletionHandler<Subscription, QueryError> handler) {
        subscriptionApi.updateSubscription(
                subscription,
                handler
        );
    }

    public void deleteSubscription(String id,
                                   CompletionHandler<Void, QueryError> handler) {
        subscriptionApi.deleteSubscription(
                id,
                handler
        );
    }

    // Transfer

    public void getTransfers(String id,
                             List<String> addresses,
                             UnsignedLong beginBlockNumber,
                             UnsignedLong endBlockNumber,
                             CompletionHandler<List<Transfer>, QueryError> handler) {
        getTransfers(
                id,
                addresses,
                beginBlockNumber,
                endBlockNumber,
                null,
                handler
        );
    }

    public void getTransfers(String id,
                             List<String> addresses,
                             UnsignedLong beginBlockNumber,
                             UnsignedLong endBlockNumber,
                             @Nullable Integer maxPageSize,
                             CompletionHandler<List<Transfer>, QueryError> handler) {
        transferApi.getTransfers(
                id,
                addresses,
                beginBlockNumber,
                endBlockNumber,
                maxPageSize,
                handler
        );
    }

    public void getTransfer(String id,
                            CompletionHandler<Transfer, QueryError> handler) {
        transferApi.getTransfer(
                id,
                handler
        );
    }

    // Transactions

    public void getTransactions(String id,
                                List<String> addresses,
                                @Nullable UnsignedLong beginBlockNumber,
                                @Nullable UnsignedLong endBlockNumber,
                                boolean includeRaw,
                                boolean includeProof,
                                CompletionHandler<List<Transaction>, QueryError> handler) {
        getTransactions(
                id,
                addresses,
                beginBlockNumber,
                endBlockNumber,
                includeRaw,
                includeProof,
                null,
                handler
        );
    }

    public void getTransactions(String id,
                                List<String> addresses,
                                @Nullable UnsignedLong beginBlockNumber,
                                @Nullable UnsignedLong endBlockNumber,
                                boolean includeRaw,
                                boolean includeProof,
                                @Nullable Integer maxPageSize,
                                CompletionHandler<List<Transaction>, QueryError> handler) {
        transactionApi.getTransactions(
                id,
                addresses,
                beginBlockNumber,
                endBlockNumber,
                includeRaw,
                includeProof,
                maxPageSize,
                handler
        );
    }

    public void getTransaction(String id,
                               boolean includeRaw,
                               boolean includeProof,
                               CompletionHandler<Transaction, QueryError> handler) {
        transactionApi.getTransaction(
                id,
                includeRaw,
                includeProof,
                handler
        );
    }

    public void createTransaction(String id,
                                  String hashAsHex,
                                  byte[] tx,
                                  CompletionHandler<Void, QueryError> handler) {
        transactionApi.createTransaction(
                id,
                hashAsHex,
                tx,
                handler
        );
    }

    // Blocks

    public void getBlocks(String id,
                          UnsignedLong beginBlockNumber,
                          UnsignedLong endBlockNumber,
                          boolean includeTx,
                          boolean includeTxRaw,
                          boolean includeTxProof,
                          CompletionHandler<List<Block>, QueryError> handler) {
        getBlocks(
                id,
                beginBlockNumber,
                endBlockNumber,
                includeTx,
                includeTxRaw,
                includeTxProof,
                null,
                handler)
        ;
    }

    public void getBlocks(String id,
                          UnsignedLong beginBlockNumber,
                          UnsignedLong endBlockNumber,
                          boolean includeTx,
                          boolean includeTxRaw,
                          boolean includeTxProof,
                          @Nullable Integer maxPageSize,
                          CompletionHandler<List<Block>, QueryError> handler) {
        blockApi.getBlocks(
                id,
                beginBlockNumber,
                endBlockNumber,
                false,
                includeTx,
                includeTxRaw,
                includeTxProof,
                maxPageSize,
                handler);
    }

    public void getBlocksWithRaw(String id,
                                 UnsignedLong beginBlockNumber,
                                 UnsignedLong endBlockNumber,
                                 @Nullable Integer maxPageSize,
                                 CompletionHandler<List<Block>, QueryError> handler) {
        blockApi.getBlocks(
                id,
                beginBlockNumber,
                endBlockNumber,
                true,
                false,
                false,
                false,
                maxPageSize,
                handler
        );
    }

    public void getBlock(String id,
                         boolean includeTx,
                         boolean includeTxRaw,
                         boolean includeTxProof,
                         CompletionHandler<Block, QueryError> handler) {
        blockApi.getBlock(
                id,
                false,
                includeTx,
                includeTxRaw,
                includeTxProof,
                handler
        );
    }

    public void getBlockWithRaw(String id,
                                CompletionHandler<Block, QueryError> handler) {
        blockApi.getBlock(
                id,
                true,
                false,
                false,
                false,
                handler
        );
    }

    // ETH Balance

    public void getBalanceAsEth(String networkName,
                                String address,
                                CompletionHandler<String, QueryError> handler) {
        ethBalanceApi.getBalanceAsEth(
                networkName,
                address,
                ridGenerator.getAndIncrement(),
                handler
        );
    }

    public void getBalanceAsTok(String networkName,
                                String address,
                                String tokenAddress,
                                CompletionHandler<String, QueryError> handler) {
        ethBalanceApi.getBalanceAsTok(
                networkName,
                address,
                tokenAddress,
                ridGenerator.getAndIncrement(),
                handler
        );
    }

    // ETH Gas

    public void getGasPriceAsEth(String networkName,
                                 CompletionHandler<String, QueryError> handler) {
        ethGasApi.getGasPriceAsEth(
                networkName,
                ridGenerator.getAndIncrement(),
                handler
        );
    }

    public void getGasEstimateAsEth(String networkName,
                                    String from,
                                    String to,
                                    String amount,
                                    String data,
                                    CompletionHandler<String, QueryError> handler) {
        ethGasApi.getGasEstimateAsEth(
                networkName,
                from,
                to,
                amount,
                data,
                ridGenerator.getAndIncrement(),
                handler
        );
    }

    // ETH Token

    public void getTokensAsEth(CompletionHandler<List<EthToken>, QueryError> handler) {
        ethTokenApi.getTokensAsEth(
                ridGenerator.getAndIncrement(),
                handler
        );
    }

    // ETH Block

    public void getBlockNumberAsEth(String networkName,
                                    CompletionHandler<String, QueryError> handler) {
        ethBlockApi.getBlockNumberAsEth(
                networkName,
                ridGenerator.getAndIncrement(),
                handler
        );
    }

    // ETH Transfer

    public void submitTransactionAsEth(String networkName,
                                       String transaction,
                                       CompletionHandler<String, QueryError> handler) {
        ethTransferApi.submitTransactionAsEth(
                networkName,
                transaction,
                ridGenerator.getAndIncrement(),
                handler
        );
    }

    public void getTransactionsAsEth(String networkName,
                                     String address,
                                     UnsignedLong begBlockNumber,
                                     UnsignedLong endBlockNumber,
                                     CompletionHandler<List<EthTransaction>, QueryError> handler) {
        ethTransferApi.getTransactionsAsEth(
                networkName,
                address,
                begBlockNumber,
                endBlockNumber,
                ridGenerator.getAndIncrement(),
                handler
        );
    }

    public void getNonceAsEth(String networkName,
                              String address,
                              CompletionHandler<String, QueryError> handler) {
        ethTransferApi.getNonceAsEth(
                networkName,
                address,
                ridGenerator.getAndIncrement(),
                handler
        );
    }

    public void getLogsAsEth(String networkName,
                             @Nullable String contract,
                             String address,
                             String event,
                             UnsignedLong begBlockNumber,
                             UnsignedLong endBlockNumber,
                             CompletionHandler<List<EthLog>, QueryError> handler) {
        ethTransferApi.getLogsAsEth(
                networkName,
                contract,
                address,
                event,
                begBlockNumber,
                endBlockNumber,
                ridGenerator.getAndIncrement(),
                handler
        );
    }

    public void getBlocksAsEth(String networkName,
                               String address,
                               UnsignedInteger interests,
                               UnsignedLong blockStart,
                               UnsignedLong blockEnd,
                               CompletionHandler<List<UnsignedLong>, QueryError> handler) {
        ethTransferApi.getBlocksAsEth(
                networkName,
                address,
                interests,
                blockStart,
                blockEnd,
                ridGenerator.getAndIncrement(),
                handler
        );
    }
}
