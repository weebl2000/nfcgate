package de.tu_darmstadt.seemoo.nfcgate.gui.component;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import androidx.core.content.FileProvider;

import android.util.Log;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

import de.tu_darmstadt.seemoo.nfcgate.BuildConfig;
import de.tu_darmstadt.seemoo.nfcgate.R;

public class ContentShare {
    public interface IFileShareable {
        void write(OutputStream stream) throws IOException;
    }

    // state variables
    private final Context mContext;
    private String mPrefix;
    private String mExtension;
    private String mMimeType;
    private Intent mShareIntent;

    public ContentShare(Context context) {
        mContext = context;

        // defaults
        mPrefix = "";
        mExtension = ".bin";
        mMimeType = "application/*";
    }

    public ContentShare setPrefix(String prefix) {
        mPrefix = prefix;
        return this;
    }

    public ContentShare setExtension(String extension) {
        mExtension = extension;
        return this;
    }

    public ContentShare setMimeType(String mimeType) {
        mMimeType = mimeType;
        return this;
    }

    public ContentShare setFile(IFileShareable share) {
        // ensure share directory exists
        final File shareDir = new File(mContext.getCacheDir() + "/share/");
        shareDir.mkdir();

        // create file with given prefix and extension
        final File file = new File(shareDir, mPrefix + mExtension);
        try (final OutputStream stream = new FileOutputStream(file)) {
            // write to file (overwrites if already exists)
            share.write(stream);
        }
        catch (IOException e) {
            Toast.makeText(mContext, mContext.getString(R.string.share_error), Toast.LENGTH_LONG).show();
            Log.e("FileShare", "Error sharing file", e);
            return this;
        }

        // generate file provider URI for the sharing app
        Uri uri = FileProvider.getUriForFile(mContext, BuildConfig.APPLICATION_ID, file);

        // create intent with binary content type
        mShareIntent = new Intent(Intent.ACTION_SEND)
                .setType(mMimeType)
                .putExtra(Intent.EXTRA_STREAM, uri)
                .setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

        return this;
    }

    public ContentShare setText(String text) {
        mShareIntent = new Intent(Intent.ACTION_SEND)
                .setType(mMimeType)
                .putExtra(Intent.EXTRA_TEXT, text);

        return this;
    }

    public void share() {
        // open chooser and start selected intent
        mContext.startActivity(Intent.createChooser(mShareIntent, null));
    }
}
