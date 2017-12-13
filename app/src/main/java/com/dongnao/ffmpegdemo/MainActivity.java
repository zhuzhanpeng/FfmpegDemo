package com.dongnao.ffmpegdemo;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;

import java.io.File;
public class MainActivity extends Activity {
    String path="/sdcard/input.mp4";
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
callMerge();
    }

    public native void nativeSyncronize(String path,Surface surface);
    public native void playNativeVideo(String path,Surface surface);
    public native void playNativeAudio(String path);
    public native void nativeTranscoding(String path);
    public native void nativeSplitVideo(String path);
    public native void natveMergeVideo(String intputName1,
                                       String inputName2,
                                       String inputName3,
                                       String inputName4,
                                       String inputName5);

    public void playVideo(View view){
        playNativeVideo(path,surfaceView.getHolder().getSurface());
    }
    public void playAudio(View view){
        playNativeAudio(path);
    }
    public void transcoding(View view){
        nativeTranscoding(path);
    }
    public void splitVideo(View view){
        nativeSplitVideo(path);
    }
    long start;
    public void mergeVideo(View view){
        callMerge();
    }
    public void syncronize(View view){
        String input = new File(Environment.getExternalStorageDirectory(), "input.mp4").getAbsolutePath();
        nativeSyncronize(input,surfaceView.getHolder().getSurface());
    }

    private void callMerge(){
        start=System.currentTimeMillis();
        natveMergeVideo("/sdcard/combine1.mp4",
                "/sdcard/combine2.mp4",
                "/sdcard/combine3.mp4",
                "/sdcard/combine4.mp4",
                "/sdcard/combine5.mp4");
        Log.e("time", "mergeVideo:"+(System.currentTimeMillis()-start));;
    }
}
