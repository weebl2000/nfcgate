package de.tu_darmstadt.seemoo.nfcgate.gui.fragment;

import android.os.Bundle;
import android.widget.Toast;

import androidx.preference.PreferenceFragmentCompat;

import de.tu_darmstadt.seemoo.nfcgate.R;
import de.tu_darmstadt.seemoo.nfcgate.network.UserTrustManager;

public class SettingsFragment extends PreferenceFragmentCompat {
    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        addPreferencesFromResource(R.xml.preferences);

        findPreference("reset_usertrust").setOnPreferenceClickListener((preference) -> {
            UserTrustManager.getInstance().clearTrust();
            Toast.makeText(getContext(), R.string.settings_adv_replay_toast, Toast.LENGTH_LONG).show();
            return true;
        });
    }
}
