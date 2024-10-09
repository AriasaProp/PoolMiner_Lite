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
        private int indexCount = 0;

        public Lists() {}

        protected Lists(Parcel in) {
            indexCount = in.readInt();
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
            logs[indexCount++ % SIZE] = new ConsoleItem(lvl, msg);
        }

        public void add(ConsoleItem ci) {
            logs[indexCount++ % SIZE] = ci;
        }

        public ConsoleItem get(int index) {
            return logs[(indexCount + SIZE - (index % SIZE)) % SIZE];
        }
        public int getSize () {
        	return logs[SIZE - 1] != null ? SIZE : indexCount;
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
            dest.writeInt(indexCount);
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
