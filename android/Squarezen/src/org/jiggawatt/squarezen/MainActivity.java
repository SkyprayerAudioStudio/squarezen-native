package org.jiggawatt.squarezen;

import java.io.File;
import java.io.FilenameFilter;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;

import android.app.ActionBar;
import android.app.FragmentTransaction;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.os.Environment;
import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

public class MainActivity extends Activity implements AudioManager.OnAudioFocusChangeListener {
	private static final int SAMPLE_RATE = 44100;
	
	private AudioManager audioManager;
	private AudioTrack audioTrack;
	private int minBufferSize;
	private ByteBuffer[] pcmFromNative;
	private byte[] buf;
	private Runnable audioRunner = null;
	private boolean stopAudioRunner, playing;
	private int bufferToPlay;
	private String[] ymFiles;
	private Thread t = null;
	private byte[] ymState;
	DrawingPanel drawingPanel;
	Window activityWindow = null;

	private class CustomListAdapter extends ArrayAdapter<String> {

	    private Context mContext;
	    private int id;
	    private List <String>items ;

	    public CustomListAdapter(Context context, int textViewResourceId , List<String> list ) 
	    {
	        super(context, textViewResourceId, list);           
	        mContext = context;
	        id = textViewResourceId;
	        items = list ;
	    }

	    @Override
	    public View getView(int position, View v, ViewGroup parent)
	    {
	        View mView = v ;
	        if(mView == null){
	            LayoutInflater vi = (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	            mView = vi.inflate(id, null);
	        }

	        TextView text = (TextView) mView.findViewById(R.id.textView);

	        if(items.get(position) != null )
	        {
	            text.setTextColor(Color.WHITE);
	            text.setText(items.get(position));
	            text.setBackgroundColor(Color.argb(96,0,0,0)); 
	        }

	        return mView;
	    }

	}
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        /*minBufferSize = AudioTrack.getMinBufferSize(SAMPLE_RATE,
        											AudioFormat.CHANNEL_OUT_STEREO,
        											AudioFormat.ENCODING_PCM_16BIT);
        Log.e("YmPlay", "minBufferSize = " + minBufferSize);
        minBufferSize *= 2;
        
        try {
            audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,
            		                    SAMPLE_RATE,
            		                    AudioFormat.CHANNEL_OUT_STEREO,
            		                    AudioFormat.ENCODING_PCM_16BIT,
            		                    minBufferSize,
            		                    AudioTrack.MODE_STREAM);
        } catch (Exception e) {
        	Log.e("YmPlay", "Unable to create AudioTrack: " + e.toString());
        }
        
        pcmFromNative = new ByteBuffer[2];
        pcmFromNative[0] = ByteBuffer.allocateDirect(minBufferSize);
        pcmFromNative[1] = ByteBuffer.allocateDirect(minBufferSize);
        buf = new byte[minBufferSize*2];*/

        File dir = new File(Environment.getExternalStorageDirectory().getPath()+"/YM");
        ymFiles = dir.list(
        	    new FilenameFilter()
        	    {
        	        public boolean accept(File dir, String name)
        	        {
        	        	String nameUpper = name.toUpperCase();
        	            return nameUpper.endsWith(".YM") ||
        	            	   nameUpper.endsWith(".VGM") ||
        	            	   nameUpper.endsWith(".VGZ") ||
        	            	   nameUpper.endsWith(".GBS") ||
        	            	   nameUpper.endsWith(".NSF") ||
        	            	   nameUpper.endsWith(".SID");
        	        }
        	    });

        ArrayAdapter<String> adapter = new CustomListAdapter(this,
                R.layout.custom_list, Arrays.asList(ymFiles));
        ListView songsListView = (ListView) findViewById(R.id.listView1);
        Log.e("Squarezen", "listView = " + songsListView + ", adapter = " + adapter);
        songsListView.setAdapter(adapter);
        songsListView.setVisibility(View.VISIBLE);
        songsListView.bringToFront();

        Log.e("Squarezen", "songlist = " + ymFiles.length);
        
        songsListView.setOnItemClickListener(new ListView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> a, View v, int i, long l) {
            	stopSong();
            	playSong(ymFiles[i]);
                //Log.e("YmPlay", "Clicked item " + i + " (" + ymFiles[i] + ")");
            }
        });
         
        ymState = new byte[24];
        drawingPanel = (DrawingPanel) findViewById(R.id.surfaceView1);
        drawingPanel.setParent(this);
        
        stopAudioRunner = false;
        playing = false;
        
        audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);

        final Button button = (Button) findViewById(R.id.button1);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	Log.e("Squarezen", "Button1 clicked");
            	stopSong();
            	finish();
                // Perform action on click
            }
        });        
    }


    
    @Override
    public void onStart() {
    	setVolumeControlStream(AudioManager.STREAM_MUSIC);
    	super.onStart();
    }
    
    
    @Override
    public void onAudioFocusChange(int focusChange) {
    	if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
            // Pause playback
        } else if (focusChange == AudioManager.AUDIOFOCUS_GAIN) {
            // Resume playback      	
        } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS) {
        	stopSong();
        }   	
    }
    
    public void stopSong() {
    	if (playing) {
	    	stopAudioRunner = true;
	    	//while (stopAudioRunner) {}
	    	//audioTrack.stop();
	    	Close();
	    	playing = false;
	    	audioRunner = null;
    	}
    	audioManager.abandonAudioFocus(this);
    }
    
    public void playSong(String songName) {
    	int result = audioManager.requestAudioFocus(this, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
    	if (result != AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
    		//am.unregisterMediaButtonEventReceiver(RemoteControlReceiver);
    		Log.e("Squarezen", "Failed to get audio focus");
    	}

    	Prepare(Environment.getExternalStorageDirectory().getPath() + "/YM/" + songName);
    	playing = true;
    	
    	try {
	    	TextView titleText = (TextView) findViewById(R.id.textView1);
	    	byte[] title = GetTitle();
	    	titleText.setText(new String(title, 0, title.length, "ISO-8859-1"));
	    	
	    	TextView authorText = (TextView) findViewById(R.id.textView2);
	    	byte[] author = GetArtist();
	    	authorText.setText(new String(author, 0, author.length, "ISO-8859-1"));
    	} catch (Exception e) {
    		Log.e("Squarezen", e.toString());
    	}
  	        
        activityWindow = getWindow();
        activityWindow.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);       		
    }

    
    @Override
    public void onStop() {
    	//stopSong();
    	setVolumeControlStream(AudioManager.USE_DEFAULT_STREAM_TYPE);
    	super.onStop();
    }
    
    
    @Override
    public void onDestroy() {
    	//stopSong();
    	if (activityWindow != null) {
    		activityWindow.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    	}
    	Close();
    	Exit();
    	super.onDestroy();
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) { //, MenuInflater menuInflater) {
        // Inflate the menu; this adds items to the action bar if it is present.
    	getMenuInflater().inflate(R.menu.main, menu);
    	//super.onCreateOptionsMenu(menu,  menuInflater);
    	return true;
    }

        
    static {
    	System.loadLibrary("emu-players");
    }
    
    public native void Prepare(String filePath);
    public native void Close();
    public native void Exit();
    public native void GetBuffer(ByteBuffer byteBuffer);
    public native byte[] GetTitle();
    public native byte[] GetArtist();
    public native void Run(int numSamples, ByteBuffer byteBuffer);
    public native void NextSubSong();
    
    //public native void GetState(byte[] state);    
}
