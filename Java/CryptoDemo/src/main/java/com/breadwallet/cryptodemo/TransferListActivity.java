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

import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.PaymentProtocolRequestType;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferConfirmation;
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

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.List;

public class TransferListActivity extends AppCompatActivity implements DefaultSystemListener {

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

    private Wallet wallet;

    private Adapter transferAdapter;

    private ClipboardManager clipboardManager;

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

        Button payView = findViewById(R.id.pay_view);
        payView.setOnClickListener(v -> showPaymentMenu(TransferListActivity.this, wallet));

        Button sweepView = findViewById(R.id.sweep_view);
        sweepView.setOnClickListener(v -> TransferCreateSweepActivity.start(TransferListActivity.this, wallet));

        RecyclerView transfersView = findViewById(R.id.transfer_recycler_view);
        transfersView.addItemDecoration(new DividerItemDecoration(getApplicationContext(), DividerItemDecoration.VERTICAL));

        RecyclerView.LayoutManager transferLayoutManager = new LinearLayoutManager(this, LinearLayoutManager.VERTICAL, false);
        transfersView.setLayoutManager(transferLayoutManager);

        transferAdapter = new Adapter((transfer) -> TransferDetailsActivity.start(this, wallet, transfer));
        transfersView.setAdapter(transferAdapter);

        Toolbar toolbar = findViewById(R.id.toolbar_view);
        toolbar.setTitle(String.format("Wallet: %s", wallet.getName()));
        setSupportActionBar(toolbar);

        CoreCryptoApplication.getDispatchingSystemListener().addWalletListener(wallet, this);
        ApplicationExecutors.runOnUiExecutor(() -> {
            List<TransferViewModel> vms = TransferViewModel.create(wallet.getTransfers());
            runOnUiThread(() -> transferAdapter.set(vms));
        });
    }

    @Override
    protected void onDestroy() {
        CoreCryptoApplication.getDispatchingSystemListener().removeWalletListener(wallet, this);

        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_transfer_list, menu);
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
                showSyncToDepthMenu();
                return true;
            case R.id.action_toggle_mode:
                showSelectModeMenu();
                return true;
        }
        return false;
    }

    @Override
    public void handleTransferEvent(System system, WalletManager manager, Wallet wallet, Transfer transfer, TranferEvent event) {
        ApplicationExecutors.runOnUiExecutor(() -> event.accept(new DefaultTransferEventVisitor<Void>() {
            @Override
            public Void visit(TransferCreatedEvent event) {
                TransferViewModel vm = TransferViewModel.create(transfer);
                runOnUiThread(() -> transferAdapter.add(vm));
                return null;
            }

            @Override
            public Void visit(TransferChangedEvent event) {
                TransferViewModel vm = TransferViewModel.create(transfer);
                runOnUiThread(() -> transferAdapter.changed(vm));
                return null;
            }


            @Override
            public Void visit(TransferDeletedEvent event) {
                TransferViewModel vm = TransferViewModel.create(transfer);
                runOnUiThread(() -> transferAdapter.remove(vm));
                return null;
            }
        }));
    }

    private void connect() {
        ApplicationExecutors.runOnBlockingExecutor(() -> wallet.getWalletManager().connect(null));
    }

    private void disconnect() {
        ApplicationExecutors.runOnBlockingExecutor(() -> wallet.getWalletManager().disconnect());
    }

    private void showSyncToDepthMenu() {
        new AlertDialog.Builder(this)
                .setTitle("Sync Depth")
                .setSingleChoiceItems(new String[]{"From Last Confirmed Send", "From Last Trusted Block", "From Creation"},
                        -1,
                        (d, w) -> {
                            ApplicationExecutors.runOnBlockingExecutor(() -> {
                                switch (w) {
                                    case 0: wallet.getWalletManager().syncToDepth(WalletManagerSyncDepth.FROM_LAST_CONFIRMED_SEND); break;
                                    case 1: wallet.getWalletManager().syncToDepth(WalletManagerSyncDepth.FROM_LAST_TRUSTED_BLOCK); break;
                                    default: wallet.getWalletManager().syncToDepth(WalletManagerSyncDepth.FROM_CREATION); break;
                                }
                            });
                            d.dismiss();
                        })
                .show();
    }

    private void showSelectModeMenu() {
        ApplicationExecutors.runOnUiExecutor(() -> {
            WalletManager wm = wallet.getWalletManager();
            System system = wm.getSystem();
            Network network = wm.getNetwork();
            List<WalletManagerMode> modes = system.getSupportedWalletManagerModes(network);

            String[] itemTexts = new String[modes.size()];
            WalletManagerMode[] itemModes = new WalletManagerMode[modes.size()];
            for (int i = 0; i < itemTexts.length; i++) {
                itemModes[i] = modes.get(i);
                itemTexts[i] = itemModes[i].toString();
            }

            runOnUiThread(() -> {
                new AlertDialog.Builder(this)
                        .setTitle("Sync Mode")
                        .setSingleChoiceItems(itemTexts,
                                -1,
                                (d, w) -> {
                                    ApplicationExecutors.runOnBlockingExecutor(() -> wm.setMode(itemModes[w]));
                                    d.dismiss();
                                })
                        .show();
            });
        });
    }

    private void showPaymentMenu(Activity context, Wallet wallet) {
        new AlertDialog.Builder(this)
                .setTitle("Payment Protocol")
                .setSingleChoiceItems(new String[]{PaymentProtocolRequestType.BITPAY.name(), PaymentProtocolRequestType.BIP70.name()},
                        -1,
                        (d, w) -> {
                            switch (w) {
                                case 0: TransferCreatePaymentActivity.start(context, wallet, PaymentProtocolRequestType.BITPAY); break;
                                case 1: TransferCreatePaymentActivity.start(context, wallet, PaymentProtocolRequestType.BIP70); break;
                                default: break;
                            };
                            d.dismiss();
                        })
                .show();
    }

    private void copyReceiveAddress() {
        ApplicationExecutors.runOnUiExecutor(() -> {
            String value = wallet.getTarget().toString();
            clipboardManager.setPrimaryClip(ClipData.newPlainText("ReceiveAddress", value));

            String escapedValue = Html.escapeHtml(value);
            Spanned message = Html.fromHtml(String.format("Copied receive address <b>%s</b> to clipboard", escapedValue));

            runOnUiThread(() -> {
                    Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT).show();
            });
        });
    }

    private interface OnItemClickListener<T> {
        void onItemClick(T item);
    }

    private static class TransferViewModel implements Comparable<TransferViewModel> {

        private static final ThreadLocal<DateFormat> DATE_FORMAT = new ThreadLocal<DateFormat>() {
            @Override protected DateFormat initialValue() {
                return DateFormat.getDateTimeInstance(DateFormat.LONG, DateFormat.LONG);
            }
        };

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

        final Transfer transfer;
        final int transferHashCode;
        final TransferConfirmation confirmation;

        final String dateText;
        final String addressText;
        final String amountText;
        final String feeText;
        final String stateText;

        private TransferViewModel(Transfer transfer) {
            this.transfer = transfer;
            this.confirmation = transfer.getConfirmation().orNull();
            this.transferHashCode = transfer.hashCode();

            this.dateText = confirmation == null ? "<pending>" : DATE_FORMAT.get().format(confirmation.getConfirmationTime());
            this.addressText = transfer.getHash().transform(h -> String.format("Hash: %s", h)).or("<pending>");
            this.amountText = transfer.getAmountDirected().toString();
            this.feeText = String.format("Fee: %s", transfer.getFee());
            this.stateText = String.format("State: %s", transfer.getState());
        }

        @Override
        public int compareTo(TransferViewModel vm) {
            if (transfer.equals(vm.transfer)) return 0;

            if (confirmation != null && vm.confirmation != null) {
                int blockCompare = vm.confirmation.getBlockNumber().compareTo(confirmation.getBlockNumber());
                if (blockCompare != 0) return blockCompare;

                return vm.confirmation.getTransactionIndex().compareTo(confirmation.getTransactionIndex());

            } else if (confirmation != null) {
                return 1;
            } else if (vm.confirmation != null) {
                return -1;
            } else {
                return vm.transferHashCode - this.transferHashCode;
            }
        }

        boolean areContentsTheSame(TransferViewModel vm) {
            return dateText.equals(vm.dateText) &&
                    addressText.equals(vm.addressText) &&
                    amountText.equals(vm.amountText) &&
                    feeText.equals(vm.feeText) &&
                    stateText.equals(vm.stateText);
        }

        boolean areItemsTheSame(TransferViewModel vm) {
            return transfer.equals(vm.transfer);
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
                    return t1.areContentsTheSame(t2);
                }

                @Override
                public boolean areItemsTheSame(TransferViewModel t1, TransferViewModel t2) {
                    return t1.areItemsTheSame(t2);
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
            TransferViewModel vm = viewModels.get(i);

            vh.itemView.setOnClickListener(v -> listener.onItemClick(vm.transfer));
            vh.dateView.setText(vm.dateText);
            vh.amountView.setText(vm.amountText);
            vh.addressView.setText(vm.addressText);
            vh.feeView.setText(vm.feeText);
            vh.stateView.setText(vm.stateText);
        }

        @Override
        public int getItemCount() {
            return viewModels.size();
        }

        private void set(List<TransferViewModel> newTransfers) {
            viewModels.addAll(newTransfers);
        }

        private void add(TransferViewModel transfer) {
            viewModels.add(transfer);
        }

        private void remove(TransferViewModel transfer) {
            viewModels.remove(transfer);
        }

        private void changed(TransferViewModel transfer) {
            viewModels.add(transfer);
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
