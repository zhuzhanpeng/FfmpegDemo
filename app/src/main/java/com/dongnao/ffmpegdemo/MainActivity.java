package com.dongnao.ffmpegdemo;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Toast;

import java.io.File;
public class MainActivity extends Activity {
    String path="http://joymedia.oss-cn-hangzhou.aliyuncs.com/joyMedia/live_id_41.m3u8";
    static{
        System.loadLibrary("native-lib");
    }
    private SurfaceView surfaceView;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        CrashHandler.getInstance().init(this,"");
        setContentView(R.layout.activity_main);
        surfaceView= (SurfaceView) findViewById(R.id.surfaceView);

    }

    public native void playNativeVideo(String path, Surface surface);
    public native void playNativeAudio(String path);
    public native void nativeSyncronize(String path);
    public native void nativeTranscoding(String path);
    public native void nativeSplitVideo(String path);
    public void playVideo(View view) {
        String input = new File(Environment.getExternalStorageDirectory(), "input.mp4").getAbsolutePath();
        Toast.makeText(this,""+input,Toast.LENGTH_SHORT).show();
        playNativeVideo(path,surfaceView.getHolder().getSurface());
    }

    public void playAudio(View view){
        String input = new File(Environment.getExternalStorageDirectory(), "input.mp4").getAbsolutePath();
        playNativeAudio(input);
    }

    public void syncronize(View view){
        String input = new File(Environment.getExternalStorageDirectory(), "input.mp4").getAbsolutePath();
        nativeSyncronize(input);
    }

    public void transcoding(View view){
        String input = new File(Environment.getExternalStorageDirectory(), "input.mp4").getAbsolutePath();
        nativeTranscoding(input);
    }

    public void splitVideo(View view){
        String input = new File(Environment.getExternalStorageDirectory(), "input.mp4").getAbsolutePath();
        long startTime=System.currentTimeMillis();
        nativeSplitVideo(input);
        long endTime=System.currentTimeMillis();
        Log.e("MainActivity", "splitVideo: "+(endTime-startTime));
    }
}
