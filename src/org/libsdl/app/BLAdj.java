package org.libsdl.app;

import android.provider.Settings;
import android.util.Log;
import android.view.WindowManager;

public class BLAdj {
	public static void adjBL(int native_bl)
	{
		bl=native_bl;
		
		Settings.System.putInt(SDLActivity.mSingleton.getContentResolver(),
				android.provider.Settings.System.SCREEN_BRIGHTNESS,
				bl); 
		
		bl = Settings.System.getInt(SDLActivity.mSingleton.getContentResolver(),
				Settings.System.SCREEN_BRIGHTNESS, -1);
		
		//---------------------------------------------------------
		WindowManager.LayoutParams lp = SDLActivity.mSingleton.getWindow().getAttributes();
		Float tmpFloat = (float)bl / 255;
		if (0 < tmpFloat && tmpFloat <= 1) {
			lp.screenBrightness = tmpFloat;
		}

		SDLActivity.mSingleton.getWindow().setAttributes(lp);
		
		Log.d("java_adjBL", "adjusting bl in java");
	}
	
	public static int bl;
}
