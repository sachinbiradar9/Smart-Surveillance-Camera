#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <time.h>
#include <windows.h>

using namespace std;
using namespace cv;

//our sensitivity value to be used in the absdiff() function
//for higher sensitivity, use a lower value
const static int SENSITIVITY_VALUE = 40;
//size of blur used to smooth the intensity image output from absdiff() function
const static int BLUR_SIZE = 10;
//these two can be toggled by pressing 'd' or 't'
bool debugMode;
bool trackingEnabled;

//int to string helper function
string intToString(int number){

	//this function has a number input and string output
	std::stringstream ss;
	ss << number;
	return ss.str();
}

string getDateTime(){
	//get the system time
	SYSTEMTIME theTime;
	GetLocalTime(&theTime);
	//create string to store the date and time
	string dateTime;
	//convert year to string
	string year = intToString(theTime.wYear);
	//use stringstream to add a leading '0' to the month (ie. 3 -> 03)
	//we use 'setw(2)' so that we force the string 2 characters wide with a zero in front of it.
	//if the month is '10' then it will remain '10'
	std::stringstream m;
	m<<std::setfill('0')<<std::setw(2)<< theTime.wMonth;
	string month = m.str();
	//day
	std::stringstream d;
	d<<std::setfill('0')<<std::setw(2)<< theTime.wDay;
	string day = d.str();
	//hour
	std::stringstream hr;
	hr<<setfill('0')<<std::setw(2)<<theTime.wHour;
	string hour = hr.str();
	//minute
	std::stringstream min;
	min<<setfill('0')<<std::setw(2)<<theTime.wMinute;
	string minute = min.str();
	//second
	std::stringstream sec;
	sec<<setfill('0')<<std::setw(2)<<theTime.wSecond;
	string second = sec.str();

	//here we use the year, month, day, hour, minute info to create a custom string
	//this will be displayed in the bottom left corner of our video feed.
	dateTime = year + "-" + month + "-" + day + "  " + hour + ":" + minute + ":" + second;

	return dateTime;
}
string getDateTimeForFile(){
	//this function does the exact same as "getDateTime()" 
	//however it returns a string that can be used as a filename
	SYSTEMTIME theTime;
	GetLocalTime(&theTime);
	string dateTime;

	string year = intToString(theTime.wYear);

	std::stringstream m;
	m<<std::setfill('0')<<std::setw(2)<< theTime.wMonth;
	string month = m.str();

	std::stringstream d;
	d<<std::setfill('0')<<std::setw(2)<< theTime.wDay;
	string day = d.str();

	std::stringstream hr;
	hr<<setfill('0')<<std::setw(2)<<theTime.wHour;
	string hour = hr.str();

	std::stringstream min;
	min<<setfill('0')<<std::setw(2)<<theTime.wMinute;
	string minute = min.str();

	std::stringstream sec;
	sec<<setfill('0')<<std::setw(2)<<theTime.wSecond;
	string second = sec.str();

	//here we use "_" instead of "-" and ":"
	//if we try to save a filename with "-" or ":" in it we will get an error.
	dateTime = year + "_" + month + "_" + day + "_" + hour + "h" + minute + "m" + second + "s";

	return dateTime;
}
bool detectMotion(Mat thresholdImage, Mat &cameraFeed){
	//create motionDetected variable.
	bool motionDetected = false;
	//create temp Mat for threshold image
	Mat temp;
	thresholdImage.copyTo(temp);
	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	//find contours of filtered image using openCV findContours function
	//findContours(temp,contours,hierarchy,CV_RETR_CCOMP,CV_CHAIN_APPROX_SIMPLE );// retrieves all contours
	findContours(temp,contours,hierarchy,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE );// retrieves external contours

	//if contours vector is not empty, we have found some objects
	//we can simply say that if the vector is not empty, motion in the video feed has been detected.
	if(contours.size()>0)motionDetected=true;
	else motionDetected = false;

	return motionDetected;

}
int main(){
	//set recording and startNewRecording initially to false.
	bool recording = false;
	bool startNewRecording = false;
	int inc=0;
	bool firstRun = true;
	//if motion is detected in the video feed, we will know to start recording.
	bool motionDetected = false;

	//pause and resume code (if needed)
	bool pause = false;
	//set debug mode and trackingenabled initially to false
	//these can be toggled using 'd' and 't'
	debugMode = false;
	trackingEnabled = false;
	//set up the matrices that we will need
	//the two frames we will be comparing
	Mat frame1,frame2;
	//their grayscale images (needed for absdiff() function)
	Mat grayImage1,grayImage2;
	//resulting difference image
	Mat differenceImage;
	//thresholded difference image (for use in findContours() function)
	Mat thresholdImage;
	//video capture object.
	VideoCapture capture;
	capture.open(0);
	VideoWriter oVideoWriter;//create videoWriter object, not initialized yet
	double dWidth = capture.get(CV_CAP_PROP_FRAME_WIDTH); //get the width of frames of the video
	double dHeight = capture.get(CV_CAP_PROP_FRAME_HEIGHT); //get the height of frames of the video

	cout << "Frame Size = " << dWidth << "x" << dHeight << endl;

	//set framesize for use with videoWriter
	Size frameSize(static_cast<int>(dWidth), static_cast<int>(dHeight));

	if(!capture.isOpened()){
		cout<<"ERROR ACQUIRING VIDEO FEED\n";
		getchar();
		return -1;
	}
	while(1){

		if(startNewRecording==true){

			oVideoWriter  = VideoWriter("D:/MyVideo"+intToString(inc)+".avi", CV_FOURCC('D', 'I', 'V', '3'), 5, frameSize, true); //initialize the VideoWriter object 
			recording = true;
			startNewRecording = false;
			cout<<"New video file created D:/MyVideo"+intToString(inc)+".avi "<<endl;

			if ( !oVideoWriter.isOpened() ) //if not initialize the VideoWriter successfully, exit the program
			{
				cout << "ERROR: Failed to initialize video writing" << endl;
				getchar();
				return -1;
			}

		}

		//read first frame
		capture.read(frame1);
		//convert frame1 to gray scale for frame differencing
		cv::cvtColor(frame1,grayImage1,COLOR_BGR2GRAY);
		//copy second frame
		capture.read(frame2);
		//convert frame2 to gray scale for frame differencing
		cv::cvtColor(frame2,grayImage2,COLOR_BGR2GRAY);
		//perform frame differencing with the sequential images. This will output an "intensity image"
		//do not confuse this with a threshold image, we will need to perform thresholding afterwards.
		cv::absdiff(grayImage1,grayImage2,differenceImage);
		//threshold intensity image at a given sensitivity value
		cv::threshold(differenceImage,thresholdImage,SENSITIVITY_VALUE,255,THRESH_BINARY);
		if(debugMode==true){
			//show the difference image and threshold image
			cv::imshow("Difference Image",differenceImage);
			cv::imshow("Threshold Image", thresholdImage);
		}else{
			//if not in debug mode, destroy the windows so we don't see them anymore
			cv::destroyWindow("Difference Image");
			cv::destroyWindow("Threshold Image");
		}
		//blur the image to get rid of the noise. This will output an intensity image
		cv::blur(thresholdImage,thresholdImage,cv::Size(BLUR_SIZE,BLUR_SIZE));
		//threshold again to obtain binary image from blur output
		cv::threshold(thresholdImage,thresholdImage,SENSITIVITY_VALUE,255,THRESH_BINARY);
		if(debugMode==true){
			//show the threshold image after it's been "blurred"

			imshow("Final Threshold Image",thresholdImage);

		}
		else {
			//if not in debug mode, destroy the windows so we don't see them anymore
			cv::destroyWindow("Final Threshold Image");
		}

		//if tracking enabled, search for Motion
		if(trackingEnabled){
			//detectMotion function will return true if motion is detected, else it will return false.
			//set motionDetected boolean to the returned value.
			motionDetected = detectMotion(thresholdImage,frame1);
		}else{ 
			//reset our variables if tracking is disabled
			recording = false;
			motionDetected = false;
		}
		//draw time stamp to video in bottom left corner. We draw it before we write so that it is written on the video file.
		rectangle(frame1,Point(0,460),Point(200,480),Scalar(255,255,255),-1);
		putText(frame1,getDateTime(),Point(0,480),1,1,Scalar(0,0,0),2);

		//if we're in recording mode, write to file
		if(recording){

			oVideoWriter.write(frame1);
			//show "REC" in top left corner in red
			//be sure to do this AFTER you write to the file so that "REC" doesn't show up
			//on the recorded video.
			putText(frame1,"REC",Point(0,60),2,2,Scalar(0,0,255),2);


		}
		if(motionDetected){
			//show "MOTION DETECTED" in bottom left corner in green
			//once again, be sure to do this AFTER you write to the file so that "MOTION DETECTED" doesn't show up
			//on the recorded video. Place this code above if(recording) to see what I'm talking about.
			putText(frame1,"MOTION DETECTED",cv::Point(0,420),2,2,cv::Scalar(0,255,0));

			//set recording to true since there is motion in the video feed.
			recording = true;

			if(firstRun == true){

				string videoFileName = "D:/"+getDateTimeForFile()+".avi";
				cout << "File has been opened for writing: " << videoFileName<<endl;
				oVideoWriter  = VideoWriter(videoFileName, CV_FOURCC('D', 'I', 'V', '3'), 5, frameSize, true);

				if ( !oVideoWriter.isOpened() ) 
				{
					cout << "ERROR: Failed to initialize video writing" << endl;
					getchar();
					return -1;
				}
				firstRun = false;


			}

		}else recording = false;

		//show our captured frame
		imshow("Frame1",frame1);

		//check to see if a button has been pressed.
		//this delay is necessary for proper operation of this program
		//if removed, frames will not have enough time to referesh and a blank 
		//image will appear.
		switch(waitKey(30)){

		case 27: //'esc' key has been pressed, exit program.
			return 0;
		case 116: //'t' has been pressed. this will toggle tracking
			trackingEnabled = !trackingEnabled;
			if(trackingEnabled == false) cout<<"Tracking disabled."<<endl;
			else cout<<"Tracking enabled."<<endl;
			break;
		case 100: //'d' has been pressed. this will debug mode
			debugMode = !debugMode;
			if(debugMode == false) cout<<"Debug mode disabled."<<endl;
			else cout<<"Debug mode enabled."<<endl;
			break;
		case 112: //'p' has been pressed. this will pause/resume the code.
			pause = !pause;
			if(pause == true){ cout<<"Code paused, press 'p' again to resume"<<endl;
			while (pause == true){
				//stay in this loop until 
				switch (waitKey()){
					//a switch statement inside a switch statement? Mind blown.
				case 112: 
					//change pause back to false
					pause = false;
					cout<<"Code Resumed"<<endl;
					break;
				}
			}
			}

		case 114:
			//'r' has been pressed.
			//toggle recording mode
			recording =!recording;

			if(firstRun == true){

				cout << "New Recording Started" << endl;
				oVideoWriter  = VideoWriter("D:/MyVideo0.avi", CV_FOURCC('D', 'I', 'V', '3'), 5, frameSize, true);

				if ( !oVideoWriter.isOpened() ) 
				{
					cout << "ERROR: Failed to initialize video writing" << endl;
					getchar();
					return -1;
				}
				firstRun = false;


			}
			else {if (!recording)cout << "Recording Stopped" << endl;

			else cout << "Recording Started" << endl;
			}
			break;

		case 110:
			//'n' has been pressed
			//start new video file
			startNewRecording = true;
			cout << "New Recording Started" << endl;
			//increment video file name
			inc+=1;
			break; 

		}

	}

	return 0;

}