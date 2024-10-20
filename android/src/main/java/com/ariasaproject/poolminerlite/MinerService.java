package com.ariasaproject.poolminerlite;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;

import androidx.annotation.Keep;
import androidx.core.app.NotificationCompat;

import static com.ariasaproject.poolminerlite.MainApplication.NOTIFICATION_ID;
import static com.ariasaproject.poolminerlite.MainApplication.NOTIFICATION_CHANNEL_ID;

public class MinerService extends Service {
    MinerViewModel mVM;
    LocalBinder local = new LocalBinder();
    NotificationCompat.Builder notifBuilder;
    
    @Override
    public void onCreate() {
        super.onCreate();
        mVM = ((MainApplication) getApplication()).getMinerViewModel();
        notifBuilder = new NotificationCompat.Builder(this, NOTIFICATION_CHANNEL_ID);
        notifBuilder.setSmallIcon(R.mipmap.ic_launcher_foreground);
        notifBuilder.setContentTitle(getString(R.string.app_name));
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
                startForeground(NOTIFICATION_ID, notifBuilder
                                .setContentText("Service is running in the foreground")
                                .build());
                break;
            default:
                break;
        }
        mVM.postState(state);
    }

    @Keep
    private synchronized void sendMessageConsole(byte i, String msg) {
        mVM.postLog(i, msg);
    }

    private native void nativeStart(String[] strings, int[] ints);

    private native boolean nativeRunning();

    private native void nativeStop();

    public class LocalBinder extends Binder {
        public int State;

        protected LocalBinder() {}

        public void StartMine(String[] dats, int[] dati) {
            if (MinerService.this.nativeRunning()) StopMine();
            MinerService.this.updateState(1);
            MinerService.this.nativeStart(dats, dati);
        }

        public boolean isRunning() {
            return MinerService.this.nativeRunning();
        }

        public void StopMine() {
            if (!MinerService.this.nativeRunning()) return;
            MinerService.this.updateState(3);
            MinerService.this.nativeStop();
            MinerService.this.stopSelf();
        }
    }
}
