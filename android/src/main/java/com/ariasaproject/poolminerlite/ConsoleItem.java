package com.ariasaproject.poolminerlite;

import android.os.Parcel;
import android.os.Parcelable;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

public class ConsoleItem extends Object implements Parcelable {
    private static final DateFormat logDateFormat = new SimpleDateFormat("[HH:mm:ss] ");
    
    public final String time, msg;
    public final int color;
    

    public ConsoleItem(int c, String m) {
        time = logDateFormat.format(new Date());
        msg = m;
        color = c;
    }

    protected ConsoleItem(Parcel in) {
        String[] strings = new String[3];
        in.readStringArray(strings);
        time = strings[0];
        msg = strings[1];
        color = Integer.parseInt(strings[2]);
    }

    public static final Parcelable.Creator<ConsoleItem> CREATOR =
            new Parcelable.Creator<ConsoleItem>() {
                @Override
                public ConsoleItem createFromParcel(Parcel in) {
                    return new ConsoleItem(in);
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
        dest.writeStringArray(new String[] {time, msg, String.valueOf(color)}); // Tambahkan nilai warna ke dalam array
    }
}
