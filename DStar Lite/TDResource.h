#ifndef TDRESOURCE_H
#define TDRESOURCE_H

#include <string>
#include <iostream>
#include <time.h>
#include <fstream>
#include <string> 
#include <vector>
#include <list>
#include "map.h"
#include <iterator>     // std::back_inserter

using namespace std;

struct TDPoint
{
	unsigned int x;
	unsigned int y;
	unsigned int z;

	bool operator == (const TDPoint & obj) const //���� ��==�� ���������������� const �����ˣ�����ᱨ���������http://www.cnblogs.com/SZxiaochun/p/7731900.html��
	{
		return x == obj.x && y == obj.y && z==obj.z; //����ƥ�������������Լ��趨
	}

};

// ��·���ж�ȡ���ɺ�����
class TDResource
{
public:
	static vector<TDPoint> ReadRealTimeEnviMap(string path);
	static bool TDResource::WritePathtoTxt(list<Map::Cell*> tempPath, string path);
	//static vector<TDPoint> TDResource::vectors_intersection(vector<TDPoint> v1, vector<TDPoint> v2);
	//static vector<TDPoint> TDResource::vectors_set_union(vector<TDPoint> v1, vector<TDPoint> v2);
	//static vector<TDPoint> TDResource::deletevectors(vector<TDPoint> all, vector<TDPoint> sub);
private:

};

#endif
