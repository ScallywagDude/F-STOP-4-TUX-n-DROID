package com.fstop.game;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

import org.libsdl.app.SDLActivity;

/**
 * First-run screen. The game's data ships inside the APK; the engine needs ordinary files, so they are
 * unpacked to the app's own storage once, then the game starts straight away on every later launch.
 */
public class MainActivity extends Activity {
	private static final String TAG = "FSTOP";
	private static final String MANIFEST = "fstop_manifest.txt";
	private static final int DATA_VERSION = 18;

	private ProgressBar mBar;
	private TextView mText;

	@Override
	protected void onCreate(Bundle state) {
		super.onCreate(state);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

		SharedPreferences prefs = getSharedPreferences("fstop", 0);
		final File root = getExternalFilesDir(null);
		final List<String[]> files = readManifest();
		if (files.isEmpty() || prefs.getInt("data_version", 0) == DATA_VERSION) {
			launch();
			return;
		}

		LinearLayout layout = new LinearLayout(this);
		layout.setOrientation(LinearLayout.VERTICAL);
		layout.setGravity(Gravity.CENTER);
		layout.setBackgroundColor(Color.BLACK);
		layout.setPadding(120, 0, 120, 0);
		TextView title = new TextView(this);
		title.setText("F-STOP");
		title.setTextColor(Color.WHITE);
		title.setTextSize(32);
		title.setGravity(Gravity.CENTER);
		mText = new TextView(this);
		mText.setTextColor(Color.LTGRAY);
		mText.setGravity(Gravity.CENTER);
		mText.setText("Preparing game files. This only happens once...");
		mBar = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
		mBar.setMax(1000);
		layout.addView(title);
		layout.addView(mText);
		layout.addView(mBar);
		setContentView(layout);

		new Worker(root, files).start();
	}

	private static void enableMulticore(File root) throws java.io.IOException {
		File cfg = new File(root, "fstop/cfg/config.cfg");
		if (!cfg.exists()) return;
		byte[] data = new byte[(int) cfg.length()];
		try (java.io.DataInputStream in = new java.io.DataInputStream(new java.io.FileInputStream(cfg))) { in.readFully(data); }
		String text = new String(data, "ISO-8859-1").replace("mat_queue_mode \"0\"", "mat_queue_mode \"-1\"");
		try (java.io.FileOutputStream out = new java.io.FileOutputStream(cfg)) { out.write(text.getBytes("ISO-8859-1")); }
	}

	private class Worker extends Thread {
		private final File mRoot;
		private final List<String[]> mFiles;
		Worker(File root, List<String[]> files) { mRoot = root; mFiles = files; }
		@Override public void run() {
			try {
				if (getSharedPreferences("fstop", 0).getInt("data_version", 0) < 7) enableMulticore(mRoot);
				unpack(mRoot, mFiles);
				getSharedPreferences("fstop", 0).edit().putInt("data_version", DATA_VERSION).commit();
				runOnUiThread(new Done());
			} catch (Exception e) {
				Log.e(TAG, "unpack failed", e);
				runOnUiThread(new Failed(e.getMessage()));
			}
		}
	}
	private class Done implements Runnable {
		@Override public void run() { launch(); }
	}
	private class Failed implements Runnable {
		private final String mMessage;
		Failed(String message) { mMessage = message; }
		@Override public void run() {
			mText.setText("Could not prepare the game files. Check that the phone has free space, then reopen F-STOP.\n\n" + mMessage);
		}
	}
	private class Progress implements Runnable {
		private final int mValue;
		Progress(int value) { mValue = value; }
		@Override public void run() { mBar.setProgress(mValue); }
	}

	private List<String[]> readManifest() {
		List<String[]> out = new ArrayList<String[]>();
		try {
			BufferedReader r = new BufferedReader(new InputStreamReader(getAssets().open(MANIFEST)));
			String line;
			while ((line = r.readLine()) != null) {
				int tab = line.indexOf('\t');
				if (tab > 0) out.add(new String[] { line.substring(tab + 1), line.substring(0, tab) });
			}
			r.close();
		} catch (Exception e) {
			Log.i(TAG, "no bundled data manifest");
		}
		return out;
	}

	private void unpack(File root, List<String[]> files) throws Exception {
		long total = 0, done = 0;
		for (String[] f : files) total += Long.parseLong(f[1]);
		AssetManager am = getAssets();
		byte[] buffer = new byte[1 << 20];
		int last = -1;
		for (String[] f : files) {
			File out = new File(root, f[0]);
			out.getParentFile().mkdirs();
			// only big archives are worth skipping; small files can change without changing size
			if (out.exists() && out.length() == Long.parseLong(f[1]) && out.length() > 4194304L) { done += out.length(); continue; }
			InputStream in = am.open("fstop_data/" + f[0], AssetManager.ACCESS_STREAMING);
			FileOutputStream os = new FileOutputStream(out);
			int n;
			while ((n = in.read(buffer)) > 0) {
				os.write(buffer, 0, n);
				done += n;
				final int p = (int) (done * 1000 / Math.max(total, 1));
				if (p != last) {
					last = p;
					runOnUiThread(new Progress(p));
				}
			}
			os.close();
			in.close();
		}
	}

	private void launch() {
		Intent intent = new Intent(this, SDLActivity.class);
		intent.putExtra("gamedir", "fstop");
		startActivity(intent);
		finish();
	}
}
