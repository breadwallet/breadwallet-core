/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.CompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Block;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Multimap;
import com.google.common.primitives.UnsignedLong;
import com.google.common.primitives.UnsignedLongs;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Semaphore;

import static com.google.common.base.Preconditions.checkArgument;

public class BlockApi {

    private static final UnsignedLong PAGINATION_COUNT = UnsignedLong.valueOf(5000);

    private final BdbApiClient jsonClient;
    private final ExecutorService executorService;

    public BlockApi(BdbApiClient jsonClient, ExecutorService executorService) {
        this.jsonClient = jsonClient;
        this.executorService = executorService;
    }

    public void getBlocks(String id, UnsignedLong beginBlockNumber, UnsignedLong endBlockNumber, boolean includeRaw,
                          boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                          CompletionHandler<List<Block>> handler) {
        executorService.submit(() -> getBlocksOnExecutor(id, beginBlockNumber, endBlockNumber, includeRaw, includeTx,
                includeTxRaw, includeTxProof, handler));
    }

    public void getBlock(String id, boolean includeRaw,
                         boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                         CompletionHandler<Block> handler) {
        Multimap<String, String> params = ImmutableListMultimap.of(
                "include_raw", String.valueOf(includeRaw),
                "include_tx", String.valueOf(includeTx),
                "include_tx_raw", String.valueOf(includeTxRaw),
                "include_tx_proof", String.valueOf(includeTxProof));

        jsonClient.sendGetWithId("blocks", id, params, Block::asBlock, handler);
    }

    private void getBlocksOnExecutor(String id, UnsignedLong beginBlockNumber, UnsignedLong endBlockNumber, boolean includeRaw,
                                     boolean includeTx, boolean includeTxRaw, boolean includeTxProof,
                                     CompletionHandler<List<Block>> handler) {
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

        for (UnsignedLong i = beginBlockNumber; i.compareTo(endBlockNumber) < 0 && error[0] == null; i = i.plus(PAGINATION_COUNT)) {
            ImmutableListMultimap.Builder<String, String> paramsBuilder = ImmutableListMultimap.builder();
            paramsBuilder.putAll(baseParams);
            paramsBuilder.put("start_height", i.toString());
            paramsBuilder.put("end_height", UnsignedLongs.toString(UnsignedLongs.min(i.plus(PAGINATION_COUNT).longValue(), endBlockNumber.longValue())));
            ImmutableMultimap<String, String> params = paramsBuilder.build();

            jsonClient.sendGetForArray("blocks", params, Block::asBlocks,
                    new CompletionHandler<List<Block>>() {
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
