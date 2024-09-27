package org.cagnulen.qdomyoszwift;

import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.content.Intent;
import android.location.LocationManager;
import android.content.pm.PackageManager;
import android.provider.Settings;
import android.util.Log;

public class LocationHelper {
    private static final String TAG = "LocationHelper";
    private static boolean isLocationEnabled = false;
    private static boolean isBluetoothEnabled = false;

    public static boolean start(Context context) {
        Log.d(TAG, "Starting LocationHelper check...");

        isLocationEnabled = isLocationEnabled(context);
        isBluetoothEnabled = isBluetoothEnabled();

        return isLocationEnabled && isBluetoothEnabled;
    }

    public static boolean hasGps(Context context) {
        PackageManager packageManager = context.getPackageManager();
        return packageManager.hasSystemFeature(PackageManager.FEATURE_LOCATION_GPS);
    }
    
    public static void requestPermissions(Context context) {
        if (!isLocationEnabled || !isBluetoothEnabled) {
            Log.d(TAG, "Some services are disabled. Prompting user...");
            if (!isLocationEnabled) {
                promptEnableLocation(context);
            }
            if (!isBluetoothEnabled) {
                promptEnableBluetooth(context);
            }
        } else {
            Log.d(TAG, "All services are already enabled.");
        }
    }

    public static boolean check(Context context) {
        return isLocationEnabled(context) && isBluetoothEnabled();
    }

    private static boolean isLocationEnabled(Context context) {
        if(!hasGps(context)) {
            Log.d(TAG, "Device doesn't have the GPS, so I can't force the popup");
            return true;
        }
        LocationManager locationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
        if (locationManager == null) {
            Log.d(TAG, "LocationManager is null, device might not have GPS. Returning true.");
            return true;
        }        
        return locationManager != null && locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER);
    }

    private static boolean isBluetoothEnabled() {
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        return bluetoothAdapter != null && bluetoothAdapter.isEnabled();
    }

    private static void promptEnableLocation(Context context) {
        Log.d(TAG, "Prompting to enable Location...");
        Intent intent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
    }

    private static void promptEnableBluetooth(Context context) {
        Log.d(TAG, "Prompting to enable Bluetooth...");
        Intent intent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
    }
}
