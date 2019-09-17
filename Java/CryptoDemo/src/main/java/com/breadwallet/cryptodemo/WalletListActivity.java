/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.cryptodemo;

import android.support.annotation.NonNull;
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
import java.util.List;

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

        walletsAdapter.set(new ArrayList<>(CoreCryptoApplication.getSystem().getWallets()));
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
            case R.id.action_connect:
                for (WalletManager wm: CoreCryptoApplication.getSystem().getWalletManagers()) {
                    wm.connect();
                }
                return true;
            case R.id.action_sync:
                for (WalletManager wm: CoreCryptoApplication.getSystem().getWalletManagers()) {
                    wm.sync();
                }
                return true;
            case R.id.action_disconnect:
                for (WalletManager wm: CoreCryptoApplication.getSystem().getWalletManagers()) {
                    wm.disconnect();
                }
                return true;
            case R.id.action_reset:
                CoreCryptoApplication.resetSystem();
                walletsAdapter.clear();
                return true;
        }
        return false;
    }

    @Override
    public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        runOnUiThread(() -> {
            event.accept(new DefaultWalletEventVisitor<Void>() {
                @Override
                public Void visit(WalletBalanceUpdatedEvent event) {
                    walletsAdapter.changed(wallet);
                    return null;
                }

                @Override
                public Void visit(WalletChangedEvent event) {
                    walletsAdapter.changed(wallet);
                    return null;
                }

                @Override
                public Void visit(WalletCreatedEvent event) {
                    walletsAdapter.add(wallet);
                    return null;
                }

                @Override
                public Void visit(WalletDeletedEvent event) {
                    walletsAdapter.remove(wallet);
                    return null;
                }
            });
        });
    }

    private interface OnItemClickListener<T> {
        void onItemClick(T item);
    }

    private static class Adapter extends RecyclerView.Adapter<ViewHolder> {

        private final OnItemClickListener<Wallet> listener;
        private final SortedList<Wallet> wallets;

        Adapter(OnItemClickListener<Wallet> listener) {
            this.listener = listener;
            this.wallets = new SortedList<>(Wallet.class, new SortedListAdapterCallback<Wallet>(this) {
                @Override
                public int compare(Wallet w1, Wallet w2) {
                    int managerCompare = w1.getWalletManager().getName().compareTo(w2.getWalletManager().getName());
                    return managerCompare != 0 ? managerCompare : w1.getName().compareTo(w2.getName());
                }

                @Override
                public boolean areContentsTheSame(Wallet w1, Wallet w2) {
                    return false;
                }

                @Override
                public boolean areItemsTheSame(Wallet w1, Wallet w2) {
                    return w1.equals(w2);
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
            Wallet wallet = wallets.get(i);
            Amount balance = wallet.getBalance();

            String currencyText = String.format("%s (%s)", wallet.getName(), wallet.getWalletManager().getNetwork());
            String balanceText = balance.toStringAsUnit(balance.getUnit(), null).or("---");


            vh.itemView.setOnClickListener(v -> listener.onItemClick(wallets.get(i)));
            vh.currencyView.setText(currencyText);
            vh.symbolView.setText(balanceText);
        }

        @Override
        public int getItemCount() {
            return wallets.size();
        }

        private void set(List<Wallet> newWallets) {
            wallets.replaceAll(newWallets);
        }

        private void add(Wallet wallet) {
            wallets.add(wallet);
        }

        private void remove(Wallet wallet) {
            wallets.remove(wallet);
        }

        private void changed(Wallet wallet) {
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
