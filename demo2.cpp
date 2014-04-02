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

typedef struct _TR
{
	Tracker * trk;
	vector<FloatRect> boxSeq;
}TR;

vector<TR> trackers;
//vector<Tracker *> trackers;
//vector<vector<FloatRect>> boxVectors;

void draw_all(Mat frame);
void clear_trackers();

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

	/* tracking */
	cv::namedWindow(windowName , cv::WINDOW_AUTOSIZE);
	Mat result(conf.frameHeight, conf.frameWidth, CV_8UC3);
	for (int i = startFrame; i < endFrame; )
	{
		cout << "Frame: " << i << endl;
		/* Read in the current frame */
		char frmPath[128];
		sprintf(frmPath, imgFormat.c_str(), i);
		Mat frameOrig = cv::imread(frmPath, 0);
		cvtColor(frameOrig, result, CV_GRAY2RGB);
		Mat frame = frameOrig;

		/* Loop over all trackers */
		if (i == startFrame)
		{
			cout << "Initialization ..." << endl;
			for (int j = 0; j < 2; j ++)
			{
				Tracker * trkPtr = new Tracker(globalConf);	
				FloatRect initBB(126-j*3,44-j*3, 108+j*6, 131+j*6);
				trkPtr->Initialise(frame, initBB);
				TR wrapper;
				wrapper.trk = trkPtr;
				wrapper.boxSeq.push_back(initBB);
				trackers.push_back(wrapper);
				//trackers.push_back(trkPtr);
				//vector<FloatRect> boxes;
				//boxes.push_back(initBB);
				//boxVectors.push_back(boxes);
				//trackers[j]->Initialise(frame, initBB);
			}
		}

		for (int j = 0; j < trackers.size(); j ++)
		{
			if (trackers[j].trk->IsInitialised())
			{
				trackers[j].trk->Track(frame);
				FloatRect r = trackers[j].trk->GetBB();
				//cout << r.XMin() << ", " << r.YMin() << endl;
				trackers[j].boxSeq.push_back(r);
			}
			/*if (trackers[j]->IsInitialised())
			{
				trackers[j]->Track(frame);
				FloatRect r = trackers[j]->GetBB();
				cout << r.XMin() << ", " << r.YMin() << endl;
				boxVectors[j].push_back(trackers[j]->GetBB());
			}*/
		}

		draw_all(result);

		char c = cv::waitKey(0);
		if (c == 27) // esc
			break;
		else
			i ++;
	}
	clear_trackers();
}

void clear_trackers()
{
	for (int i = 0; i < trackers.size(); i ++)
	{
		//delete trackers[i];
		delete trackers[i].trk;
	}
}

void draw_all(Mat frame)
{
	for (int i = 0; i < trackers.size(); i ++)
	{
		//IntRect bbox(boxVectors[i].back());
		IntRect bbox(trackers[i].boxSeq.back());
		rectangle(frame, Point(bbox.XMin(), bbox.YMin()), Point(bbox.XMax(), bbox.YMax()), CV_RGB(0, 255, 0));
	}
	imshow(windowName, frame);
}


int main()
{
	cout << "demo" << endl;
	demo();
	return 0;
}