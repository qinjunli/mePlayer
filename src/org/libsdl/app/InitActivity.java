package org.libsdl.app;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.ImageButton;
import android.view.View.OnClickListener;

public class InitActivity extends Activity {

	public  ImageButton localFileButton;
	public  ImageButton streamFileButton;
	//public static EditText	  inputText;
	public  AlertDialog.Builder dialogBuilder;
	
	public EditText siteInput;
	public View inputView;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		
		//隐藏标题栏
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        //隐藏信号栏
        getWindow().setFlags(
        		WindowManager.LayoutParams.FLAG_FULLSCREEN, 
        		WindowManager.LayoutParams.FLAG_FULLSCREEN);
		setContentView(R.layout.init);
		
		localFileButton=(ImageButton)findViewById(R.id.localFButton);
		streamFileButton=(ImageButton)findViewById(R.id.streamFButton);
		
		
		
			
						
		regFunc();				
	}
	
	public void regFunc()
	{
		dialogBuilder=new AlertDialog.Builder(this);

		streamFileButton.setOnClickListener(new OnClickListener(){
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				LayoutInflater layoutInflater=(LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);
				inputView=layoutInflater.inflate(R.layout.text_view, null);
				siteInput=(EditText)inputView.findViewById(R.id.editSiteInput);
			
				dialogBuilder.setView(inputView).setTitle("请输入流媒体地址")
					.setPositiveButton("打开", new DialogInterface.OnClickListener(){
	
						@Override
						public void onClick(DialogInterface arg0, int arg1) {
							// TODO Auto-generated method stub
							String input=siteInput.getText().toString();
							arg0.dismiss();
							startPlayer(input);
						}
						
					}).setNegativeButton("取消", new DialogInterface.OnClickListener(){
	
						@Override
						public void onClick(DialogInterface arg0, int arg1) {
							// TODO Auto-generated method stub
							arg0.dismiss();
						}
						
					}).show();
			}
		});
		
		localFileButton.setOnClickListener(new OnClickListener(){

			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				startFileExplorer();
			}
			
		});
	}
	private void startPlayer(String filename)
	{
		Intent i=new Intent(this,SDLActivity.class);
		i.putExtra("inputfile", filename);
		startActivity(i);
	}
	
	private void startFileExplorer()
	{
		Intent i=new Intent(this,FileExplorer.class);
		startActivity(i);
	}
	
	
}

