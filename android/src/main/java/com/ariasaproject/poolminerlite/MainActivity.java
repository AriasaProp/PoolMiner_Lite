package com.ariasaproject.poolminerlite;

import android.app.ActivityManager;
import android.app.ActivityManager.RunningServiceInfo;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.IBinder;
import android.os.Looper;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.PowerManager;
import android.os.StrictMode;
import android.provider.Settings;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.SeekBar;

import static android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatButton;
import androidx.appcompat.widget.AppCompatCheckBox;
import androidx.appcompat.widget.AppCompatEditText;
import androidx.appcompat.widget.AppCompatSeekBar;
import androidx.appcompat.widget.AppCompatTextView;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.Adapter;

import java.text.DateFormat;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;

public class MainActivity extends AppCompatActivity implements ServiceConnection {
    static {
        System.loadLibrary("ext");
    }
 
    MinerService.LocalBinder dataService = null;
    private final StringBuilder sb = new StringBuilder();
    private static final int MAX_LOG_COUNT = 50;
    private ArrayList<ConsoleItem> logList;
    Adapter adpt;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        Intent intent = new Intent(this, MinerService.class);
        bindService(intent, this, Context.BIND_AUTO_CREATE);
        boolean serviceWasRunning = false;
        for (RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (MinerService.class.getName().equals(service.service.getClassName())) {
                serviceWasRunning = true;
                break;
            }
        }
        if (!serviceWasRunning) {
            startService(intent);
        }
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
        sb_cpu.setMax(Math.max(Runtime.getRuntime().availableProcessors(), 1));
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
        if (savedInstanceState != null) {
            logList = savedInstanceState.getParcelableArrayList(KEYBUNDLE_CONSOLE);
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
            logList = new ArrayList<ConsoleItem>(MAX_LOG_COUNT);
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
                    if (check) {
                        window.addFlags(FLAG_KEEP_SCREEN_ON);
                    } else {
                        window.clearFlags(FLAG_KEEP_SCREEN_ON);
                    }
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
                    public void onBindViewHolder(ConsoleItemHolder holder, int position) {
                        ConsoleItem c = logList.get(position);
                        holder.bindLog(c.time, c.msg);
                    }

                    @Override
                    public int getItemCount() {
                        return logList.size();
                    }
                };
        cv.setAdapter(adpt);
        // check feature
        checkBatteryOptimizations();
        sH.sendMessage(sH.obtainMessage(MSG_STATE, MSG_STATE_NONE));
    }
    private static final int REQUEST_BATTERY_OPTIMIZATIONS = 1001;
    private void checkBatteryOptimizations() {
        PowerManager powerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
        if (powerManager != null && !powerManager.isIgnoringBatteryOptimizations(getPackageName())) {
            // Jika izin tidak diizinkan, tampilkan dialog untuk meminta izin
            Intent intent = new Intent();
            intent.setAction(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS);
            intent.setData(Uri.parse("package:" + getPackageName()));
            startActivityForResult(intent, REQUEST_BATTERY_OPTIMIZATIONS);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        switch (requestCode) {
            default:
                break;
            case REQUEST_BATTERY_OPTIMIZATIONS:
                PowerManager powerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
                if (powerManager != null && powerManager.isIgnoringBatteryOptimizations(getPackageName())) {
                    // Izin diberikan, lanjutkan dengan operasi normal
                } else {
                    // Izin ditolak, berikan pengguna instruksi lebih lanjut atau tindakan yang
                    // sesuai
                }
                break;
        }
    }
    
    //Service Connection
    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
        dataService = (MinerService.LocalBinder) service;
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
    }
    
    
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putParcelableArrayList(KEYBUNDLE_CONSOLE, logList);
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
            logList = savedInstanceState.getParcelableArrayList(KEYBUNDLE_CONSOLE);
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
    protected void onDestroy() {
        super.onDestroy();
        if (isFinishing()) {
            stopService(new Intent(this, MinerService.class));
        }
        unbindService(this);
    }
    
    final Handler sH = new Handler(Looper.getMainLooper()) {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                default:
                    break;
                case MSG_UPDATE:
                    switch (msg.arg1) {
                        default:
                            break;
                        case MSG_UPDATE_SPEED:
                            float hash_speed = (float)msg.obj;
                            int unit_step = 0;
                            while (unit_step < UnitHash.length && hash_speed > 1000.0f) {
                                hash_speed /= 1000.0f;
                                unit_step++;
                            }
                            tv_s.setText(String.format("%.3f %s/Sec", hash_speed, UnitHash[unit_step]));
                            break;
                        case MSG_UPDATE_ACCEPTED:
                            tv_ra.setText(String.format("%03d", (long)msg.obj));
                            break;
                        case MSG_UPDATE_REJECTED:
                            tv_rr.setText(String.format("%03d", (long)msg.obj));
                            break;
                        case MSG_UPDATE_CONSOLE:
                            logList.add(new ConsoleItem((String)msg.obj));
                            adpt.notifyDataSetChanged();
                            break;
                    }
                    break;
                case MSG_STATE:
                    switch (msg.arg1) {
                        default:
                        case MSG_STATE_NONE:
                            btn_stopmine.setVisibility(View.GONE);
                            btn_stopmine.setEnabled(false);
                            btn_startmine.setVisibility(View.VISIBLE);
                            btn_startmine.setEnabled(true);
                            tv_s.setText("000");
                            // enable all user Input
                            input_container.setVisibility(View.VISIBLE);
                            status_container.setVisibility(View.GONE);
                            break;
                        case MSG_STATE_ONSTART:
                            btn_stopmine.setVisibility(View.GONE);
                            btn_stopmine.setEnabled(false);
                            btn_startmine.setVisibility(View.VISIBLE);
                            btn_startmine.setEnabled(false);
                            // disable all user Input
                            input_container.setVisibility(View.GONE);
                            status_container.setVisibility(View.VISIBLE);
                            break;
                        case MSG_STATE_RUNNING:
                            btn_stopmine.setVisibility(View.VISIBLE);
                            btn_stopmine.setEnabled(true);
                            btn_startmine.setVisibility(View.GONE);
                            btn_startmine.setEnabled(false);
                            // disable all user Input
                            input_container.setVisibility(View.GONE);
                            status_container.setVisibility(View.VISIBLE);
                            break;
                        case MSG_STATE_ONSTOP:
                            btn_stopmine.setVisibility(View.VISIBLE);
                            btn_stopmine.setEnabled(false);
                            btn_startmine.setVisibility(View.GONE);
                            btn_startmine.setEnabled(false);
                            // disable all user Input
                            input_container.setVisibility(View.GONE);
                            status_container.setVisibility(View.VISIBLE);
                            break;
                    }
                    break;
            }
        }
    };
    
    // button function
    public void toStartMining(View v) {
        String url = sb.append(et_serv.getText()).toString();
        sb.setLength(0);
        int port = Integer.parseInt(sb.append(et_port.getText()).toString());
        sb.setLength(0);
        String user = sb.append(et_user.getText()).toString();
        sb.setLength(0);
        String pass = sb.append(et_pass.getText()).toString();
        sb.setLength(0);
        tv_showInput.setText(
                String.format(
                        "server = %s:%d \nauth = %s:%s\nuse %d threads",
                        url, port, user, pass, sb_cpu.getProgress()));
        SharedPreferences.Editor editor = getPreferences(Context.MODE_PRIVATE).edit();
        editor.putString(PREF_URL, url);
        editor.putInt(PREF_PORT, port);
        editor.putString(PREF_USER, user);
        editor.putString(PREF_PASS, pass);
        editor.putInt(PREF_CPU_USAGE, sb_cpu.getProgress());
        editor.commit();

        sH.sendMessage(sH.obtainMessage(MSG_STATE, MSG_STATE_ONSTART));
        sH.sendMessageDelayed(sH.obtainMessage(MSG_STATE, MSG_STATE_RUNNING), 5000);
        //mService.startMining(url, port, user, pass, sb_cpu.getProgress());
    }

    public void toStopMining(View v) {
        //mService.stopMining();
        sH.sendMessage(sH.obtainMessage(MSG_STATE, MSG_STATE_ONSTOP));
        sH.sendMessageDelayed(sH.obtainMessage(MSG_STATE, MSG_STATE_NONE), 5000);
    }
    
    public static class ConsoleItemHolder extends RecyclerView.ViewHolder {
        private AppCompatTextView time;
        private AppCompatTextView msg;

        public ConsoleItemHolder(View itemView) {
            super(itemView);
            time = itemView.findViewById(R.id.text1);
            msg = itemView.findViewById(R.id.text2);
        }

        public void bindLog(String t, String m) {
            time.setText(t);
            msg.setText(m);
        }
    }
    private static final DateFormat logDateFormat = new SimpleDateFormat("[HH:mm:ss] ");
    public static class ConsoleItem extends Object implements Parcelable {
        public final String time, msg;

        public ConsoleItem(String m) {
            time = logDateFormat.format(new Date());
            msg = m;
        }

        protected ConsoleItem(Parcel in) {
            String[] strings = new String[2];
            in.readStringArray(strings);
            time = strings[0];
            msg = strings[1];
        }

        public static final Parcelable.Creator<ConsoleItem> CREATOR =
                new Parcelable.Creator<ConsoleItem>() {
                    @Override
                    public ConsoleItem createFromParcel(Parcel in) {
                        return new ConsoleItem(in);
                    }

                    @Override
                    public ConsoleItem[] newArray(int size) {
                        return new ConsoleItem[size];
                    }
                };

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeStringArray(new String[] {time, msg});
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

    // Message id for all Handler
    static final int MSG_UPDATE = 1;
    static final int MSG_STATE = 2;

    public static final int MSG_STATE_NONE = 0;
    public static final int MSG_STATE_ONSTART = 1;
    public static final int MSG_STATE_RUNNING = 2;
    public static final int MSG_STATE_ONSTOP = 3;

    public static final int MSG_UPDATE_SPEED = 1;
    public static final int MSG_UPDATE_ACCEPTED = 2;
    public static final int MSG_UPDATE_REJECTED = 3;
    public static final int MSG_UPDATE_STATUS = 4;
    public static final int MSG_UPDATE_CONSOLE = 5;
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
    //unit hash
    public static final String[] UnitHash = new String[]{
      "Hash",
      "kHash",
      "MHash",
      "GHash",
      "THash",
      "PHash",
      "EHash",
      "ZHash",
      "YHash",
    };
}