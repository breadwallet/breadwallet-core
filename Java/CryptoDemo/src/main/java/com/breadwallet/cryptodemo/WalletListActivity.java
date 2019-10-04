/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.cryptodemo;

import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.util.SortedList;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.Toolbar;
import android.support.v7.widget.util.SortedListAdapterCallback;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.events.system.DefaultSystemListener;
import com.breadwallet.crypto.events.wallet.DefaultWalletEventVisitor;
import com.breadwallet.crypto.events.wallet.WalletBalanceUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletCreatedEvent;
import com.breadwallet.crypto.events.wallet.WalletDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletEvent;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class WalletListActivity extends AppCompatActivity implements DefaultSystemListener {

    private Adapter walletsAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_wallet_list);

        CoreCryptoApplication.initialize(this);

        RecyclerView walletsView = findViewById(R.id.wallet_recycler_view);
        walletsView.addItemDecoration(new DividerItemDecoration(getApplicationContext(), DividerItemDecoration.VERTICAL));

        RecyclerView.LayoutManager walletsLayoutManager = new LinearLayoutManager(this);
        walletsView.setLayoutManager(walletsLayoutManager);

        walletsAdapter = new Adapter((wallet) -> TransferListActivity.start(this, wallet));
        walletsView.setAdapter(walletsAdapter);

        Toolbar toolbar = findViewById(R.id.toolbar_view);
        setSupportActionBar(toolbar);
    }

    @Override
    protected void onResume() {
        super.onResume();

        CoreCryptoApplication.getDispatchingSystemListener().addSystemListener(this);
        walletsAdapter.set(WalletViewModel.create(CoreCryptoApplication.getSystem().getWallets()));
    }

    @Override
    protected void onPause() {
        super.onPause();

        CoreCryptoApplication.getDispatchingSystemListener().removeSystemListener(this);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_wallet_list, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_add_wallet:
                AlertDialog dialog;

                List<Currency> missingCurrencies = getMissingCurrencies();
                if (missingCurrencies.isEmpty()) {
                    // all currencies have wallets; ignore
                    dialog = new AlertDialog.Builder(this)
                            .setMessage("All currencies added and accounted for!")
                            .create();

                } else {
                    // extract the currencies and their descriptions
                    String[] itemTexts = new String[missingCurrencies.size()];
                    Currency[] itemCurrencies = new Currency[missingCurrencies.size()];
                    for (int i = 0; i < missingCurrencies.size(); i++) {
                        itemCurrencies[i] = missingCurrencies.get(i);
                        itemTexts[i] = itemCurrencies[i].getCode();
                    }

                    // prompt for addition
                    dialog = new AlertDialog.Builder(this)
                            .setSingleChoiceItems(itemTexts,
                                    -1,
                                    (d, w) -> {
                                        for (WalletManager manager : CoreCryptoApplication.getSystem().getWalletManagers()) {
                                            if (manager.getNetwork().hasCurrency(itemCurrencies[w])) {
                                                manager.registerWalletFor(itemCurrencies[w]);
                                                break;
                                            }
                                        }
                                        d.dismiss();
                                    })
                            .create();
                }

                dialog.show();
                return true;
            case R.id.action_connect:
                CoreCryptoApplication.getSystem().connectAll();
                return true;
            case R.id.action_sync:
                for (WalletManager wm: CoreCryptoApplication.getSystem().getWalletManagers()) {
                    wm.sync();
                }
                return true;
            case R.id.action_disconnect:
                CoreCryptoApplication.getSystem().disconnectAll();
                return true;
            case R.id.action_update_fees:
                CoreCryptoApplication.getSystem().updateNetworkFees(null);
                return true;
            case R.id.action_reset:
                CoreCryptoApplication.resetSystem();
                walletsAdapter.clear();
                return true;
            case R.id.action_wipe:
                CoreCryptoApplication.wipeSystem();
                walletsAdapter.clear();
                return true;
        }
        return false;
    }

    private List<Currency> getMissingCurrencies() {
        Set<Currency> missingCurrencies = new HashSet<>();

        for (WalletManager manager: CoreCryptoApplication.getSystem().getWalletManagers()) {
            // get all currencies for existing wallet managers
            missingCurrencies.addAll(manager.getNetwork().getCurrencies());

            for (Wallet wallet: manager.getWallets()) {
                // remove the currencies for existing wallets
                 missingCurrencies.remove(wallet.getCurrency());
            }
        }

        // sort by currency code
        List<Currency> currencies = new ArrayList<>(missingCurrencies);
        Collections.sort(currencies, (o1, o2) -> o1.getCode().compareTo(o2.getCode()));
        return currencies;
    }

    @Override
    public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        event.accept(new DefaultWalletEventVisitor<Void>() {
            @Override
            public Void visit(WalletBalanceUpdatedEvent event) {
                WalletViewModel vm = WalletViewModel.create(wallet);
                runOnUiThread(() -> {
                    walletsAdapter.changed(vm);
                });
                return null;
            }

            @Override
            public Void visit(WalletChangedEvent event) {
                WalletViewModel vm = WalletViewModel.create(wallet);
                runOnUiThread(() -> {
                    walletsAdapter.changed(vm);
                });
                return null;
            }

            @Override
            public Void visit(WalletCreatedEvent event) {
                WalletViewModel vm = WalletViewModel.create(wallet);
                runOnUiThread(() -> {
                    walletsAdapter.add(vm);
                });
                return null;
            }

            @Override
            public Void visit(WalletDeletedEvent event) {
                WalletViewModel vm = WalletViewModel.create(wallet);
                runOnUiThread(() -> {
                    walletsAdapter.remove(vm);
                });
                return null;
            }
        });
    }

    private interface OnItemClickListener<T> {
        void onItemClick(T item);
    }

    private static class WalletViewModel implements Comparable<WalletViewModel > {

        private static List<WalletViewModel> create(List<? extends Wallet> wallets) {
            List<WalletViewModel> vms = new ArrayList<>(wallets.size());
            for (Wallet t: wallets) {
                vms.add(create(t));
            }
            return vms;
        }

        private static WalletViewModel create(Wallet wallet) {
            return new WalletViewModel(wallet);
        }

        private final Wallet wallet;
        private final Amount balance;
        private final String name;
        private final Network network;

        private WalletViewModel(Wallet wallet) {
            this.wallet = wallet;
            this.balance = wallet.getBalance();
            this.name = wallet.getName();
            this.network = wallet.getWalletManager().getNetwork();
        }

        private Amount getBalance() {
            return balance;
        }

        private String getName() {
            return name;
        }

        private Network getNetwork() {
            return network;
        }

        private Wallet getWallet() {
            return wallet;
        }

        @Override
        public boolean equals(Object object) {
            if (this == object) {
                return true;
            }

            if (!(object instanceof WalletViewModel)) {
                return false;
            }

            WalletViewModel that = (WalletViewModel) object;
            return balance.equals(that.balance) &&
                    name.equals(that.name) &&
                    network.equals(that.network);
        }

        @Override
        public int hashCode() {
            return Objects.hash(balance, name, network);
        }

        @Override
        public int compareTo(WalletViewModel vm2) {
            Wallet w1 = this.getWallet();
            Wallet w2 = vm2.getWallet();

            int managerCompare = w1.getWalletManager().getName().compareTo(w2.getWalletManager().getName());
            return managerCompare != 0 ? managerCompare : w1.getName().compareTo(w2.getName());
        }
    }

    private static class Adapter extends RecyclerView.Adapter<ViewHolder> {

        private final OnItemClickListener<Wallet> listener;
        private final SortedList<WalletViewModel> wallets;

        Adapter(OnItemClickListener<Wallet> listener) {
            this.listener = listener;
            this.wallets = new SortedList<>(WalletViewModel.class, new SortedListAdapterCallback<WalletViewModel>(this) {
                @Override
                public int compare(WalletViewModel w1, WalletViewModel w2) {
                    return w1.compareTo(w2);
                }

                @Override
                public boolean areContentsTheSame(WalletViewModel w1, WalletViewModel w2) {
                    return w1.equals(w2);
                }

                @Override
                public boolean areItemsTheSame(WalletViewModel w1, WalletViewModel w2) {
                    return w1.getWallet().equals(w2.getWallet());
                }
            });
        }

        @NonNull
        @Override
        public ViewHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int i) {
            View v = LayoutInflater.from(viewGroup.getContext()).inflate(R.layout.layout_wallet_item, viewGroup, false);
            return new ViewHolder(v);
        }

        @Override
        public void onBindViewHolder(@NonNull ViewHolder vh, int i) {
            WalletViewModel wallet = wallets.get(i);
            Amount balance = wallet.getBalance();

            String currencyText = String.format("%s (%s)", wallet.getName(), wallet.getNetwork());
            String balanceText = balance.toStringAsUnit(balance.getUnit(), null).or("---");

            vh.itemView.setOnClickListener(v -> listener.onItemClick(wallet.getWallet()));
            vh.currencyView.setText(currencyText);
            vh.symbolView.setText(balanceText);
        }

        @Override
        public int getItemCount() {
            return wallets.size();
        }

        private void set(List<WalletViewModel> newWallets) {
            wallets.replaceAll(newWallets);
        }

        private void add(WalletViewModel wallet) {
            wallets.add(wallet);
        }

        private void remove(WalletViewModel wallet) {
            wallets.remove(wallet);
        }

        private void changed(WalletViewModel wallet) {
            int index = wallets.indexOf(wallet);
            if (index != -1) {
                wallets.updateItemAt(index, wallet);
            }
        }

        private void clear() {
            wallets.clear();
        }
    }

    private static class ViewHolder extends RecyclerView.ViewHolder {

        private TextView currencyView;
        private TextView symbolView;

        private ViewHolder(@NonNull View view) {
            super(view);

            currencyView = view.findViewById(R.id.item_currency);
            symbolView = view.findViewById(R.id.item_symbol);
        }
    }
}
