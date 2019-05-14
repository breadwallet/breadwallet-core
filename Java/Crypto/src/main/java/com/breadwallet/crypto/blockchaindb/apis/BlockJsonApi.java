package com.breadwallet.crypto.blockchaindb.apis;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.Block;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.Multimap;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Semaphore;

import static com.google.common.base.Preconditions.checkArgument;

public class BlockJsonApi {

    private static final int PAGINATION_COUNT = 5000;

    private final JsonApiClient jsonClient;
    private final ExecutorService executorService;

    public BlockJsonApi(JsonApiClient jsonClient, ExecutorService executorService) {
        this.jsonClient = jsonClient;
        this.executorService = executorService;
    }

    public void getBlocks(String id, long beginBlockNumber, long endBlockNumber, boolean includeRaw,
                          boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                          BlockchainCompletionHandler<List<Block>> handler) {
        executorService.submit(() -> getBlocksOnExecutor(id, beginBlockNumber, endBlockNumber, includeRaw, includeTx, includeTxRaw, includeTxProof, handler));
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

        jsonClient.makeRequest(path, params, new JsonApiCompletionObjectHandler() {
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

    private void getBlocksOnExecutor(String id, long beginBlockNumber, long endBlockNumber, boolean includeRaw,
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

            jsonClient.makeRequest("transactions", paramBuilders.build(), new JsonApiCompletionArrayHandler() {
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
}
