package com.valvesoftware;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.util.Log;
import java.io.File;
import java.util.Locale;

// The engine's native code looks these methods up by this exact class name.
public class ValveActivity2 {
	public static native void setArgs(String args);
	public static native int setenv(String name, String value, int overwrite);

	// The engine loads its fonts by path from the app's private files folder.
	private static void copyFonts(Context context) {
		try {
			String[] names = context.getAssets().list("fonts");
			for (String name : names) {
				File out = new File(context.getExternalFilesDir(null), name);
				if (out.exists() && out.length() > 0) continue;
				java.io.InputStream in = context.getAssets().open("fonts/" + name);
				java.io.FileOutputStream os = new java.io.FileOutputStream(out);
				byte[] buffer = new byte[65536];
				int n;
				while ((n = in.read(buffer)) > 0) os.write(buffer, 0, n);
				os.close();
				in.close();
			}
		} catch (Exception e) {
			Log.e("FSTOP", "could not copy fonts", e);
		}
	}

	private static String gamePath(Context context) {
		return context.getExternalFilesDir(null).getAbsolutePath();
	}

	static public boolean preInit(Context context, Intent intent) {
		return new File(gamePath(context), "fstop/gameinfo.txt").exists();
	}

	static public void initNatives(Context context, Intent intent) {
		ApplicationInfo appinf = context.getApplicationInfo();
		String gamepath = gamePath(context);

		String extra = "";
		try {
			File f = new File(gamepath, "fstop_args.txt");
			if (f.exists() && (appinf.flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0) {
				java.io.BufferedReader r = new java.io.BufferedReader(new java.io.FileReader(f));
				String line = r.readLine();
				r.close();
				if (line != null) extra = " " + line.trim();
			}
		} catch (Exception e) {
			Log.w("FSTOP", "could not read args", e);
		}
		String argv = "-game fstop -nosteam -novid" + extra;

		// Earlier test builds opened up the private data folder; always keep it private.
		try { android.system.Os.chmod(appinf.dataDir, 0700); } catch (Exception e) { Log.w("FSTOP", "chmod", e); }

		copyFonts(context);

		setenv("LANG", Locale.getDefault().toString(), 1);
		setenv("APP_DATA_PATH", new File(gamepath).getParent(), 1);
		setenv("APP_LIB_PATH", appinf.nativeLibraryDir, 1);
		setenv("VALVE_GAME_PATH", gamepath, 1);

		// The on-screen controls only ever appear on devices that have a touchscreen.
		boolean touch = context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN);
		setenv("FSTOP_HAS_TOUCHSCREEN", touch ? "1" : "0", 1);

		Log.v("FSTOP", "argv=" + argv + " touch=" + touch);
		setArgs(argv);
	}
}
