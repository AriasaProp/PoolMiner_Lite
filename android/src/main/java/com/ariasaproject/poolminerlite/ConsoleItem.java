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
        this(c, logDateFormat.format(new Date()), m);
    }

    protected ConsoleItem(int c, String d, String m) {
        time = d;
        msg = m;
        color = c;
    }

    public static class Lists extends Object implements Parcelable {
        public static final int SIZE = 100;
        private final ConsoleItem[] logs = new ConsoleItem[SIZE];

        public Lists() {}

        protected Lists(Parcel in) {
            String[] strings = new String[3 * SIZE];
            in.readStringArray(strings);
            for (int i = 0; i < SIZE; ++i) {
                if (strings[i * 3] == null) break;
                logs[i] =
                        new ConsoleItem(
                                Integer.parseInt(strings[i * 3]),
                                strings[i * 3 + 1],
                                strings[i * 3 + 2]);
            }
        }

        public void add(int lvl, String msg) {
            for (int i = SIZE - 1; i > 0; --i) {
                if (logs[i - 1] == null) continue;
                logs[i] = logs[i - 1];
            }
            logs[0] = new ConsoleItem(lvl, msg);
        }

        public void add(ConsoleItem ci) {
            for (int i = SIZE - 1; i > 0; --i) {
                if (logs[i - 1] == null) continue;
                logs[i] = logs[i - 1];
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
                    public Lists[] newArray(int size) {
                        return new Lists[size];
                    }
                };

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            String[] strings = new String[3 * SIZE];
            for (int i = 0; i < SIZE; ++i) {
                if (logs[i] == null) break;
                strings[i * 3] = String.valueOf(logs[i].color);
                strings[i * 3 + 1] = logs[i].time;
                strings[i * 3 + 2] = logs[i].msg;
            }
            dest.writeStringArray(strings);
        }
    }
}
