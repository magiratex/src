#include "Tracker.h"
#include "Config.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include "opencv/cv.h"
#include "opencv/highgui.h"

using namespace std;
using namespace cv;

string windowName("wxi");
Config globalConf;

typedef struct _TR
{
	int id;
	Tracker * trk;
	vector<FloatRect> boxSeq;
}TR;

vector<TR> trackers;
bool draw_signal = false;
bool select_signal = false;
bool move_signal = false;
CvPoint moveDist;
int selectedID = -1;
CvRect box;
Mat frame, latest; /* frame is original image, latest is the one allowing user to adjust */
vector<CvRect> boxes; /* store the temporary positions */
int staticID = 0;

void draw_all();
void temp_draw_all();
void draw_box(CvRect bbox);
void clear_trackers();
void my_mouse_callback(int event, int x, int y, int flags, void * param);
int my_rand_int(int scale);
void clear_all_signals();
void init_tracker(CvRect bbox);
void delete_tracker(int i);
void write_output(ofstream & ofile, int timestep);

/************************************************************************/
/* 
						Main routine
*/
/************************************************************************/

void demo()
{
	/* Data source */ 
	//string imgPathBase = "./sequences/girl/";
	//string framesFilePath = imgPathBase + "girl_frames.txt";
	//string outputFilePath = imgPathBase + "girl_tracks.txt";
	//string imgFormat = imgPathBase + "imgs/img%05d.png";
	/*string imgPathBase = "./sequences/crossroad/";
	string framesFilePath = imgPathBase + "crossroad_frames.txt";
	string outputFilePath = imgPathBase + "crossroad_tracks.txt";
	string imgFormat = imgPathBase + "imgs/%06d.jpg";*/
	string imgPathBase = "./sequences/passing/";
	string framesFilePath = imgPathBase + "passing_frames.txt";
	string outputFilePath = imgPathBase + "passing_tracks.txt";
	string imgFormat = imgPathBase + "imgs/%06d.jpg";

	/* Input */
	ifstream framesFile;
	int startFrame, endFrame;
	string info;

	framesFile.open(framesFilePath.c_str(), ios::in);
	getline(framesFile, info);
	sscanf(info.c_str(), "%d,%d", &startFrame, &endFrame);
	cout << startFrame << ", " << endFrame << endl;
	framesFile.close();

	ofstream ofile;
	ofile.open(outputFilePath.c_str(), ios::out);

	/* configuration */
	string configPath = "config.txt";
	Config conf(configPath);
	globalConf = conf;

	/* tracking */
	cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
	Mat result;
	cvSetMouseCallback(windowName.c_str(), my_mouse_callback);
	vector<CvRect> bak;
	for (int i = startFrame; i < endFrame; )
	{
		cout << "Frame: " << i << endl;
		/* Read in the current frame */
		char frmPath[128];
		sprintf(frmPath, imgFormat.c_str(), i);
		//Mat frameOrig = cv::imread(frmPath, 0);
		//cvtColor(frameOrig, result, CV_GRAY2RGB);
		Mat frameOrig = cv::imread(frmPath, 1);
		frame = frameOrig.clone();

		cout << frmPath << endl;
		imshow(windowName, frame);
		
		/*if (i == startFrame)
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
		}
		}*/

		/* Loop over all trackers */
		boxes.clear();
		for (int j = 0; j < trackers.size(); j ++)
		{
			if (trackers[j].trk->IsInitialised())
			{
				trackers[j].trk->Track(frame);
				FloatRect r = trackers[j].trk->GetBB();
				trackers[j].boxSeq.push_back(r);
				CvRect bb;
				bb.x = r.XMin();
				bb.y = r.YMin();
				bb.width = r.Width();
				bb.height = r.Height();
				boxes.push_back(bb);
			}
		}

		draw_all();

		bak = boxes;
		cout << "You can start adjusting the results now ..." << endl;


		char c = cv::waitKey(0);
		if (c == 27) // esc
			break;
		else if (c == 'd') /* undo */
		{
			boxes = bak;
			draw_all();
		}
		else /* move on to next frame */
		{
			/*	update the tracking:
				- add a new box
				- position adjusted
				- box deleted
			*/
			
			/* - add a new box */
			for (int j = bak.size(); j < boxes.size(); j ++)
			{
				init_tracker(boxes[j]);
			}
			
			/* - position adjusted or deleted */
			for (int j = 0; j < bak.size(); )
			{
				/* deleted */
				if (boxes[j].x + boxes[j].y + boxes[j].width + boxes[j].height == 0)
				{
					delete_tracker(j);
					trackers.erase(trackers.begin() + j);
					boxes.erase(boxes.begin() + j);
					bak.erase(bak.begin() + j);
					continue;
				}

				/* position changed */
				if (boxes[j].x != bak[j].x || boxes[j].y != bak[j].y)
				{
					trackers[j].trk->Reset();
					FloatRect initBB(boxes[j].x, boxes[j].y, boxes[j].width, boxes[j].height);
					trackers[j].trk->Initialise(frame, initBB);
					trackers[j].boxSeq[trackers[j].boxSeq.size()-1] = initBB;
				}

				j ++;
			}

			draw_all(); 
			write_output(ofile, i);
			i ++;
		}
	}
	clear_trackers();

	
	ofile.close();
}

void write_output(ofstream & ofile, int timestep)
{	
	ostringstream oss;
	for (int i = 0; i < trackers.size(); i ++)	
	{
		FloatRect fr = trackers[i].boxSeq.back();
		oss << timestep << "\t"
			<< trackers[i].id << "\t"
			<< fr.XMin() << "\t"
			<< fr.YMin() << "\t"
			<< fr.Width() << "\t"
			<< fr.Height() << "\n";
	}
	ofile << oss.str();
	ofile.flush();
}

int main()
{
	cout << "demo" << endl;
	demo();
	return 0;
}

/************************************************************************/
/* 
							Trackers' update
*/
/************************************************************************/

void init_tracker(CvRect bbox)
{
	Tracker * trkPtr = new Tracker(globalConf);	
	FloatRect initBB(bbox.x, bbox.y, bbox.width, bbox.height);
	trkPtr->Initialise(frame, initBB);
	TR wrapper;
	wrapper.id = staticID ++;
	wrapper.trk = trkPtr;
	wrapper.boxSeq.push_back(initBB);
	trackers.push_back(wrapper);
}

void delete_tracker(int i)
{
	delete trackers[i].trk;
}

void clear_trackers()
{
	for (int i = 0; i < trackers.size(); i ++)
	{
		delete trackers[i].trk;
	}
}

/************************************************************************/
/* 
							Draw rectangles
*/
/************************************************************************/

/* use for drawing rectangles when the results are confirmed */
void draw_all()
{
	latest = frame.clone();
	for (int i = 0; i < trackers.size(); i ++)
	{
		IntRect bbox(trackers[i].boxSeq.back());
		rectangle(latest, Point(bbox.XMin(), bbox.YMin()), Point(bbox.XMax(), bbox.YMax()), CV_RGB(0, 255, 0));
	}
	imshow(windowName, latest);
}

/* use for drawing rectangles during editing */
void temp_draw_all()
{
	latest = frame.clone();
	for (int i = 0; i < boxes.size(); i ++)
	{	
		if (boxes[i].x + boxes[i].y + boxes[i].width + boxes[i].height > 0)
			rectangle(latest, boxes[i], CV_RGB(0, 255, 0));
	}

	cv::imshow(windowName, latest);
}

/* use for drawing rectangles during editing, highlight the selected one */
void draw_box(CvRect bbox)
{
	latest = frame.clone();

	if( bbox.width < 0 )
	{
		bbox.x += bbox.width;
		bbox.width *= -1;
	}

	if( bbox.height < 0 )
	{
		bbox.y += bbox.height;
		bbox.height *= -1;
	}
	if (selectedID != -1)
		rectangle(latest, bbox, CV_RGB(0, 0, 255));
	else
		rectangle(latest, bbox, CV_RGB(0, 255, 0));

	for (int i = 0; i < boxes.size(); i ++)
	{
		if (i != selectedID)
			rectangle(latest, boxes.at(i), CV_RGB(0, 255, 0));
	}

	cv::imshow(windowName, latest);
}


/************************************************************************/
/* 
							User interface
*/
/************************************************************************/

void my_mouse_callback(int event, int x, int y, int flags, void * param)
{
	vector<int> overlapSelect;
	switch( event )
	{
	case CV_EVENT_MOUSEMOVE: 
		if ( draw_signal )
		{
			box.width = x - box.x;
			box.height = y - box.y;
			draw_box(box);
		}
		if (move_signal)
		{
			//cout << "2. selected: " << selectedID << endl;
			/*move the box*/
			box.x = x - moveDist.x;
			box.y = y - moveDist.y;
			draw_box(box);
		}
		break;

	case CV_EVENT_RBUTTONDOWN:
		clear_all_signals();
		draw_signal = true;
		box = cvRect(x, y, 0, 0);
		break;

	case CV_EVENT_RBUTTONUP:
		draw_signal = false;
		if (abs(box.height) > 3 && abs(box.width) > 3)
		{
			/* add a new tracker */
			if( box.width < 0 )
			{
				box.x += box.width;
				box.width *= -1;
			}

			if( box.height < 0 )
			{
				box.y += box.height;
				box.height *= -1;
			}
			boxes.push_back(box);
		}
		clear_all_signals();
		temp_draw_all();
		
		break;

	case CV_EVENT_LBUTTONDOWN:
		clear_all_signals();
		/*select a specific box*/
		for (int i = 0; i < boxes.size(); i ++)
		{
			CvRect r = boxes.at(i);
			if (x >= r.x && y >= r.y && x <= r.x+r.width && y <= r.y+r.height)
			{
				overlapSelect.push_back(i);
			}
		}

		if (overlapSelect.size() > 0)
		{
			selectedID = my_rand_int(overlapSelect.size());
			selectedID = overlapSelect[selectedID];
			box = boxes[selectedID];
		}

		/* if click on */
		if (selectedID != -1)
		{
			//cout << "1. selected: " << selectedID << endl;
			select_signal = true;
			move_signal = true;

			/*compute relative distance away from box.x and box.y*/
			moveDist = cvPoint(x - box.x, y - box.y);

			draw_box(boxes[selectedID]);
		}
		//else
		//{
		//	//cout << "click on nothing" << endl;
		//}
		
		
		break;
	case CV_EVENT_LBUTTONUP:
		move_signal = false;
		if (selectedID != -1)
		{
			//cout << "3. selected: " << selectedID << endl;
			boxes[selectedID] = box;
			draw_box(boxes[selectedID]);
		}
		else
		{
			temp_draw_all();
		}
		break;


	case CV_EVENT_MBUTTONDOWN: // special usage
		
		if (select_signal && selectedID != -1)
		{
			cout << "delete!" << endl;
			//trackers.erase(trackers.begin() + selectedID);
			//cout << trackers.size() << endl;
			boxes[selectedID].x = 0;
			boxes[selectedID].y = 0;
			boxes[selectedID].width = 0;
			boxes[selectedID].height = 0;
			clear_all_signals();
		}
		temp_draw_all();
		break;

	}
}

int my_rand_int(int scale)
{
	//srand(197);
	return rand() % scale;

}

void clear_all_signals()
{
	draw_signal = false;
	select_signal = false;
	move_signal = false;
	moveDist = cvPoint(0, 0);
	selectedID = -1;
}


