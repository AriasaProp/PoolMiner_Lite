package com.ariasaproject.poolminerlite;

import android.os.Parcel;
import android.os.Parcelable;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

public class ConsoleItem {
    private static final DateFormat logDateFormat = new SimpleDateFormat("[HH:mm:ss] ");

    public final String time, msg;
    public final byte color;

    public ConsoleItem(byte c, String m) {
        this(c, logDateFormat.format(new Date()), m);
    }

    protected ConsoleItem(byte c, String d, String m) {
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
            byte[] c = in.createByteArray();
            in.readByteArray(c);
            String[] s = in.createStringArray();
            in.readStringArray(s);

            for (int i = 0; i < SIZE; ++i) {
                if (s[i * 2] == null) break;
                logs[i] = new ConsoleItem(c[i], s[i * 2], s[i * 2 + 1]);
            }
        }

        public void add(byte lvl, String msg) {
            logs[indexCount++ % SIZE] = new ConsoleItem(lvl, msg);
        }

        public void add(ConsoleItem ci) {
            logs[indexCount++ % SIZE] = ci;
        }

        public ConsoleItem get(int index) {
            return logs[(indexCount + SIZE - (index % SIZE)) % SIZE];
        }

        public int getSize() {
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
            String[] s = new String[SIZE * 2]
            byte[] c = new byte[SIZE];
            for (int i = 0; i < SIZE; ++i) {
                if (logs[i] == null) break;
                c[i] = logs[i].color;
                s[i * 2] = logs[i].time;
                s[i * 2 + 1] = logs[i].msg;
            }

            dest.writeInt(indexCount);
            dest.writeByteArray(c);
            dest.writeStringArray(s);
        }
    }
}
