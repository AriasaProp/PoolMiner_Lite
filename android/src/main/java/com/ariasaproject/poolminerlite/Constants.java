package com.ariasaproject.poolminerlite;

public class Constants {
    /*
    public static final String SERVICE_START_MINE = "startService";
    public static final String SERVICE_STOP_MINE = "stopService";
    */

    public static final String INTENT_COMUNICATION_EVENT = "intent_comunication_event";

    public static final String EXTRA_MINER_SPEED = "extra_miner_speed";
    public static final String EXTRA_MINER_RESULT = "extra_miner_result";
    public static final String EXTRA_MINER_LOG = "extra_miner_log";
    public static final String EXTRA_MINER_STATE = "extra_miner_state";

    public static final int EXTRA_MINER_STATE_NONE = 0;
    public static final int EXTRA_MINER_STATE_ONSTART = 1;
    public static final int EXTRA_MINER_STATE_RUNNING = 2;
    public static final int EXTRA_MINER_STATE_ONSTOP = 3;
}
