/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.cryptodemo;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
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

import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.WalletManagerState;
import com.breadwallet.crypto.events.system.DefaultSystemListener;
import com.breadwallet.crypto.events.wallet.DefaultWalletEventVisitor;
import com.breadwallet.crypto.events.wallet.WalletBalanceUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletCreatedEvent;
import com.breadwallet.crypto.events.wallet.WalletDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.walletmanager.DefaultWalletManagerEventVisitor;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncProgressEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStartedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStoppedEvent;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;
import com.google.common.collect.Lists;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class WalletListActivity extends AppCompatActivity implements DefaultSystemListener {

    private static final int WALLET_CHUNK_SIZE = 10;

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
        loadWallets();
    }

    @Override
    protected void onPause() {
        CoreCryptoApplication.getDispatchingSystemListener().removeSystemListener(this);

        super.onPause();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_wallet_list, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_connect:
                connect();
                return true;
            case R.id.action_disconnect:
                disconnect();
                return true;
            case R.id.action_sync:
                sync();
                return true;
            case R.id.action_wipe:
                wipe();
                return true;
            case R.id.action_update_fees:
                updateFees();
                return true;
            case R.id.action_add_wallet:
                showAddWalletMenu();
                return true;

        }
        return false;
    }

    @Override
    public void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event) {
        ApplicationExecutors.runOnUiExecutor(() -> event.accept(new DefaultWalletManagerEventVisitor<Void>() {
            @Override
            public Void visit(WalletManagerSyncStartedEvent event) {
                updateWallets(manager);
                return null;
            }

            @Nullable
            @Override
            public Void visit(WalletManagerSyncProgressEvent event) {
                updateWalletsForSync(manager, event.getPercentComplete());
                return null;
            }

            @Override
            public Void visit(WalletManagerSyncStoppedEvent event) {
                updateWallets(manager);
                return null;
            }
        }));
    }

    @Override
    public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        ApplicationExecutors.runOnUiExecutor(() -> event.accept(new DefaultWalletEventVisitor<Void>() {
            @Override
            public Void visit(WalletBalanceUpdatedEvent event) {
                updateWallet(wallet);
                return null;
            }

            @Override
            public Void visit(WalletChangedEvent event) {
                updateWallet(wallet);
                return null;
            }

            @Override
            public Void visit(WalletCreatedEvent event) {
                addWallet(wallet);
                return null;
            }

            @Override
            public Void visit(WalletDeletedEvent event) {
                removeWallet(wallet);
                return null;
            }
        }));
    }

    private void loadWallets() {
        ApplicationExecutors.runOnUiExecutor(() -> {
            List<? extends Wallet> wallets = CoreCryptoApplication.getSystem().getWallets();
            List<WalletViewModel> viewModels = WalletViewModel.create(wallets);
            Collections.sort(viewModels, WalletViewModel::compare);
            runOnUiThread(walletsAdapter::clear);
            for (List<WalletViewModel> viewModelsChunk: Lists.partition(viewModels, WALLET_CHUNK_SIZE)) {
                runOnUiThread(() -> walletsAdapter.add(viewModelsChunk));
            }
        });
    }

    private void addWallet(Wallet wallet) {
        ApplicationExecutors.runOnUiExecutor(() -> {
            WalletViewModel vm = WalletViewModel.create(wallet);
            runOnUiThread(() -> walletsAdapter.add(vm));
        });
    }

    private void removeWallet(Wallet wallet) {
        ApplicationExecutors.runOnUiExecutor(() -> {
            WalletViewModel vm = WalletViewModel.create(wallet);
            runOnUiThread(() -> walletsAdapter.remove(vm));
        });
    }

    private void updateWallet(Wallet wallet) {
        ApplicationExecutors.runOnUiExecutor(() -> {
            WalletViewModel vm = WalletViewModel.create(wallet);
            runOnUiThread(() -> walletsAdapter.update(vm));
        });
    }

    private void updateWallets(WalletManager manager) {
        ApplicationExecutors.runOnUiExecutor(() -> {
            List<? extends Wallet> wallets = manager.getWallets();
            List<WalletViewModel> vms = WalletViewModel.create(wallets);
            runOnUiThread(() -> walletsAdapter.update(vms));
        });
    }

    private void updateWalletsForSync(WalletManager manager, float percentComplete) {
        ApplicationExecutors.runOnUiExecutor(() -> {
            List<? extends Wallet> wallets = manager.getWallets();
            List<WalletViewModel> vms = WalletViewModel.create(wallets, percentComplete);
            runOnUiThread(() -> walletsAdapter.update(vms));
        });
    }

    private void connect() {
        ApplicationExecutors.runOnBlockingExecutor(() -> CoreCryptoApplication.getSystem().connectAll());
    }

    private void disconnect() {
        ApplicationExecutors.runOnBlockingExecutor(() -> CoreCryptoApplication.getSystem().disconnectAll());
    }

    private void sync() {
        ApplicationExecutors.runOnBlockingExecutor(() -> {
            for (WalletManager wm : CoreCryptoApplication.getSystem().getWalletManagers()) {
                wm.sync();
            }
        });
    }

    private void updateFees() {
        ApplicationExecutors.runOnBlockingExecutor(() -> CoreCryptoApplication.getSystem().updateNetworkFees(null));
    }

    private void wipe() {
        ApplicationExecutors.runOnBlockingExecutor(() -> {
            CoreCryptoApplication.wipeSystem();
            runOnUiThread(this::recreate);
        });
    }

    private void showAddWalletMenu() {
        ApplicationExecutors.runOnUiExecutor(() -> {
            Set<Currency> missingSet = new HashSet<>();

            List<? extends WalletManager> managers = CoreCryptoApplication.getSystem().getWalletManagers();
            for (WalletManager manager: managers) {
                // get all currencies for existing wallet managers
                missingSet.addAll(manager.getNetwork().getCurrencies());

                for (Wallet wallet: manager.getWallets()) {
                    // remove the currencies for existing wallets
                    missingSet.remove(wallet.getCurrency());
                }
            }

            // sort by currency code
            List<Currency> missingList = new ArrayList<>(missingSet);
            Collections.sort(missingList, (o1, o2) -> o1.getCode().compareTo(o2.getCode()));

            // all currencies have wallets; ignore
            if (missingList.isEmpty()) {
                runOnUiThread(() -> new AlertDialog.Builder(this)
                        .setTitle("Add Wallet")
                        .setMessage("All currencies added and accounted for!")
                        .setCancelable(false)
                        .setNeutralButton("Ok", (d, w) -> { })
                        .show());

                // extract the currencies and their descriptions
            } else {
                String[] itemTexts = new String[missingList.size()];
                Currency[] itemCurrencies = new Currency[missingList.size()];
                for (int i = 0; i < missingList.size(); i++) {
                    itemCurrencies[i] = missingList.get(i);
                    itemTexts[i] = itemCurrencies[i].getCode();
                }

                runOnUiThread(() -> new AlertDialog.Builder(this)
                        .setTitle("Add Wallet")
                        .setSingleChoiceItems(itemTexts,
                                -1,
                                (d, w) -> {
                                    for (WalletManager manager : managers) {
                                        if (manager.getNetwork().hasCurrency(itemCurrencies[w])) {
                                            manager.registerWalletFor(itemCurrencies[w]);
                                            break;
                                        }
                                    }
                                    d.dismiss();
                                })
                        .show());
            }
        });
    }

    private interface OnItemClickListener<T> {
        void onItemClick(T item);
    }

    private static class WalletViewModel {

        static int compare(WalletViewModel vm1, WalletViewModel vm2) {
            if (vm1.wallet.equals(vm2.wallet)) return 0;

            int managerCompare = vm1.walletManagerName().compareTo(vm2.walletManagerName());
            if (0 != managerCompare) return managerCompare;

            return vm1.walletName().compareTo(vm2.walletName());
        }

        static boolean areContentsTheSame(WalletViewModel vm1, WalletViewModel vm2) {
            return vm1.isSyncing() == vm2.isSyncing() &&
                    vm1.currencyText().equals(vm2.currencyText()) &&
                    vm1.balanceText().equals(vm2.balanceText()) &&
                    vm1.syncText().equals(vm2.syncText());
        }

        static boolean areItemsTheSame(WalletViewModel vm1, WalletViewModel vm2) {
            return vm1.wallet.equals(vm2.wallet);
        }

        static List<WalletViewModel> create(List<? extends Wallet> wallets) {
            List<WalletViewModel> vms = new ArrayList<>(wallets.size());
            for (Wallet w: wallets) {
                vms.add(create(w));
            }
            return vms;
        }

        static List<WalletViewModel> create(List<? extends Wallet> wallets, float percentComplete) {
            List<WalletViewModel> vms = new ArrayList<>(wallets.size());
            for (Wallet w: wallets) {
                vms.add(create(w, percentComplete));
            }
            return vms;
        }

        static WalletViewModel create(Wallet wallet) {
            return create(wallet, 0);
        }

        static WalletViewModel create(Wallet wallet, float percentComplete) {
            return new WalletViewModel(wallet, percentComplete);
        }

        final Wallet wallet;
        private final float percentComplete;

        final Supplier<String> walletName;
        final Supplier<String> walletManagerName;
        final Supplier<WalletManagerState> walletManagerState;

        final Supplier<String> currencyText;
        final Supplier<String> balanceText;
        final Supplier<String> syncText;

        WalletViewModel(Wallet wallet, float percentComplete) {
            this.wallet = wallet;
            this.percentComplete = percentComplete;

            this.walletName = Suppliers.memoize(wallet::getName);

            Supplier<WalletManager> walletManagerSupplier = Suppliers.memoize(wallet::getWalletManager);
            Supplier<Network> networkSupplier = Suppliers.memoize(() -> walletManagerSupplier.get().getNetwork());
            this.walletManagerState = Suppliers.memoize(() -> walletManagerSupplier.get().getState());
            this.walletManagerName = Suppliers.memoize(() -> networkSupplier.get().getName());

            this.balanceText = Suppliers.memoize(() -> wallet.getBalance().toString());
            this.currencyText = Suppliers.memoize(() -> String.format("%s (%s)", walletName(), walletManagerName()));
            this.syncText = Suppliers.memoize(() -> String.format("Syncing at %.2f%%", percentComplete()));
        }

        String walletName() {
            return walletName.get();
        }

        String walletManagerName() {
            return walletManagerName.get();
        }

        boolean isSyncing() {
            return 0 != percentComplete && walletManagerState.get().equals(WalletManagerState.SYNCING());
        }

        float percentComplete() {
            return percentComplete;
        }

        String currencyText() {
            return currencyText.get();
        }

        String balanceText() {
            return balanceText.get();
        }

        String syncText() {
            return syncText.get();
        }
    }

    private static class WalletViewModelSortedListAdapterCallback extends SortedListAdapterCallback<WalletViewModel> {

        WalletViewModelSortedListAdapterCallback(Adapter adapter) {
            super(adapter);
        }

        @Override
        public int compare(WalletViewModel t1, WalletViewModel t2) {
            return WalletViewModel.compare(t1, t2);
        }

        @Override
        public boolean areContentsTheSame(WalletViewModel t1, WalletViewModel t2) {
            return WalletViewModel.areContentsTheSame(t1, t2);
        }

        @Override
        public boolean areItemsTheSame(WalletViewModel t1, WalletViewModel t2) {
            return WalletViewModel.areItemsTheSame(t1, t2);
        }
    }

    private static class Adapter extends RecyclerView.Adapter<ViewHolder> {

        final OnItemClickListener<Wallet> listener;
        final SortedList<WalletViewModel> viewModels;

        Adapter(OnItemClickListener<Wallet> listener) {
            this.listener = listener;
            this.viewModels = new SortedList<>(WalletViewModel.class, new WalletViewModelSortedListAdapterCallback(this));
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
            vh.currencyView.setText(vm.currencyText());
            vh.symbolView.setText(vm.balanceText());
            vh.syncView.setText(vm.isSyncing() ? vm.syncText() : "");
            vh.syncView.setVisibility(vm.isSyncing() ? View.VISIBLE : View.GONE);
        }

        @Override
        public int getItemCount() {
            return viewModels.size();
        }

        void clear() {
            viewModels.clear();
        }

        void add(List<WalletViewModel> wallets) {
            viewModels.addAll(wallets);
        }

        void add(WalletViewModel wallet) {
            viewModels.add(wallet);
        }

        void remove(WalletViewModel wallet) {
            viewModels.remove(wallet);
        }

        void update(WalletViewModel wallet) {
            viewModels.add(wallet);
        }

        void update(List<WalletViewModel> wallets) {
            viewModels.addAll(wallets);
        }
    }

    private static class ViewHolder extends RecyclerView.ViewHolder {

        TextView currencyView;
        TextView symbolView;
        TextView syncView;

        ViewHolder(@NonNull View view) {
            super(view);

            currencyView = view.findViewById(R.id.item_currency);
            symbolView = view.findViewById(R.id.item_symbol);
            syncView = view.findViewById(R.id.item_sync_status);
        }
    }
}
