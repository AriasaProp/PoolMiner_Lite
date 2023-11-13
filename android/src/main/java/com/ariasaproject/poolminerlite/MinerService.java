package com.ariasaproject.poolminerlite;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.Binder;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.os.Build;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.annotation.Keep;

public class MinerService extends Service {
    MinerViewModel mVM;
    LocalBinder local = new LocalBinder();
    private static final int NOTIFICATION_ID = 1;
    private static final String NOTIFICATION_CHANNEL_ID = "notif_miner";
    private static final String NOTIFICATION_TITLE = "Miner Service - Lite";
    
    
    @Override
    public void onCreate() {
        super.onCreate();
        mVM = ((MainApplication)getApplication()).getMinerViewModel();
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
    private void updateSpeed (float speed) {
        mVM.postSpeed(speed);
    }
    @Keep
    private void updateResult (boolean result) {
        mVM.postResult(result);
    }
    @Keep
    private void updateState (int state) {
        mVM.postResult(state);
    }
    
    @Keep
    private void sendMessageConsole (int lvl, String msg) {
        mVM.postLog(new ConsoleItem(lvl, msg));
    }
    
    private native void nativeStart(String[] strings, int[] ints);
    private native boolean nativeRunning();
    private native void nativeStop();
    
    public class LocalBinder extends Binder {
        public int State;
        protected LocalBinder() {}
        
        public void StartMine() {
            if (MinerService.this.nativeRunning()) StopMine();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                NotificationChannel channel = new NotificationChannel(
                    NOTIFICATION_CHANNEL_ID,
                    NOTIFICATION_TITLE,
                    NotificationManager.IMPORTANCE_DEFAULT
                );
                NotificationManager notificationManager = MinerService.this.getSystemService(NotificationManager.class);
                notificationManager.createNotificationChannel(channel);
            }
            MinerService.this.startForeground(NOTIFICATION_ID,
            new NotificationCompat.Builder(MinerService.this, NOTIFICATION_CHANNEL_ID)
                .setSmallIcon(R.mipmap.ic_launcher_foreground)
                .setContentTitle(NOTIFICATION_TITLE)
                .setContentText("Service is running in the foreground")
                .build()
            );
            MinerService.this.nativeStart(new String[]{"text1", "text2", "text3", "text4"}, new int[]{80,1,9,9});
        }
        public boolean isRunning() {
            return MinerService.this.nativeRunning();
        }
        public void StopMine() {
            if (!MinerService.this.nativeRunning()) return;
            MinerService.this.nativeStop();
            MinerService.this.stopForeground(true);
            MinerService.this.stopSelf();
        }
    }
}



