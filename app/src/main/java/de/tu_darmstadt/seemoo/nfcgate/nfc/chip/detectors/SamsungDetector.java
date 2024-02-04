package de.tu_darmstadt.seemoo.nfcgate.nfc.chip.detectors;

import android.util.Pair;

import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import de.tu_darmstadt.seemoo.nfcgate.nfc.chip.NfcChipGuess;

/**
 * Samsung NFC chip name detector.
 * Uses the Samsung HAL configuration file.
 * Samsung configurations have not been observed to contain the chip name directly,
 * so we use information from the firmware filename.
 */
public class SamsungDetector extends BaseConfigLineDetector {
    @Override
    protected List<String> getConfigFilenames() {
        return Arrays.asList("libnfc-sec-vendor.conf");
    }

    @Override
    protected boolean onLine(String line, NfcChipGuess guess) {
        Pair<String, String> keyVal = splitConfigLine(line);

        if (keyVal != null) {
            if ("TRANS_DRIVER".equals(keyVal.first)) {
                String device = keyVal.second;
                // existence of this device node confirms this is (or is not) the correct config
                if (!fileExists(device))
                    return false;

                guess.confidence = 0.9f;
                if (guess.chipName == null)
                    guess.chipName = "Samsung Unknown";
            }
            else if ("FW_FILE_NAME".equals(keyVal.first)
                    || "RF_FILE_NAME".equals(keyVal.first)) {
                guess.improveConfidence(0.2f);
                guess.chipName = "Samsung " + formatFirmwareName(keyVal.second);
            }
        }

        return true;
    }

    private static String formatFirmwareName(String firmware) {
        Pattern pattern = Pattern.compile("^\\w+_(\\w+)_");
        Matcher matcher = pattern.matcher(firmware);

        return matcher.lookingAt() && matcher.groupCount() > 0 ?
                Objects.requireNonNull(matcher.group(1)).toUpperCase() : null;
    }
}
