package com.ariasaproject.poolminerlite;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

public class MinerService extends Service {
    LocalBinder local = new LocalBinder();

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
        super.onDestroy();
    }
    
    public static class LocalBinder extends IBinder {
        protected LocalBinder() {}
    }
}