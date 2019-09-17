/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.blockchaindb.DataTask;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Block;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.Currency;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transfer;
import com.breadwallet.crypto.blockchaindb.models.brd.EthLog;
import com.breadwallet.crypto.blockchaindb.models.brd.EthToken;
import com.breadwallet.crypto.blockchaindb.models.brd.EthTransaction;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import org.junit.Before;
import org.junit.Test;

import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.Semaphore;

import okhttp3.Call;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;

import static org.junit.Assert.*;

public class BlockchainDbAIT {

    private static final String BDB_BASE_URL = "https://api.blockset.com";
    private static final String BRD_AUTH_TOKEN = "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzUxMiJ9.eyJzdWIiOiJkZWI2M2UyOC0wMzQ1LTQ4ZjYtOWQxNy1jZTgwY2JkNjE3Y2IiLCJicmQ6Y3QiOiJjbGkiLCJleHAiOjkyMjMzNzIwMzY4NTQ3NzUsImlhdCI6MTU2Njg2MzY0OX0.FvLLDUSk1p7iFLJfg2kA-vwhDWTDulVjdj8YpFgnlE62OBFCYt4b3KeTND_qAhLynLKbGJ1UDpMMihsxtfvA0A";
    private static final String API_BASE_URL = "https://stage2.breadwallet.com";

    private static final String ETH_EVENT_ERC20_TRANSFER = "0xa9059cbb";

    private BlockchainDb blockchainDb;

    @Before
    public void setup() {
        DataTask decoratedSynchronousDataTask = (client, request, callback) -> {
            Request decoratedRequest = request.newBuilder()
                    .header("Authorization", "Bearer " + BRD_AUTH_TOKEN)
                    .build();
            synchronousDataTask.execute(client, decoratedRequest, callback);
        };
        blockchainDb = new BlockchainDb(new OkHttpClient(),
                BDB_BASE_URL, decoratedSynchronousDataTask,
                API_BASE_URL, synchronousDataTask);
    }

    // BDB

    @Test
    public void testGetBlocks() {
        SynchronousCompletionHandler<List<Block>> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getBlocks("bitcoin-mainnet", UnsignedLong.ZERO, UnsignedLong.valueOf(1000),
                false, true, true, true, handler);
        List<Block> blocks = handler.dat().get();
        assertNotEquals(0, blocks.size());

        // TODO: Expand these tests
    }

    @Test
    public void testGetBlock() {
        SynchronousCompletionHandler<Block> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getBlock("bitcoin-mainnet:000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f",
                false, false, false, false, handler);
        Block block = handler.dat().get();
        assertNotNull(block);
        assertEquals(block.getId(), "bitcoin-mainnet:000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");

        // TODO: Expand these tests
    }

    @Test
    public void testGetBlockchains() {
        SynchronousCompletionHandler<List<Blockchain>> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getBlockchains(true, handler);
        List<Blockchain> blockchains = handler.dat().get();
        assertNotEquals(0, blockchains.size());

        blockchainDb.getBlockchains(handler);
        blockchains = handler.dat().get();
        assertNotEquals(0, blockchains.size());

        // TODO: Expand these tests
    }

    @Test
    public void testGetBlockchain() {
        SynchronousCompletionHandler<Blockchain> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getBlockchain("bitcoin-mainnet", handler);
        Blockchain blockchain = handler.dat().get();
        assertNotNull(blockchain);
        assertEquals(blockchain.getId(), "bitcoin-mainnet");
        assertEquals(blockchain.getConfirmationsUntilFinal(), UnsignedInteger.valueOf(6));

        // TODO: Expand these tests
    }

    @Test
    public void testGetCurrencies() {
        SynchronousCompletionHandler<List<Currency>> handler = new SynchronousCompletionHandler<>();
        List<Currency> currencies;

        blockchainDb.getCurrencies("bitcoin-mainnet", handler);
        currencies = handler.dat().get();
        assertNotEquals(0, currencies.size());

        blockchainDb.getCurrencies(handler);
        currencies = handler.dat().get();
        assertNotEquals(0, currencies.size());

        // TODO: Expand these tests
    }

    @Test
    public void testGetCurrency() {
        SynchronousCompletionHandler<Currency> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getCurrency("bitcoin-mainnet:__native__", handler);
        Currency currency = handler.dat().get();
        assertNotNull(currency);
        assertEquals(currency.getCode(), "btc");

        // TODO: Expand these tests
    }

    @Test
    public void testGetTransfers() {
        SynchronousCompletionHandler<List<Transfer>> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getTransfers("bitcoin-mainnet", Arrays.asList("1JfbZRwdDHKZmuiZgYArJZhcuuzuw2HuMu"), handler);
        List<Transfer> transfers = handler.dat().get();
        assertNotNull(transfers);

        // TODO: Expand these tests
    }

    @Test
    public void testGetTransfer() {
        SynchronousCompletionHandler<Transfer> handler = new SynchronousCompletionHandler<>();

        String transferId = "bitcoin-mainnet:63522845d294ee9b0188ae5cac91bf389a0c3723f084ca1025e7d9cdfe481ce1:1";
        blockchainDb.getTransfer(transferId, handler);
        Transfer transfer = handler.dat().get();
        assertNotNull(transfer);
        assertEquals(transferId, transfer.getId());

        // TODO: Expand these tests
    }

    @Test
    public void testGetTransactions() {
        SynchronousCompletionHandler<List<Transaction>> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getTransactions("bitcoin-mainnet", Arrays.asList("1JfbZRwdDHKZmuiZgYArJZhcuuzuw2HuMu"),
                UnsignedLong.ZERO, UnsignedLong.valueOf(500000),
                true, true, handler);
        List<Transaction> transactions = handler.dat().get();
        assertNotEquals(0, transactions.size());
    }

    @Test
    public void testSubsciptions() {
        // TODO: Add testing around creating/updating/getting/deleting
    }

    @Test
    public void testGetWallet() {
        // TODO: Add testing around creating/updating/getting/deleting
    }

    // BRD

    @Test
    public void testGetBalanceAsEth() {
        SynchronousCompletionHandler<String> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getBalanceAsEth("mainnet", "0x04d542459de6765682D21771D1ba23dC30Fb675F", handler);
        String output = handler.dat().get();
        assertFalse(output.isEmpty());
    }

    @Test
    public void testGetBalanceAsTok() {
        SynchronousCompletionHandler<String> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getBalanceAsTok("mainnet", "0x04d542459de6765682D21771D1ba23dC30Fb675F",
                "0xE41d2489571d322189246Dafa5EBDE1f4699F498", handler);
        String output = handler.dat().get();
        assertFalse(output.isEmpty());
    }

    @Test
    public void testGetGasPriceAsEth() {
        SynchronousCompletionHandler<String> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getGasPriceAsEth("mainnet", handler);
        String output = handler.dat().get();
        assertFalse(output.isEmpty());
    }

    @Test
    public void testGetGasEstimateAsEth() {
        SynchronousCompletionHandler<String> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getGasEstimateAsEth("mainnet", "0x04d542459de6765682D21771D1ba23dC30Fb675F",
                "0x04d542459de6765682D21771D1ba23dC30Fb675F", "0x10000", "0x1234567890ABCDEF", handler);
        String output = handler.dat().get();
        assertFalse(output.isEmpty());
    }

    @Test
    public void testSubmitTransactionAsEth() {
        // TODO: Add test here
    }

    @Test
    public void testGetTransactionsAsEth() {
        SynchronousCompletionHandler<List<EthTransaction>> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getTransactionsAsEth("mainnet", "0x04d542459de6765682D21771D1ba23dC30Fb675F", UnsignedLong.ZERO,
                UnsignedLong.valueOf(7778000), handler);
        List<EthTransaction> output = handler.dat().get();
        assertFalse(output.isEmpty());
    }

    @Test
    public void testGetLogsAsEth() {
        SynchronousCompletionHandler<List<EthLog>> handler = new SynchronousCompletionHandler<>();

        // TODO: This test doesn't return anything; should it?
        // blockchainDb.getLogsAsEth("mainnet", null, "0x04d542459de6765682D21771D1ba23dC30Fb675F",
        //         ETH_EVENT_ERC20_TRANSFER, 0, 7778000, handler);
        // List<EthLog> output = handler.dat().get();
        // assertFalse(output.isEmpty());
    }

    @Test
    public void testGetBlocksAsEth() {
        SynchronousCompletionHandler<List<Long>> handler = new SynchronousCompletionHandler<>();

        // TODO: This test doesn't return anything; should it?
        // blockchainDb.getBlocksAsEth("mainnet", "0x04d542459de6765682D21771D1ba23dC30Fb675F", 0xF, 0, 7778000, handler);
        // List<Long> output = handler.dat().get();
        // assertFalse(output.isEmpty());
    }

    @Test
    public void testGetBlockNumberAsEth() {
        SynchronousCompletionHandler<String> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getBlockNumberAsEth("mainnet", handler);
        String output = handler.dat().get();
        assertFalse(output.isEmpty());
    }

    @Test
    public void testGetNonceAsEth() {
        SynchronousCompletionHandler<String> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getNonceAsEth("mainnet", "0x04d542459de6765682D21771D1ba23dC30Fb675F", handler);
        String output = handler.dat().get();
        assertFalse(output.isEmpty());
    }

    @Test
    public void testGetTokensAsEth() {
        SynchronousCompletionHandler<List<EthToken>> handler = new SynchronousCompletionHandler<>();

        blockchainDb.getTokensAsEth(handler);
        List<EthToken> output = handler.dat().get();
        assertFalse(output.isEmpty());
    }

    // Helpers

    private static class SynchronousCompletionHandler<T> implements CompletionHandler<T, QueryError> {

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

    private static final DataTask synchronousDataTask = (client, request, callback) -> {
        Call call = client.newCall(request);
        try (Response r = call.execute()) {
            callback.onResponse(call, r);

        } catch (IOException e) {
            callback.onFailure(call, e);
        }
    };
}
