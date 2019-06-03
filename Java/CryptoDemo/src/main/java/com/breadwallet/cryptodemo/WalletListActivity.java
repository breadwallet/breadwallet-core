package com.breadwallet.cryptodemo;

import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.events.wallet.WalletBalanceUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletCreatedEvent;
import com.breadwallet.crypto.events.wallet.WalletDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.wallet.WalletEventVisitor;
import com.breadwallet.crypto.events.wallet.WalletFeeBasisUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletListener;
import com.breadwallet.crypto.events.wallet.WalletTransferAddedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferSubmittedEvent;

import java.util.ArrayList;
import java.util.List;

public class WalletListActivity extends AppCompatActivity implements WalletListener {

    private List<Wallet> wallets = new ArrayList<>();

    private RecyclerView walletsView;
    private RecyclerView.Adapter walletsAdapter;
    private RecyclerView.LayoutManager walletsLayoutManager;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_wallet_list);

        CoreCryptoApplication.initialize(this);

        walletsView = findViewById(R.id.wallet_recycler_view);
        walletsView.addItemDecoration(new DividerItemDecoration(getApplicationContext(), DividerItemDecoration.VERTICAL));

        walletsLayoutManager = new LinearLayoutManager(this);
        walletsView.setLayoutManager(walletsLayoutManager);

        walletsAdapter = new Adapter(wallets, (wallet) -> TransferListActivity.start(this, wallet));
        walletsView.setAdapter(walletsAdapter);
    }

    @Override
    protected void onResume() {
        super.onResume();

        CoreCryptoApplication.getListener().addListener(this);

        wallets.clear();
        wallets.addAll(CoreCryptoApplication.getSystem().getWallets());
        walletsAdapter.notifyDataSetChanged();
    }

    @Override
    protected void onPause() {
        super.onPause();

        CoreCryptoApplication.getListener().removeListener(this);
    }

    @Override
    public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        runOnUiThread(() -> {
            event.accept(new WalletEventVisitor<Void>() {
                @Override
                public Void visit(WalletBalanceUpdatedEvent event) {
                    int index = wallets.indexOf(wallet);
                    if (index != -1) {
                        walletsAdapter.notifyItemChanged(index);
                    }
                    return null;
                }

                @Override
                public Void visit(WalletChangedEvent event) {
                    return null;
                }

                @Override
                public Void visit(WalletCreatedEvent event) {
                    int index = wallets.indexOf(wallet);
                    if (index == -1) {
                        index = wallets.size();
                        wallets.add(index, wallet);
                        walletsAdapter.notifyItemInserted(index);
                    }
                    return null;
                }

                @Override
                public Void visit(WalletDeletedEvent event) {
                    int index = wallets.indexOf(wallet);
                    if (index != -1) {
                        wallets.remove(index);
                        walletsAdapter.notifyItemRemoved(index);
                    }
                    return null;
                }

                @Override
                public Void visit(WalletFeeBasisUpdatedEvent event) {
                    return null;
                }

                @Override
                public Void visit(WalletTransferAddedEvent event) {
                    return null;
                }

                @Override
                public Void visit(WalletTransferChangedEvent event) {
                    return null;
                }

                @Override
                public Void visit(WalletTransferDeletedEvent event) {
                    return null;
                }

                @Override
                public Void visit(WalletTransferSubmittedEvent event) {
                    return null;
                }
            });
        });
    }

    private interface OnItemClickListener<T> {
        void onItemClick(T item);
    }

    private static class Adapter extends RecyclerView.Adapter<ViewHolder> {

        private List<Wallet> wallets;
        private OnItemClickListener<Wallet> listener;

        Adapter(List<Wallet> wallets, OnItemClickListener<Wallet> listener) {
            this.wallets = wallets;
            this.listener = listener;
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

            vh.itemView.setOnClickListener(v -> listener.onItemClick(wallet));
            vh.currencyView.setText(currencyText);
            vh.symbolView.setText(balanceText);
        }

        @Override
        public int getItemCount() {
            return wallets.size();
        }
    }

    private static class ViewHolder extends RecyclerView.ViewHolder {

        public TextView currencyView;
        public TextView symbolView;

        public ViewHolder(@NonNull View view) {
            super(view);

            currencyView = view.findViewById(R.id.item_currency);
            symbolView = view.findViewById(R.id.item_symbol);
        }
    }
}
