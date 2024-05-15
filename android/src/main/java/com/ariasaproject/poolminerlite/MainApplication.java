package com.ariasaproject.poolminerlite;

import android.app.Application;
import android.content.res.Configuration;

import androidx.lifecycle.ViewModelProvider;

public class MainApplication extends Application {
    private MinerViewModel minerViewModel;

    @Override
    public void onCreate() {
        super.onCreate();
        minerViewModel = new ViewModelProvider(this).get(MinerViewModel.class);
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
