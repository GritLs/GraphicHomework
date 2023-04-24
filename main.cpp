#include <graphics.h>
#include <stack>
#include <vector>
#include <iostream>
using namespace std;
using std::stack;				// 使用STL的栈
using std::vector;				// 使用STL的数组容器


// 游戏信息
#define WIN_WIDTH	400			// 窗口的宽度(单位：像素)
#define WIN_HEIGHT	300			// 窗口的高度(单位：像素)
// !!注：由于随机生成算法的原因，地图宽高只能为奇数
#define GAME_WIDTH	41			// 地图的宽度(单位：块)
#define GAME_HEIGHT	51			// 地图的高度(单位：块)

#define WALL		1			// 墙壁的数字标记
#define GROUND		0			// 地面的数字标记
#define FILLSTATE	2			// 加油站的数字标记
#define ENDPOS		3			// 终点的数字标记

#define MAXVIEW		8.0			// 最大的视野
#define MINVIEW		1			// 最小的视野
#define FILLNUM		10			// 加油站的数量
#define DARKTIME	12			// 视野下降1图块所需的时间

// 全局变量列表
int		g_BlockSize;			// 块大小
int		g_GameMap[GAME_HEIGHT][GAME_WIDTH];	// 地图(宽高单位为块)
POINT	g_EndPos;				// 终点位置
POINT   g_PlayerPos;			// 玩家在地图上的位置
POINT	g_CameraPos;			// 摄像机(屏幕左上角)在地图上的位置
IMAGE	g_MapImage;				// 地图的图片(由于地图是固定的，在不改变缩放的情况下只需要绘制一次)
double	g_ViewArray;			// 视野
UINT	g_BeginTime;			// 游戏开始时的时间
UINT	g_LastFillTime;			// 上次为油灯加油的时间
vector<POINT> viewSquare; //视野范围

// 函数列表
void initGame();				// 初始化游戏
void endGame();					// 结束游戏
void draw();					// 绘制函数
bool upDate();					// 数据更新函数
void absDelay(int delay);		// 绝对延迟

bool canMove(POINT pos);		// 判断某个位置是否可以移动
void computeCameraPos();		// 计算摄像机在地图上的位置
void drawPlayer();				// 绘制人物的函数

//三种多边形，分别为墙体、油灯、终点，地面不填充
struct ThreeTypePolygons {
    vector<vector<POINT>> polygonWall;      //墙体多边形
    vector<vector<POINT>> polygonFillState; //油灯多边形
    vector<vector<POINT>> polygonEndPos;    //终点位置多边形
}polygonOrigin, polygonCurrent;
ThreeTypePolygons BlockToLine();             // 将墙体的块表示成点集用于多边形分割算法
void clip_polygons(const vector<POINT> &viewSquare); //实现多边形裁剪
// here test
void draw_current(const ThreeTypePolygons &polygons);
int main()
{
    initGame();

    while (1)
    {
        if (!upDate()) break;	// 更新
        draw();					// 绘制
        absDelay(16);			// 绝对延迟 16 毫秒，控制每秒 60 帧
    }

    endGame();
    return 0;
}

void initGame()
{
    g_BlockSize = 32;			// 初始图块大小为 32 个像素
    srand(GetTickCount());		// 初始化随机数生成

    // 初始化间隔室
    for (int i = 0; i < GAME_HEIGHT; i++)
    {
        for (int j = 0; j < GAME_WIDTH; j++)
        {
            if (i % 2 == 0 || j % 2 == 0)	// 奇数行奇数列设为墙壁
                g_GameMap[i][j] = WALL;
            else
                g_GameMap[i][j] = GROUND;
        }
    }

    // 随机生成地图(使用深度优先遍历)
    stack<POINT> stepStack;		// 步骤栈
    vector<POINT>  stepPoint;	// 四周的点
    POINT nowPoint;				// 当前步的所在点
    stepStack.push({ 1,1 });	// 写入初始点 (1,1) 作为起点
    nowPoint = { 1,1 };
    g_GameMap[1][1] = 0xFFFF;	// 标记这个点
    while (!stepStack.empty())	// 只要步骤栈不空就继续循环
    {
        // 得到四周的点
        POINT tempPoint;
        for (int i = -1; i <= 1; i += 2)
        {
            tempPoint = { nowPoint.x,nowPoint.y + i * 2 };	// 计算点
            // 判断坐标是否合法
            if (tempPoint.x >= 0 && tempPoint.x <= GAME_WIDTH - 1 &&
                tempPoint.y >= 0 && tempPoint.y <= GAME_HEIGHT - 1 &&
                g_GameMap[tempPoint.y][tempPoint.x] != 0xFFFF)
            {
                stepPoint.push_back(tempPoint);
            }
            tempPoint = { nowPoint.x + i * 2 ,nowPoint.y };	// 计算点
            // 判断坐标是否合法
            if (tempPoint.x >= 0 && tempPoint.x <= GAME_WIDTH - 1 &&
                tempPoint.y >= 0 && tempPoint.y <= GAME_HEIGHT - 1 &&
                g_GameMap[tempPoint.y][tempPoint.x] != 0xFFFF)
            {
                stepPoint.push_back(tempPoint);
            }
        }

        // 根据周围点的量选择操作
        if (stepPoint.empty())				// 如果周围点都被遍历过了
        {
            stepStack.pop();				// 出栈当前点
            if (!stepStack.empty())
                nowPoint = stepStack.top();	// 更新当前点
        }
        else
        {
            stepStack.push(stepPoint[rand() % stepPoint.size()]);	// 入栈当前点
            g_GameMap[(nowPoint.y + stepStack.top().y) / 2][(nowPoint.x + stepStack.top().x) / 2] = GROUND;	// 打通墙壁
            nowPoint = stepStack.top();		// 更新当前点
            g_GameMap[nowPoint.y][nowPoint.x] = 0xFFFF;				// 标记当前点
        }
        stepPoint.clear();					// 清空周围点以便下一次循环
    }

    // 清洗标记点
    for (int i = 0; i < GAME_HEIGHT; i++)
    {
        for (int j = 0; j < GAME_WIDTH; j++)
        {
            if (g_GameMap[i][j] == 0xFFFF)
                g_GameMap[i][j] = GROUND;
        }
    }

    // 随机生成加油站的位置
    for (int i = 0; i < FILLNUM; i++)
    {
        POINT fillPoint = { rand() % GAME_WIDTH,rand() % GAME_HEIGHT };
        // 保证在空地生成加油站
        while (g_GameMap[fillPoint.y][fillPoint.x] != GROUND)
            fillPoint = { rand() % GAME_WIDTH,rand() % GAME_HEIGHT };
        // 标记油灯
        g_GameMap[fillPoint.y][fillPoint.x] = FILLSTATE;
    }

    //////////////////生成完墙和灯之后，将墙和灯用线段表示////////////////
    polygonOrigin = BlockToLine();
    g_GameMap[GAME_HEIGHT - 2][GAME_WIDTH - 2] = ENDPOS;		// 标记终点
    g_EndPos = { GAME_WIDTH - 2,GAME_HEIGHT - 2 };				// 确定终点位置
    g_ViewArray = MAXVIEW;				// 初始视野是最大的
    g_BeginTime = GetTickCount();		// 开始计时
    g_LastFillTime = GetTickCount();	// 油灯加油的时间
    g_PlayerPos = { g_BlockSize * 3 / 2,g_BlockSize * 3 / 2 };	// 初始化人的位置
    computeCameraPos();					// 计算摄像机的位置
    initgraph(WIN_WIDTH, WIN_HEIGHT);	// 初始化画布
    setbkmode(TRANSPARENT);				// 设置背景为透明
    BeginBatchDraw();					// 开始缓冲绘制
}



//返回一个三种块类型的点集的结构体用于裁剪算法的实现
ThreeTypePolygons BlockToLine(){
    // 创建一个表示多边形数组的向量
    ThreeTypePolygons drawArea;

    // 遍历 g_GameMap
    for (int i = 0; i < GAME_HEIGHT; i++) {
        for (int j = 0; j < GAME_WIDTH; j++) {
            vector<POINT> polygon;
            int x1 = j * g_BlockSize;
            int y1 = i * g_BlockSize;
            int x2 = x1 +g_BlockSize;
            int y2 = y1;
            int x3 = x1+ g_BlockSize;
            int y3 = y1+ g_BlockSize;
            int x4 = x1;
            int y4 = y1+ g_BlockSize;

            polygon.push_back({x1, y1});
            polygon.push_back({x2, y2});
            polygon.push_back({x3, y3});
            polygon.push_back({x4, y4});

            if (g_GameMap[i][j] == WALL) {

                drawArea.polygonWall.push_back(polygon);
            }
            if (g_GameMap[i][j] == FILLSTATE ) {
                polygon.clear();
                int bias = int(g_BlockSize/5);
                polygon.push_back({x1+bias, y1+bias});
                polygon.push_back({x2-bias, y2+bias});
                polygon.push_back({x3-bias, y3-bias});
                polygon.push_back({x4+bias, y4-bias});

                drawArea.polygonFillState.push_back(polygon);
            }
            if (g_GameMap[i][j] == ENDPOS) {

                drawArea.polygonEndPos.push_back(polygon);
            }
        }
    }

    return drawArea;
}



void endGame()
{
    EndBatchDraw();						// 结束缓冲绘制
    closegraph();						// 关闭画布
}

void draw()
{
    //设置背景色
    setbkcolor(WHITE);
    // 清空设备
    cleardevice();
    //测试画图
    draw_current(polygonCurrent);
    // 绘制人
    drawPlayer();
    FlushBatchDraw();	// 刷新屏幕
}

bool upDate()
{
    POINT nextPos = g_PlayerPos;		// 下一个位置

    // 获取键盘输入并计算下一个位置
    if (GetKeyState(VK_UP) & 0x8000)	nextPos.y -= 3;
    if (GetKeyState(VK_DOWN) & 0x8000)	nextPos.y += 3;
    if (GetKeyState(VK_LEFT) & 0x8000)	nextPos.x -= 3;
    if (GetKeyState(VK_RIGHT) & 0x8000)	nextPos.x += 3;

    // 如果下一个位置不合法
    if (!canMove(nextPos))
    {
        if (canMove({ g_PlayerPos.x, nextPos.y }))		// y 轴移动合法
            nextPos = { g_PlayerPos.x, nextPos.y };
        else if (canMove({ nextPos.x, g_PlayerPos.y }))	// x 轴移动合法
            nextPos = { nextPos.x, g_PlayerPos.y };
        else											// 都不合法
            nextPos = g_PlayerPos;
    }

    // 如果是油灯则更新时间
    if (g_GameMap[nextPos.y / g_BlockSize][nextPos.x / g_BlockSize] == FILLSTATE)
        g_LastFillTime = GetTickCount();
        // 如果是终点则通关
    else if (g_GameMap[nextPos.y / g_BlockSize][nextPos.x / g_BlockSize] == ENDPOS)
    {
        outtextxy(WIN_WIDTH / 2 - 40, WIN_HEIGHT / 2 - 12, _T("恭喜过关！"));
        FlushBatchDraw();
        Sleep(1000);
        return false;
    }
    g_PlayerPos = nextPos;						// 更新位置
    computeCameraPos();							// 计算摄像机的位置

    // 根据时间缩减视野
    static unsigned int lastTime = GetTickCount();
    int loseTime = GetTickCount() - g_LastFillTime;			// 计算流失的时间
    g_ViewArray = MAXVIEW - loseTime / 1000.0 / DARKTIME;	// 每一段时间油灯的照明力会下降一个图块
    if (g_ViewArray < MINVIEW) g_ViewArray = MINVIEW;

    int r = int(g_BlockSize * g_ViewArray + 0.5)/2;	// 计算视野半径
    // 把视野正方形表示出来
    viewSquare = {{g_PlayerPos.x-r,g_PlayerPos.y+r},
                {g_PlayerPos.x-r,g_PlayerPos.y-r},
                {g_PlayerPos.x+r,g_PlayerPos.y-r},
                {g_PlayerPos.x+r,g_PlayerPos.y+r}};//reverse


    // 在这里应用裁剪算法，拿视野区域进行裁剪
    // 实现Sutherland-Hodgman算法，传入视野正方形的结构体，然后以这个正方形为边界对polygonOrigin
    // 进行裁剪，并将结果存入polygonCurrent
    clip_polygons(viewSquare);
    return true;
}

bool inside(POINT p, POINT A, POINT B) {
    return (B.x - A.x) * (p.y - A.y) > (B.y - A.y) * (p.x - A.x);
}

POINT intersection(POINT prev, POINT cur, POINT A, POINT B) {
    POINT ret;
    if(A.y==B.y){
        ret.y = A.y;
        ret.x = prev.x + (A.y-prev.y)*(cur.x-prev.x)/(cur.y-prev.y);
    }
    else{
        ret.x = A.x;
        ret.y = prev.y + (A.x-prev.x)*(cur.y-prev.y)/(cur.x-prev.x);
    }
    return ret;
}

vector<POINT> sutherland_hodgman_clip(const vector<POINT> &polygon, const vector<POINT> &clip) {
    vector<POINT> output = polygon;

    for (int i = 0; i < 4; ++i) {
        POINT A = clip[i];
        POINT B = clip[(i + 1) % 4];

        vector<POINT> input = output;
        output.clear();

        for (size_t j = 0; j < input.size(); ++j) {
            POINT cur = input[j];
            POINT prev = input[(j + input.size() - 1) % input.size()];
            POINT p_int;
            if (inside(cur, A, B)) {
                if (!inside(prev, A, B)) {
                    p_int = intersection(prev, cur, A, B);
                    output.push_back(p_int);
                }
                output.push_back(cur);
            } else if (inside(prev, A, B)) {
                p_int = intersection(prev, cur, A, B);
                output.push_back(p_int);
            }
        }
    }
    return output;
}

void clip_polygons(const vector<POINT> &viewSquare) {
//    ThreeTypePolygons polygonCurrent;
    polygonCurrent.polygonWall.clear();
    polygonCurrent.polygonFillState.clear();
    polygonCurrent.polygonEndPos.clear();
    for (const auto &polygon : polygonOrigin.polygonWall) {
        polygonCurrent.polygonWall.push_back(sutherland_hodgman_clip(polygon, viewSquare));
    }
    for (const auto &polygon : polygonOrigin.polygonFillState) {
        polygonCurrent.polygonFillState.push_back(sutherland_hodgman_clip(polygon, viewSquare));
    }
    for (const auto &polygon : polygonOrigin.polygonEndPos) {
        polygonCurrent.polygonEndPos.push_back(sutherland_hodgman_clip(polygon, viewSquare));
    }
}


void absDelay(int delay)
{
    static int curtime = GetTickCount();
    static int pretime = GetTickCount();
    while (curtime - pretime < delay)
    {
        curtime = GetTickCount();
        Sleep(1);
    }
    pretime = curtime;
}

bool canMove(POINT pos)
{
    int range = 3;
    // 只要外接矩形的四个顶点不在墙壁内就必定合法
    return g_GameMap[(pos.y - range) / g_BlockSize][(pos.x - range) / g_BlockSize] != WALL &&
    g_GameMap[(pos.y + range) / g_BlockSize][(pos.x + range) / g_BlockSize] != WALL &&
    g_GameMap[(pos.y - range) / g_BlockSize][(pos.x + range) / g_BlockSize] != WALL &&
    g_GameMap[(pos.y + range) / g_BlockSize][(pos.x - range) / g_BlockSize] != WALL;
}

void computeCameraPos()
{
    // 以人物位置为中心计算摄像机的理论位置
    g_CameraPos.x = g_PlayerPos.x - WIN_WIDTH / 2;
    g_CameraPos.y = g_PlayerPos.y - WIN_HEIGHT / 2;

    // 防止摄像机越界
    if (g_CameraPos.x < 0)										g_CameraPos.x = 0;
    if (g_CameraPos.y < 0)										g_CameraPos.y = 0;
    if (g_CameraPos.x > GAME_WIDTH * g_BlockSize - WIN_WIDTH)	g_CameraPos.x = GAME_WIDTH * g_BlockSize - WIN_WIDTH;
    if (g_CameraPos.y > GAME_HEIGHT * g_BlockSize - WIN_HEIGHT)	g_CameraPos.y = GAME_HEIGHT * g_BlockSize - WIN_HEIGHT;
}


void vectorToPointArray(const std::vector<POINT>& points, POINT*& point_array)
{
    point_array = new POINT[points.size()];
    for (size_t i = 0; i < points.size(); i++) {
        point_array[i] = points[i];
    }
}

void draw_current(const ThreeTypePolygons &polygons) {

    int x1 = viewSquare[1].x-g_CameraPos.x;
    int y1 = viewSquare[1].y-g_CameraPos.y;
    int x2 = viewSquare[3].x-g_CameraPos.x;
    int y2 = viewSquare[3].y-g_CameraPos.y;
    setfillcolor(BLACK);
    setlinecolor(BLACK);
    fillrectangle(0,0,x1,getheight());
    fillrectangle(0,0,getwidth(),y1);
    fillrectangle(x2,0,getwidth(),getheight());
    fillrectangle(0,y2,getwidth(),getheight());


    setfillcolor(RGB(130,57,53));
    setlinecolor(RGB(130,57,53));
    for (auto polygon : polygons.polygonWall) {
        for (size_t i = 0; i < polygon.size(); ++i) {
            polygon[i].x = polygon[i].x - g_CameraPos.x;
            polygon[i].y = polygon[i].y - g_CameraPos.y;
        }

        POINT* point_array = nullptr;
        vectorToPointArray(polygon, point_array);
        fillpolygon(point_array,polygon.size());
    }

    setfillcolor(RGB(244,208,0));
    setlinecolor(RGB(244,208,0));
    for (auto polygon : polygons.polygonFillState) {
        for (size_t i = 0; i < polygon.size(); ++i) {
            polygon[i].x = polygon[i].x - g_CameraPos.x;
            polygon[i].y = polygon[i].y - g_CameraPos.y;
        }

        POINT* point_array = nullptr;
        vectorToPointArray(polygon, point_array);
        fillpolygon(point_array,polygon.size());
    }
    setfillcolor(BLUE);
    for (auto polygon : polygons.polygonEndPos) {
        for (size_t i = 0; i < polygon.size(); ++i) {
            polygon[i].x = polygon[i].x - g_CameraPos.x;
            polygon[i].y = polygon[i].y - g_CameraPos.y;
        }

        POINT* point_array = nullptr;
        vectorToPointArray(polygon, point_array);
        fillpolygon(point_array,polygon.size());
    }
}

void drawPlayer()
{
    setfillcolor(RGB(252, 213, 11));
    solidcircle(g_PlayerPos.x - g_CameraPos.x, g_PlayerPos.y - g_CameraPos.y, 3);
}

