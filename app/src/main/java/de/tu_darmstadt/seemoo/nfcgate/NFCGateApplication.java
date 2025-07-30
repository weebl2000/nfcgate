package de.tu_darmstadt.seemoo.nfcgate;

import android.app.Application;
import android.os.Build;
import android.util.Log;

import org.lsposed.hiddenapibypass.HiddenApiBypass;

public class NFCGateApplication extends Application {
    private static final String TAG = "NFCGateApplication";

    @Override
    public void onCreate() {
        super.onCreate();
        
        // Initialize AndroidHiddenApiBypass early to bypass hidden API restrictions
        // This is critical for Android 16 compatibility with NFC APEX modules
        try {
            Log.i(TAG, "Initializing hidden API bypass for Android " + Build.VERSION.SDK_INT);
            
            // Initialize bypass with comprehensive exemptions
            boolean success = HiddenApiBypass.addHiddenApiExemptions("L");
            if (success) {
                Log.i(TAG, "Core library (L) exemptions added successfully");
                
                // Add additional specific exemptions for NFC and APEX
                HiddenApiBypass.addHiddenApiExemptions("Landroid/nfc/");
                HiddenApiBypass.addHiddenApiExemptions("Landroid/os/");
                HiddenApiBypass.addHiddenApiExemptions("Lcom/android/nfc/");
                HiddenApiBypass.addHiddenApiExemptions("Landroid/content/pm/");
                
                Log.i(TAG, "Additional NFC and APEX exemptions added");
                Log.i(TAG, "Hidden API bypass initialization completed successfully");
            } else {
                Log.w(TAG, "Failed to add core library exemptions");
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to initialize hidden API bypass", e);
        } catch (Throwable t) {
            Log.e(TAG, "Critical error during hidden API bypass initialization", t);
        }
    }
}