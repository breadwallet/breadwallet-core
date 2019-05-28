package com.breadwallet.crypto;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.Currency;
import com.breadwallet.crypto.blockchaindb.models.bdb.CurrencyDenomination;
import com.google.common.base.Function;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Phaser;
import java.util.concurrent.atomic.AtomicInteger;

final class NetworkDiscovery {

    interface Callback {
        void discovered(List<Network> networks);
    }

    static void discoverNetworks(BlockchainDb query, List<String> networksNeeded, Callback callback) {
        // TODO: This semaphore stuff is nonsense; clean it up
        List<Network> networks = new ArrayList<>();
        CountUpAndDownLatch latch = CountUpAndDownLatch.wrap(() -> {
            callback.discovered(networks);
        });

        getBlockChains(query, Blockchain.DEFAULT_BLOCKCHAINS, blockchainModels -> {
            for (Blockchain blockchainModel : blockchainModels) {
                String blockchainModelId = blockchainModel.getId();
                if (!networksNeeded.contains(blockchainModelId)) {
                    continue;
                }

                final List<Currency> defaultCurrencies = new ArrayList<>();
                for (Currency currency :
                        Currency.DEFAULT_CURRENCIES) {
                    if (currency.getBlockchainId().equals(blockchainModelId)) {
                        defaultCurrencies.add(currency);
                    }
                }

                Map<com.breadwallet.crypto.Currency, NetworkAssociation> associations = new HashMap<>();

                getCurrencies(query, blockchainModelId, defaultCurrencies, currencyModels -> {
                    for (Currency currencyModel : currencyModels) {
                        if (!blockchainModelId.equals(currencyModel.getBlockchainId())) {
                            continue;
                        }

                        com.breadwallet.crypto.Currency currency = new com.breadwallet.crypto.Currency(
                                currencyModel.getId(),
                                currencyModel.getName(),
                                currencyModel.getCode(),
                                currencyModel.getType());

                        Unit baseUnit = currencyDenominationToBaseUnit(currency, currencyModel.getDenominations());
                        List<Unit> units = currencyDenominationToUnits(currency, currencyModel.getDenominations(),
                                baseUnit);

                        units.add(0, baseUnit);
                        Collections.sort(units, (o1, o2) -> {
                            int d1 = o1.getDecimals();
                            int d2 = o2.getDecimals();
                            int result = d2 - d1;
                            int absResult = Math.abs(result);
                            return result == 0 ? result : (result / absResult);
                        });
                        Unit defaultUnit = units.get(0);

                        associations.put(currency, new NetworkAssociation(baseUnit, defaultUnit, new HashSet<>(units)));
                    }

                    networks.add(Network.create(
                            blockchainModel.getId(),
                            blockchainModel.getName(),
                            blockchainModel.isMainnet(),
                            findCurrency(associations, blockchainModel),
                            blockchainModel.getBlockHeight(),
                            associations));

                    return null;
                }, latch);
            }
            return null;
        }, latch);
    }


    private static void getBlockChains(BlockchainDb query,
                                       Collection<Blockchain> defaultBlockchains,
                                       Function<Collection<Blockchain>, Void> func,
                                       CountUpAndDownLatch latch) {
        latch.countUp();
        query.getBlockchains(new BlockchainCompletionHandler<List<Blockchain>>() {
            @Override
            public void handleData(List<Blockchain> newBlockchains) {
                try {
                    Map<String, Blockchain> merged = new HashMap<>();
                    for (Blockchain blockchain : defaultBlockchains) {
                        merged.put(blockchain.getId(), blockchain);
                    }

                    for (Blockchain blockchain : newBlockchains) {
                        merged.put(blockchain.getId(), blockchain);
                    }

                    func.apply(merged.values());
                } finally {
                    latch.countDown();
                }
            }

            @Override
            public void handleError(QueryError error) {
                try {
                    func.apply(defaultBlockchains);
                } finally {
                    latch.countDown();
                }
            }
        });
    }

    private static void getCurrencies(BlockchainDb query, String blockchainId,
                                      Collection<Currency> defaultCurrencies,
                                      Function<Collection<Currency>, Void> func,
                                      CountUpAndDownLatch latch) {
        latch.countUp();
        query.getCurrencies(blockchainId, new BlockchainCompletionHandler<List<Currency>>() {
            @Override
            public void handleData(List<Currency> newCurrencies) {
                try {
                    Map<String, Currency> merged = new HashMap<>();
                    for (Currency currency : defaultCurrencies) {
                        merged.put(currency.getId(), currency);
                    }

                    for (Currency currency : newCurrencies) {
                        merged.put(currency.getId(), currency);
                    }

                    func.apply(merged.values());
                } finally {
                    latch.countDown();
                }
            }

            @Override
            public void handleError(QueryError error) {
                try {
                    func.apply(defaultCurrencies);
                } finally {
                    latch.countDown();
                }
            }
        });
    }

    private static Unit currencyDenominationToBaseUnit(com.breadwallet.crypto.Currency currency,
                                                       List<CurrencyDenomination> denominations) {
        for (CurrencyDenomination denomination : denominations) {
            if (denomination.getDecimals() == 0) {
                String uids = String.format("%s-%s", currency.getName(), denomination.getCode());
                return new Unit(currency, uids, denomination.getName(), denomination.getSymbol());
            }
        }
        // TODO: This isn't how we should handle this (this is based on swift)
        throw new IllegalStateException("Missing base unit");
    }

    private static List<Unit> currencyDenominationToUnits(com.breadwallet.crypto.Currency currency,
                                                          List<CurrencyDenomination> denominations, Unit base) {
        List<Unit> units = new ArrayList<>();
        for (CurrencyDenomination denomination : denominations) {
            if (denomination.getDecimals() != 0) {
                String uids = String.format("%s-%s", currency.getName(), denomination.getCode());
                units.add(new Unit(currency, uids, denomination.getName(), denomination.getSymbol(), base,
                        denomination.getDecimals()));
            }
        }
        return units;
    }

    private static com.breadwallet.crypto.Currency findCurrency(Map<com.breadwallet.crypto.Currency,
            NetworkAssociation> associations, Blockchain blockchainModel) {
        String code = blockchainModel.getCurrency().toLowerCase();
        for (com.breadwallet.crypto.Currency currency : associations.keySet()) {
            if (code.equals(currency.getCode())) {
                return currency;
            }
        }
        throw new IllegalStateException(String.format("Missing currency %s: defaultUnit",
                blockchainModel.getCurrency()));
    }

    private static class CountUpAndDownLatch {

        static CountUpAndDownLatch wrap(Runnable runnable) {
            return new CountUpAndDownLatch(runnable);
        }

        private final Runnable runnable;
        private final AtomicInteger count;

        CountUpAndDownLatch(Runnable runnable) {
            this.count = new AtomicInteger(0);
            this.runnable = runnable;
        }

        void countUp() {
            count.getAndIncrement();
        }

        void countDown() {
            if (0 == count.decrementAndGet()) {
                runnable.run();
            }
        }
    }
}
