package com.ariasaproject.poolminerlite;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Application;
import android.content.res.Configuration;

import androidx.lifecycle.ViewModelProvider.AndroidViewModelFactory;

public class MainApplication extends Application {
    private MinerViewModel minerViewModel;
    public static final int NOTIFICATION_ID = 1;
    public static final String NOTIFICATION_CHANNEL_ID = "notif_miner";

    @Override
    public void onCreate() {
        super.onCreate();
        minerViewModel = new AndroidViewModelFactory(this).create(MinerViewModel.class);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID, getString(R.string.app_name), NotificationManager.IMPORTANCE_DEFAULT);
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }
    }

    @Override
    public void onLowMemory() {
        super.onLowMemory();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    public MinerViewModel getMinerViewModel() {
        return minerViewModel;
    }
}
