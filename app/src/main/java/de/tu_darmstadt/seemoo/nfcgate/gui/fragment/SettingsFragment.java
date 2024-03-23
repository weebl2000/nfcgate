package de.tu_darmstadt.seemoo.nfcgate.gui.fragment;

import android.os.Bundle;

import androidx.preference.PreferenceFragmentCompat;

import de.tu_darmstadt.seemoo.nfcgate.R;

public class SettingsFragment extends PreferenceFragmentCompat {
    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        addPreferencesFromResource(R.xml.preferences);
    }
}
