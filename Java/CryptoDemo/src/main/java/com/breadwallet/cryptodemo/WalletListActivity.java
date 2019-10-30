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

        CoreCryptoApplication.getDispatchingSystemListener().addSystemListener(this);
        ApplicationExecutors.runOnUiExecutor(() -> {
            List<WalletViewModel> vms = WalletViewModel.create(CoreCryptoApplication.getSystem().getWallets());
            runOnUiThread(() -> walletsAdapter.set(vms));
        });
    }

    @Override
    protected void onDestroy() {
        CoreCryptoApplication.getDispatchingSystemListener().removeSystemListener(this);

        super.onDestroy();
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
                            .setTitle("Add Wallet")
                            .setMessage("All currencies added and accounted for!")
                            .setCancelable(false)
                            .setNeutralButton("Ok", (d, w) -> { })
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
                            .setTitle("Add Wallet")
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

                runOnUiThread(dialog::show);
                return true;
            case R.id.action_connect:
                ApplicationExecutors.runOnBlockingExecutor(() -> CoreCryptoApplication.getSystem().connectAll());
                return true;
            case R.id.action_sync:
                ApplicationExecutors.runOnBlockingExecutor(() -> {
                    for (WalletManager wm : CoreCryptoApplication.getSystem().getWalletManagers()) {
                        wm.sync();
                    }
                });
                return true;
            case R.id.action_disconnect:
                ApplicationExecutors.runOnBlockingExecutor(() -> CoreCryptoApplication.getSystem().disconnectAll());
                return true;
            case R.id.action_update_fees:
                ApplicationExecutors.runOnBlockingExecutor(() -> CoreCryptoApplication.getSystem().updateNetworkFees(null));
                return true;
            case R.id.action_reset:
                ApplicationExecutors.runOnBlockingExecutor(() -> {
                    CoreCryptoApplication.resetSystem();
                    runOnUiThread(this::recreate);
                });
                return true;
            case R.id.action_wipe:
                ApplicationExecutors.runOnBlockingExecutor(() -> {
                    CoreCryptoApplication.wipeSystem();
                    runOnUiThread(this::recreate);
                });
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
        ApplicationExecutors.runOnUiExecutor(() -> event.accept(new DefaultWalletEventVisitor<Void>() {
            @Override
            public Void visit(WalletBalanceUpdatedEvent event) {
                WalletViewModel vm = WalletViewModel.create(wallet);
                runOnUiThread(() -> walletsAdapter.changed(vm));
                return null;
            }

            @Override
            public Void visit(WalletChangedEvent event) {
                WalletViewModel vm = WalletViewModel.create(wallet);
                runOnUiThread(() -> walletsAdapter.changed(vm));
                return null;
            }

            @Override
            public Void visit(WalletCreatedEvent event) {
                WalletViewModel vm = WalletViewModel.create(wallet);
                runOnUiThread(() -> walletsAdapter.add(vm));
                return null;
            }

            @Override
            public Void visit(WalletDeletedEvent event) {
                WalletViewModel vm = WalletViewModel.create(wallet);
                runOnUiThread(() -> walletsAdapter.remove(vm));
                return null;
            }
        }));
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

        final Wallet wallet;
        final String walletName;
        final String managerName;

        final String currencyText;
        final String balanceText;

        private WalletViewModel(Wallet wallet) {
            this.wallet = wallet;
            this.walletName = wallet.getName();
            this.managerName = wallet.getWalletManager().getName();

            Amount balance = wallet.getBalance();
            Network network = wallet.getWalletManager().getNetwork();

            this.currencyText = String.format("%s (%s)", walletName, network);
            this.balanceText = balance.toStringAsUnit(balance.getUnit(), null).or("---");
        }

        @Override
        public int compareTo(WalletViewModel vm2) {
            if (wallet.equals(vm2.wallet)) return 0;

            int managerCompare = this.managerName.compareTo(vm2.managerName);
            if (0 != managerCompare) return managerCompare;

            return walletName.compareTo(vm2.walletName);
        }

        boolean areContentsTheSame(WalletViewModel vm) {
            return currencyText.equals(vm.currencyText) &&
                    balanceText.equals(vm.balanceText);
        }

        boolean areItemsTheSame(WalletViewModel vm) {
            return wallet.equals(vm.wallet);
        }
    }

    private static class Adapter extends RecyclerView.Adapter<ViewHolder> {

        private final OnItemClickListener<Wallet> listener;
        private final SortedList<WalletViewModel> viewModels;

        Adapter(OnItemClickListener<Wallet> listener) {
            this.listener = listener;
            this.viewModels = new SortedList<>(WalletViewModel.class, new SortedListAdapterCallback<WalletViewModel>(this) {
                @Override
                public int compare(WalletViewModel w1, WalletViewModel w2) {
                    return w1.compareTo(w2);
                }

                @Override
                public boolean areContentsTheSame(WalletViewModel w1, WalletViewModel w2) {
                    return w1.areContentsTheSame(w2);
                }

                @Override
                public boolean areItemsTheSame(WalletViewModel w1, WalletViewModel w2) {
                    return w1.areItemsTheSame(w2);
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
            WalletViewModel vm = viewModels.get(i);

            vh.itemView.setOnClickListener(v -> listener.onItemClick(vm.wallet));
            vh.currencyView.setText(vm.currencyText);
            vh.symbolView.setText(vm.balanceText);
        }

        @Override
        public int getItemCount() {
            return viewModels.size();
        }

        private void set(List<WalletViewModel> newWallets) {
            viewModels.addAll(newWallets);
        }

        private void add(WalletViewModel wallet) {
            viewModels.add(wallet);
        }

        private void remove(WalletViewModel wallet) {
            viewModels.remove(wallet);
        }

        private void changed(WalletViewModel wallet) {
            viewModels.add(wallet);
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
