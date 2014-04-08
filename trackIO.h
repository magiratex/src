/** ------------------------------------

	Class for reading data from our tracking results
	
	------------------------------------ **/

#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

class trackIO
{
	typedef struct fdata
	{
		int tid, aid;
		float px, py, wid, hgt;
	}FD;

public:
	trackIO();
	~trackIO();

public:
	/*  read data from file	*/
	void readDataFile(string filename);

	/* query data at timestep t */
	void queryFrame(int t, vector<vector<float>> &seg);

	/* query agent id */
	void queryAgent(int id, vector<vector<float>> &seg);
	void queryAgent(int id, int t, vector<vector<float>> &seg);

	/* query the max agent ID */
	int queryMaxID();

	/* update data by adding individual data at given frame */
	void updateFrame(vector<float> indv);

	/* initiatively save data */
	void saveData(string outputFileName);

private:
	void convFormat(FD fdat, vector<float> &vdat);

public:
	vector<struct fdata> alldata;
	string outputFileName;
};

