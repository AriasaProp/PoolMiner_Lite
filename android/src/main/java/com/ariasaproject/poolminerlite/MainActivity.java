package com.ariasaproject.poolminerlite;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.PowerManager;
import android.provider.Settings;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.fragment.app.Fragment;
import androidx.viewpager2.adapter.FragmentStateAdapter;
import androidx.viewpager2.widget.ViewPager2;

import com.ariasaproject.poolminerlite.fragments.ConfigFragment;
import com.ariasaproject.poolminerlite.fragments.MinerFragment;
import com.ariasaproject.poolminerlite.fragments.NewsFragment;
import com.ariasaproject.poolminerlite.ConsoleItem;
import com.ariasaproject.poolminerlite.MinerViewModel;
import com.google.android.material.tabs.TabLayout;
import com.google.android.material.tabs.TabLayoutMediator;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("ext");
    }

    FragmentStateAdapter pagerAdapter;
    public ConsoleItem.Lists logList;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        if (savedInstanceState != null) {
            logList = savedInstanceState.getParcelable(KEYBUNDLE_CONSOLE);
        } else {
            logList = new ConsoleItem.Lists();
        }
        
        final MinerViewModel mvm = ((MainApplication) getApplication()).getMinerViewModel();
        
        mvm.registerMainObs(this,(log)->logList.add(log));
        
        super.onCreate(savedInstanceState);
        
        setContentView(R.layout.activity_main);
        setSupportActionBar((Toolbar) findViewById(R.id.toolbar));
        // tabs and viewpager
        final TabLayout tabLayout = findViewById(R.id.tabLayout);
        final ViewPager2 viewPager = findViewById(R.id.viewPager);

        pagerAdapter =
                new FragmentStateAdapter(this) {
                    @NonNull
                    @Override
                    public Fragment createFragment(int position) {
                        switch (position) {
                            case 0:
                                return new NewsFragment();
                            default:
                            case 1:
                                return new MinerFragment();
                            case 2:
                                return new ConfigFragment();
                        }
                    }

                    @Override
                    public int getItemCount() {
                        return 3;
                    }
                };
        viewPager.setAdapter(pagerAdapter);
        new TabLayoutMediator(
                        tabLayout,
                        viewPager,
                        true,
                        true,
                        (t, i) -> {
                            switch (i) {
                                case 0:
                                    t.setText(R.string.tab_chat);
                                    break;
                                default:
                                case 1:
                                    t.setText(R.string.tab_miner);
                                    break;
                                case 2:
                                    t.setText(R.string.tab_config);
                                    break;
                            }
                        })
                .attach();
        viewPager.setCurrentItem(1, false);
        // check feature
        checkBatteryOptimizations();
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
    
    @Override
    public void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putParcelable(KEYBUNDLE_CONSOLE, logList);
    }
    
    @Override
    protected void OnDestroy() {
    		super.OnDestroy();
    		mvm.unregisterMainObs();
    }
    
    
    private static final String KEYBUNDLE_CONSOLE = "bundle_console";
}
