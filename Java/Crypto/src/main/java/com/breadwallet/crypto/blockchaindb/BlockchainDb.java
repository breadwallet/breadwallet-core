package com.breadwallet.crypto.blockchaindb;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryJsonParseError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.errors.QueryNoDataError;
import com.breadwallet.crypto.blockchaindb.errors.QuerySubmissionError;
import com.breadwallet.crypto.blockchaindb.errors.QueryUrlError;
import com.breadwallet.crypto.blockchaindb.models.Block;
import com.breadwallet.crypto.blockchaindb.models.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.Currency;
import com.breadwallet.crypto.blockchaindb.models.Subscription;
import com.breadwallet.crypto.blockchaindb.models.Transaction;
import com.breadwallet.crypto.blockchaindb.models.Transfer;
import com.breadwallet.crypto.blockchaindb.models.Wallet;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Multimap;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Semaphore;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.HttpUrl;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.ResponseBody;

import static com.google.common.base.Preconditions.checkArgument;

public class BlockchainDb {

    public static final MediaType MEDIA_TYPE_JSON = MediaType.parse("application/json; charset=utf-8");

    private static final int PAGINATION_COUNT = 5000;

    private final OkHttpClient client;

    private final BlockchainDataTask bdbDataTask;
    private final BlockchainDataTask apiDataTask;

    private final String bdbBaseURL;
    private final String apiBaseURL;

    private final ExecutorService executorService = Executors.newCachedThreadPool();
    private final BlockchainDataTask defaultDataTask =
            (client, request, callback) -> client.newCall(request).enqueue(callback);

    public BlockchainDb(OkHttpClient client, String bdbBaseURL, @Nullable BlockchainDataTask bdbDataTask,
                        String apiBaseURL, @Nullable BlockchainDataTask apiDataTask) {
        this.client = client;
        this.bdbBaseURL = bdbBaseURL;
        this.bdbDataTask = bdbDataTask == null ? defaultDataTask : bdbDataTask;
        this.apiBaseURL = apiBaseURL;
        this.apiDataTask = apiDataTask == null ? defaultDataTask : apiDataTask;
    }

    // Blockchain

    public void getBlockchains(BlockchainCompletionHandler<List<Blockchain>> handler) {
        getBlockchains(handler, false);
    }

    public void getBlockchains(BlockchainCompletionHandler<List<Blockchain>> handler, boolean ismainnet) {
        Multimap<String, String> params = ImmutableListMultimap.of("testnet", Boolean.valueOf(!ismainnet).toString());
        bdbMakeRequest("blockchains", params, new BdbRequestCompletionArrayHandler() {
            @Override
            public void handleData(JSONArray json, boolean more) {
                checkArgument(!more);
                Optional<List<Blockchain>> blockchains = Blockchain.asBlockchains(json);
                if (blockchains.isPresent()) {
                    handler.handleData(blockchains.get());
                } else {
                    handler.handleError(new QueryModelError("Transform error"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    public void getBlockchain(String id, BlockchainCompletionHandler<Blockchain> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("blockchains/%s", id);
        bdbMakeRequest(path, ImmutableListMultimap.of(), new BdbRequestCompletionObjectHandler() {
            @Override
            public void handleData(JSONObject json, boolean more) {
                checkArgument(!more);
                Optional<Blockchain> blockchain = Blockchain.asBlockchain(json);
                if (blockchain.isPresent()) {
                    handler.handleData(blockchain.get());
                } else {
                    handler.handleError(new QueryModelError("Transform error"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    // Currency

    public void getCurrencies(BlockchainCompletionHandler<List<Currency>> handler) {
        getCurrencies(null, handler);
    }

    public void getCurrencies(@Nullable String id, BlockchainCompletionHandler<List<Currency>> handler) {
        Multimap<String, String> params = id == null ? ImmutableMultimap.of() : ImmutableListMultimap.of(
                "blockchain_id", id);
        bdbMakeRequest("currencies", params, new BdbRequestCompletionArrayHandler() {
            @Override
            public void handleData(JSONArray json, boolean more) {
                checkArgument(!more);
                Optional<List<Currency>> currencies = Currency.asCurrencies(json);
                if (currencies.isPresent()) {
                    handler.handleData(currencies.get());
                } else {
                    handler.handleError(new QueryModelError("Transform error"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    public void getCurrency(String id, BlockchainCompletionHandler<Currency> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("currencies/%s", id);
        bdbMakeRequest(path, ImmutableListMultimap.of(), new BdbRequestCompletionObjectHandler() {
            @Override
            public void handleData(JSONObject json, boolean more) {
                checkArgument(!more);
                Optional<Currency> currency = Currency.asCurrency(json);
                if (currency.isPresent()) {
                    handler.handleData(currency.get());
                } else {
                    handler.handleError(new QueryModelError("Transform error"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    // Subscription

    public void getOrCreateSubscription(Subscription subscription, BlockchainCompletionHandler<Subscription> handler) {
        getSubscription(subscription.getId(), new BlockchainCompletionHandler<Subscription>() {
            @Override
            public void handleData(Subscription data) {
                handler.handleData(data);
            }

            @Override
            public void handleError(QueryError error) {
                createSubscription(subscription, handler);
            }
        });
    }

    public void getSubscription(String id, BlockchainCompletionHandler<Subscription> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("subscriptions/%s", id);
        makeSubscriptionRequest(path, "GET", handler);
    }

    public void createSubscription(Subscription subscription, BlockchainCompletionHandler<Subscription> handler) {
        makeSubscriptionRequest(subscription, "subscriptions","POST", handler);
    }

    public void updateSubscription(Subscription subscription, BlockchainCompletionHandler<Subscription> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("subscriptions/%s", subscription.getId());
        makeSubscriptionRequest(subscription, path, "POST", handler);
    }

    public void deleteSubscription(String id, BlockchainCompletionHandler<Subscription> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("subscriptions/%s", id);
        makeSubscriptionRequest(path, "DELETE", handler);
    }

    private void makeSubscriptionRequest(Subscription subscription, String path, String httpMethod, BlockchainCompletionHandler<Subscription> handler) {
        makeRequest(bdbDataTask, bdbBaseURL, path, ImmutableMultimap.of(), Subscription.asJson(subscription), httpMethod, new BlockchainCompletionHandler<JSONObject>() {
            @Override
            public void handleData(JSONObject data) {
                Optional<Subscription> optionalSubscription = Subscription.asSubscription(data);
                if (optionalSubscription.isPresent()) {
                    handler.handleData(optionalSubscription.get());
                } else {
                    handler.handleError(new QueryModelError("Missed subscription"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    private void makeSubscriptionRequest(String path, String httpMethod, BlockchainCompletionHandler<Subscription> handler) {
        makeRequest(bdbDataTask, bdbBaseURL, path, ImmutableMultimap.of(), null, httpMethod, new BlockchainCompletionHandler<JSONObject>() {
            @Override
            public void handleData(JSONObject data) {
                Optional<Subscription> optionalSubscription = Subscription.asSubscription(data);
                if (optionalSubscription.isPresent()) {
                    handler.handleData(optionalSubscription.get());
                } else {
                    handler.handleError(new QueryModelError("Missed subscription"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    // TODO: Add createSubscription call

    // Transfer

    public void getTransfers(String id, List<String> addresses, BlockchainCompletionHandler<List<Transfer>> handler) {
        ImmutableListMultimap.Builder<String, String> paramBuilders = ImmutableListMultimap.builder();
        paramBuilders.put("blockchain_id", id);
        for (String address : addresses) paramBuilders.put("address", address);
        Multimap<String, String> params = paramBuilders.build();

        bdbMakeRequest("transfers", params, new BdbRequestCompletionArrayHandler() {
            @Override
            public void handleData(JSONArray json, boolean more) {
                checkArgument(!more);  // TODO: Should this be here? Its not in the swift version
                Optional<List<Transfer>> transfers = Transfer.asTransfers(json);
                if (transfers.isPresent()) {
                    handler.handleData(transfers.get());
                } else {
                    handler.handleError(new QueryModelError("Transform error"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    public void getTransfer(String id, BlockchainCompletionHandler<Transfer> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("transfers/%s", id);
        bdbMakeRequest(path, ImmutableListMultimap.of(), new BdbRequestCompletionObjectHandler() {
            @Override
            public void handleData(JSONObject json, boolean more) {
                checkArgument(!more);
                Optional<Transfer> transfer = Transfer.asTransfer(json);
                if (transfer.isPresent()) {
                    handler.handleData(transfer.get());
                } else {
                    handler.handleError(new QueryModelError("Transform error"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    // Wallet

    public void getOrCreateWallet(Wallet wallet, BlockchainCompletionHandler<Wallet> handler) {
        getWallet(wallet.getId(), new BlockchainCompletionHandler<Wallet>() {
            @Override
            public void handleData(Wallet data) {
                handler.handleData(data);
            }

            @Override
            public void handleError(QueryError error) {
                createWallet(wallet, handler);
            }
        });
    }

    public void getWallet(String id, BlockchainCompletionHandler<Wallet> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("wallets/%s", id);
        makeWalletRequest(path, "GET", handler);
    }

    public void createWallet(Wallet wallet, BlockchainCompletionHandler<Wallet> handler) {
        makeWalletRequest(wallet, "wallets", "POST", handler);
    }

    public void updateWallet(Wallet wallet, BlockchainCompletionHandler<Wallet> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("wallets/%s", wallet.getId());
        makeWalletRequest(wallet, path, "PUT", handler);
    }

    public void deleteWallet(String id, BlockchainCompletionHandler<Wallet> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("wallets/%s", id);
        makeWalletRequest(path, "DELETE", handler);
    }

    private void makeWalletRequest(Wallet wallet, String path, String httpMethod, BlockchainCompletionHandler<Wallet> handler) {
        makeRequest(bdbDataTask, bdbBaseURL, path, ImmutableMultimap.of(), Wallet.asJson(wallet), httpMethod, new BlockchainCompletionHandler<JSONObject>() {
            @Override
            public void handleData(JSONObject data) {
                Optional<Wallet> optionalWallet = Wallet.asWallet(data);
                if (optionalWallet.isPresent()) {
                    handler.handleData(optionalWallet.get());
                } else {
                    handler.handleError(new QueryModelError("Missed wallet"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    private void makeWalletRequest(String path, String httpMethod, BlockchainCompletionHandler<Wallet> handler) {
        makeRequest(bdbDataTask, bdbBaseURL, path, ImmutableMultimap.of(), null, httpMethod, new BlockchainCompletionHandler<JSONObject>() {
            @Override
            public void handleData(JSONObject data) {
                Optional<Wallet> optionalWallet = Wallet.asWallet(data);
                if (optionalWallet.isPresent()) {
                    handler.handleData(optionalWallet.get());
                } else {
                    handler.handleError(new QueryModelError("Missed wallet"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    // Transactions

    public void getTransactions(String id, List<String> addresses, long beginBlockNumber, long endBlockNumber,
                                boolean includeRaw, boolean includeProof,
                                BlockchainCompletionHandler<List<Transaction>> handler) {
        executorService.submit(() -> getTransactionsOnExecutor(id, addresses, beginBlockNumber, endBlockNumber,
                includeRaw, includeProof, handler));
    }

    private void getTransactionsOnExecutor(String id, List<String> addresses, long beginBlockNumber, long endBlockNumber,
                                           boolean includeRaw, boolean includeProof,
                                           BlockchainCompletionHandler<List<Transaction>> handler) {
        final QueryError[] error = {null};
        List<Transaction> allTransactions = new ArrayList<>();
        Semaphore sema = new Semaphore(0);

        ImmutableListMultimap.Builder<String, String> paramBuilders = ImmutableListMultimap.builder();
        paramBuilders.put("blockchain_id", id);
        paramBuilders.put("include_proof", String.valueOf(includeProof));
        paramBuilders.put("include_raw", String.valueOf(includeRaw));
        for (String address : addresses) paramBuilders.put("address", address);

        for (long i = beginBlockNumber; i < endBlockNumber && error[0] == null; i += PAGINATION_COUNT) {
            paramBuilders.put("start_height", String.valueOf(beginBlockNumber));
            paramBuilders.put("end_height", String.valueOf(Math.min(beginBlockNumber + PAGINATION_COUNT,
                    endBlockNumber)));

            bdbMakeRequest("transactions", paramBuilders.build(), new BdbRequestCompletionArrayHandler() {
                @Override
                public void handleData(JSONArray json, boolean more) {
                    Optional<List<Transaction>> transactions = Transaction.asTransactions(json);
                    if (transactions.isPresent()) {
                        allTransactions.addAll(transactions.get());
                    } else {
                        error[0] = new QueryModelError("Transform error");
                    }

                    sema.release();
                }

                @Override
                public void handleError(QueryError error) {
                    sema.release();
                }
            });

            sema.acquireUninterruptibly();
        }

        if (error[0] != null) {
            handler.handleError(error[0]);
        } else {
            handler.handleData(allTransactions);
        }
    }

    public void getTransaction(String id, boolean includeRaw, boolean includeProof,
                               BlockchainCompletionHandler<Transaction> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("transactions/%s", id);
        Multimap<String, String> params = ImmutableListMultimap.of("include_proof", String.valueOf(includeProof),
                "include_raw", String.valueOf(includeRaw));

        bdbMakeRequest(path, params, new BdbRequestCompletionObjectHandler() {
            @Override
            public void handleData(JSONObject json, boolean more) {
                checkArgument(!more);
                Optional<Transaction> transaction = Transaction.asTransaction(json);
                if (transaction.isPresent()) {
                    handler.handleData(transaction.get());
                } else {
                    handler.handleError(new QueryModelError("Transform error"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    // TOOD: Add putTransaction()

    // Blocks

    public void getBlocks(String id, long beginBlockNumber, long endBlockNumber, boolean includeRaw,
                          boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                          BlockchainCompletionHandler<List<Block>> handler) {
        executorService.submit(() -> getBlocksOnExecutor(id, beginBlockNumber, endBlockNumber, includeRaw, includeTx, includeTxRaw, includeTxProof, handler));
    }

    public void getBlocksOnExecutor(String id, long beginBlockNumber, long endBlockNumber, boolean includeRaw,
                                    boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                                    BlockchainCompletionHandler<List<Block>> handler) {
        final boolean[] moreResults = {false};
        final QueryError[] error = {null};
        List<Block> allBlocks = new ArrayList<>();
        Semaphore sema = new Semaphore(0);

        ImmutableListMultimap.Builder<String, String> paramBuilders = ImmutableListMultimap.builder();
        paramBuilders.put("blockchain_id", id);
        paramBuilders.put("include_raw", String.valueOf(includeRaw));
        paramBuilders.put("include_tx", String.valueOf(includeTx));
        paramBuilders.put("include_tx_raw", String.valueOf(includeTxRaw));
        paramBuilders.put("include_tx_proof", String.valueOf(includeTxProof));

        for (long i = beginBlockNumber; i < endBlockNumber && error[0] == null; i += PAGINATION_COUNT) {
            paramBuilders.put("start_height", String.valueOf(beginBlockNumber));
            paramBuilders.put("end_height", String.valueOf(Math.min(beginBlockNumber + PAGINATION_COUNT,
                    endBlockNumber)));

            bdbMakeRequest("transactions", paramBuilders.build(), new BdbRequestCompletionArrayHandler() {
                @Override
                public void handleData(JSONArray json, boolean more) {
                    Optional<List<Block>> blocks = Block.asBlocks(json);
                    if (blocks.isPresent()) {
                        allBlocks.addAll(blocks.get());
                    } else {
                        error[0] = new QueryModelError("Transform error");
                    }

                    sema.release();
                }

                @Override
                public void handleError(QueryError error) {
                    sema.release();
                }
            });

            sema.acquireUninterruptibly();
        }

        if (error[0] != null) {
            handler.handleError(error[0]);
        } else {
            handler.handleData(allBlocks);
        }
    }

    public void getBlock(String id, boolean includeRaw,
                         boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                         BlockchainCompletionHandler<Block> handler) {
        String path = String.format("blocks/%s", id);
        Multimap<String, String> params = ImmutableListMultimap.of(
                "include_raw", String.valueOf(includeRaw),
                "include_tx", String.valueOf(includeTx),
                "include_tx_raw", String.valueOf(includeTxRaw),
                "include_tx_proof", String.valueOf(includeTxProof));

        bdbMakeRequest(path, params, new BdbRequestCompletionObjectHandler() {
            @Override
            public void handleData(JSONObject json, boolean more) {
                checkArgument(!more);
                Optional<Block> block = Block.asBlock(json);
                if (block.isPresent()) {
                    handler.handleData(block.get());
                } else {
                    handler.handleError(new QueryModelError("Transform error"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    // Internal

    private void bdbMakeRequest(String path, Multimap<String, String> params,
                                BdbRequestCompletionObjectHandler handler) {
        makeRequest(bdbDataTask, bdbBaseURL, path, params, null, "GET", new BlockchainCompletionHandler<JSONObject>() {
            @Override
            public void handleData(JSONObject json) {
                JSONObject jsonPage = json.optJSONObject("page");
                boolean full = (jsonPage == null ? 0 : jsonPage.optInt("total_pages", 0)) > 1;

                // TODO: why is this always set to false in the swift
                boolean more = false && full;
                handler.handleData(json, more);
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    private void bdbMakeRequest(String path, Multimap<String, String> params,
                                BdbRequestCompletionArrayHandler handler) {
        makeRequest(bdbDataTask, bdbBaseURL, path, params, null, "GET", new BlockchainCompletionHandler<JSONObject>() {
            @Override
            public void handleData(JSONObject json) {
                JSONObject jsonEmbedded = json.optJSONObject("_embedded");
                JSONArray jsonEmbeddedData = jsonEmbedded == null ? null : jsonEmbedded.optJSONArray(path);

                JSONObject jsonPage = json.optJSONObject("page");
                boolean full = (jsonPage == null ? 0 : jsonPage.optInt("total_pages", 0)) > 1;

                // TODO: why is this always set to false in the swift
                boolean more = false && full;

                if (jsonEmbedded == null || jsonEmbeddedData == null) {
                    handler.handleError(new QueryModelError("Data expected"));

                } else {
                    handler.handleData(jsonEmbeddedData, more);
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    private <T> void makeRequest(BlockchainDataTask dataTask, String baseUrl, String path,
                                 Multimap<String, String> params, @Nullable JSONObject json, String httpMethod,
                                 BlockchainCompletionHandler<T> handler) {
        HttpUrl.Builder urlBuilder = HttpUrl.parse(baseUrl).newBuilder();
        urlBuilder.addPathSegments(path);
        for (Map.Entry<String, String> entry : params.entries()) {
            String key = entry.getKey();
            String value = entry.getValue();
            urlBuilder.addQueryParameter(key, value);
        }

        Request.Builder requestBuilder = new Request.Builder();
        requestBuilder.url(urlBuilder.build());
        requestBuilder.addHeader("accept", "application/json");
        requestBuilder.method(httpMethod, json == null ? null : RequestBody.create(MEDIA_TYPE_JSON, json.toString()));

        sendRequest(requestBuilder.build(), dataTask, handler);
    }

    private <T> void sendRequest(Request request, BlockchainDataTask dataTask, BlockchainCompletionHandler handler) {
        dataTask.execute(client, request, new Callback() {
            @Override
            public void onResponse(Call call, Response response) throws IOException {
                int responseCode = response.code();
                if (responseCode == 200) {
                    try (ResponseBody responseBody = response.body()) {
                        if (responseBody == null) {
                            handler.handleError(new QueryNoDataError());
                        } else {
                            try {
                                handler.handleData(new JSONObject(responseBody.string()));
                            } catch (JSONException e) {
                                handler.handleError(new QueryJsonParseError(e.getMessage()));
                            }
                        }
                    }
                } else {
                    handler.handleError(new QueryUrlError("Status: " + responseCode));
                }
            }

            @Override
            public void onFailure(Call call, IOException e) {
                // TODO: Do we want to propagate this?
                handler.handleError(new QuerySubmissionError(e.getMessage()));
            }
        });
    }

    private interface BdbRequestCompletionArrayHandler {
        void handleData(JSONArray json, boolean more);

        void handleError(QueryError error);
    }

    private interface BdbRequestCompletionObjectHandler {
        void handleData(JSONObject json, boolean more);

        void handleError(QueryError error);
    }
}
