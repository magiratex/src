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
	Scalar color;
}TR;

vector<TR> trackers;
bool draw_signal = false;
bool select_signal = false;
bool move_signal = false;
CvPoint moveDist;
int selectedID = -1;
FloatRect box;
Scalar theColor;
Mat frame, latest; /* frame is original image, latest is the one allowing user to adjust */
vector<FloatRect> boxes; /* store the temporary positions */
vector<Scalar> colors; 
int staticID = 0;
float scale = 2.0;

void draw_all();
void temp_draw_all();
void draw_box(FloatRect bbox, Scalar c);
void clear_trackers();
void my_mouse_callback(int event, int x, int y, int flags, void * param);
int my_rand_int(int scale);
void clear_all_signals();
void init_tracker(FloatRect bbox, Scalar ccol);
void delete_tracker(int i);
void write_output(ofstream & ofile, int timestep);
void rectangle(Mat& rMat, const FloatRect& rRect, const Scalar& rColour, int thickness);
void draw_traj();

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
	string imgPathBase = "./sequences/crossroad/";
	string framesFilePath = imgPathBase + "crossroad_frames.txt";
	string outputFilePath = imgPathBase + "crossroad_tracks.txt";
	string imgFormat = imgPathBase + "imgs/%06d.jpg";
	/*string imgPathBase = "./sequences/passing/";
	string framesFilePath = imgPathBase + "passing_frames.txt";
	string outputFilePath = imgPathBase + "passing_tracks.txt";
	string imgFormat = imgPathBase + "imgs/%06d.jpg";*/

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
	vector<FloatRect> bak;
	vector<Scalar> bakColors;

	for (int i = startFrame; i < endFrame; )
	{
		cout << "Frame: " << i << endl;
		/* Read in the current frame */
		char frmPath[128];
		sprintf(frmPath, imgFormat.c_str(), i);
		//Mat frameOrig = cv::imread(frmPath, 0);
		//cvtColor(frameOrig, result, CV_GRAY2RGB);
		Mat frameOrig = cv::imread(frmPath, 1);
		//frame = frameOrig.clone();
		resize(frameOrig, frame, Size(frameOrig.cols * scale, frameOrig.rows * scale));

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
		colors.clear();
		for (int j = 0; j < trackers.size(); j ++)
		{
			if (trackers[j].trk->IsInitialised())
			{
				trackers[j].trk->Track(frame);
				FloatRect r = trackers[j].trk->GetBB();
				trackers[j].boxSeq.push_back(r);
				boxes.push_back(r);
				colors.push_back(trackers[j].color);
			}
		}

		draw_traj();
		draw_all();

		bak = boxes;
		bakColors = colors;
		cout << "You can start adjusting the results now ..." << endl;


		char c = cv::waitKey(0);
		if (c == 27) // esc
			break;
		else if (c == 'd') /* undo */
		{
			boxes = bak;
			colors = bakColors;
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
				init_tracker(boxes[j], colors[j]);
			}
			
			/* - position adjusted or deleted */
			for (int j = 0; j < bak.size(); )
			{
				/* deleted */
				if (boxes[j].XMin() + boxes[j].YMin() + boxes[j].Width() + boxes[j].Height() == 0)
				{
					delete_tracker(j);
					trackers.erase(trackers.begin() + j);
					boxes.erase(boxes.begin() + j);
					colors.erase(colors.begin() + j);
					bak.erase(bak.begin() + j);
					bakColors.erase(bakColors.begin() + j);
					continue;
				}

				/* position changed */
				if (boxes[j].XMin() != bak[j].XMin() || boxes[j].YMin() != bak[j].YMin())
				{
					trackers[j].trk->Reset();
					FloatRect initBB(boxes[j].XMin(), boxes[j].YMin(), boxes[j].Width(), boxes[j].Height());
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
			<< fr.XMin()/scale << "\t"
			<< fr.YMin()/scale << "\t"
			<< fr.Width()/scale << "\t"
			<< fr.Height()/scale << "\n";
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

void init_tracker(FloatRect bbox, Scalar ccol)
{
	Tracker * trkPtr = new Tracker(globalConf);	
	FloatRect initBB(bbox.XMin(), bbox.YMin(), bbox.Width(), bbox.Height());
	trkPtr->Initialise(frame, initBB);
	TR wrapper;
	wrapper.id = staticID ++;
	wrapper.trk = trkPtr;
	wrapper.boxSeq.push_back(initBB);
	wrapper.color = ccol;
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
							Drawing
*/
/************************************************************************/

void draw_traj()
{
	for (int i = 0; i < trackers.size(); i ++)
	{
		for (int j = 0; j < trackers[i].boxSeq.size()-1; j ++)
		{
			FloatRect r1, r2;
			r1 = trackers[i].boxSeq[j];
			r2 = trackers[i].boxSeq[j+1];
			line(frame, 
				Point(r1.XMin()+r1.Width()/2, r1.YMin()+r1.Height()/2),
				Point(r2.XMin()+r2.Width()/2, r2.YMin()+r2.Height()/2),
				trackers[i].color);
				//CV_RGB(0, 255, 0));
		}
	}
	//imshow(windowName, frame);
}

/* use for drawing rectangles when the results are confirmed */
void draw_all()
{
	latest = frame.clone();
	for (int i = 0; i < trackers.size(); i ++)
	{
		IntRect bbox(trackers[i].boxSeq.back());
		rectangle(latest, 
				Point(bbox.XMin(), bbox.YMin()), 
				Point(bbox.XMax(), bbox.YMax()), 
				trackers[i].color);
				//CV_RGB(0, 255, 0));
	}
	imshow(windowName, latest);
}

/* use for drawing rectangles during editing */
void temp_draw_all()
{
	latest = frame.clone();
	for (int i = 0; i < boxes.size(); i ++)
	{	
		if (boxes[i].XMin() + boxes[i].YMin() + boxes[i].Width() + boxes[i].Height() > 0)
			rectangle(latest, 
					Point(boxes[i].XMin(), boxes[i].YMin()), 
					Point(boxes[i].XMax(), boxes[i].YMax()), 
					colors[i]);
					//CV_RGB(0, 255, 0));
	}
	cout << "Latest color is: " << colors[boxes.size()-1][0] << ","
		<< colors[boxes.size()-1][1] << ","
		<< colors[boxes.size()-1][2] << endl;
	cv::imshow(windowName, latest);
}

/* use for drawing rectangles during editing, highlight the selected one */
void draw_box(FloatRect bbox, Scalar c)
{
	latest = frame.clone();

	if( bbox.Width() < 0 )
	{
		bbox.SetXMin(bbox.XMin() + bbox.Width());
		bbox.SetWidth(bbox.Width() * -1);
		//bbox.x += bbox.width;
		//bbox.width *= -1;
	}

	if( bbox.Height() < 0 )
	{
		bbox.SetYMin(bbox.YMin() + bbox.Height());
		bbox.SetHeight(bbox.Height() * -1);
		//bbox.y += bbox.height;
		//bbox.height *= -1;
	}

	if (selectedID != -1)
		rectangle(latest, 
			Point(bbox.XMin(), bbox.YMin()), 
			Point(bbox.XMax(), bbox.YMax()), 
			//c);
			CV_RGB(255, 0, 0), 3);
	else
		rectangle(latest, 
			Point(bbox.XMin(), bbox.YMin()), 
			Point(bbox.XMax(), bbox.YMax()), 
			c);
			//CV_RGB(0, 255, 0));

	for (int i = 0; i < boxes.size(); i ++)
	{
		if (i != selectedID)
			rectangle(latest, 
				Point(boxes[i].XMin(), boxes[i].YMin()), 
				Point(boxes[i].XMax(), boxes[i].YMax()), 
				colors[i]);
				//CV_RGB(0, 255, 0));
	}

	cv::imshow(windowName, latest);
}

void rectangle(Mat& rMat, const FloatRect& rRect, const Scalar& rColour, int thickness = 2)
{
	IntRect r(rRect);
	rectangle(rMat, Point(r.XMin(), r.YMin()), Point(r.XMax(), r.YMax()), rColour, thickness);
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
			box.SetWidth(x - box.XMin());
			box.SetHeight(y - box.YMin());
			draw_box(box, theColor);
		}
		if (move_signal)
		{
			//cout << "2. selected: " << selectedID << endl;
			/*move the box*/
			box.SetXMin(x - moveDist.x);
			box.SetYMin(y - moveDist.y);
			draw_box(box, theColor);
		}
		break;

	case CV_EVENT_RBUTTONDOWN:
		clear_all_signals();
		draw_signal = true;
		box.SetXMin(x);
		box.SetYMin(y);
		box.SetWidth(0);
		box.SetHeight(0);
		theColor = CV_RGB(128, rand()%255, rand()%255);
		cout << "Generate a new color: " << theColor[0] << "," 
										 << theColor[1] << "," 
										 << theColor[2] << ","
										 << endl;
		break;

	case CV_EVENT_RBUTTONUP:
		draw_signal = false;
		if (abs(box.Height()) > 3 && abs(box.Width()) > 3)
		{
			/* add a new tracker */
			if( box.Width() < 0 )
			{
				box.SetXMin(box.XMin() + box.Width());
				box.SetWidth(box.Width() * -1);
			}

			if( box.Height() < 0 )
			{
				box.SetYMin(box.YMin() + box.Height());
				box.SetHeight(box.Height() * -1);
			}
			boxes.push_back(box);
			colors.push_back(theColor);
			cout << "Push in the new color: " << theColor[0] << "," 
				<< theColor[1] << "," 
				<< theColor[2] << ","
				<< endl;
			cout << "Sizes of boxes and colors: " << boxes.size() << " " << colors.size() << endl;
		}
		clear_all_signals();
		temp_draw_all();
		
		break;

	case CV_EVENT_LBUTTONDOWN:
		clear_all_signals();
		/*select a specific box*/
		for (int i = 0; i < boxes.size(); i ++)
		{
			FloatRect r = boxes[i];
			if (x >= r.XMin() && y >= r.YMin() && x <= r.XMax() && y <= r.YMax())
			{
				overlapSelect.push_back(i);
			}
		}

		if (overlapSelect.size() > 0)
		{
			selectedID = my_rand_int(overlapSelect.size());
			selectedID = overlapSelect[selectedID];
			box = boxes[selectedID];
			theColor = colors[selectedID];
		}

		/* if click on */
		if (selectedID != -1)
		{
			//cout << "1. selected: " << selectedID << endl;
			select_signal = true;
			move_signal = true;

			/*compute relative distance away from box.x and box.y*/
			moveDist = cvPoint(x - box.XMin(), y - box.YMin());

			draw_box(boxes[selectedID], theColor);
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
			draw_box(boxes[selectedID], colors[selectedID]);
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
			boxes[selectedID].SetXMin(0);
			boxes[selectedID].SetYMin(0);
			boxes[selectedID].SetWidth(0);
			boxes[selectedID].SetHeight(0);
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


