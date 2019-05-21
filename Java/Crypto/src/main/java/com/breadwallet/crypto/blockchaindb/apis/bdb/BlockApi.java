package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Block;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Multimap;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Semaphore;

import static com.google.common.base.Preconditions.checkArgument;

public class BlockApi {

    private static final int PAGINATION_COUNT = 5000;

    private final BdbApiClient jsonClient;
    private final ExecutorService executorService;

    public BlockApi(BdbApiClient jsonClient, ExecutorService executorService) {
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

        jsonClient.sendGetRequest(path, params, Block::asBlock, handler);
    }

    private void getBlocksOnExecutor(String id, long beginBlockNumber, long endBlockNumber, boolean includeRaw,
                                     boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                                     BlockchainCompletionHandler<List<Block>> handler) {
        final QueryError[] error = {null};
        List<Block> allBlocks = new ArrayList<>();
        Semaphore sema = new Semaphore(0);

        ImmutableListMultimap.Builder<String, String> baseBuilder = ImmutableListMultimap.builder();
        baseBuilder.put("blockchain_id", id);
        baseBuilder.put("include_raw", String.valueOf(includeRaw));
        baseBuilder.put("include_tx", String.valueOf(includeTx));
        baseBuilder.put("include_tx_raw", String.valueOf(includeTxRaw));
        baseBuilder.put("include_tx_proof", String.valueOf(includeTxProof));
        ImmutableMultimap<String, String> baseParams = baseBuilder.build();

        for (long i = beginBlockNumber; i < endBlockNumber && error[0] == null; i += PAGINATION_COUNT) {
            ImmutableListMultimap.Builder<String, String> paramsBuilder = ImmutableListMultimap.builder();
            paramsBuilder.putAll(baseParams);
            paramsBuilder.put("start_height", String.valueOf(i));
            paramsBuilder.put("end_height", String.valueOf(Math.min(i + PAGINATION_COUNT,
                    endBlockNumber)));
            ImmutableMultimap<String, String> params = paramsBuilder.build();

            jsonClient.sendGetRequest("transactions", params, Block::asBlocks, new BlockchainCompletionHandler<List<Block>>() {
                @Override
                public void handleData(List<Block> blocks) {
                    allBlocks.addAll(blocks);
                    sema.release();
                }

                @Override
                public void handleError(QueryError e) {
                    error[0] = e;
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
