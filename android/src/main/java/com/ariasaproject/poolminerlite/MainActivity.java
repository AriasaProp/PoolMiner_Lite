package com.ariasaproject.poolminerlite;

import static android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.os.PowerManager;
import android.provider.Settings;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.SeekBar;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatButton;
import androidx.appcompat.widget.AppCompatCheckBox;
import androidx.appcompat.widget.AppCompatEditText;
import androidx.appcompat.widget.AppCompatSeekBar;
import androidx.appcompat.widget.AppCompatTextView;
import androidx.core.text.HtmlCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.Adapter;

public class MainActivity extends AppCompatActivity implements ServiceConnection {
    static {
        System.loadLibrary("ext");
    }

    MinerService.LocalBinder dataService = null;
    private final StringBuilder sb = new StringBuilder();
    private ConsoleItem.Lists logList;
    private int accepted_result, rejected_result;
    Adapter adpt;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // define section layout
        input_container = (ViewGroup) findViewById(R.id.input_container);
        status_container = (ViewGroup) findViewById(R.id.status_container);
        // define showInput
        tv_showInput = (AppCompatTextView) findViewById(R.id.show_userInput);
        // text status
        AppCompatTextView smpl = (AppCompatTextView) findViewById(R.id.console_msg);
        String htmlContent = "<html><body><table>" +
                        "<tr><th>Time</th><th>Messages</th></tr>" +
                        "<tr><td>9:00 AM</td><td>Hi there!</td></tr>" +
                        "<tr><td>10:30 AM</td><td>How are you?</td></tr>" +
                        "<tr><td>11:30 AM</td><td>How are you?</td></tr>" +
                    "</table></body></html>";
        smpl.setText(HtmlCompat.fromHtml(htmlContent,HtmlCompat.FROM_HTML_MODE_LEGACY));
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
        adpt = new Adapter<ConsoleItemHolder>() {
            final LayoutInflater inflater = LayoutInflater.from(MainActivity.this);

            @Override
            public ConsoleItemHolder onCreateViewHolder(ViewGroup parent, int viewType) {
                View itemView = inflater.inflate(R.layout.console_item, parent, false);
                return new ConsoleItemHolder(itemView);
            }

            @Override
            public void onBindViewHolder(ConsoleItemHolder holder, int position) {
                holder.bindLog(logList.get(position));
            }

            @Override
            public int getItemCount() {
                return logList.SIZE;
            }
        };
        cv.setAdapter(adpt);
        // check feature
        checkBatteryOptimizations();
        bindService(new Intent(this, MinerService.class), this, Context.BIND_AUTO_CREATE);
    }

    private static final int REQUEST_BATTERY_OPTIMIZATIONS = 1001;

    private void checkBatteryOptimizations() {
        PowerManager powerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
        if (powerManager != null
                && !powerManager.isIgnoringBatteryOptimizations(getPackageName())) {
            // Jika izin tidak diizinkan, tampilkan dialog untuk meminta izin
            Intent intent = new Intent();
            intent.setAction(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS);
            intent.setData(Uri.parse("package:" + getPackageName()));
            startActivityForResult(intent, REQUEST_BATTERY_OPTIMIZATIONS);
        }
    }

    private void updateState(int state) {
        if (mainStateCurrent == state) return;
        switch (state) {
            default:
            case MINE_STATE_NONE:
                switch (mainStateCurrent) {
                    default:
                    case MINE_STATE_NONE:
                        logList.add(1, "Wellcome User!");
                        break;
                    case MINE_STATE_ONSTART:
                        logList.add(1, "Service mining failed to start!");
                        break;
                    case MINE_STATE_RUNNING:
                        logList.add(1, "Jumped from running to none state, for now is imposible!");
                        break;
                    case MINE_STATE_ONSTOP:
                        logList.add(1, "Service mining successful to stop!");
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
                        logList.add(1, "Service mining starting!");
                        break;
                    case MINE_STATE_RUNNING:
                        logList.add(1, "Jumped from running to onStart, is imposible from now!");
                        break;
                    case MINE_STATE_ONSTOP:
                        logList.add(1, "Jumped from onStop to onStart, is imposible for now!");
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
                        logList.add(1, "Jumped from none to running, is imposible from now!");
                        break;
                    case MINE_STATE_ONSTART:
                        logList.add(1, "Service mining successful to start!");
                        break;
                    case MINE_STATE_ONSTOP:
                        logList.add(1, "Jumped from onStop to running, is imposible from now!");
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
                        logList.add(1, "Jumped from none to onStop, is imposible from now");
                        break;
                    case MINE_STATE_ONSTART:
                        logList.add(
                                1, "Jumped from onStart to onStop state, for now is imposible!");
                        break;
                    case MINE_STATE_RUNNING:
                        logList.add(1, "Service mining try to stop!");
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
        adpt.notifyDataSetChanged();
        mainStateCurrent = state;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        switch (requestCode) {
            default:
                break;
            case REQUEST_BATTERY_OPTIMIZATIONS:
                PowerManager powerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
                if (powerManager != null
                        && powerManager.isIgnoringBatteryOptimizations(getPackageName())) {
                    // Izin diberikan, lanjutkan dengan operasi normal
                } else {
                    // Izin ditolak, berikan pengguna instruksi lebih lanjut atau tindakan yang
                    // sesuai
                }
                break;
        }
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
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putParcelable(KEYBUNDLE_CONSOLE, logList);
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
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        if (savedInstanceState != null) {
            logList = savedInstanceState.getParcelable(KEYBUNDLE_CONSOLE);
            adpt.notifyDataSetChanged();
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
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        MinerViewModel mvm = ((MainApplication) getApplication()).getMinerViewModel();
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
                this,
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
                (log) -> {
                    logList.add(log);
                    adpt.notifyDataSetChanged();
                });
    }

    @Override
    protected void onPause() {
        super.onPause();
        ((MainApplication) getApplication()).getMinerViewModel().unregisterObs();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unbindService(this);
    }

    int mainStateCurrent = -1;

    // button function
    public void toStartMining(View v) {
        updateState(MINE_STATE_ONSTART);
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
        SharedPreferences.Editor editor = getPreferences(Context.MODE_PRIVATE).edit();
        editor.putString(PREF_URL, dats[0]);
        editor.putString(PREF_USER, dats[1]);
        editor.putString(PREF_PASS, dats[2]);
        editor.putInt(PREF_PORT, dati[0]);
        editor.putInt(PREF_CPU_USAGE, dati[1]);
        editor.commit();

        dataService.StartMine(dats, dati);
    }

    public void toStopMining(View v) {
        updateState(MINE_STATE_ONSTOP);
        dataService.StopMine();
    }

    private class ConsoleItemHolder extends RecyclerView.ViewHolder {
        private AppCompatTextView time;
        private AppCompatTextView msg;

        public ConsoleItemHolder(View itemView) {
            super(itemView);
            time = itemView.findViewById(R.id.text1);
            msg = itemView.findViewById(R.id.text2);
        }

        public void bindLog(ConsoleItem ci) {
            if (ci == null) {
                time.setTextColor(getResources().getColor(R.color.console_text_debug));
                msg.setTextColor(getResources().getColor(R.color.console_text_debug));
                time.setText("");
                msg.setText("");
            } else {
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
                time.setTextColor(getResources().getColor(id));
                msg.setTextColor(getResources().getColor(id));
                time.setText(ci.time);
                msg.setText(ci.msg);
            }
        }
    }

    ViewGroup input_container, status_container;
    AppCompatTextView tv_s, tv_ra, tv_rr, tv_info;
    AppCompatTextView tv_showInput;
    AppCompatEditText et_serv, et_port, et_user, et_pass;
    AppCompatButton btn_startmine, btn_stopmine;
    AppCompatSeekBar sb_cpu;
    AppCompatCheckBox cb_screen_awake;

    // key bundles in temporary safe
    private static final String KEYBUNDLE_CONSOLE = "bundle_console";
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
    public static final String DEFAULT_URL = "stratum+tcp://us2.litecoinpool.org";
    public static final String DEFAULT_USER = "Ariasa.test";
    public static final String DEFAULT_PASS = "1234";

    public static final int DEFAULT_PORT = 3333;

    // unit hash
    public static final String[] UnitHash =
            new String[] {
                "Hash", "kHash", "MHash", "GHash", "THash", "PHash", "EHash", "ZHash", "YHash",
            };
}
