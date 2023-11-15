package com.ariasaproject.poolminerlite;

import android.os.Parcel;
import android.os.Parcelable;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

public class ConsoleItem {
    private static final DateFormat logDateFormat = new SimpleDateFormat("[HH:mm:ss] ");
    
    public final String time, msg;
    public final int color;
    

    public ConsoleItem(int c, String m) {
        time = logDateFormat.format(new Date());
        msg = m;
        color = c;
    }
    protected ConsoleItem(String t, String m, int c) {
        time = t;
        msg = m;
        color = c;
    }
    
    
    public static class Lists extends Object implements Parcelable {
        private static final int MAX_LOG_COUNT = 50;
        private final ConsoleItem[] logs = new ConsoleItem[MAX_LOG_COUNT];
        
        protected Lists() {}
        protected Lists(Parcel in) {
            String[] strings = new String[3*MAX_LOG_COUNT];
            in.readStringArray(strings);
            for (int i = 0; i < MAX_LOG_COUNT; ++i) {
                logs[i] = new ConsoleItem(strings[i*3], strings[i*3+1], Integer.parseInt(strings[i*3+2]));
            }
        }
        
        public void add(int lvl, String msg) {
            for (int i = MAX_LOG_COUNT - 1; i > 0; --i) {
                if (logs[i - 1] == null) continue;
                logs[i] = logs[i - 1]
            }
            logs[0] = new ConsoleItem(lvl, msg);
        }
        
        public void add(ConsoleItem ci) {
            for (int i = MAX_LOG_COUNT - 1; i > 0; --i) {
                if (logs[i - 1] == null) continue;
                logs[i] = logs[i - 1]
            }
            logs[0] = ci;
        }
        
        public ConsoleItem get(int index) {
            return logs[index];
        }
    
    
        public static final Parcelable.Creator<Lists> CREATOR =
                new Parcelable.Creator<Lists>() {
                    @Override
                    public Lists createFromParcel(Parcel in) {
                        return new Lists(in);
                    }
    
                    @Override
                    public ConsoleItem[] newArray(int size) {
                        return new ConsoleItem[size];
                    }
                };
    
        @Override
        public int describeContents() {
            return 0;
        }
        
        @Override
        public void writeToParcel(Parcel dest, int flags) {
            String[] strings = new String[3*MAX_LOG_COUNT];
            for (int i = 0; i < MAX_LOG_COUNT; ++i) {
                strings[i*3] = logs[i].time;
                strings[i*3+1] = logs[i].msg;
                strings[i*3+2] = String.valueOf(logs[i].color);
            } 
            dest.writeStringArray(strings); 
        }
        
    }
}
