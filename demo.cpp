#include "Tracker.h"
#include "Config.h"

#include <iostream>
#include <fstream>

#include "opencv/cv.h"
#include "opencv/highgui.h"

using namespace std;
using namespace cv;

string windowName("wxi");
Mat target;
Config globalConf;

void demo()
{
	string imgPathBase = "./sequences/girl/";
	string framesFilePath = imgPathBase + "girl_frames.txt";
	string imgFormat = imgPathBase + "imgs/img%05d.png";
	ifstream framesFile;

	framesFile.open(framesFilePath.c_str(), ios::in);

	int startFrame, endFrame;
	string info;

	getline(framesFile, info);
	sscanf(info.c_str(), "%d,%d", &startFrame, &endFrame);
	cout << startFrame << ", " << endFrame << endl;
	framesFile.close();


	/* configuration */
	string configPath = "config.txt";
	Config conf(configPath);
	globalConf = conf;
	FloatRect initBB(128,46,104,127); // FIXME
	Tracker tracker(globalConf);
	/* tracking */
	cv::namedWindow(windowName , cv::WINDOW_AUTOSIZE);
	Mat result(conf.frameHeight, conf.frameWidth, CV_8UC3);
	for (int i = startFrame; i <= endFrame; )
	{
		/* Read in the current frame */
		char frmPath[128];
		sprintf(frmPath, imgFormat.c_str(), i);
		Mat frameOrig = cv::imread(frmPath, 0);
		cvtColor(frameOrig, result, CV_GRAY2RGB);
		Mat frame = frameOrig;

		/* Loop over all trackers */
		if (i == startFrame)
		{
			tracker.Initialise(frame, initBB);
		}

		if (tracker.IsInitialised())
		{
			cout << "Tracking ..." << endl;
			tracker.Track(frame);
			IntRect bbox(tracker.GetBB());
			rectangle(result, Point(bbox.XMin(), bbox.YMin()), Point(bbox.XMax(), bbox.YMax()), CV_RGB(0, 255, 0));
			imshow(windowName, result);
		}

		
		char c = cv::waitKey(5);
		if (c == 27) // esc
		{
			break;
		}
		else
		{
			i ++;
		}
	}
}


int main()
{
	cout << "demo" << endl;
	demo();
	return 0;
}