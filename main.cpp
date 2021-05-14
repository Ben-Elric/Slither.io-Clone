#include <easyx.h>
#include <conio.h>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <windef.h>

#pragma comment( lib, "MSIMG32.LIB")
#pragma comment(lib, "winmm.lib")//声音库




/* 窗口大小 */
const short ScreenWidth = 800;
const short ScreenHeight = 600;//生成窗口

/* 全局变量 */
long ret = 0;
MOUSEMSG msg;//EasyX控制鼠标函数，保存鼠标信息
const short gap = 20;
const short xSide = ScreenWidth / 2 + gap;
const short ySide = ScreenHeight / 2 + gap;
IMAGE* imgMap = new IMAGE(ScreenWidth * 4, ScreenHeight * 4);
const double PI = 3.1415926;
const short nodeSize = 17;//节点长度
const short nodeGap = 16;
const short stepLen = 4;
const short frame = 4;//length与结点增加的关系
const short snakeSpecies = 20;//AI蛇数
int killCount = 0;//计数
int mapX = 0;
int mapY = 0;


typedef struct _FOOD//绘制食物，需要和屏幕分辨率对应，以免减慢游戏进程
{
	int x;
	int y;
	int r;
	COLORREF c;
}Food;
Food* food;
short nFood = 520; //食物数量
IMAGE imgFood(nodeSize, nodeSize);
const COLORREF mapMainColor = BLACK; //用COLORREF创建COLOR VALUE
const COLORREF playerColor0 = RGB(120, 0, 0);//低位字节为红色值，其次为绿，蓝
const COLORREF playerColor1 = RGB(200, 0, 0);
const COLORREF playerColor2 = RGB(255, 255, 0);
const COLORREF mapLineColor = RGB(32, 32, 32);




void putTimage(int x, int y, IMAGE* srcimg, IMAGE* dstimg = NULL, UINT transparentcolor = 0)
{
	HDC dstDC = GetImageHDC(dstimg);// 获取绘图设备句柄(HDC)
	HDC srcDC = GetImageHDC(srcimg);
	int w = srcimg->getwidth();
	int h = srcimg->getheight();
	// 使用 Windows GDI 函数实现透明位图
	TransparentBlt(dstDC, x, y, w, h, srcDC, 0, 0, w, h, transparentcolor);//TranspaarentBlt ?
}
// 透明贴图函数：
// 参数：x, y:	目标贴图位置
//		srcimg: 源 IMAGE 对象指针。NULL 表示默认窗体
//		dstimg: 目标 IMAGE 对象指针。NULL 表示默认窗体
//		transparentcolor: 透明色。srcimg 的该颜色并不会复制到 dstimg 上，从而实现透明贴图



/* 蛇基类 */
class SnakeBase
{
public:
	// 构造函数
	SnakeBase()
	{
		Count++;
		isDead = false;
		length = 50 + rand() % 25;//生成随机数，确定初始长度
		nNode = length / 5;
		maxNode = 9999;
		imgHead = imgNode = imgTail = nullptr;
		headNode = tailNode = nullptr;
		nodeMsg = nullptr;
	}

	// 析构函数
	virtual ~SnakeBase()
	{
		Count--;
		Node* temp = headNode;//方向指针
		while (temp != nullptr)
		{
			headNode = headNode->nextNode;
			delete temp;
			temp = headNode;
		}
		delete imgHead, imgNode, imgTail;
		if (nodeMsg != nullptr)
			delete[] nodeMsg;// 释放节点结构体
	}


	// 设置image：绘制蛇的图像
	void SetImage(COLORREF headColor, COLORREF nodeColor, COLORREF tailColor)
	{
		imgHead = new IMAGE(nodeSize, nodeSize);
		imgNode = new IMAGE(nodeSize, nodeSize);
		imgTail = new IMAGE(nodeSize, nodeSize);
		SetWorkingImage(imgHead);// 设置当前绘图设备
		setfillcolor(headColor);	// 设置当前填充颜色
		solidcircle(nodeSize / 2, nodeSize / 2, nodeSize / 2);// 画填充圆(无边框)
		setfillcolor(tailColor);
		solidcircle(nodeSize / 2, nodeSize / 2, 2);//绘制蛇头包括一个圆和一个小圆


		SetWorkingImage(imgNode);
		setfillcolor(nodeColor);
		solidcircle(nodeSize / 2, nodeSize / 2, nodeSize / 2);


		SetWorkingImage(imgTail);
		setfillcolor(tailColor);
		solidcircle(nodeSize / 2, nodeSize / 2, nodeSize / 2);


		SetWorkingImage();//设置清零
	}


	// 创建各节点
	void Createnode(int headX, int headY)
	{
		headNode = new Node;//开辟一个头节点
		//初始化
		headNode->lastNode = nullptr;
		headNode->nextNode = nullptr;
		headNode->x = headX;
		headNode->y = headY;

		Node* temp = headNode;

		for (int i = 0; i < nNode - 1; i++)//nNode不停刷新，为了增加节点
		{
			Node* newNode = new Node;//开辟一新节点
			newNode->lastNode = temp;
			newNode->x = temp->x;
			newNode->y = temp->y;
			temp->nextNode = newNode;
			temp = newNode;
		}//每一次使得temp的lastNode和nextNode都是tempnode 连续创造各节点
		temp->nextNode = nullptr;

		tailNode = temp;//尾节点为空，定义结束条件
		temp = nullptr;//此时各节点创造完成，给temp为空
	}


	// 绘制蛇身：生成图像
	void ShowBody()
	{
		int n = nNode % 2;
		Node* temp = tailNode;
		while (temp != headNode)//基于上面CreateNode生成图像
		{
			if (temp->x - nodeSize / 2 + mapX >= nodeSize * -1 && temp->x - nodeSize / 2 + mapX <= ScreenWidth
				&& temp->y - nodeSize / 2 + mapY >= nodeSize / -2 && temp->y - nodeSize / 2 + mapY <= ScreenHeight)

				putTimage(temp->x - nodeSize / 2 + mapX, temp->y - nodeSize / 2 + mapY, n % 2 == 0 ? imgNode : imgTail);
			//对各节点进行贴图
			temp = temp->lastNode;
			n++;

		}

		if (temp->x - nodeSize / 2 + mapX >= nodeSize * -1 && temp->x - nodeSize / 2 + mapX <= ScreenWidth
			&& temp->y - nodeSize / 2 + mapY >= nodeSize / -2 && temp->y - nodeSize / 2 + mapY <= ScreenHeight)
			putTimage(temp->x - nodeSize / 2 + mapX, temp->y - nodeSize / 2 + mapY, imgHead);
		temp = nullptr;
	}


	// 刷新数据
	void FlushData(short& n, int& dx, int& dy)
	{
		if (n == frame)
		{
			nodeMsg = new POINT[nNode];//POINT结构定义点的x和y坐标。确定鼠标位置
			Node* temp = headNode;
			int i = 0;
			while (temp != nullptr) //Createnode里的循环
			{
				nodeMsg[i].x = temp->x;
				nodeMsg[i].y = temp->y;
				temp = temp->nextNode;
				i++;
			}
		}

		Node* temp = tailNode;
		int i = nNode - 2;//除了headNode&tailNode
		while (temp != headNode)
		{
			if (n == 1)
			{
				temp->x = nodeMsg[i].x;
				temp->y = nodeMsg[i].y;
			}
			else
			{
				temp->x += int(stepLen * cos(atan2(nodeMsg[i].y - temp->y, nodeMsg[i].x - temp->x)));//基于temp节点为坐标原点的方向角的投影
				temp->y += int(stepLen * sin(atan2(nodeMsg[i].y - temp->y, nodeMsg[i].x - temp->x)));//步长*方向向量
			}
			temp = temp->lastNode;
			i--;
		}

		temp = nullptr;
		headNode->x += dx;//头指针的移动
		headNode->y += dy;

		if (n == 1)
		{
			delete[] nodeMsg;
			nodeMsg = nullptr;
		}
	}

	// 据长度添加节点
	void SetNode(int ex = 0)
	{
		int n = nNode;
		length += ex;
		n = length / 5 - n;
		if (n > 0)
		{
			while (nNode < maxNode && n != 0)//增加节点
			{
				n--;
				nNode++;
				Node* newNode = new Node;
				newNode->lastNode = tailNode;
				newNode->nextNode = nullptr;
				newNode->x = tailNode->x;
				newNode->y = tailNode->y;
				tailNode->nextNode = newNode;
				tailNode = newNode;
				newNode = nullptr;
			}
		}
		else if (n < 0)//加速减少节点
		{
			while (nNode > 10 && n != 0)
			{
				n++;
				nNode--;
				Node* temp = tailNode;
				tailNode = tailNode->lastNode;//回退
				tailNode->nextNode = nullptr;
				delete temp;//释放
				temp = nullptr;
			}
			if (nNode == 10)
				length = 50;
		}
	}

	// 吃到食物
	bool GetFood(int k, int x, int y)
	{
		int len = nodeGap + 5;
		if (x - headNode->x < len && headNode->x - x < len && y - headNode->y < len && headNode->y - y < len)//正方形判定条件
		{
			food[k].x = rand() % (imgMap->getwidth() - xSide * 2) + xSide;//设置随机点
			food[k].y = rand() % (imgMap->getheight() - ySide * 2) + ySide;
			food[k].r = rand() % 2 + 3;//设置随机密度
			food[k].c = HSVtoRGB(float(rand() % 360), rand() % 1000 / 2000.0f + 0.5f, rand() % 1000 / 2000.0f + 0.5f);//转换 HSV 颜色为 RGB 颜色
			return true;
		}
		return false;
	}


protected:
	long length;
	int nNode;
	int maxNode;
	IMAGE* imgHead, * imgNode, * imgTail;
	POINT* nodeMsg;//鼠标
public:
	// 蛇节点
	typedef struct _NODE
	{
		int x;
		int y;
		struct _NODE* lastNode;
		struct _NODE* nextNode;
	}Node;
	bool isDead;
	static int Count;
	Node* headNode, * tailNode;
};
int SnakeBase::Count = 0;




/* Player 类 */
class Player :public SnakeBase
{
public:
	// 构造函数
	Player() :SnakeBase()
	{
		SetImage(playerColor0, playerColor1, playerColor2);
		int headX = rand() % (imgMap->getwidth() - xSide * 2 - nodeSize * 10) + xSide + nodeSize;
		int headY = rand() % (imgMap->getheight() - ySide * 2 - nodeSize * 12) + ySide + nodeSize;//开始时头结点随机出现
		Createnode(headX, headY);
		nt = frame; 
	}
	// 是否死亡//?????
	void IsDead()
	{
		if (headNode->x <= xSide || headNode->x >= imgMap->getwidth() - 1 - xSide || headNode->y <= ySide || headNode->y >= imgMap->getheight() - 1 - ySide)//超出游戏边界了
			isDead = true;
		else
		{
			double radian = atan2(headNode->y - headNode->nextNode->y, headNode->x - headNode->nextNode->x);//radian为弧度
			int x = ScreenWidth / 2 + int(nodeSize / 2 * cos(radian));
			int y = ScreenHeight / 2 + int(nodeSize / 2 * sin(radian));
			COLORREF c = getpixel(x, y);// 获取点的颜色
			if (!(c == mapLineColor || c == mapMainColor))//不在map里面
				isDead = true;
			else
			{
				for (int i = 1; i < 8; i++)
				{
					x = ScreenWidth / 2 + int(nodeSize / 2 * cos(radian));
					y = ScreenHeight / 2 + int(nodeSize / 2 * sin(radian));
					c = getpixel(x, y);
					if (!(c == mapLineColor || c == mapMainColor))
					{
						isDead = true;
						break;
					}
					x = ScreenWidth / 2 + int(nodeSize / 2 * cos(radian));
					y = ScreenHeight / 2 + int(nodeSize / 2 * sin(radian));
					c = getpixel(x, y);
					if (!(c == mapLineColor || c == mapMainColor))
					{
						isDead = true;
						break;
					}
				}
			}
		}
	}

	// 移动
	void Move(int& ex, int& mapX, int& mapY, short& ddx, short& ddy)
	{
		int dx = int(stepLen * cos(atan2(ddy, ddx)));
		int dy = int(stepLen * sin(atan2(ddy, ddx)));
		mapX -= dx;
		mapY -= dy;
		FlushData(nt, dx, dy);
		for (int k = 0; k < nFood; k++)
			if (GetFood(k, food[k].x, food[k].y))//吃到食物增加长度
				ex += food[k].r / 2;//判断节点个数
		nt--;
		if (nt <= 0)
		{
			nt = frame;
			while (MouseHit())// 检查是否存在鼠标消息
				msg = GetMouseMsg();// 获取一个鼠标消息。如果没有，就等待
			ddx = msg.x - ScreenWidth / 2;
			ddy = msg.y - ScreenHeight / 2;
			SetNode(ex);
			ex = 0;
		}
	}
	// 打印成绩
	void Print()//打印字符串
	{
		settextcolor(WHITE);
		settextstyle(20, 0, _T("Times New Roman"));
		TCHAR str[32] = { 0 };
		_stprintf_s(str, _T("Length：%d"), length);
		outtextxy(5, 5, str);
		_stprintf_s(str, _T("Node：%d"), nNode);
		outtextxy(5, 25, str);
		_stprintf_s(str, _T("Blood：%d"), killCount);
		outtextxy(5, 45, str);
	}
public:
	short nt;
};



/* AI 类 */
class AI :public SnakeBase
{
public:
	// 构造函数
	AI(Player* player)
	{
		p = player;
		COLORREF c0 = HSVtoRGB(float(rand() % 360), 1.0f, 0.4f);
		COLORREF c1 = HSVtoRGB(float(rand() % 360), 1.0f, 0.8f);
		COLORREF c2 = HSVtoRGB(float(rand() % 360), 1.0f, 1.0f);
		SetImage(c0, c1, c2);
		int headX = 0;
		int headY = 0;
		do
		{
			headX = rand() % (imgMap->getwidth() - xSide * 2 - nodeSize * 3) + xSide + nodeSize;
			headY = rand() % (imgMap->getheight() - ySide * 2 - nodeSize * 5) + ySide + nodeSize;//随机生成AI蛇的位置
		} 		while (IsInPlayer(headX, headY));
		Createnode(headX, headY);
		nt = frame;
		minLine = 3 * frame;
		curLine = minLine + (rand() % 12) * frame;//增加AI蛇长
		ddx = (rand() % ScreenWidth) * (rand() % 1000 < 500 ? 1 : -1);
		ddy = (rand() % ScreenHeight) * (rand() % 1000 < 500 ? 1 : -1);//判断象限
		dx = int(stepLen * cos(atan2(ddy, ddx)));
		dy = int(stepLen * sin(atan2(ddy, ddx)));
		isFast = false;//AI蛇一开始不加速
		ct = 0;
		exp = 0;
	}

	// 是否死亡
	bool IsDead(int& ex)
	{
		if (headNode->x <= xSide || headNode->x >= imgMap->getwidth() - 1 - xSide || headNode->y <= ySide || headNode->y >= imgMap->getheight() - 1 - ySide)
			return true;
		else
		{
			bool is_dead = IsInPlayer(headNode->x, headNode->y);//如果AI蛇接触到player
			if (is_dead)
			{
				killCount++;
				ex += nNode;// ?
			}
			return is_dead;
		}
	}

	// 位置是否接触 player
	bool IsInPlayer(int headX, int headY)
	{
		Node* temp = p->headNode;
		while (temp != nullptr)
		{
			if (headX - temp->x < nodeSize && temp->x - headX < nodeSize && headY - temp->y < nodeSize && temp->y - headY < nodeSize)//如果player撞到AI蛇
				return true;
			temp = temp->nextNode;//player的headnode与AI蛇各结点的判定
		}
		return false;
	}


	// 移动
	void Move(int& ex)
	{
		if (curLine <= 0)
		{
			double rad = atan2(ddy, ddx);
			isFast = rand() % 1200 < 100 ? true : false;
			curLine = minLine + (rand() % 21) * frame;

			do
			{
				ddx = (rand() % ScreenWidth + 40) * (rand() % 1000 < 500 ? 1 : -1);
				ddy = (rand() % ScreenHeight + 30) * (rand() % 1000 < 500 ? 1 : -1);
			} while (fabs(atan2(ddy, ddx) - rad) > PI / 6 * 5);
			dx = int(stepLen * cos(atan2(ddy, ddx)));
			dy = int(stepLen * sin(atan2(ddy, ddx)));
		}//随机行走

		FlushData(nt, dx, dy);

		for (int k = 0; k < nFood; k++)
			if (GetFood(k, food[k].x, food[k].y))
				exp += food[k].r / 2;
		nt--;
		curLine--;

		if (nt <= 0)
		{
			nt = frame;
			if (rand() % 1000 < 120)
			{
				SetNode(rand() % 5 + exp);
				exp = 0;
			}
			if (rand() % 1000 < 80)
				curLine = 0;
			if (rand() % 1000 < 900 && curLine > 0)		// 90% 概率避免出界
			{
				int deadX = headNode->x + int((rand() % 50 + nodeGap) * cos(atan2(ddy, ddx)));
				int deadY = headNode->y + int((rand() % 50 + nodeGap) * sin(atan2(ddy, ddx)));
				if (deadX <= xSide || deadX >= imgMap->getwidth() - 1 - xSide)
				{
					ddx *= -1;
					ddy += rand() % 30 + 30;
				}
				if (deadY <= ySide || deadY >= imgMap->getheight() - 1 - ySide)
				{
					ddy *= -1;
					ddx += rand() % 40 + 40;
				}
				dx = int(stepLen * cos(atan2(ddy, ddx)));
				dy = int(stepLen * sin(atan2(ddy, ddx)));
			}

			if (rand() % 1000 < rand() % 400 + 400 && curLine > 0)	// 40%-80%(也可以理解为60%) 概率躲避 player
			{
				int deadX = headNode->x + int((nodeGap + 5) * cos(atan2(ddy, ddx)));
				int deadY = headNode->y + int((nodeGap + 5) * sin(atan2(ddy, ddx)));
				if (IsInPlayer(deadX, deadY))
				{
					ddx = -1 * ddx + rand() % 40 + 40;
					ddy = -1 * ddy + rand() % 30 + 30;
				}
				dx = int(stepLen * cos(atan2(ddy, ddx)));
				dy = int(stepLen * sin(atan2(ddy, ddx)));
			}
		}


		isDead = IsDead(ex);
		if (isDead)
		{
			int n = nNode / 2;
			Node* temp = headNode->nextNode->nextNode;
			while (n--)
			{
				for (int k = 0; k < nFood; k++)
				{
					if (food[k].x < -1 * mapX - 2 * nodeSize  || food[k].x > -1 * mapX + 2 * nodeSize + ScreenWidth
						|| food[k].y < -1 * mapY - 2 * nodeSize || food[k].y > -1 * mapY + 2 * nodeSize + ScreenHeight)
					{
						food[k].x = temp->x + (rand() % 5 + 3) * (rand() % 100 < 50 ? 1 : -1);
						food[k].y = temp->y + (rand() % 5 + 3) * (rand() % 100 < 50 ? 1 : -1);
						food[k].r = nodeSize / 2;
						food[k].c = HSVtoRGB(float(rand() % 360), rand() % 1000 / 2000.0f + 0.5f, rand() % 1000 / 2000.0f + 0.5f);
						break;//死亡后生成food
					}
					else if (k == nFood - 1)
					{
						n = 0;
						break;
					}
				}
				temp = temp->nextNode;
			}
			temp = nullptr;
		}
	}


private:
	short nt;
	int minLine;
	int curLine;
	int ddx, ddy;
	int dx, dy;
	Player* p;
public:
	bool isFast;
	clock_t ct;
	int exp;
};






/* 游戏主体类 */


class Game
{
public:
	// 构造函数
	Game()
	{
		//布置bk
		flushTime = 32;//布置bk
		setbkmode(TRANSPARENT);// 设置背景混合模式
		BeginBatchDraw();// 开始批量绘制
		SetWorkingImage(imgMap);// 设置当前绘图设备
		setlinecolor(mapLineColor);// 设置当前线条颜色
		setfillcolor(mapMainColor);
		setbkcolor(MAGENTA);
		cleardevice();
		//布置结束
	

		//布置方格线
		solidrectangle(xSide, ySide, imgMap->getwidth() - 1 - xSide, imgMap->getheight() - 1 - ySide);// 画填充矩形(无边框)
		for (int i = gap + xSide; i < imgMap->getwidth() - xSide; i += gap)
			line(i, ySide, i, imgMap->getheight() - 1 - ySide);
		for (int j = gap + ySide; j < imgMap->getheight() - ySide; j += gap)// 画方格线
			line(xSide, j, imgMap->getwidth() - 1 - xSide, j);
		SetWorkingImage();
		//布置结束


		//生成人物
		player = new Player;
		mapX = -1 * (player->headNode->x - ScreenWidth / 2);
		mapY = -1 * (player->headNode->y - ScreenHeight / 2);
		ai = new Ai;
		ai->ais = new AI(player);
		Ai* temp = ai;
		for (int aiNum = 1; aiNum < 16; aiNum++)//调大数量可以变成另一个游戏
		{
			temp->next = new Ai;
			temp = temp->next;
			temp->ais = new AI(player);
		}
		temp->next = ai;
		temp = nullptr;
		//生成ai结束
		

		//生成食物
		food = new Food[nFood];
		for (int k = 0; k < nFood; k++)
		{
			food[k].x = rand() % (imgMap->getwidth() - xSide * 2) + xSide;
			food[k].y = rand() % (imgMap->getheight() - ySide * 2) + ySide;
			food[k].r = rand() % 2 + 3;
			food[k].c = HSVtoRGB(float(rand() % 360), rand() % 1000 / 2000.0f + 0.5f, rand() % 1000 / 2000.0f + 0.5f);
		}
		//生成食物结束


		Draw();
		outtextxy((ScreenWidth - textwidth(_T("Left or Right key to accelerate"))) / 2, ScreenHeight / 4 * 3 + 25, _T("Left or Right key to accelerate"));
		outtextxy((ScreenWidth - textwidth(_T("Push any key to continue"))) / 2, ScreenHeight / 4 * 3 + 50, _T("Push any key to continue"));
		FlushBatchDraw();
		ret = _getwch();
	}

	// 构造函数
	~Game()
	{
		EndBatchDraw();// 结束批量绘制，并执行未完成的绘制任务
		delete imgMap;
		delete player;
		if (ai != nullptr)
		{
			Ai* temp = ai->next;
			while (temp != ai)
			{
				delete temp->ais;
				temp = temp->next;
			}
			delete ai->ais;
			delete ai;
		}//批量释放AI
	}



	// 绘制食物
	void DrawFood()
	{
		for (int k = 0; k < nFood; k++)
		{
			if (food[k].x - 9 + mapX >= 0 && food[k].x - 9 + mapX <= ScreenWidth && food[k].y - 9 + mapY >= 0 && food[k].y - 9 + mapY <= ScreenHeight)
			{
				if (food[k].r < 6)
				{
					setfillcolor(food[k].c);//开一数组随机生成颜色
					solidcircle(food[k].x + mapX, food[k].y + mapY, food[k].r);// 画填充圆(无边框)
				}
				else
				{
					SetWorkingImage(&imgFood);
					cleardevice();// 清屏
					setfillcolor(food[k].c);

					void solidcircle(int x, int y, int radius);		
					solidcircle(nodeSize / 2, nodeSize / 2, nodeSize / 2);

					setfillcolor(BLACK);
					solidcircle(nodeSize / 2, nodeSize / 2, nodeSize / 5);

					SetWorkingImage();
					putTimage(food[k].x - nodeSize / 2 + mapX, food[k].y - nodeSize / 2 + mapY, &imgFood);
				}
			}
		}
	}


	// 绘制游戏内界面
	void Draw()
	{
		putimage(mapX, mapY, imgMap);
		DrawFood();
		Ai* temp = ai->next;
		while (temp != ai)
		{
			temp->ais->ShowBody();
			temp = temp->next;
		}
		temp->ais->ShowBody();

		player->IsDead();
		player->ShowBody();
		player->Print();
		FlushBatchDraw();// 执行未完成的绘制任务
	}

	// 运行函数
	void Running()
	{
		short ddx = 0;
		short ddy = 0;
		time_t ct = clock() - time_t(100);
		Ai* temp = ai->next;
		while (temp != ai)
		{
			temp->ais->ct = clock();
			temp = temp->next;
		}
		temp->ais->ct = clock();
		temp = temp->next;
		int ex = 0;

		bool isFast = false;
		while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000) && !player->isDead)//接受键盘信号除了esc
		{
			if (GetAsyncKeyState('P') & 0x8000)
				ret = _getwch();
			if (clock() - ct > flushTime)
			{
				ct = clock();
				isFast = (msg.mkLButton || msg.mkRButton) ? true : false;//按下鼠标左右键加速
				player->Move(ex, mapX, mapY, ddx, ddy);
				if (isFast)
				{
					player->Move(ex, mapX, mapY, ddx, ddy);
					if (rand() % 1000 < 100)
						ex -= 2;//消耗长度
				}
				Draw();
			}



			if (clock() - temp->ais->ct  >  flushTime &&  !player->isDead )
			{
				temp->ais->ct = clock();
				temp->ais->Move(ex);
				if (temp->ais->isDead)
				{
					delete temp->ais;
					temp->ais = new AI(player);
					temp->ais->ct = clock();
				}
				else if (temp->ais->isFast)
				{
					temp->ais->Move(ex);
					if (temp->ais->isDead)
					{
						delete temp->ais;
						temp->ais = new AI(player);
						temp->ais->ct = clock();
					}
				}
				ai = ai->next;
				temp = temp->next;
			}
		}
	}

private:
	int flushTime;
	Player* player;
	typedef struct _AI
	{
		AI* ais;
		struct _AI* next;
	}Ai;
	Ai* ai;
};





/* 主函数 */
int main()
{
	initgraph(ScreenWidth, ScreenHeight);
	srand((unsigned)time(NULL));
	Game game;//运行
	game.Running();
	settextcolor(WHITE);
	settextstyle(50, 0, _T("Times New Roman"));//死亡字幕
	outtextxy((ScreenWidth - textwidth(_T("YOU DIED"))) / 2, ScreenHeight / 2.7, _T("YOU DIED"));
	settextstyle(20, 0, _T("Times New Roman"));
	outtextxy((ScreenWidth - textwidth(_T("Push any key to out"))) / 2, ScreenHeight / 4 * 3 + 50, _T("Push any key to out"));
	FlushBatchDraw();
	while (_kbhit())//防止程序关闭
		ret = _getwch();
	ret = _getwch();
	closegraph();//退出窗口
	return 0;
}
