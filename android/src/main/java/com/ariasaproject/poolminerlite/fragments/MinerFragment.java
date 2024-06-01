package com.ariasaproject.poolminerlite.fragments;

import static android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.SeekBar;

import androidx.appcompat.widget.AppCompatButton;
import androidx.appcompat.widget.AppCompatCheckBox;
import androidx.appcompat.widget.AppCompatEditText;
import androidx.appcompat.widget.AppCompatSeekBar;
import androidx.appcompat.widget.AppCompatTextView;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.Adapter;

import com.ariasaproject.poolminerlite.ConsoleItem;

public class MinerFragment extends Fragment {
    private final StringBuilder sb = new StringBuilder();
    private ConsoleItem.Lists logList;
    private int accepted_result, rejected_result;
    Adapter adpt;

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);
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
        ViewGroup root = inflater.inflate(R.layout.fragment_miner, container, false);
        // define section layout
        input_container = (ViewGroup) findViewById(R.id.input_container);
        status_container = (ViewGroup) findViewById(R.id.status_container);
        // define showInput
        tv_showInput = (AppCompatTextView) findViewById(R.id.show_userInput);
        // text status
        tv_s = (AppCompatTextView) findViewById(R.id.speed_tv);
        tv_ra = (AppCompatTextView) findViewById(R.id.resulta_tv);
        tv_rr = (AppCompatTextView) findViewById(R.id.resultr_tv);
        // button
        btn_startmine = (AppCompatButton) findViewById(R.id.button_startmine);
        btn_stopmine = (AppCompatButton) findViewById(R.id.button_stopmine);
        // editable
        et_serv = (AppCompatEditText) findViewById(R.id.server_et);
        et_port = (AppCompatEditText) findViewById(R.id.port_et);
        et_user = (AppCompatEditText) findViewById(R.id.user_et);
        et_pass = (AppCompatEditText) findViewById(R.id.password_et);
        sb_cpu = (AppCompatSeekBar) findViewById(R.id.cpuSeek);
        sb_cpu.setMax(Math.max(Runtime.getRuntime().availableProcessors() - 2, 1));
        final AppCompatTextView cuv = (AppCompatTextView) findViewById(R.id.cpu_usage_view);
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
        cb_screen_awake = (AppCompatCheckBox) findViewById(R.id.settings_checkBox_keepscreenawake);
        sb_cpu.setProgress(1); // main
        if (savedInstanceState != null) {
            logList = savedInstanceState.getParcelable(KEYBUNDLE_CONSOLE);
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
            logList = new ConsoleItem.Lists();
            SharedPreferences settings = getPreferences(Context.MODE_PRIVATE);
            et_serv.setText(settings.getString(PREF_URL, DEFAULT_URL));
            et_port.setText(String.valueOf(settings.getInt(PREF_PORT, DEFAULT_PORT)));
            et_user.setText(settings.getString(PREF_USER, DEFAULT_USER));
            et_pass.setText(settings.getString(PREF_PASS, DEFAULT_PASS));
            sb_cpu.setProgress(settings.getInt(PREF_CPU_USAGE, 1)); // old
        }
        final Window window = getWindow();
        cb_screen_awake.setChecked((window.getAttributes().flags & FLAG_KEEP_SCREEN_ON) != 0);
        cb_screen_awake.setOnCheckedChangeListener(
                (cb, check) -> {
                    if (check) window.addFlags(FLAG_KEEP_SCREEN_ON);
                    else window.clearFlags(FLAG_KEEP_SCREEN_ON);
                });
        // log Adapter
        final RecyclerView cv = (RecyclerView) findViewById(R.id.console_view);
        cv.setLayoutManager(new LinearLayoutManager(this, LinearLayoutManager.VERTICAL, false));
        adpt =
                new Adapter<ConsoleItemHolder>() {
                    final LayoutInflater inflater = LayoutInflater.from(MainActivity.this);

                    @Override
                    public ConsoleItemHolder onCreateViewHolder(ViewGroup parent, int viewType) {
                        View itemView = inflater.inflate(R.layout.console_item, parent, false);
                        return new ConsoleItemHolder(itemView);
                    }

                    @Override
                    public void onBindViewHolder(ConsoleItemHolder h, int p) {
                        final ConsoleItem ci = logList.get(p);
                        if (ci == null) {
                            h.root.setVisibility(View.GONE);
                        } else {
                            h.root.setVisibility(View.VISIBLE);
                            int id;
                            switch (ci.color) {
                                default:
                                case 0:
                                    id = R.color.console_text_debug;
                                    break;
                                case 1:
                                    id = R.color.console_text_info;
                                    break;
                                case 2:
                                    id = R.color.console_text_success;
                                    break;
                                case 3:
                                    id = R.color.console_text_warning;
                                    break;
                                case 4:
                                    id = R.color.console_text_error;
                                    break;
                            }
                            h.time.setTextColor(getResources().getColor(id));
                            h.msg.setTextColor(getResources().getColor(id));
                            h.desc.setTextColor(getResources().getColor(id));
                            h.time.setText(ci.time);
                            h.msg.setText(ci.msg);
                            h.desc.setText(ci.desc);
                            h.desc.setVisibility(View.GONE);
                            h.root.setOnClickListener(
                                    new View.OnClickListener() {
                                        @Override
                                        public void onClick(View v) {
                                            if (h.desc.getVisibility() == View.VISIBLE) {
                                                h.desc.setVisibility(View.GONE);
                                            } else {
                                                h.desc.setVisibility(View.VISIBLE);
                                            }
                                        }
                                    });
                        }
                    }

                    @Override
                    public int getItemCount() {
                        return ConsoleItem.Lists.SIZE;
                    }
                };
        cv.setAdapter(adpt);

        return root;
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
    }

    @Override
    public void onPause() {
        super.onPause();
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
        public AppCompatTextView desc;

        public ConsoleItemHolder(View itemView) {
            super(itemView);
            root = itemView.findViewById(R.id.console_item_root);
            time = itemView.findViewById(R.id.text1);
            msg = itemView.findViewById(R.id.text2);
            desc = itemView.findViewById(R.id.text3);
        }
    }
}
