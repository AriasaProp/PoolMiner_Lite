package com.ariasaproject.poolminerlite;

import android.app.Application;

import androidx.annotation.NonNull;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import androidx.lifecycle.Observer;

public class MinerViewModel extends AndroidViewModel {
    protected MutableLiveData<Float> Miner_Speed = new MutableLiveData(0.0f);
    protected MutableLiveData<Boolean> Miner_Result = new MutableLiveData();
    protected MutableLiveData<Float> Miner_State = new MutableLiveData(0);
    protected MutableLiveData<ConsoleItem> Miner_Log = new MutableLiveData();

    public MinerViewModel(@NonNull Application application) {
        super(application);
    }
    public int getCounter() {
        return counter;
    }
    
    Observer<Float> spdObs;
    Observer<Float> rsltObs;
    Observer<Float> intObs;
    Observer<Float> logObs;
    
    public void registerObs(LifecycleOwner lo, Observer<Float> _spdObs, Observer<Boolean> _rsltObs, Observer<Integer> _intObs, Observer<ConsoleItem> _logObs) {
        spdObs = _spdObs;
        rsltObs = _rsltObs;
        intObs = _intObs;
        logObs = _logObs;
        
        Miner_Speed.observe(lo, spdObs);
        Miner_Result.observe(lo, rsltObs);
        Miner_State.observe(lo, intObs);
        Miner_Log.observe(lo, logObs);
    }
    
    public void unregisterObs() {
        Miner_Speed.removeObserve(spdObs);
        Miner_Result.removeObserve(rsltObs);
        Miner_State.removeObserve(intObs);
        Miner_Log.removeObserve(logObs);
    }
    
    public void postSpeed(float speed) {
        Miner_Speed.postValue(speed);
    }
    public void postResult(boolean result) {
        Miner_Result.postValue(result);
    }
    public void postState(int state) {
        Miner_State.postValue(state);
    }
    public void postLog(int lvl, String log) {
        Miner_Log.postValue(new ConsoleItem(lvl, log));
    }
}



