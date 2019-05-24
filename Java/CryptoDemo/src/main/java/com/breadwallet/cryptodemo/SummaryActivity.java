package com.breadwallet.cryptodemo;

import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.Currency;
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

public class SummaryActivity extends AppCompatActivity implements WalletListener {

    private List<Wallet> wallets = new ArrayList<>();

    private RecyclerView summaryRecyclerView;
    private RecyclerView.Adapter summaryAdapter;
    private RecyclerView.LayoutManager summaryLayoutManager;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_summary);

        summaryRecyclerView = findViewById(R.id.summary_recycler_view);
        summaryRecyclerView.hasFixedSize();

        summaryLayoutManager = new LinearLayoutManager(this);
        summaryRecyclerView.setLayoutManager(summaryLayoutManager);

        summaryAdapter = new SummaryAdapter(wallets);
        summaryRecyclerView.setAdapter(summaryAdapter);

        CoreCryptoApplication.listener.addListener(this);
    }

    @Override
    protected void onResume() {
        super.onResume();

        wallets.clear();
        wallets.addAll(CoreCryptoApplication.system.getWallets());

        summaryAdapter.notifyDataSetChanged();
    }

    @Override
    public void handleWalletEvent(System system, WalletManager manager, Wallet wallet, WalletEvent event) {
        runOnUiThread(() -> {
            event.accept(new WalletEventVisitor<Void>() {
                @Override
                public Void visit(WalletBalanceUpdatedEvent event) {
                    int index = wallets.indexOf(wallet);
                    if (index != -1) {
                        summaryAdapter.notifyItemChanged(index);
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
                        summaryAdapter.notifyItemInserted(index);
                    }
                    return null;
                }

                @Override
                public Void visit(WalletDeletedEvent event) {
                    int index = wallets.indexOf(wallet);
                    if (index != -1) {
                        wallets.remove(index);
                        summaryAdapter.notifyItemRemoved(index);
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

    private static class SummaryViewHolder extends RecyclerView.ViewHolder {

        public TextView currencyView;
        public TextView symbolView;

        public SummaryViewHolder(@NonNull View view) {
            super(view);
            currencyView = view.findViewById(R.id.summary_item_currency);
            symbolView = view.findViewById(R.id.summary_item_symbol);
        }
    }

    private static class SummaryAdapter extends RecyclerView.Adapter<SummaryViewHolder> {

        private List<Wallet> wallets;

        SummaryAdapter(List<Wallet> wallets) {
            this.wallets = wallets;
        }

        @NonNull
        @Override
        public SummaryViewHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int i) {
            View v = LayoutInflater.from(viewGroup.getContext()).inflate(R.layout.layout_summary_item, viewGroup, false);
            return new SummaryViewHolder(v);
        }

        @Override
        public void onBindViewHolder(@NonNull SummaryViewHolder vh, int i) {
            Wallet wallet = wallets.get(i);
            Amount balance = wallet.getBalance();

            String currencyText = String.format("%s (%s)", wallet.getName(), wallet.getWalletManager().getNetwork());
            String balanceText = balance.toStringAsUnit(balance.getUnit(), null).or("---");

            vh.currencyView.setText(currencyText);
            vh.symbolView.setText(balanceText);
        }

        @Override
        public int getItemCount() {
            return wallets.size();
        }
    }
}
