package com.ariasaproject.poolminerlite;

import android.app.Application;
import android.content.res.Configuration;

public class MainApplication extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        // Inisialisasi kode atau sumber daya global di sini
    }

    @Override
    public void onLowMemory() {
        super.onLowMemory();
        // Dipecat ketika sistem sedang kehabisan memori
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        // Dipecat saat konfigurasi perangkat berubah (misalnya, rotasi layar)
    }
}