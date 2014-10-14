package org.libsdl.app;

import java.io.File;
import java.util.Arrays;
import java.util.Comparator;

import android.app.ListActivity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.ListView;
import android.widget.TextView;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;

class FileExplorerAdapter extends BaseAdapter {
	
	private File[] 							mFiles;
	private LayoutInflater 					mInflater;
	private Context 						mContext;
	private boolean 						isTop;
	
	public FileExplorerAdapter(Context context, File[] files,boolean root) {
		mFiles = files;
        mInflater = LayoutInflater.from(context);
        mContext = context;
        isTop=root;
	}
	
	public int getCount() {
		return mFiles.length;
	}

	public Object getItem(int position) {
		return mFiles[position];
	}

	public long getItemId(int position) {
		return position;
	}
	
	private void setRow(ViewHolder holder, int position) {
		File file = mFiles[position];
		holder.text.setText(file.getName());
		
		if(position==0&&!isTop)
		{
			holder.text.setText("上级目录："+file.getName());
			holder.icon.setImageDrawable(mContext.getResources().getDrawable(R.drawable.back));
		}
		else if(file.isDirectory())
			holder.icon.setImageDrawable(mContext.getResources().getDrawable(R.drawable.folder1));
		else{
			if(FileExplorer.checkExtension(file))
				holder.icon.setImageDrawable(mContext.getResources().getDrawable(R.drawable.movie1));
			else
				holder.icon.setImageDrawable(mContext.getResources().getDrawable(R.drawable.applications));
		}
	}

	public View getView(int position, View convertView, ViewGroup parent) {
		// A ViewHolder keeps references to children views to avoid unneccessary calls
        // to findViewById() on each row.
        ViewHolder holder;

        // When convertView is not null, we can reuse it directly, there is no need
        // to reinflate it. We only inflate a new View when the convertView supplied
        // by ListView is null.
        if (convertView == null) {
            convertView = mInflater.inflate(R.layout.file_explorer_row, null);

            // Creates a ViewHolder and store references to the two children views
            // we want to bind data to.
            holder = new ViewHolder();
            holder.text = (TextView) convertView.findViewById(R.id.textview_rowtext);
            holder.icon = (ImageView) convertView.findViewById(R.id.imageview_rowicon);

            convertView.setTag(holder);
        } else {
            // Get the ViewHolder back to get fast access to the TextView
            // and the ImageView.
            holder = (ViewHolder) convertView.getTag();
        }

		// Bind the data efficiently with the holder.
		setRow(holder, position);
		
        return convertView;
	}
	
	private static class ViewHolder {
	    TextView text;
	    ImageView icon;
	}
}

public class FileExplorer extends ListActivity {

	private String 			mRoot = "/sdcard";
	private TextView 		mTextViewLocation;
	private File[]			mFiles;

	//public static final String EXTENSION =".avi.mp4.mkv.264";
	public static final String EXTENSION2[]={".avi",".mp4",".mkv",".264"};
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.file_explorer);
		mTextViewLocation = (TextView) findViewById(R.id.textview_path);
		getDirectory(mRoot);
	}
	
	protected static boolean checkExtension(File file) {
		//String[] exts = FFMpeg.EXTENSIONS;
		for(int i=0;i<EXTENSION2.length;i++) {
			if(file.getName().indexOf(EXTENSION2[i]) > 0) {
				return true;
			}
		}
		return false;
	}
	/*
	static class DirFilter implements FilenameFilter {  
		  
        private String filter[];  
  
        public DirFilter(String filter[]) {  
            this.filter = filter;  
        }  
  
        
        public boolean accept(File dir, String name) {  
            String f = new File(name).getName(); 
           
            for(int i=0;i<filter.length;i++)
            {
            	if (f.indexOf(filter[i])!=-1) {
					return true;
				}
            }
            return false;  
        }  
  
    }  
	/*
	private void listFiles() {
		// TODO Auto-generated method stub
		try{
			File f=new File(mRoot);
			File[] temp=f.listFiles(new DirFilter(EXTENSION2));
			//File[] temp=f.listFiles();
			mTextViewLocation.setText("Location: " + mRoot);
			
			mFiles=temp;
			setListAdapter(new FileExplorerAdapter(this, temp));
			
		}catch(Exception ex){
			
		}
	}
/*
	protected static boolean checkExtension(File file) {
		
		if(file.getName().indexOf(EXTENSION) > 0) {
			return true;
		}else
			return false;
	}
*/
	
	private void sortFilesByDirectory(File[] files) {
		Arrays.sort(files, new Comparator<File>() {
			public int compare(File f1, File f2) {
				return Long.valueOf(f1.length()).compareTo(f2.length());
			}
		});
	}

	private void getDirectory(String dirPath) {
		try {
			mTextViewLocation.setText("当前目录:  " + dirPath);
	
			File f = new File(dirPath);
			File[] temp = f.listFiles();
			
			sortFilesByDirectory(temp);
			
			File[] files = null;
			if(!dirPath.equals(mRoot)) {
				files = new File[temp.length + 1];
				System.arraycopy(temp, 0, files, 1, temp.length);
				files[0] = new File(f.getParent());
			} else {
				files = temp;
			}
			
			mFiles = files;
			setListAdapter(new FileExplorerAdapter(this, files, temp.length == files.length));
		} catch(Exception ex) {
//			ffplayAndroid.show(this, "Error", ex.getMessage());
		}
	}
	
	
	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {
		File file = mFiles[position];

		if (file.isDirectory()) {
			if (file.canRead())
				getDirectory(file.getAbsolutePath());
		}else 
			if(checkExtension(file)) 
				startPlayer(file.getAbsolutePath());
	}
	
	private void startPlayer(String filePath) {
    	Intent i = new Intent(this, SDLActivity.class);
    	i.putExtra("inputfile", filePath);
    	//SDLActivity.nativeSetfile(filePath);
    	startActivity(i);
    }	
}
