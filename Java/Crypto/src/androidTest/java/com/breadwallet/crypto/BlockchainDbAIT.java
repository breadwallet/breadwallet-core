package com.breadwallet.crypto;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.BlockchainDataTask;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.Currency;
import com.breadwallet.crypto.blockchaindb.models.Subscription;
import com.breadwallet.crypto.blockchaindb.models.Transaction;
import com.breadwallet.crypto.blockchaindb.models.Transfer;
import com.breadwallet.crypto.blockchaindb.models.Wallet;

import org.junit.Before;
import org.junit.Test;

import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.Semaphore;

import okhttp3.Call;
import okhttp3.OkHttpClient;
import okhttp3.Response;

import static org.junit.Assert.*;

public class BlockchainDbAIT {

    private static final String BDB_BASE_URL = "https://test-blockchaindb-api.brd.tools";
    private static final String API_BASE_URL = "https://stage2.breadwallet.com";

    private static class SynchronousCompletionHandler<T> implements BlockchainCompletionHandler<T>{

        private final Semaphore sema = new Semaphore(0);

        private T data;
        private QueryError error;

        public Optional<T> dat() {
            sema.acquireUninterruptibly();
            return Optional.ofNullable(data);
        }

        public Optional<QueryError> err() {
            sema.acquireUninterruptibly();
            return Optional.ofNullable(error);
        }

        @Override
        public void handleData(T data) {
            this.data = data;
            this.error = null;
            sema.release();
        }

        @Override
        public void handleError(QueryError error) {
            this.data = null;
            this.error = error;
            sema.release();
        }
    }

    private static final BlockchainDataTask synchronousDataTask = (client, request, callback) -> {
        Call call = client.newCall(request);
        try (Response r = call.execute()){
            callback.onResponse(call, r);

        } catch (IOException e) {
            callback.onFailure(call, e);
        }
    };

    private BlockchainDb blockchainDb;

    @Before
    public void setup() {
        blockchainDb = new BlockchainDb(new OkHttpClient(),
                BDB_BASE_URL, synchronousDataTask, API_BASE_URL, synchronousDataTask);
    }

    @Test
    public void testGetBlockchains() {
        SynchronousCompletionHandler<List<Blockchain>> handler = new SynchronousCompletionHandler();
        blockchainDb.getBlockchains(handler);

        List<Blockchain> blockchains = handler.dat().get();
        assertEquals(1, blockchains.size());

        // TODO: Expand these tests
    }

    @Test
    public void testGetBlockchain() {
        SynchronousCompletionHandler<Blockchain> handler = new SynchronousCompletionHandler();
        blockchainDb.getBlockchain("bitcoin-mainnet", handler);

        Blockchain blockchain = handler.dat().get();
        assertNotNull(blockchain);

        // TODO: Expand these tests
    }

    @Test
    public void testGetCurrencies() {
        SynchronousCompletionHandler<List<Currency>> handler = new SynchronousCompletionHandler();
        blockchainDb.getCurrencies(handler);

        List<Currency> currencies = handler.dat().get();
        assertEquals(1, currencies.size());

        // TODO: Expand these tests
    }

    @Test
    public void testGetCurrency() {
        SynchronousCompletionHandler<Currency> handler = new SynchronousCompletionHandler();
        blockchainDb.getCurrency("bitcoin-mainnet", handler);

        Currency currency = handler.dat().get();
        assertNotNull(currency);

        // TODO: Expand these tests
    }

    @Test
    public void testGetSubsciptions() {
        SynchronousCompletionHandler<Subscription> handler = new SynchronousCompletionHandler();
        blockchainDb.getSubscription("1234-1234-4324-4211", handler);

        Subscription subscription = handler.dat().get();
        assertNotNull(subscription);
    }

    @Test
    public void testGetTransfers() {
        SynchronousCompletionHandler<List<Transfer>> handler = new SynchronousCompletionHandler();
        blockchainDb.getTransfers("bitcoin-mainnet", Arrays.asList("1JfbZRwdDHKZmuiZgYArJZhcuuzuw2HuMu"), handler);

        // TODO: Revisit this after getting clarification on the endpoint
//        List<Transfer> transfers = handler.dat().get();
//        assertNotNull(transfers);
    }

    @Test
    public void testGetTransfer() {
        SynchronousCompletionHandler<Transfer> handler = new SynchronousCompletionHandler<>();
        blockchainDb.getTransfer("bitcoin-mainnet:93b28d9c4a6c2656caeeae6602cc1c51162be7097877a5bcf25538de96371fe8:3", handler);

        Transfer transfer = handler.dat().get();
        assertNotNull(transfer);
    }

    @Test
    public void testGetWallet() {
        SynchronousCompletionHandler<Wallet> handler = new SynchronousCompletionHandler<>();
        blockchainDb.getWallet("my-wallet-id", handler);

        Wallet wallet = handler.dat().get();
        assertNotNull(wallet);
    }

    @Test
    public void testGetTransactions() {
        SynchronousCompletionHandler<List<Transaction>> handler = new SynchronousCompletionHandler<>();
        blockchainDb.getTransactions("bitcoin-mainnet", Arrays.asList("1JfbZRwdDHKZmuiZgYArJZhcuuzuw2HuMu"), 0, 320000, true, true, handler);

        List<Transaction> transactions = handler.dat().get();
        assertNotEquals(0, transactions.size());
    }
}
