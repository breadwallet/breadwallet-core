package com.breadwallet.cryptodemo;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferConfirmation;
import com.breadwallet.crypto.TransferHash;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.transfer.TransferChangedEvent;
import com.breadwallet.crypto.events.transfer.TransferCreatedEvent;
import com.breadwallet.crypto.events.transfer.TransferDeletedEvent;
import com.breadwallet.crypto.events.transfer.TransferEventVisitor;
import com.breadwallet.crypto.events.transfer.TransferListener;
import com.google.common.base.Optional;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import javax.annotation.Nullable;

public class TransferListActivity extends AppCompatActivity implements TransferListener {

    private static final DateFormat DATE_FORMAT = DateFormat.getDateTimeInstance(DateFormat.LONG, DateFormat.LONG);

    private static final String EXTRA_WALLET_NAME = "com.breadwallet.cryptodemo,TransferListActivity.EXTRA_WALLET_NAME";

    private static final Comparator<Transfer> OLDEST_FIRST_COMPARATOR = (o1, o2) -> {
        Optional<TransferConfirmation> oc1 = o1.getConfirmation();
        Optional<TransferConfirmation> oc2 = o2.getConfirmation();

        if (oc1.isPresent() && oc2.isPresent()) {
            TransferConfirmation c1 = oc1.get();
            TransferConfirmation c2 = oc2.get();

            int blockCompare = c1.getBlockNumber().compareTo(c2.getBlockNumber());
            int indexCompare = c1.getTransactionIndex().compareTo(c2.getTransactionIndex());
            return (blockCompare != 0 ? blockCompare : indexCompare);

        } else if (oc1.isPresent()) {
            return -1;
        } else if (oc2.isPresent()) {
            return 1;
        } else {
            return o1.hashCode() - o2.hashCode();
        }
    };

    private static final Comparator<Transfer> NEWEST_FIRST_COMPARATOR = Collections.reverseOrder(OLDEST_FIRST_COMPARATOR);

    private static final Comparator<Transfer> DEFAULT_COMPARATOR = NEWEST_FIRST_COMPARATOR;

    public static void start(Activity callerActivity, Wallet wallet) {
        Intent intent = new Intent(callerActivity, TransferListActivity.class);
        intent.putExtra(EXTRA_WALLET_NAME, wallet.getName());
        callerActivity.startActivity(intent);
    }

    @Nullable
    private static Wallet getWallet(Intent intent) {
        String walletName = intent.getStringExtra(EXTRA_WALLET_NAME);
        for(Wallet wallet: CoreCryptoApplication.getSystem().getWallets()) {
            if (wallet.getName().equals(walletName)) {
                return wallet;
            }
        }
        return null;
    }

    private Wallet wallet;
    private List<Transfer> transfers = new ArrayList<>();

    private Button createView;
    private RecyclerView transfersView;
    private RecyclerView.Adapter transferAdapter;
    private RecyclerView.LayoutManager transferLayoutManager;
    private Comparator<Transfer> transferComparator;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_transfer_list);

        CoreCryptoApplication.initialize(this);

        wallet = getWallet(getIntent());
        if (null == wallet) {
            finish();
            return;
        }

        transferComparator = DEFAULT_COMPARATOR;

        createView = findViewById(R.id.create_view);
        createView.setOnClickListener(v -> TransferCreateActivity.start(TransferListActivity.this, wallet));

        transfersView = findViewById(R.id.transfer_recycler_view);
        transfersView.addItemDecoration(new DividerItemDecoration(getApplicationContext(), DividerItemDecoration.VERTICAL));

        transferLayoutManager = new LinearLayoutManager(this);
        transfersView.setLayoutManager(transferLayoutManager);

        transferAdapter = new Adapter(transfers, (transfer) -> TransferDetailsActivity.start(this, wallet, transfer));
        transfersView.setAdapter(transferAdapter);

        ActionBar actionBar = getSupportActionBar();
        actionBar.setTitle(String.format("Wallet: %s", wallet.getName()));
    }

    @Override
    protected void onResume() {
        super.onResume();

        CoreCryptoApplication.getListener().addListener(this);

        transfers.clear();
        transfers.addAll(getTransfers());
        transferAdapter.notifyDataSetChanged();
    }

    @Override
    protected void onPause() {
        super.onPause();

        CoreCryptoApplication.getListener().removeListener(this);
    }

    @Override
    public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
        runOnUiThread(() -> {
            if (manager.equals(this.wallet.getWalletManager()) && wallet.equals(this.wallet)) {
                event.accept(new TransferEventVisitor<Void>() {
                    @Override
                    public Void visit(TransferChangedEvent event) {
                        List<? extends Transfer> changedTransfers = getTransfers();
                        int changedIndex = Collections.binarySearch(changedTransfers, transfer, transferComparator);
                        int transfersIndex = Collections.binarySearch(transfers, transfer, transferComparator);
                        if (changedIndex != transfersIndex) {
                            transfers.clear();
                            transfers.addAll(changedTransfers);
                            transferAdapter.notifyDataSetChanged();
                        } else if (transfersIndex >= 0) {
                            transferAdapter.notifyItemChanged(transfersIndex);
                        }
                        return null;
                    }

                    @Override
                    public Void visit(TransferCreatedEvent event) {
                        int index = Collections.binarySearch(transfers, transfer, transferComparator);
                        if (index < 0) {
                            index = Math.abs(index) - 1;
                            transfers.add(index, transfer);
                            transferAdapter.notifyItemInserted(index);
                        }
                        return null;
                    }

                    @Override
                    public Void visit(TransferDeletedEvent event) {
                        int index = Collections.binarySearch(transfers, transfer, transferComparator);
                        if (index >= 0) {
                            transfers.remove(index);
                            transferAdapter.notifyItemRemoved(index);
                        }
                        return null;
                    }
                });
            }
        });
    }

    private List<? extends Transfer> getTransfers() {
        List<? extends Transfer> walletTransfers = wallet.getTransfers();
        Collections.sort(walletTransfers, transferComparator);
        return walletTransfers;
    }

    private interface OnItemClickListener<T> {
        void onItemClick(T item);
    }

    private static class Adapter extends RecyclerView.Adapter<ViewHolder> {

        private List<Transfer> transfers;
        private OnItemClickListener<Transfer> listener;

        Adapter(List<Transfer> transfers, OnItemClickListener<Transfer> listener) {
            this.transfers = transfers;
            this.listener = listener;
        }

        @NonNull
        @Override
        public ViewHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int i) {
            View v = LayoutInflater.from(viewGroup.getContext()).inflate(R.layout.layout_transfer_item, viewGroup, false);
            return new ViewHolder(v);
        }

        @Override
        public void onBindViewHolder(@NonNull ViewHolder vh, int i) {
            Transfer transfer = transfers.get(i);

            String dateText = transfer.getConfirmation()
                    .transform((c) -> DATE_FORMAT.format(c.getConfirmationTime())).or("<pending>");

            String addressText = transfer.getHash().transform(TransferHash::toString).or("<pending>");
            addressText = String.format("Hash: %s", addressText);

            String amountText = transfer.getAmountDirected().toString();
            String feeText = String.format("Fee: %s", transfer.getFee());

            String stateText = String.format("State: %s", transfer.getState());

            vh.itemView.setOnClickListener(v -> listener.onItemClick(transfer));
            vh.dateView.setText(dateText);
            vh.amountView.setText(amountText);
            vh.addressView.setText(addressText);
            vh.feeView.setText(feeText);
            vh.stateView.setText(stateText);
        }

        @Override
        public int getItemCount() {
            return transfers.size();
        }
    }

    private static class ViewHolder extends RecyclerView.ViewHolder {

        public TextView dateView;
        public TextView amountView;
        public TextView addressView;
        public TextView feeView;
        public TextView stateView;

        public ViewHolder(@NonNull View view) {
            super(view);

            dateView = view.findViewById(R.id.item_date);
            amountView = view.findViewById(R.id.item_amount);
            addressView = view.findViewById(R.id.item_address);
            feeView = view.findViewById(R.id.item_fee);
            stateView = view.findViewById(R.id.item_state);
        }
    }
}
