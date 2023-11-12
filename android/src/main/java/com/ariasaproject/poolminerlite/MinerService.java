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


public class MinerService extends Service {
    LocalBinder local = new LocalBinder();
    private static final int NOTIFICATION_ID = 1;
    private static final String NOTIFICATION_CHANNEL_ID = "notif_miner";
    private static final String NOTIFICATION_TITLE = "Miner Service - Lite";
    
    
    @Override
    public void onCreate() {
        super.onCreate();
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
        stopForeground(true);
        stopSelf();
        super.onDestroy();
    }
    
    public class LocalBinder extends Binder {
        public int State;
        protected boolean running;
        protected LocalBinder() {
            running = false;
        }
        
        
        public boolean StartMine() {
            if (running) StopMine();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                NotificationChannel channel = new NotificationChannel(
                    NOTIFICATION_CHANNEL_ID,
                    NOTIFICATION_TITLE,
                    NotificationManager.IMPORTANCE_DEFAULT
                );
                NotificationManager notificationManager = getSystemService(NotificationManager.class);
                notificationManager.createNotificationChannel(channel);
            }
            MinerService.this.startForeground(NOTIFICATION_ID,
            new NotificationCompat.Builder(this, NOTIFICATION_CHANNEL_ID)
                .setSmallIcon(R.mipmap.ic_launcher_foreground)
                .setContentTitle(NOTIFICATION_TITLE)
                .setContentText("Service is running in the foreground")
                .build()
            );
            running = true;
        }
        public boolean isRunning() {
            return running;
        }
        public void StopMine() {
            if (!running) return;
            MinerService.this.stopForeground(true);
            MinerService.this.stopSelf();
            running = false;
        }
    }
}

