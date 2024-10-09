package com.ariasaproject.poolminerlite.fragments;

import static android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.res.TypedArray;
import android.os.Bundle;
import android.os.IBinder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.SeekBar;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatButton;
import androidx.appcompat.widget.AppCompatCheckBox;
import androidx.appcompat.widget.AppCompatEditText;
import androidx.appcompat.widget.AppCompatSeekBar;
import androidx.appcompat.widget.AppCompatTextView;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.Adapter;

import com.ariasaproject.poolminerlite.ConsoleItem;
import com.ariasaproject.poolminerlite.MainApplication;
import com.ariasaproject.poolminerlite.MinerService;
import com.ariasaproject.poolminerlite.MinerViewModel;
import com.ariasaproject.poolminerlite.R;

public class MinerFragment extends Fragment implements ServiceConnection {
    private final StringBuilder sb = new StringBuilder();
    private int accepted_result, rejected_result;
    Adapter adpt;

    MinerService.LocalBinder dataService = null;

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);
        context.bindService(
                new Intent(context, MinerService.class), this, Context.BIND_AUTO_CREATE);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Nullable
    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View root = inflater.inflate(R.layout.fragment_miner, container, false);
        // define button click listener
        // define section layout
        input_container = (ViewGroup) root.findViewById(R.id.input_container);
        status_container = (ViewGroup) root.findViewById(R.id.status_container);
        // define showInput
        tv_showInput = (AppCompatTextView) root.findViewById(R.id.show_userInput);
        // text status
        tv_s = (AppCompatTextView) root.findViewById(R.id.speed_tv);
        tv_ra = (AppCompatTextView) root.findViewById(R.id.resulta_tv);
        tv_rr = (AppCompatTextView) root.findViewById(R.id.resultr_tv);
        // button
        btn_startmine = (AppCompatButton) root.findViewById(R.id.button_startmine);
        btn_startmine.setOnClickListener(
                v -> {
                    String[] dats = new String[4];
                    dats[0] = sb.append(et_serv.getText()).toString();
                    sb.setLength(0);
                    dats[1] = sb.append(et_user.getText()).toString();
                    sb.setLength(0);
                    dats[2] = sb.append(et_pass.getText()).toString();
                    sb.setLength(0);
                    dats[3] = "";

                    int[] dati = new int[4];
                    dati[0] = Integer.parseInt(sb.append(et_port.getText()).toString());
                    sb.setLength(0);
                    dati[1] = sb_cpu.getProgress();
                    dati[2] = 0;
                    dati[3] = 0;

                    tv_showInput.setText(
                            String.format(
                                    "server = %s:%d \nauth = %s:%s\nuse %d threads",
                                    dats[0], dati[0], dats[1], dats[2], dati[1]));
                    SharedPreferences.Editor editor =
                            getActivity().getPreferences(Context.MODE_PRIVATE).edit();
                    editor.putString(PREF_URL, dats[0]);
                    editor.putString(PREF_USER, dats[1]);
                    editor.putString(PREF_PASS, dats[2]);
                    editor.putInt(PREF_PORT, dati[0]);
                    editor.putInt(PREF_CPU_USAGE, dati[1]);
                    editor.commit();

                    dataService.StartMine(dats, dati);
                });
        btn_stopmine = (AppCompatButton) root.findViewById(R.id.button_stopmine);
        btn_stopmine.setOnClickListener(
                v -> {
                    dataService.StopMine();
                });
        // editable
        et_serv = (AppCompatEditText) root.findViewById(R.id.server_et);
        et_port = (AppCompatEditText) root.findViewById(R.id.port_et);
        et_user = (AppCompatEditText) root.findViewById(R.id.user_et);
        et_pass = (AppCompatEditText) root.findViewById(R.id.password_et);
        sb_cpu = (AppCompatSeekBar) root.findViewById(R.id.cpuSeek);
        sb_cpu.setMax(Math.max(Runtime.getRuntime().availableProcessors() - 2, 1));
        final AppCompatTextView cuv = (AppCompatTextView) root.findViewById(R.id.cpu_usage_view);
        sb_cpu.setOnSeekBarChangeListener(
                new SeekBar.OnSeekBarChangeListener() {
                    @Override
                    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                        cuv.setText(String.format("%d Thread Usage", progress));
                    }

                    @Override
                    public void onStartTrackingTouch(SeekBar seekBar) {}

                    @Override
                    public void onStopTrackingTouch(SeekBar seekBar) {}
                });
        // checkbox
        cb_screen_awake =
                (AppCompatCheckBox) root.findViewById(R.id.settings_checkBox_keepscreenawake);
        sb_cpu.setProgress(1); // main
        if (savedInstanceState != null) {
            CharSequence[] texts = savedInstanceState.getCharSequenceArray(KEYBUNDLE_TEXTS);
            tv_s.setText(texts[0]);
            tv_ra.setText(texts[1]);
            tv_rr.setText(texts[2]);
            tv_showInput.setText(texts[3]);
            et_serv.setText(texts[4]);
            et_port.setText(texts[5]);
            et_user.setText(texts[6]);
            et_pass.setText(texts[7]);
            int[] ints = savedInstanceState.getIntArray(KEYBUNDLE_INTS);
            sb_cpu.setProgress(ints[0]); // old
        } else {
            SharedPreferences settings = getActivity().getPreferences(Context.MODE_PRIVATE);
            et_serv.setText(settings.getString(PREF_URL, DEFAULT_URL));
            et_port.setText(String.valueOf(settings.getInt(PREF_PORT, DEFAULT_PORT)));
            et_user.setText(settings.getString(PREF_USER, DEFAULT_USER));
            et_pass.setText(settings.getString(PREF_PASS, DEFAULT_PASS));
            sb_cpu.setProgress(settings.getInt(PREF_CPU_USAGE, 1)); // old
        }
        final Window window = getActivity().getWindow();
        cb_screen_awake.setChecked((window.getAttributes().flags & FLAG_KEEP_SCREEN_ON) != 0);
        cb_screen_awake.setOnCheckedChangeListener(
                (cb, check) -> {
                    if (check) window.addFlags(FLAG_KEEP_SCREEN_ON);
                    else window.clearFlags(FLAG_KEEP_SCREEN_ON);
                });
        // log Adapter
        final RecyclerView cv = (RecyclerView) root.findViewById(R.id.console_view);
        cv.setLayoutManager(
                new LinearLayoutManager(getContext(), LinearLayoutManager.VERTICAL, false));
        adpt =
                new Adapter<ConsoleItemHolder>() {
                		final MainActivity ma = ((MainActivity)getActivity());
                    final LayoutInflater inflater = LayoutInflater.from(getActivity());
                    final TypedArray colors =
                            getContext()
                                    .getResources()
                                    .obtainTypedArray(R.array.console_text_colors);

                    @Override
                    public ConsoleItemHolder onCreateViewHolder(ViewGroup parent, int viewType) {
                        View itemView = inflater.inflate(R.layout.console_item, parent, false);
                        return new ConsoleItemHolder(itemView);
                    }

                    @Override
                    public void onBindViewHolder(ConsoleItemHolder h, int p) {
                        final ConsoleItem ci = ma.logList.get(p);
                        if (ci == null) {
                            h.root.setVisibility(View.GONE);
                        } else {
                            h.root.setVisibility(View.VISIBLE);
                            int color = colors.getColor((int)ci.color, 0);
                            h.time.setTextColor(color);
                            h.msg.setTextColor(color);
                            h.time.setText(ci.time);
                            h.msg.setText(ci.msg);
                        }
                    }

                    @Override
                    public int getItemCount() {
                        return ma.logList.getSize();
                    }
                };
        cv.setAdapter(adpt);
        return root;
    }

    int mainStateCurrent = -1;

    private void updateState(int state) {
        if (mainStateCurrent == state) return;
        MinerViewModel mvm = ((MainApplication) getActivity().getApplication()).getMinerViewModel();
        switch (state) {
            default:
            case MINE_STATE_NONE:
                switch (mainStateCurrent) {
                    default:
                    case MINE_STATE_NONE:
                        mvm.postLog(
                                1,
                                "Wellcome User!, This is the first log message that youp should"
                                        + " receive.");
                        break;
                    case MINE_STATE_ONSTART:
                        mvm.postLog(1, "Failed to start.");
                        break;
                    case MINE_STATE_RUNNING:
                        mvm.postLog(
                                1,
                                "Skipped State: This state was jumped from running to none state,"
                                        + " for now is imposible!");
                        break;
                    case MINE_STATE_ONSTOP:
                        mvm.postLog(1, "Service mining successful to stop!");
                        break;
                }
                btn_stopmine.setVisibility(View.GONE);
                btn_stopmine.setEnabled(false);
                btn_startmine.setVisibility(View.VISIBLE);
                btn_startmine.setEnabled(true);
                tv_s.setText("000,00 Hash/Sec");
                // enable all user Input
                input_container.setVisibility(View.VISIBLE);
                status_container.setVisibility(View.GONE);
                break;
            case MINE_STATE_ONSTART:
                switch (mainStateCurrent) {
                    default:
                    case MINE_STATE_NONE:
                        mvm.postLog(
                                1,
                                "Starting, Waiting for connecting, subscribing, authorizing and get"
                                        + " the first job.");
                        break;
                    case MINE_STATE_RUNNING:
                        mvm.postLog(
                                1,
                                "Skipped State: Jumped from running to onStart, that imposible!");
                        break;
                    case MINE_STATE_ONSTOP:
                        mvm.postLog(
                                1, "Skipped State: Jumped from onStop to onStart, that imposible!");
                        break;
                }
                btn_stopmine.setVisibility(View.GONE);
                btn_stopmine.setEnabled(false);
                btn_startmine.setVisibility(View.VISIBLE);
                btn_startmine.setEnabled(false);
                // disable all user Input
                input_container.setVisibility(View.GONE);
                status_container.setVisibility(View.VISIBLE);
                accepted_result = rejected_result = 0;
                tv_ra.setText("000");
                tv_rr.setText("000");
                break;
            case MINE_STATE_RUNNING:
                switch (mainStateCurrent) {
                    default:
                    case MINE_STATE_NONE:
                        mvm.postLog(
                                1,
                                "Skipped State: Jumped from none to running, is imposible from"
                                        + " now!");
                        break;
                    case MINE_STATE_ONSTART:
                        mvm.postLog(1, "Service mining successful to start!");
                        break;
                    case MINE_STATE_ONSTOP:
                        mvm.postLog(
                                1,
                                "Skipped State: Jumped from onStop to running, is imposible from"
                                        + " now!");
                        break;
                }
                accepted_result = rejected_result = 0;
                tv_ra.setText("000");
                tv_rr.setText("000");
                btn_stopmine.setVisibility(View.VISIBLE);
                btn_stopmine.setEnabled(true);
                btn_startmine.setVisibility(View.GONE);
                btn_startmine.setEnabled(false);
                // disable all user Input
                input_container.setVisibility(View.GONE);
                status_container.setVisibility(View.VISIBLE);
                break;
            case MINE_STATE_ONSTOP:
                switch (mainStateCurrent) {
                    default:
                    case MINE_STATE_NONE:
                        mvm.postLog(
                                1,
                                "Skipped State: Jumped from none to onStop, is imposible from now");
                        break;
                    case MINE_STATE_ONSTART:
                        mvm.postLog(
                                1,
                                "Skipped State: Jumped from onStart to onStop state, for now is"
                                        + " imposible!");
                        break;
                    case MINE_STATE_RUNNING:
                        mvm.postLog(1, "Service mining try to stop!");
                        break;
                }
                btn_stopmine.setVisibility(View.VISIBLE);
                btn_stopmine.setEnabled(false);
                btn_startmine.setVisibility(View.GONE);
                btn_startmine.setEnabled(false);
                // disable all user Input
                input_container.setVisibility(View.GONE);
                status_container.setVisibility(View.VISIBLE);
                break;
        }
        mainStateCurrent = state;
    }

    // Service Connection
    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
        dataService = (MinerService.LocalBinder) service;
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        dataService.StopMine();
        dataService = null;
    }

    @Override
    public void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);
        CharSequence[] texts = new CharSequence[8];
        texts[0] = tv_s.getText();
        texts[1] = tv_ra.getText();
        texts[2] = tv_rr.getText();
        texts[3] = tv_showInput.getText();
        texts[4] = et_serv.getText();
        texts[5] = et_port.getText();
        texts[6] = et_user.getText();
        texts[7] = et_pass.getText();
        outState.putCharSequenceArray(KEYBUNDLE_TEXTS, texts);
        int[] ints = new int[1];
        ints[0] = sb_cpu.getProgress();
        outState.putIntArray(KEYBUNDLE_INTS, ints);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
    }

    @Override
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onResume() {
        super.onResume();
        MinerViewModel mvm = ((MainApplication) getActivity().getApplication()).getMinerViewModel();
        float speedHash = mvm.getSpeed();
        if (speedHash > 0.0f) {
            int unit_step = 0;
            while (unit_step < UnitHash.length && speedHash > 1000.0f) {
                speedHash /= 1000.0f;
                unit_step++;
            }
            tv_s.setText(String.format("%.3f %s/Sec", speedHash, UnitHash[unit_step]));
        }
        updateState(mvm.getState());
        mvm.registerObs(
                getActivity(),
                (speed) -> {
                    int unit_step = 0;
                    while (unit_step < UnitHash.length && speed > 1000.0f) {
                        speed /= 1000.0f;
                        unit_step++;
                    }
                    tv_s.setText(String.format("%.3f %s/Sec", speed, UnitHash[unit_step]));
                },
                (result) -> {
                    if (result) {
                        tv_ra.setText(String.format("%03d", ++accepted_result));
                    } else {
                        tv_rr.setText(String.format("%03d", ++rejected_result));
                    }
                },
                (state) -> updateState(state),
                (log) -> adpt.notifyDataSetChanged() );
    }

    @Override
    public void onPause() {
        super.onPause();
        ((MainApplication) getActivity().getApplication()).getMinerViewModel().unregisterObs();
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onDetach() {
        getContext().unbindService(this);
        super.onDetach();
    }

    ViewGroup input_container, status_container;
    AppCompatTextView tv_s, tv_ra, tv_rr, tv_info;
    AppCompatTextView tv_showInput;
    AppCompatEditText et_serv, et_port, et_user, et_pass;
    AppCompatButton btn_startmine, btn_stopmine;
    AppCompatSeekBar sb_cpu;
    AppCompatCheckBox cb_screen_awake;

    private class ConsoleItemHolder extends RecyclerView.ViewHolder {
        public ConstraintLayout root;
        public AppCompatTextView time;
        public AppCompatTextView msg;

        public ConsoleItemHolder(View itemView) {
            super(itemView);
            root = (ConstraintLayout) itemView.findViewById(R.id.console_item_root);
            time = (AppCompatTextView) itemView.findViewById(R.id.text1);
            msg = (AppCompatTextView) itemView.findViewById(R.id.text2);
        }
    }

    // key bundles in temporary safe
    private static final String KEYBUNDLE_TEXTS = "bundle_texts";
    private static final String KEYBUNDLE_INTS = "bundle_ints";

    public static final int MINE_STATE_NONE = 0;
    public static final int MINE_STATE_ONSTART = 1;
    public static final int MINE_STATE_RUNNING = 2;
    public static final int MINE_STATE_ONSTOP = 3;

    // preferences name
    public static final String PREF_URL = "URL";
    public static final String PREF_PORT = "PORT";
    public static final String PREF_USER = "USER";
    public static final String PREF_PASS = "PASS";
    public static final String PREF_CPU_USAGE = "CPU_USAGE";
    public static final String PREF_CONSOLE = "CONSOLE";

    // default value
    public static final String DEFAULT_URL = "us2.litecoinpool.org";
    public static final String DEFAULT_USER = "Ariasa.test";
    public static final String DEFAULT_PASS = "1234";

    public static final int DEFAULT_PORT = 8080;

    // unit hash
    public static final String[] UnitHash =
            new String[] {
                "Hash", "kHash", "MHash", "GHash", "THash", "PHash", "EHash", "ZHash", "YHash",
            };
}
