package org.libsdl.app;

import java.util.Timer;
import java.util.TimerTask;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import android.app.*;
import android.content.*;
import android.content.pm.ActivityInfo;
import android.view.*;
import android.view.ViewGroup.LayoutParams;
import android.widget.ImageButton;
import android.widget.PopupWindow;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.*;
import android.os.PowerManager.WakeLock;
import android.provider.Settings;
import android.util.Log;
import android.graphics.*;
import android.hardware.*;


/**
    SDL Activity
*/
public class SDLActivity extends Activity {

    // Main components
    public static SDLActivity mSingleton;
    public static SDLSurface mSurface;

    // This is what SDL runs in. It invokes SDL_main(), eventually
    private static Thread mSDLThread;

    // Audio
    private static Thread mAudioThread;
    private static AudioTrack mAudioTrack;

    // EGL private objects
    private static EGLContext  mEGLContext;
    private static EGLSurface  mEGLSurface;
    private static EGLDisplay  mEGLDisplay;
    private static EGLConfig   mEGLConfig;
    private static int mGLMajor, mGLMinor;
    
    PowerManager powerManager = null;
    WakeLock wakeLock = null;

    // Load the .so
    static {
        System.loadLibrary("SDL2");
        System.loadLibrary("ffmpeg");
        //System.loadLibrary("SDL2_image");
        //System.loadLibrary("SDL2_mixer");
        //System.loadLibrary("SDL2_ttf");
        System.loadLibrary("main");
    }

    // Setup
    protected void onCreate(Bundle savedInstanceState) {
        //Log.v("SDL", "onCreate()");
        super.onCreate(savedInstanceState);
        
        //隐藏标题栏
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        //隐藏信号栏
        getWindow().setFlags(
        		WindowManager.LayoutParams.FLAG_FULLSCREEN, 
        		WindowManager.LayoutParams.FLAG_FULLSCREEN);
        //保持背光常亮
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        //设置默认横屏显示
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        // So we can call stuff from static callbacks
        mSingleton = this;
        // Set up the surface
        
        
        Intent i=getIntent();
        String name=i.getStringExtra("inputfile");
        
        nativeSetfile(name);
        
        mSurface = new SDLSurface(getApplication());
        setContentView(mSurface);
        //SurfaceHolder holder = mSurface.getHolder();
        
       
    }

    
    // Events
    protected void onPause() {
        Log.v("SDL", "onPause()");
        super.onPause();
        SDLActivity.nativePause();
    }

    protected void onResume() {
        Log.v("SDL", "onResume()");
        super.onResume();
        SDLActivity.nativeResume();
    }

    protected void onDestroy() {
        super.onDestroy();
        Log.v("SDL", "onDestroy()");
        // Send a quit message to the application
        SDLActivity.nativeQuit();

        // Now wait for the SDL thread to quit
        if (mSDLThread != null) {
            try {
                mSDLThread.join();
            } catch(Exception e) {
                Log.v("SDL", "Problem stopping thread: " + e);
            }
            mSDLThread = null;

            //Log.v("SDL", "Finished waiting for SDL thread");
        }
    }

    // Messages from the SDLMain thread
    static int COMMAND_CHANGE_TITLE = 1;

    // Handler for the messages
    Handler commandHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (msg.arg1 == COMMAND_CHANGE_TITLE) {
                setTitle((String)msg.obj);
            }
        }
    };

    // Send a message from the SDLMain thread
    void sendCommand(int command, Object data) {
        Message msg = commandHandler.obtainMessage();
        msg.arg1 = command;
        msg.obj = data;
        commandHandler.sendMessage(msg);
    }
    public static void quit(){
    	nativeQuit2();
    	mSingleton.finish();
    	//mSingleton.wakeLock.release();
    }
    // C functions we call
    public static native void nativeSetfile(String fileString);
    public static native void nativeInit();
    public static native void nativeQuit();
    public static native void nativeQuit2();
    public static native void nativePause();
    public static native void nativePause2();
    public static native void nativeResume();
    public static native void nativeResume2();
    public static native void onNativeResize(int x, int y, int format);
    public static native void onNativeKeyDown(int keycode);
    public static native void onNativeKeyUp(int keycode);
    public static native void onNativeTouch(int touchDevId, int pointerFingerId,
                                            int action, float x, 
                                            float y, float p);
    public static native void onNativeAccel(float x, float y, float z);
    public static native void nativeRunAudioThread();


    // Java functions called from C
    
    public static boolean createGLContext(int majorVersion, int minorVersion) {
        return initEGL(majorVersion, minorVersion);
    }
    
    public static void flipBuffers() {
        flipEGL();
    }

    public static void setActivityTitle(String title) {
        // Called from SDLMain() thread and can't directly affect the view
        mSingleton.sendCommand(COMMAND_CHANGE_TITLE, title);
    }

    public static Context getContext() {
        return mSingleton;
    }

    public static void startApp() {
        // Start up the C app thread
        if (mSDLThread == null) {
            mSDLThread = new Thread(new SDLMain(), "SDLThread");
            mSDLThread.start();
        }
        else {
            SDLActivity.nativeResume();
        }
        /*
        if (mAdjThread==null) {
			mAdjThread=new Thread(new BLAdjThread(),"blAdjThread");
			mAdjThread.start();
		}
		*/
        //mSingleton.wakeLock.acquire();
    }
    
    // EGL functions
    public static boolean initEGL(int majorVersion, int minorVersion) {
        if (SDLActivity.mEGLDisplay == null) {
            //Log.v("SDL", "Starting up OpenGL ES " + majorVersion + "." + minorVersion);

            try {
                EGL10 egl = (EGL10)EGLContext.getEGL();

                EGLDisplay dpy = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

                int[] version = new int[2];
                egl.eglInitialize(dpy, version);

                int EGL_OPENGL_ES_BIT = 1;
                int EGL_OPENGL_ES2_BIT = 4;
                int renderableType = 0;
                if (majorVersion == 2) {
                    renderableType = EGL_OPENGL_ES2_BIT;
                } else if (majorVersion == 1) {
                    renderableType = EGL_OPENGL_ES_BIT;
                }
                int[] configSpec = {
                    //EGL10.EGL_DEPTH_SIZE,   16,
                    EGL10.EGL_RENDERABLE_TYPE, renderableType,
                    EGL10.EGL_NONE
                };
                EGLConfig[] configs = new EGLConfig[1];
                int[] num_config = new int[1];
                if (!egl.eglChooseConfig(dpy, configSpec, configs, 1, num_config) || num_config[0] == 0) {
                    Log.e("SDL", "No EGL config available");
                    return false;
                }
                EGLConfig config = configs[0];

                int EGL_CONTEXT_CLIENT_VERSION=0x3098;
                int contextAttrs[] = new int[] { EGL_CONTEXT_CLIENT_VERSION, majorVersion, EGL10.EGL_NONE };
                EGLContext ctx = egl.eglCreateContext(dpy, config, EGL10.EGL_NO_CONTEXT, contextAttrs);

                if (ctx == EGL10.EGL_NO_CONTEXT) {
                    Log.e("SDL", "Couldn't create context");
                    return false;
                }
                SDLActivity.mEGLContext = ctx;
                SDLActivity.mEGLDisplay = dpy;
                SDLActivity.mEGLConfig = config;
                SDLActivity.mGLMajor = majorVersion;
                SDLActivity.mGLMinor = minorVersion;

                SDLActivity.createEGLSurface();
            } catch(Exception e) {
                Log.v("SDL", e + "");
                for (StackTraceElement s : e.getStackTrace()) {
                    Log.v("SDL", s.toString());
                }
            }
        }
        else SDLActivity.createEGLSurface();

        return true;
    }

    public static boolean createEGLContext() {
        EGL10 egl = (EGL10)EGLContext.getEGL();
        int EGL_CONTEXT_CLIENT_VERSION=0x3098;
        int contextAttrs[] = new int[] { EGL_CONTEXT_CLIENT_VERSION, SDLActivity.mGLMajor, EGL10.EGL_NONE };
        SDLActivity.mEGLContext = egl.eglCreateContext(SDLActivity.mEGLDisplay, SDLActivity.mEGLConfig, EGL10.EGL_NO_CONTEXT, contextAttrs);
        if (SDLActivity.mEGLContext == EGL10.EGL_NO_CONTEXT) {
            Log.e("SDL", "Couldn't create context");
            return false;
        }
        return true;
    }

    public static boolean createEGLSurface() {
        if (SDLActivity.mEGLDisplay != null && SDLActivity.mEGLConfig != null) {
            EGL10 egl = (EGL10)EGLContext.getEGL();
            if (SDLActivity.mEGLContext == null) createEGLContext();

            Log.v("SDL", "Creating new EGL Surface");
            EGLSurface surface = egl.eglCreateWindowSurface(SDLActivity.mEGLDisplay, SDLActivity.mEGLConfig, SDLActivity.mSurface, null);
            if (surface == EGL10.EGL_NO_SURFACE) {
                Log.e("SDL", "Couldn't create surface");
                return false;
            }

            if (!egl.eglMakeCurrent(SDLActivity.mEGLDisplay, surface, surface, SDLActivity.mEGLContext)) {
                Log.e("SDL", "Old EGL Context doesnt work, trying with a new one");
                createEGLContext();
                if (!egl.eglMakeCurrent(SDLActivity.mEGLDisplay, surface, surface, SDLActivity.mEGLContext)) {
                    Log.e("SDL", "Failed making EGL Context current");
                    return false;
                }
            }
            SDLActivity.mEGLSurface = surface;
            return true;
        }
        return false;
    }

    // EGL buffer flip
    public static void flipEGL() {
        try {
            EGL10 egl = (EGL10)EGLContext.getEGL();

            egl.eglWaitNative(EGL10.EGL_CORE_NATIVE_ENGINE, null);

            // drawing here

            egl.eglWaitGL();

            egl.eglSwapBuffers(SDLActivity.mEGLDisplay, SDLActivity.mEGLSurface);


        } catch(Exception e) {
            Log.v("SDL", "flipEGL(): " + e);
            for (StackTraceElement s : e.getStackTrace()) {
                Log.v("SDL", s.toString());
            }
        }
    }

    // Audio
    private static Object buf;
    
    public static Object audioInit(int sampleRate, boolean is16Bit, boolean isStereo, int desiredFrames) {
        int channelConfig = isStereo ? AudioFormat.CHANNEL_CONFIGURATION_STEREO : AudioFormat.CHANNEL_CONFIGURATION_MONO;
        int audioFormat = is16Bit ? AudioFormat.ENCODING_PCM_16BIT : AudioFormat.ENCODING_PCM_8BIT;
        int frameSize = (isStereo ? 2 : 1) * (is16Bit ? 2 : 1);
        
        Log.v("SDL", "SDL audio: wanted " + (isStereo ? "stereo" : "mono") + " " + (is16Bit ? "16-bit" : "8-bit") + " " + ((float)sampleRate / 1000f) + "kHz, " + desiredFrames + " frames buffer");
        
        // Let the user pick a larger buffer if they really want -- but ye
        // gods they probably shouldn't, the minimums are horrifyingly high
        // latency already
        desiredFrames = Math.max(desiredFrames, (AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat) + frameSize - 1) / frameSize);
        
        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
                channelConfig, audioFormat, desiredFrames * frameSize, AudioTrack.MODE_STREAM);
        
        audioStartThread();
        
        Log.v("SDL", "SDL audio: got " + ((mAudioTrack.getChannelCount() >= 2) ? "stereo" : "mono") + " " + ((mAudioTrack.getAudioFormat() == AudioFormat.ENCODING_PCM_16BIT) ? "16-bit" : "8-bit") + " " + ((float)mAudioTrack.getSampleRate() / 1000f) + "kHz, " + desiredFrames + " frames buffer");
        
        if (is16Bit) {
            buf = new short[desiredFrames * (isStereo ? 2 : 1)];
        } else {
            buf = new byte[desiredFrames * (isStereo ? 2 : 1)]; 
        }
        return buf;
    }
    
    public static void audioStartThread() {
        mAudioThread = new Thread(new Runnable() {
            public void run() {
                mAudioTrack.play();
                nativeRunAudioThread();
            }
        });
        
        // I'd take REALTIME if I could get it!
        mAudioThread.setPriority(Thread.MAX_PRIORITY);
        mAudioThread.start();
    }
    
    public static void audioWriteShortBuffer(short[] buffer) {
        for (int i = 0; i < buffer.length; ) {
            int result = mAudioTrack.write(buffer, i, buffer.length - i);
            if (result > 0) {
                i += result;
            } else if (result == 0) {
                try {
                    Thread.sleep(1);
                } catch(InterruptedException e) {
                    // Nom nom
                }
            } else {
                Log.w("SDL", "SDL audio: error return from write(short)");
                return;
            }
        }
    }
    
    public static void audioWriteByteBuffer(byte[] buffer) {
        for (int i = 0; i < buffer.length; ) {
            int result = mAudioTrack.write(buffer, i, buffer.length - i);
            if (result > 0) {
                i += result;
            } else if (result == 0) {
                try {
                    Thread.sleep(1);
                } catch(InterruptedException e) {
                    // Nom nom
                }
            } else {
                Log.w("SDL", "SDL audio: error return from write(short)");
                return;
            }
        }
    }

    public static void audioQuit() {
        if (mAudioThread != null) {
            try {
                mAudioThread.join();
            } catch(Exception e) {
                Log.v("SDL", "Problem stopping audio thread: " + e);
            }
            mAudioThread = null;

            //Log.v("SDL", "Finished waiting for audio thread");
        }

        if (mAudioTrack != null) {
            mAudioTrack.stop();
            mAudioTrack = null;
        }
    }

}

/**
    Simple nativeInit() runnable
*/
class SDLMain implements Runnable {
    public void run() {
        // Runs SDL_main()
        SDLActivity.nativeInit();

        //Log.v("SDL", "SDL thread terminated");
    }
}

/**
    SDLSurface. This is what we draw on, so we need to know when it's created
    in order to do anything useful. 

    Because of this, that's where we set up the SDL thread
*/
class SDLSurface extends SurfaceView implements SurfaceHolder.Callback,
    View.OnTouchListener {

    // Sensors
    private static SensorManager mSensorManager;
    private	PopupWindow popupButtonWindow;
    private PopupWindow popupSeekbarWindow;
    private ImageButton playButton;
    private ImageButton quitButton;
    private ImageButton energyCtrlButton;
    private SeekBar blCtrlSeekBar;
    private TextView    blTextView;
    private int lowBlStore=110;//做像素补偿时 背光初始值
	protected boolean isplaying=true;
	//0 代表高亮度 、1 代表低亮度 不补偿、2 代表低亮度 +像素点补偿
	protected int energyState=0;
	Timer timer=new Timer(true);
	
	private Handler handler=null;
	
	private Thread mBLThread;
	
	private Context context;
	
/*
	class RunnableUI implements Runnable
	{
		RunnableUI(int bl)
		{
			mBL=bl;
		}
		@Override
		public void run() {
			// TODO Auto-generated method stub
			blCtrlSeekBar.setProgress(mBL);
			blTextView.setText("亮度"+mBL);
			java_setBl(mBL,true);
		}
		private int mBL;
	}
	
	class BLThread implements Runnable
	{
		@Override
		public void run() {
			// TODO Auto-generated method stub
			while(!isplaying||energyState!=2)
			{
				try {
					Thread.sleep(300);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			while(isplaying&&energyState==2)
			{
				int bl=getBl();
				Log.d("blThread", "bl:"+bl);
				handler.post(new RunnableUI(bl));
				try {
					Thread.sleep(100);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}
	}
	*/
	
    // Startup    
    public SDLSurface(Context context) {
        super(context);
        this.context=context;
        getHolder().addCallback(this); 
    
        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
        //setOnKeyListener(this); 
        setOnTouchListener(this);
        
        createPopupWindow();
        //handler=new Handler();
        //mBLThread=new Thread(new BLThread(),"bl adj from c thread");
        
        //java_setBl(255,false);
       // updateSeekBarWindow();
       // mSensorManager = (SensorManager)context.getSystemService("sensor");  */
    }
/*
    private void updateSeekBarWindow() {
		// TODO Auto-generated method stub
		int tempInt=Settings.System.getInt(context.getContentResolver(), 
				Settings.System.SCREEN_BRIGHTNESS,-1);
		blCtrlSeekBar.setProgress(tempInt);
		blTextView.setText("亮度"+tempInt);
	}
    
*/
	private void createPopupWindow() {
		// TODO Auto-generated method stub
		LayoutInflater layoutInflater=(LayoutInflater)context.
				getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		View popButtonView=layoutInflater.inflate(R.layout.pop_button_window, null);
		popupButtonWindow=new PopupWindow(popButtonView,LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT);
		
		//View popSeekView=layoutInflater.inflate(R.layout.pop_seek_window, null);
		//popupSeekbarWindow=new PopupWindow(popSeekView,LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT);
		
		playButton=(ImageButton)popButtonView.findViewById(R.id.playButton);
		quitButton=(ImageButton)popButtonView.findViewById(R.id.exitButton);
		energyCtrlButton=(ImageButton)popButtonView.findViewById(R.id.energyButton);
		//blCtrlSeekBar=(SeekBar)popSeekView.findViewById(R.id.seekBar1);
		//blTextView=(TextView)popSeekView.findViewById(R.id.textView1);
		
		
		playButton.setOnClickListener(new OnClickListener() {
			
			
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				timer.cancel();
				if(isplaying){
					playButton.setImageResource(R.drawable.play3_40);
					SDLActivity.nativePause2();
					isplaying=false;
				}else {
					playButton.setImageResource(R.drawable.pause3_40);
					SDLActivity.nativeResume2();
					isplaying=true;
				}
				rearrangeTimer();
			}
		}
		);
		
		/*
		energyCtrlButton.setOnClickListener(new OnClickListener() {
			
			
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				energyState=(energyState+1)%3;
				
				timer.cancel();
				switch (energyState) {
				case 0:
					energyCtrlButton.setImageResource(R.drawable.energy_off);
					java_setBl(255,false);
					updateSeekBarWindow();
					break;
				case 1:
					energyCtrlButton.setImageResource(R.drawable.low_bl_no_comp);
					java_setBl(lowBlStore,false);
					updateSeekBarWindow();
					break;
				case 2:
					energyCtrlButton.setImageResource(R.drawable.energy_on);
					break;
				default:
					break;
				}
				energySwitch();
				rearrangeTimer();
			}
		});
		*/
		/*
		blCtrlSeekBar.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
			
			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				rearrangeTimer();
			}
			
			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				timer.cancel();
			}
			
			@Override
			public void onProgressChanged(SeekBar seekBar, int progress,
					boolean fromUser) {
				// TODO Auto-generated method stub
				if(fromUser)
				{
					int tempInt=seekBar.getProgress();
					lowBlStore=tempInt;
					blTextView.setText("亮度"+tempInt);
					java_setBl(tempInt, false);
				}
			}
		});
		*/
		
		quitButton.setOnClickListener(new OnClickListener() {
			
			
			public void onClick(View v) {
				// TODO Auto-generated method stub
				SDLActivity.quit();
			}
		});
	}
	/*
	//private->public can called by C or not?
    public void java_setBl(int blParam,boolean fromC) {
    	Settings.System.putInt(context.getContentResolver(),
				Settings.System.SCREEN_BRIGHTNESS,blParam);
		int tempInt = Settings.System.getInt(context.getContentResolver(),
				Settings.System.SCREEN_BRIGHTNESS, -1);
		
		WindowManager.LayoutParams lParams=SDLActivity.mSingleton.
				getWindow().getAttributes();
		Float tmpFloat=(float)tempInt/255;
		if (0 < tmpFloat && tmpFloat <= 1) {
			lParams.screenBrightness = tmpFloat;
		}
		SDLActivity.mSingleton.getWindow().setAttributes(lParams);
		if(!fromC)
			setBl(tempInt);
	}
*/
	// Called when we have a valid drawing surface
    public void surfaceCreated(SurfaceHolder holder) {
        Log.v("SDL", "surfaceCreated()");
        holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
        SDLActivity.createEGLSurface();
        //mBLThread.start();
    }

    // Called when we lose the surface
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.v("SDL", "surfaceDestroyed()");
        SDLActivity.nativePause();
    }

    // Called when the surface is resized
    public void surfaceChanged(SurfaceHolder holder,
                               int format, int width, int height) {
        Log.v("SDL", "surfaceChanged()");

        int sdlFormat = 0x85151002; // SDL_PIXELFORMAT_RGB565 by default
        switch (format) {
        case PixelFormat.A_8:
            Log.v("SDL", "pixel format A_8");
            break;
        case PixelFormat.LA_88:
            Log.v("SDL", "pixel format LA_88");
            break;
        case PixelFormat.L_8:
            Log.v("SDL", "pixel format L_8");
            break;
        case PixelFormat.RGBA_4444:
            Log.v("SDL", "pixel format RGBA_4444");
            sdlFormat = 0x85421002; // SDL_PIXELFORMAT_RGBA4444
            break;
        case PixelFormat.RGBA_5551:
            Log.v("SDL", "pixel format RGBA_5551");
            sdlFormat = 0x85441002; // SDL_PIXELFORMAT_RGBA5551
            break;
        case PixelFormat.RGBA_8888:
            Log.v("SDL", "pixel format RGBA_8888");
            sdlFormat = 0x86462004; // SDL_PIXELFORMAT_RGBA8888
            break;
        case PixelFormat.RGBX_8888:
            Log.v("SDL", "pixel format RGBX_8888");
            sdlFormat = 0x86262004; // SDL_PIXELFORMAT_RGBX8888
            break;
        case PixelFormat.RGB_332:
            Log.v("SDL", "pixel format RGB_332");
            sdlFormat = 0x84110801; // SDL_PIXELFORMAT_RGB332
            break;
        case PixelFormat.RGB_565:
            Log.v("SDL", "pixel format RGB_565");
            sdlFormat = 0x85151002; // SDL_PIXELFORMAT_RGB565
            break;
        case PixelFormat.RGB_888:
            Log.v("SDL", "pixel format RGB_888");
            // Not sure this is right, maybe SDL_PIXELFORMAT_RGB24 instead?
            sdlFormat = 0x86161804; // SDL_PIXELFORMAT_RGB888
            break;
        default:
            Log.v("SDL", "pixel format unknown " + format);
            break;
        }
        SDLActivity.onNativeResize(width, height, sdlFormat);
        Log.v("SDL", "Window size:" + width + "x"+height);

        SDLActivity.startApp();
    }


    // Touch events
    public boolean onTouch(View v, MotionEvent event) {
        {
        	int eventAction=event.getAction();
        	
        	if(eventAction==MotionEvent.ACTION_DOWN){
        		if(null != popupButtonWindow && popupButtonWindow.isShowing()) {
        			timer.cancel();
        			popupButtonWindow.dismiss();
        			//popupSeekbarWindow.dismiss();
        		}else {
        			
        			popupButtonWindow.showAtLocation(this, 
        					Gravity.CENTER|Gravity.BOTTOM, 0, 0);
        			//popupSeekbarWindow.showAtLocation(this, 
        					//Gravity.CENTER|Gravity.TOP, 0, 0);
        			rearrangeTimer();
				}
             }
        	}
           
      return true;
   } 
    
    private void rearrangeTimer()
    {
    	timer=new Timer(true);
		timer.schedule(new TimerTask() {
			
			@Override
			public void run() {
				// TODO Auto-generated method stub
				if(popupButtonWindow.isShowing()){
					popupButtonWindow.dismiss();
					//popupSeekbarWindow.dismiss();
				}
			}
		}, 1000*3);
    }
    
    /*protected void energySwitch() {
		// TODO Auto-generated method stub
    	//native_energy_switch();
	}
    */
    //native method
    //private native int getBl();
   // private native void native_energy_switch();
   // private native void setBl(int bl);
    
}

