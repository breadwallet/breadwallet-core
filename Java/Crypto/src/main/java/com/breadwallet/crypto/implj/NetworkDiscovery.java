/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.blockchaindb.CompletionHandler;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.Currency;
import com.breadwallet.crypto.blockchaindb.models.bdb.CurrencyDenomination;
import com.google.common.base.Function;
import com.google.common.primitives.UnsignedInteger;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

/* package */
final class NetworkDiscovery {

    /* package */
    interface Callback {
        void discovered(List<NetworkImpl> networks);
    }

    /* package */
    static void discoverNetworks(BlockchainDb query, List<String> networksNeeded, boolean isMainnet, Callback callback) {
        List<NetworkImpl> networks = new ArrayList<>();
        CountUpAndDownLatch latch = new CountUpAndDownLatch(() -> callback.discovered(networks));

        getBlockChains(latch, query, Blockchain.DEFAULT_BLOCKCHAINS, isMainnet, blockchainModels -> {
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

                Map<CurrencyImpl, NetworkAssociation> associations = new HashMap<>();

                getCurrencies(latch, query, blockchainModelId, defaultCurrencies, currencyModels -> {
                    for (Currency currencyModel : currencyModels) {
                        if (!blockchainModelId.equals(currencyModel.getBlockchainId())) {
                            continue;
                        }

                        CurrencyImpl currency = new CurrencyImpl(
                                currencyModel.getId(),
                                currencyModel.getName(),
                                currencyModel.getCode(),
                                currencyModel.getType());

                        CurrencyDenomination baseDenomination = findFirstBaseDenomination(currencyModel.getDenominations());
                        List<CurrencyDenomination> nonBaseDenominations = findAllNonBaseDenominations(currencyModel.getDenominations());

                        UnitImpl baseUnit = currencyDenominationToBaseUnit(currency, baseDenomination);
                        List<UnitImpl> units = currencyDenominationToUnits(currency, nonBaseDenominations, baseUnit);

                        units.add(0, baseUnit);
                        Collections.sort(units, (o1, o2) -> o2.getDecimals().compareTo(o1.getDecimals()));
                        Unit defaultUnit = units.get(0);

                        associations.put(currency, new NetworkAssociation(baseUnit, defaultUnit, new HashSet<>(units)));
                    }

                    networks.add(NetworkImpl.create(
                            blockchainModel.getId(),
                            blockchainModel.getName(),
                            blockchainModel.isMainnet(),
                            findCurrency(associations, blockchainModel),
                            blockchainModel.getBlockHeight(),
                            associations));

                    return null;
                });
            }
            return null;
        });
    }

    private static void getBlockChains(CountUpAndDownLatch latch,
                                       BlockchainDb query,
                                       Collection<Blockchain> defaultBlockchains,
                                       boolean isMainnet,
                                       Function<Collection<Blockchain>, Void> func) {
        latch.countUp();
        query.getBlockchains(isMainnet, new CompletionHandler<List<Blockchain>>() {
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

    private static void getCurrencies(CountUpAndDownLatch latch,
                                      BlockchainDb query,
                                      String blockchainId,
                                      Collection<Currency> defaultCurrencies,
                                      Function<Collection<Currency>, Void> func) {
        latch.countUp();
        query.getCurrencies(blockchainId, new CompletionHandler<List<Currency>>() {
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

    private static CurrencyDenomination findFirstBaseDenomination(List<CurrencyDenomination> denominations) {
        for (CurrencyDenomination denomination : denominations) {
            if (denomination.getDecimals().equals(UnsignedInteger.ZERO)) {
                return denomination;
            }
        }
        throw new IllegalStateException("Missing base denomination");
    }

    private static List<CurrencyDenomination> findAllNonBaseDenominations(List<CurrencyDenomination> denominations) {
        List<CurrencyDenomination> newDenominations = new ArrayList<>();
        for (CurrencyDenomination denomination : denominations) {
            if (!denomination.getDecimals().equals(UnsignedInteger.ZERO)) {
                newDenominations.add(denomination);
            }
        }
        return newDenominations;
    }

    private static UnitImpl currencyDenominationToBaseUnit(CurrencyImpl currency,
                                                           CurrencyDenomination denomination) {
        String uids = String.format("%s-%s", currency.getName(), denomination.getCode());
        return new UnitImpl(currency, uids, denomination.getName(), denomination.getSymbol());
    }

    private static List<UnitImpl> currencyDenominationToUnits(CurrencyImpl currency,
                                                              List<CurrencyDenomination> denominations,
                                                              UnitImpl base) {
        List<UnitImpl> units = new ArrayList<>();
        for (CurrencyDenomination denomination : denominations) {
            String uids = String.format("%s-%s", currency.getName(), denomination.getCode());
            units.add(new UnitImpl(currency, uids, denomination.getName(), denomination.getSymbol(), base,
                      denomination.getDecimals()));
        }
        return units;
    }

    private static CurrencyImpl findCurrency(Map<CurrencyImpl,
            NetworkAssociation> associations, Blockchain blockchainModel) {
        String code = blockchainModel.getCurrency().toLowerCase();
        for (CurrencyImpl currency : associations.keySet()) {
            if (code.equals(currency.getCode())) {
                return currency;
            }
        }
        throw new IllegalStateException(String.format("Missing currency %s: defaultUnit",
                blockchainModel.getCurrency()));
    }

    private static class CountUpAndDownLatch {

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
