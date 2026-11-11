package io.github.vito2492.combobreaker;

import android.app.AlertDialog;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;
import android.widget.Toast;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * The app: SDL's activity, plus the folder chooser that brings the player's
 * own game files into the app's folder (Android/data/<app>/files/run). No
 * game files come with the app.
 */
public class ComboBreakerActivity extends SDLActivity {
    private static final int PICK_FOLDER = 4242;
    // the files the game needs, by name (MAME's kinst set)
    private static final Set<String> WANTED = new HashSet<>(Arrays.asList(
        "ki-l15d.u98", "ki-l14.u98", "ki-l13.u98",
        "u10-l1", "u11-l1", "u12-l1", "u13-l1", "u33-l1", "u34-l1", "u35-l1", "u36-l1",
        "kinst.chd", "kinst.raw"));

    // 0 asking, choosing or copying, 1 copied, 2 cancelled, 3 failed, 4 quit (read from C)
    private static volatile int pickState = 1;
    private int copied;

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }

    /** Called from C: explain what's needed, then open the system's folder chooser. */
    public void askGameFiles(final String text) {
        pickState = 0;
        runOnUiThread(() -> new AlertDialog.Builder(this)
            .setTitle("ComboBreaker")
            .setMessage(text)
            .setCancelable(false)
            .setPositiveButton("Choose folder", (d, w) -> openPicker())
            .setNegativeButton("Quit", (d, w) -> pickState = 4)
            .show());
    }

    private void openPicker() {
        try {
            startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE), PICK_FOLDER);
        } catch (Exception e) {
            pickState = 3;
        }
    }

    /** Called from C, while it waits. */
    public int pickState() {
        return pickState;
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        if (request != PICK_FOLDER) {
            super.onActivityResult(request, result, data);
            return;
        }
        if (result != RESULT_OK || data == null || data.getData() == null) {
            pickState = 2;
            return;
        }
        final Uri tree = data.getData();
        Toast.makeText(this, "Copying the game files…", Toast.LENGTH_LONG).show();
        new Thread(() -> {
            copied = 0;
            boolean ok;
            try {
                File run = new File(getExternalFilesDir(null), "run");
                run.mkdirs();
                copyTree(tree, DocumentsContract.getTreeDocumentId(tree), run, 0);
                ok = true;
            } catch (Exception e) {
                ok = false;
            }
            final String msg = ok ? copied + " game files copied" : "Could not copy the files";
            runOnUiThread(() -> Toast.makeText(this, msg, Toast.LENGTH_SHORT).show());
            pickState = ok ? 1 : 3;
        }).start();
    }

    // Copies the wanted files from a folder (and the folders inside it, two
    // levels down, e.g. kinst/), unpacking them from MAME zips too.
    private void copyTree(Uri tree, String docId, File run, int depth) throws Exception {
        Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, docId);
        String[] cols = { DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                          DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                          DocumentsContract.Document.COLUMN_MIME_TYPE };
        try (Cursor c = getContentResolver().query(children, cols, null, null, null)) {
            if (c == null) return;
            while (c.moveToNext()) {
                String id = c.getString(0), name = c.getString(1), mime = c.getString(2);
                if (name == null) continue;
                String lower = name.toLowerCase();
                Uri doc = DocumentsContract.buildDocumentUriUsingTree(tree, id);
                if (DocumentsContract.Document.MIME_TYPE_DIR.equals(mime)) {
                    if (depth < 2) copyTree(tree, id, run, depth + 1);
                } else if (WANTED.contains(lower)) {
                    try (InputStream in = getContentResolver().openInputStream(doc)) {
                        save(in, new File(run, lower));
                    }
                } else if (lower.endsWith(".zip")) {
                    try (ZipInputStream zip = new ZipInputStream(getContentResolver().openInputStream(doc))) {
                        ZipEntry e;
                        while ((e = zip.getNextEntry()) != null) {
                            String n = new File(e.getName()).getName().toLowerCase();
                            if (!e.isDirectory() && WANTED.contains(n)) save(zip, new File(run, n));
                        }
                    }
                }
            }
        }
    }

    private void save(InputStream in, File out) throws Exception {
        File tmp = new File(out.getPath() + ".part");
        try (OutputStream o = new FileOutputStream(tmp)) {
            byte[] buf = new byte[1 << 16];
            int n;
            while ((n = in.read(buf)) > 0) o.write(buf, 0, n);
        }
        if (!tmp.renameTo(out)) throw new Exception("rename " + out);
        copied++;
    }
}
