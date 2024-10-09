package com.ariasaproject.poolminerlite;

import android.app.Application;

import androidx.annotation.NonNull;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Observer;

public class MinerViewModel extends AndroidViewModel {
    protected MutableLiveData<Float> Miner_Speed = new MutableLiveData(0.0f);
    protected MutableLiveData<Boolean> Miner_Result = new MutableLiveData();
    protected MutableLiveData<Integer> Miner_State = new MutableLiveData(0);
    protected MutableLiveData<ConsoleItem> Miner_Log = new MutableLiveData();

    public MinerViewModel(@NonNull Application application) {
        super(application);
    }
    
    Observer<ConsoleItem> mainLogObs;
    public void registerMainObs(LifecycleOwner lo, Observer<ConsoleItem> _mainLogObs) {
	      mainLogObs = _mainLogObs;
	    	Miner_Log.observe(lo, mainLogObs);
    }
    public void unregisterMainObs() {
        Miner_Log.removeObserver(mainLogObs);
    }

    Observer<Float> spdObs;
    Observer<Boolean> rsltObs;
    Observer<Integer> stObs;
    Observer<ConsoleItem> logObs;

    public void registerObs(
            LifecycleOwner lo,
            Observer<Float> _spdObs,
            Observer<Boolean> _rsltObs,
            Observer<Integer> _stObs,
            Observer<ConsoleItem> _logObs) {
        spdObs = _spdObs;
        rsltObs = _rsltObs;
        stObs = _stObs;
        logObs = _logObs;

        Miner_Speed.observe(lo, spdObs);
        Miner_Result.observe(lo, rsltObs);
        Miner_State.observe(lo, stObs);
        Miner_Log.observe(lo, logObs);
    }

    public void unregisterObs() {
        Miner_Speed.removeObserver(spdObs);
        Miner_Result.removeObserver(rsltObs);
        Miner_State.removeObserver(stObs);
        Miner_Log.removeObserver(logObs);
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

    public void postLog(byte i, String msg) {
        Miner_Log.postValue(new ConsoleItem(i, msg));
    }

    public float getSpeed() {
        return Miner_Speed.getValue();
    }

    public int getState() {
        return Miner_State.getValue();
    }
}
