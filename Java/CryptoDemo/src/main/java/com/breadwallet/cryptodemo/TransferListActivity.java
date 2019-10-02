/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.cryptodemo;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
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
import android.text.Html;
import android.text.Spanned;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferConfirmation;
import com.breadwallet.crypto.TransferHash;
import com.breadwallet.crypto.TransferState;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.WalletManagerSyncDepth;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.events.system.DefaultSystemListener;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.transfer.DefaultTransferEventVisitor;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.transfer.TransferChangedEvent;
import com.breadwallet.crypto.events.transfer.TransferCreatedEvent;
import com.breadwallet.crypto.events.transfer.TransferDeletedEvent;
import com.google.common.base.Optional;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class TransferListActivity extends AppCompatActivity {

    private static final String EXTRA_WALLET_NAME = "com.breadwallet.cryptodemo.TransferListActivity.EXTRA_WALLET_NAME";

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

    private static WalletManagerMode getNextMode(WalletManager wm) {
        Network net = wm.getNetwork();
        System sys = wm.getSystem();

        List<WalletManagerMode> modes = sys.getSupportedWalletManagerModes(net);
        WalletManagerMode mode = wm.getMode();
        mode = modes.get((modes.indexOf(mode) + 1) % modes.size());
        return mode;
    }

    private Wallet wallet;

    private Adapter transferAdapter;

    private ClipboardManager clipboardManager;

    private final SystemListener walletListener = new DefaultSystemListener() {
        @Override
        public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
            event.accept(new DefaultTransferEventVisitor<Void>() {
                @Override
                public Void visit(TransferCreatedEvent event) {
                    TransferViewModel vm = TransferViewModel.create(transfer);
                    runOnUiThread(() -> {
                        transferAdapter.add(vm);
                    });
                    return null;
                }

                @Override
                public Void visit(TransferChangedEvent event) {
                    TransferViewModel vm = TransferViewModel.create(transfer);
                    runOnUiThread(() -> {
                        transferAdapter.changed(vm);
                    });
                    return null;
                }


                @Override
                public Void visit(TransferDeletedEvent event) {
                    TransferViewModel vm = TransferViewModel.create(transfer);
                    runOnUiThread(() -> {
                        transferAdapter.remove(vm);
                    });
                    return null;
                }
            });
        }
    };

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

        clipboardManager = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);

        Button sendView = findViewById(R.id.send_view);
        sendView.setOnClickListener(v -> TransferCreateSendActivity.start(TransferListActivity.this, wallet));

        Button recvView = findViewById(R.id.receive_view);
        recvView.setOnClickListener(v -> copyReceiveAddress());

        Button sweepView = findViewById(R.id.sweep_view);
        sweepView .setOnClickListener(v -> TransferCreateSweepActivity.start(TransferListActivity.this, wallet));

        RecyclerView transfersView = findViewById(R.id.transfer_recycler_view);
        transfersView.addItemDecoration(new DividerItemDecoration(getApplicationContext(), DividerItemDecoration.VERTICAL));

        RecyclerView.LayoutManager transferLayoutManager = new LinearLayoutManager(this, LinearLayoutManager.VERTICAL, true);
        transfersView.setLayoutManager(transferLayoutManager);

        transferAdapter = new Adapter((transfer) -> TransferDetailsActivity.start(this, wallet, transfer));
        transfersView.setAdapter(transferAdapter);

        Toolbar toolbar = findViewById(R.id.toolbar_view);
        toolbar.setTitle(String.format("Wallet: %s", wallet.getName()));
        setSupportActionBar(toolbar);
    }

    @Override
    protected void onResume() {
        super.onResume();

        CoreCryptoApplication.getDispatchingSystemListener().addWalletListener(wallet, walletListener);
        transferAdapter.set(TransferViewModel.create(wallet.getTransfers()));
    }

    @Override
    protected void onPause() {
        super.onPause();

        CoreCryptoApplication.getDispatchingSystemListener().removeWalletListener(wallet, walletListener);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_transfer_list, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        WalletManager wm = wallet.getWalletManager();
        menu.findItem(R.id.action_toggle_mode).setTitle("Switch to " + getNextMode(wm).name());

        return super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_connect:
                wallet.getWalletManager().connect(null);
                return true;
            case R.id.action_sync:
                new AlertDialog.Builder(this)
                        .setSingleChoiceItems(new String[]{"From Last Confirmed Send", "From Last Trusted Block", "From Creation"},
                                -1,
                                (dialog, which) -> {
                                    switch (which) {
                                        case 0: wallet.getWalletManager().syncToDepth(WalletManagerSyncDepth.FROM_LAST_CONFIRMED_SEND); break;
                                        case 1: wallet.getWalletManager().syncToDepth(WalletManagerSyncDepth.FROM_LAST_TRUSTED_BLOCK); break;
                                        default: wallet.getWalletManager().syncToDepth(WalletManagerSyncDepth.FROM_CREATION); break;
                                    }
                                    dialog.dismiss();
                                })
                        .show();
                return true;
            case R.id.action_disconnect:
                wallet.getWalletManager().disconnect();
                return true;
            case R.id.action_toggle_mode:
                WalletManager wm = wallet.getWalletManager();
                wm.setMode(getNextMode(wm));
                return true;
        }
        return false;
    }

    private void copyReceiveAddress() {
        String value = wallet.getTarget().toString();
        clipboardManager.setPrimaryClip(ClipData.newPlainText("ReceiveAddress", value));

        String escapedValue = Html.escapeHtml(value);
        Spanned message = Html.fromHtml(String.format("Copied receive address <b>%s</b> to clipboard", escapedValue));

        Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT).show();
    }

    private interface OnItemClickListener<T> {
        void onItemClick(T item);
    }

    private static class TransferViewModel implements Comparable<TransferViewModel> {

        private static List<TransferViewModel> create(List<? extends Transfer> transfers) {
            List<TransferViewModel> vms = new ArrayList<>(transfers.size());
            for (Transfer t: transfers) {
                vms.add(create(t));
            }
            return vms;
        }

        private static TransferViewModel create(Transfer transfer) {
            return new TransferViewModel(transfer);
        }

        private final Transfer transfer;
        private final TransferConfirmation confirmation;
        private final TransferHash hash;
        private final TransferState state;
        private final Amount amountDirected;
        private final Amount fee;

        private TransferViewModel(Transfer transfer) {
            this.transfer = transfer;
            this.confirmation = transfer.getConfirmation().orNull();
            this.hash = transfer.getHash().orNull();
            this.state = transfer.getState();
            this.amountDirected = transfer.getAmountDirected();
            this.fee = transfer.getFee();
        }

        private Optional<TransferConfirmation> getConfirmation() {
            return Optional.fromNullable(confirmation);
        }

        private Optional<TransferHash> getHash() {
            return Optional.fromNullable(hash);
        }

        private Amount getAmountDirected() {
            return amountDirected;
        }

        private Amount getFee() {
            return fee;
        }

        private TransferState getState() {
            return state;
        }

        private Transfer getTransfer() {
            return transfer;
        }

        @Override
        public int compareTo(TransferViewModel vm2) {
            Transfer t1 = this.getTransfer();
            Transfer t2 = vm2.getTransfer();

            Optional<TransferConfirmation> oc1 = t1.getConfirmation();
            Optional<TransferConfirmation> oc2 = t2.getConfirmation();

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
                return t1.hashCode() - t2.hashCode();
            }
        }

        @Override
        public boolean equals(Object object) {
            if (this == object) {
                return true;
            }

            if (!(object instanceof TransferViewModel)) {
                return false;
            }

            TransferViewModel that = (TransferViewModel) object;
            return Objects.equals(confirmation, that.confirmation) &&
                    Objects.equals(hash, that.hash) &&
                    state.equals(that.state) &&
                    amountDirected.equals(that.amountDirected) &&
                    fee.equals(that.fee);
        }

        @Override
        public int hashCode() {
            return Objects.hash(confirmation, hash, state, amountDirected, fee);
        }
    }

    private static class Adapter extends RecyclerView.Adapter<ViewHolder> {

        private final OnItemClickListener<Transfer> listener;
        private final SortedList<TransferViewModel> viewModels;

        Adapter(OnItemClickListener<Transfer> listener) {
            this.listener = listener;
            this.viewModels = new SortedList<>(TransferViewModel.class, new SortedListAdapterCallback<TransferViewModel>(this) {
                @Override
                public int compare(TransferViewModel t1, TransferViewModel t2) {
                    return t1.compareTo(t2);
                }

                @Override
                public boolean areContentsTheSame(TransferViewModel t1, TransferViewModel t2) {
                    return t1.equals(t2);
                }

                @Override
                public boolean areItemsTheSame(TransferViewModel t1, TransferViewModel t2) {
                    return t1.getTransfer().equals(t2.getTransfer());
                }
            });
        }

        @NonNull
        @Override
        public ViewHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int i) {
            View v = LayoutInflater.from(viewGroup.getContext()).inflate(R.layout.layout_transfer_item, viewGroup, false);
            return new ViewHolder(v);
        }

        @Override
        public void onBindViewHolder(@NonNull ViewHolder vh, int i) {
            TransferViewModel transfer = viewModels.get(i);

            String dateText = transfer.getConfirmation()
                    .transform((c) -> DateFormat.getDateTimeInstance(DateFormat.LONG, DateFormat.LONG).format(c.getConfirmationTime()))
                    .or("<pending>");

            String addressText = transfer.getHash().transform(TransferHash::toString).or("<pending>");
            addressText = String.format("Hash: %s", addressText);

            String amountText = transfer.getAmountDirected().toString();
            String feeText = String.format("Fee: %s", transfer.getFee());

            String stateText = String.format("State: %s", transfer.getState());

            vh.itemView.setOnClickListener(v -> listener.onItemClick(transfer.getTransfer()));
            vh.dateView.setText(dateText);
            vh.amountView.setText(amountText);
            vh.addressView.setText(addressText);
            vh.feeView.setText(feeText);
            vh.stateView.setText(stateText);
        }

        @Override
        public int getItemCount() {
            return viewModels.size();
        }

        private void set(List<TransferViewModel> newTransfers) {
            viewModels.replaceAll(newTransfers);
        }

        private void add(TransferViewModel transfer) {
            viewModels.add(transfer);
        }

        private void remove(TransferViewModel transfer) {
            viewModels.remove(transfer);
        }

        private void changed(TransferViewModel transfer) {
            int index = viewModels.indexOf(transfer);
            if (index != -1) {
                viewModels.updateItemAt(index, transfer);
            }
        }
    }

    private static class ViewHolder extends RecyclerView.ViewHolder {

        private TextView dateView;
        private TextView amountView;
        private TextView addressView;
        private TextView feeView;
        private TextView stateView;

        private ViewHolder(@NonNull View view) {
            super(view);

            dateView = view.findViewById(R.id.item_date);
            amountView = view.findViewById(R.id.item_amount);
            addressView = view.findViewById(R.id.item_address);
            feeView = view.findViewById(R.id.item_fee);
            stateView = view.findViewById(R.id.item_state);
        }
    }
}
