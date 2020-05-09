//#include "pch.h"  //vs2017中预编译头文件
//#include "stdafx. h" //vs2015中的预编译头文件
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "time.h"
#include <set>
#include <algorithm>
#include <unordered_set>
#include <array>
#include <stdint.h>
using namespace std;
//需要修改成自己的路径*********************************************
string srcPath = "./Example.csv";
string outPath = "./result.txt";

const uint8_t CircleLen = 14; //寻找环的最大长度
uint16_t blockMoving = 64;//第一阶段数据规律块大小
uint8_t *visited;
uint16_t path[CircleLen] = { 0 };//保存路径

uint8_t stepCount = 0;//计数器
uint32_t numCount[6] = { 0,0,0,0,0,0 };
uint32_t maxX = 0;//四次循环过程中在此进入该小块的下标最大值
array<uint16_t, CircleLen> arr;
//自定义hash
struct ArrayHash
{
	size_t operator()(const std::array<uint16_t, CircleLen>& v) const
	{
		std::hash<uint16_t> hasher;
		size_t seed = 0;
		for (uint16_t i : v)
		{
			seed ^= hasher(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}
};

unordered_set<array<uint16_t, CircleLen>, ArrayHash>setCircle[6];
uint16_t head = 0;
//链表存储相临接点的下标
typedef struct GraphNode
{
	uint16_t index;    //下标
	struct GraphNode *next;  //指向下一个的指针
}GraphNode;
//临接表
typedef struct Gra
{
	GraphNode *first;//头结点
}Gra;

bool readFile(string filename, vector<vector<uint8_t>> &G)
{
	ifstream inFile(filename, ios::in);
	if (!inFile.is_open())
	{
		cout << "open file failed !" << endl;
		return false;
	}
	string line;
	unsigned int linepos = 0;
	char c;
	while (getline(inFile, line))//getline(inFile, line)表示按行读取CSV文件中的数据
	{
		linepos = 0;
		vector<uint8_t> temp;
		while (linepos < line.size())
		{
			c = line[linepos];
			if (isdigit(c))
			{
				temp.push_back(atoi(&c));
			}

			linepos++;
		}
		G.push_back(temp);
	}
	//cout << "data row:" << G.size() << endl << "data col:" << G[0].size() << endl;
	inFile.close();
	return true;
}
//根据原始数据创建领接表
void CreateLinkList(Gra* &pdata, vector<vector<uint8_t>> &G)
{
	int row = G.size();
	int col = G[0].size();
	GraphNode *pNowNode, *pNode;
	for (int i = 0; i < row; i++)//初始化
	{
		pdata[i].first = nullptr;//领接矩阵的A的行
	}
	for (int j = 0; j < col; j++)
	{
		pdata[j + row].first = nullptr;//B的行
	}
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			if (G[i][j])
			{
				//A的临接表
				pNowNode = new GraphNode;
				pNowNode->index = j + row;
				pNowNode->next = pdata[i].first;
				pdata[i].first = pNowNode;//头插法，将数据插在前面
				//B的领接表 二部无向图，根据对称关系就可以写出B的临接关系
				pNode = new GraphNode;
				pNode->index = i;
				pNode->next = pdata[j + row].first;
				pdata[j + row].first = pNode;
			}
		}
	}
}
GraphNode *pstack[CircleLen];//保存节点信息栈
//深度优先搜索
void dfs(Gra* &pGra, int i)
{
	GraphNode *p;
	visited[i] = 1;//正在访问
	p = pGra[i].first;//头结点
	path[stepCount] = i;//记录环路路径
	pstack[stepCount] = p;
	stepCount++;
	while (stepCount)//栈非空
	{
		while (p)
		{
			if (visited[p->index] && (stepCount > 2) && (head == p->index) && (path[1] > path[stepCount - 1]))//遇到环了 并且只选择顺时针逆时针中的一半加入集合判断
			{
				//之所以用array是因为比vector快,尽管每个长度都必须设定成14，但还是比vector快 && (path[1] > path[2])
				/*if (path[1] > path[stepCount-1])
				{
					cout << "丢弃->"<<endl;
				}
				else cout << "选择->" << endl;*/
				for (uint8_t i = 0; i < stepCount; i++)
				{
					arr[i] = path[i];
					//cout << arr[i] << "->";
				}
				//cout << endl;
				for (uint8_t i = stepCount; i < CircleLen; i++)
				{
					arr[i] = 0;
				}
				sort(arr.begin(), arr.end());
				bool insertOk = setCircle[stepCount / 2 - 2].insert(arr).second;//放入集合,并返回插入是否成功，插入成功说明有不同的环路出现
				if (insertOk)//有新环路加入
				{
					maxX = head;
					for (uint8_t i = 2; i < stepCount; i += 2)//0,2,4,6是A部落的坐标
					{
						//path[i] A的坐标回到了该小块
						if (path[i]< head + blockMoving && path[i]>head)
						{
							maxX = path[i] > maxX ? path[i] : maxX;

						}
					}
					//该环能移动的次数，就是块内能出现不同圈的次数
					numCount[stepCount / 2 - 2] += (blockMoving - (maxX - head));//叠加所有次数
				}
				p = p->next;//只要不往下一层遍历就遍历同一层
			}
			else if ((!visited[p->index]) && (p->index > head) && (stepCount < CircleLen))//需要往下一层遍历
			{
				visited[p->index] = 1;
				path[stepCount] = p->index;//入栈
				pstack[stepCount] = p;
				stepCount++;
				p = pGra[p->index].first;
			}
			else//同一层遍历
			{
				p = p->next;
			}

		}
		//访问完之后需要回溯，回退的时候标记清零,回退的过程就是出栈的过程
		stepCount--;
		visited[path[stepCount]] = 0;
		p = pstack[stepCount];
		p = p->next;
	}

}
void dfsGra(Gra* &pGra, unsigned int row, unsigned int col)
{
	//二部图对称关系，只需要遍历A的临接表，即可遍历所有圈
	uint16_t loopRow = row - blockMoving;
	for (head = 0; head < loopRow; head += blockMoving)
	{
		memset(visited, 0, (row + col) * sizeof(uint8_t));
		for (int i = 0; i < 6; i++)
		{
			setCircle[i].clear();
		}
		stepCount = 0;
		dfs(pGra, head);
	}
}
//输出结果信息
void outputResult(string &filename,double times)
{
	fstream outfile(filename, ios::out);
	if (!outfile.is_open())
	{
		cout << "open output file failed!." << endl;
		return;
	}
	for (int i = 0; i < 6; i++)
	{
		outfile << "The loop count" << i * 2 + 4 << ":" << numCount[i] << endl;
	}
	outfile << "time cost:" << times << "s" << endl;
	outfile.close();
}


int main()
{
	clock_t start_time, end_time;
	start_time = clock();
	vector<vector<uint8_t>> G;//存原始数据的二维矩阵
	bool ok = readFile(srcPath, G);//读文件信息
	if (!ok)
	{
		cout << "open file failed!!!" << endl;
		cout << "Please place the data file in the same directory as the executable !!!" << endl;
		return 1;
	}
	//cout << "Read data successfull." << endl;
	unsigned int row = G.size();
	unsigned int col = G[0].size();
	blockMoving = row > 256 ? 192 : 64;//第一阶段数据块大小64，第二阶段数据块大小192
	Gra* pdata = new Gra[row + col];
	visited = new uint8_t[row + col];
	//cout << "Create an adjacent linked list. " << endl;
	CreateLinkList(pdata, G);//创建领接链表
	//cout << "Depth-first search..." << endl;

	dfsGra(pdata, row, col);//dfs搜索
	/*for (int i = 0; i < 6; i++)
	{
		cout << "numCount" << 2 * i + 4 << ":" << numCount[i] << endl;
	}*/

	//释放内存
	for (uint32_t i = 0; i < row + col; i++)
	{
		GraphNode *p1, *p2;
		p1 = pdata[i].first;
		while (p1 != nullptr)
		{
			p2 = p1->next;
			delete p1;
			p1 = p2;
		}
	}
	delete[]pdata;
	delete[]visited;

	end_time = clock();
	double Times = (double)(end_time - start_time) / 1000;
	cout << "cost time:" << Times << "s" << endl;
	outputResult(outPath, Times);//输出结果到文件

	return 0;
}
