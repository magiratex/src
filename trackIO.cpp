#include "trackIO.h"

trackIO::trackIO()
{
}


trackIO::~trackIO(void)
{
	alldata.clear();
}

void trackIO::readDataFile(string filename)
{
	ifstream fid;
	fid.open(filename);

	string line;
	while (getline(fid, line))
	{
		if (fid.eof())
			break;
		istringstream s(line);
		int tid, aid;
		float px, py, wid, hgt;
		s >> tid >> aid >> px >> py >> wid >> hgt;
		FD f;
		f.tid = tid;
		f.aid = aid;
		f.px = px;
		f.py = py;
		f.wid = wid;
		f.hgt = hgt;

		alldata.push_back(f);
	}

	fid.close();
}

void trackIO::updateFrame(vector<float> indv)
{
	int i;
	bool modified = false;
	for (i = 0; i < alldata.size(); i ++)
	{
		if (alldata[i].tid == indv[0] && alldata[i].aid == indv[1]) // update existing data
		{
			modified = true;
			alldata[i].px = indv[2];
			alldata[i].py = indv[3];
			alldata[i].wid = indv[4];
			alldata[i].hgt = indv[5];
		}
		if (alldata[i].tid > indv[0]) // add a new data instance
		{
			break;
		}
	}
	if (!modified)
	{
		FD f;
		f.tid = indv[0];
		f.aid = indv[1];
		f.px = indv[2];
		f.py = indv[3];
		f.wid = indv[4];
		f.hgt = indv[5];
		alldata.insert(alldata.begin() + i, f); // need check
	}
	
}

void trackIO::saveData(string outputFileName)
{
	ofstream fid;
	fid.open(outputFileName);

	for (int i = 0; i < alldata.size(); i ++)
	{
		ostringstream oss;
		oss << alldata[i].tid << "\t"
			<< alldata[i].aid << "\t"
			<< alldata[i].px << "\t"
			<< alldata[i].py << "\t"
			<< alldata[i].wid << "\t"
			<< alldata[i].hgt << "\n";
		fid << oss.str();
	}

	fid.flush();
	fid.close();
}

void trackIO::queryFrame(int t, vector<vector<float>> &seg)
{
	for (int i = 0; i < alldata.size(); i ++)
	{
		vector<float> indv;
		if (alldata[i].tid == t)
		{
			convFormat(alldata[i], indv);
			seg.push_back(indv);
		}
		else if (alldata[i].tid > t)
			break;
	}
}

void trackIO::queryAgent(int id, vector<vector<float>> &seg)
{
	for (int i = 0; i < alldata.size(); i ++)
	{
		vector<float> indv;
		if (alldata[i].aid == id)
		{
			convFormat(alldata[i], indv);
			seg.push_back(indv);
		}
	}
}

void trackIO::queryAgent(int id, int t, vector<vector<float>> &seg)
{
	for (int i = 0; i < alldata.size(); i ++)
	{
		vector<float> indv;
		if (alldata[i].aid == id && alldata[i].tid <= t)
		{
			convFormat(alldata[i], indv);
			seg.push_back(indv);
		}
	}
}

int trackIO::queryMaxID()
{
	// assume ids are consecutive
	int maxID = -1;
	for (int i = 0; i < alldata.size(); i ++)
	{
		if (maxID < alldata[i].aid)
			maxID = alldata[i].aid;
	}
	return maxID;
}

void trackIO::convFormat(FD fdat, vector<float> &vdat)
{
	vdat.push_back(fdat.tid);
	vdat.push_back(fdat.aid);
	vdat.push_back(fdat.px);
	vdat.push_back(fdat.py);
	vdat.push_back(fdat.wid);
	vdat.push_back(fdat.hgt);
}

