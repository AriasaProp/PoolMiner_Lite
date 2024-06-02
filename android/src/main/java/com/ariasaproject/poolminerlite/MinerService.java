package com.ariasaproject.poolminerlite;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;

import androidx.annotation.Keep;
import androidx.core.app.NotificationCompat;

public class MinerService extends Service {
    MinerViewModel mVM;
    LocalBinder local = new LocalBinder();
    private static final int NOTIFICATION_ID = 1;
    private static final String NOTIFICATION_CHANNEL_ID = "notif_miner";
    private static final String NOTIFICATION_TITLE = "Miner Service - Lite";

    @Override
    public void onCreate() {
        super.onCreate();
        mVM = ((MainApplication) getApplication()).getMinerViewModel();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel =
                    new NotificationChannel(
                            NOTIFICATION_CHANNEL_ID,
                            NOTIFICATION_TITLE,
                            NotificationManager.IMPORTANCE_DEFAULT);
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return local;
    }

    @Override
    public void onDestroy() {
        local.StopMine();
        super.onDestroy();
    }

    @Keep
    private synchronized void updateSpeed(float speed) {
        mVM.postSpeed(speed);
    }

    @Keep
    private synchronized void updateResult(boolean result) {
        mVM.postResult(result);
    }

    @Keep
    private synchronized void updateState(int state) {
        switch (state) {
            case 0:
                stopForeground(true);
                break;
            case 2:
                startForeground(
                        NOTIFICATION_ID,
                        new NotificationCompat.Builder(this, NOTIFICATION_CHANNEL_ID)
                                .setSmallIcon(R.mipmap.ic_launcher_foreground)
                                .setContentTitle(NOTIFICATION_TITLE)
                                .setContentText("Service is running in the foreground")
                                .build());
                break;
            default:
                ;
        }
        mVM.postState(state);
    }

    @Keep
    private synchronized void sendMessageConsole(int i, String msg, String desc) {
        mVM.postLog(i, msg, desc);
    }

    private native void nativeStart(String[] strings, int[] ints);

    private native boolean nativeRunning();

    private native void nativeStop();

    public class LocalBinder extends Binder {
        public int State;

        protected LocalBinder() {}

        public void StartMine(String[] dats, int[] dati) {
            if (MinerService.this.nativeRunning()) StopMine();
            MinerService.this.nativeStart(dats, dati);
        }

        public boolean isRunning() {
            return MinerService.this.nativeRunning();
        }

        public void StopMine() {
            if (!MinerService.this.nativeRunning()) return;
            MinerService.this.nativeStop();
            MinerService.this.stopSelf();
        }
    }
}
