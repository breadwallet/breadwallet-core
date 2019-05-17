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
import com.breadwallet.crypto.blockchaindb.apis.brd.EthLogApi;
import com.breadwallet.crypto.blockchaindb.apis.brd.EthNonceApi;
import com.breadwallet.crypto.blockchaindb.apis.brd.EthTransactionApi;
import com.breadwallet.crypto.blockchaindb.apis.brd.EthBlockApi;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Block;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.Currency;
import com.breadwallet.crypto.blockchaindb.models.bdb.Subscription;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transfer;
import com.breadwallet.crypto.blockchaindb.models.bdb.Wallet;
import com.breadwallet.crypto.blockchaindb.models.brd.EthLog;
import com.breadwallet.crypto.blockchaindb.models.brd.EthTransaction;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Semaphore;

import okhttp3.OkHttpClient;

public class BlockchainDb {

    // TODO: Where should this reside?
    private static final String ETH_EVENT_ERC20_TRANSFER = "0xa9059cbb";

    private final ExecutorService executorService;

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
    private final EthLogApi ethLogApi;
    private final EthNonceApi ethNonceApi;
    private final EthTransactionApi ethTransactionApi;

    public BlockchainDb(OkHttpClient client, String bdbBaseURL, @Nullable BlockchainDataTask bdbDataTask,
                        String apiBaseURL, @Nullable BlockchainDataTask apiDataTask) {
        BlockchainDataTask defaultDataTask = (cli, request, callback) -> cli.newCall(request).enqueue(callback);
        bdbDataTask = bdbDataTask == null ? defaultDataTask : bdbDataTask;


        BdbApiClient bdbClient = new BdbApiClient(client, bdbBaseURL, bdbDataTask);
        BrdApiClient brdClient = new BrdApiClient(client, apiBaseURL, apiDataTask);

        this.executorService = Executors.newCachedThreadPool();

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
        this.ethLogApi = new EthLogApi(brdClient);
        this.ethNonceApi = new EthNonceApi(brdClient);
        this.ethTransactionApi = new EthTransactionApi(brdClient);
    }

    // Blockchain

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

    // ETH Transaction

    public void submitTransactionAsEth(String networkName, String transaction, int rid,
                                       BlockchainCompletionHandler<String> handler) {
        ethTransactionApi.submitTransactionAsEth(networkName, transaction, rid, handler);
    }

    public void getTransactionsAsEth(String networkName, String address, long begBlockNumber, long endBlockNumber,
                                     int rid, BlockchainCompletionHandler<List<EthTransaction>> handler) {
        ethTransactionApi.getTransactionsAsEth(networkName, address, begBlockNumber, endBlockNumber, rid, handler);
    }

    // ETH Log

    public void getLogsAsEth(String networkName, @Nullable String contract, String address, String event,
                             long begBlockNumber, long endBlockNumber, int rid, BlockchainCompletionHandler<List<EthLog>> handler) {
        ethLogApi.getLogsAsEth(networkName, contract, address, event, begBlockNumber, endBlockNumber, rid, handler);
    }

    // ETH Block

    public void getBlocksAsEth(String networkName, String address, int interests, long blockStart, long blockEnd,
                               int rid, BlockchainCompletionHandler<List<Long>> handler) {
        executorService.submit(() -> getBlocksAsEthOnExecutor(networkName, address, interests, blockStart, blockEnd,
                rid, handler));
    }

    public void getBlockNumberAsEth(String networkName, int rid, BlockchainCompletionHandler<String> handler) {
        ethBlockApi.getBlockNumberAsEth(networkName, rid, handler);
    }

    private void getBlocksAsEthOnExecutor(String networkName, String address, int interests, long blockStart,
                                          long blockEnd, int rid, BlockchainCompletionHandler<List<Long>> handler) {
        // TODO: Move this out to an API (either combine transaction/logs or pass in the APIs to the block api)
        final QueryError[] error = {null};

        List<EthTransaction> transactions = new ArrayList<>();
        Semaphore transactionsSema = new Semaphore(0);
        getTransactionsAsEth(networkName, address, blockStart, blockEnd, rid, new BlockchainCompletionHandler<List<EthTransaction>>() {
            @Override
            public void handleData(List<EthTransaction> data) {
                transactions.addAll(data);
                transactionsSema.release();
            }

            @Override
            public void handleError(QueryError e) {
                error[0] = e;
                transactionsSema.release();
            }
        });

        List<EthLog> logs = new ArrayList<>();
        Semaphore logsSema = new Semaphore(0);
        getLogsAsEth(networkName, null, address, ETH_EVENT_ERC20_TRANSFER, blockStart, blockEnd, rid, new BlockchainCompletionHandler<List<EthLog>>() {
            @Override
            public void handleData(List<EthLog> data) {
                logs.addAll(data);
                logsSema.release();
            }

            @Override
            public void handleError(QueryError e) {
                error[0] = e;
                logsSema.release();
            }
        });

        transactionsSema.acquireUninterruptibly();
        logsSema.acquireUninterruptibly();

        if (error[0] != null) {
            handler.handleError(error[0]);
        } else {
            List<Long> numbers = new ArrayList<>();
            for (EthTransaction transaction: transactions) {
                boolean include = (0 != (interests & (1 << 0)) && address.equals(transaction.getSourceAddr())) ||
                        (0 != (interests & (1 << 1)) && address.equals(transaction.getTargetAddr()));
                if (include) {
                    try {
                        numbers.add(Long.decode(transaction.getBlockNumber()));
                    } catch (NumberFormatException e) {
                        // TODO: How do we want to handle this?
                    }
                }
            }

            for (EthLog log: logs) {
                List<String> topics = log.getTopics();
                boolean include = topics.size() == 3 &&
                        ((0 != (interests & (1 << 2)) && address.equals(topics.get(1))) ||
                                (0 != (interests & (1 << 3)) && address.equals(topics.get(2))));
                if (include) {
                    try {
                        numbers.add(Long.decode(log.getBlockNumber()));
                    } catch (NumberFormatException e) {
                        // TODO: How do we want to handle this?
                    }
                }
            }

            handler.handleData(numbers);
        }
    }

    // ETH Nonce

    public void getNonceAsEth(String networkName, String address, int rid, BlockchainCompletionHandler<String> handler) {
        ethNonceApi.getNonceAsEth(networkName, address, rid, handler);
    }

    // TODO: Add getTokensAsEth
}
