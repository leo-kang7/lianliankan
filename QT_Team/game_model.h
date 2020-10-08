#ifndef GAMEMODEL_H
#define GAMEMODEL_H

#include <vector>

// ------------ 全局变量 ------------ //
// 最大行和列数
// MAX_ROW 最大行数
// MAX_COL 最大列数
const int MAX_ROW = 15;
const int MAX_COL = 20;

// 游戏可玩图片数量
const int MAX_ICON = 25;

// 游戏状态
enum GameStatus
{
    REAGY,
    PLAYING,
    PAUSE,
    WIN,
    OVER
};

// 游戏难度，不同的方块数
const int kBasicNum = MAX_ROW * MAX_COL * 0.3;
const int kMediumNum = MAX_ROW * MAX_COL * 0.7;
const int kHardNum = MAX_ROW * MAX_COL;

enum GameLevel
{
    BASIC,
    MEDIUM,
    HARD
};

// 用于绘制线段的连接点
struct PaintPoint
{
    PaintPoint(int _x, int _y) : x(_x), y (_y) {}
    int x;
    int y;
};

class GameModel
{
public:
    GameModel();
    virtual ~GameModel();
public:
    void startGame();                                           // 开始游戏
    void startGame(GameLevel level);
    GameStatus checkGameStatus();                               // 判断获得游戏状态，是否输赢
    bool linkTwoTiles(int srcX, int srcY, int dstX,int dstY);   // 连接起点和终点方块，连接是否成功
    bool isFrozen();                                            // 判断是否已经成为了僵局
    bool isWin();                                               // 检查游戏是否结束
    int *getHint();                                             // 获得提示
    int *getGameMap();                                          // 获得地图
    std::vector<PaintPoint> paintPoints;                        // 用于绘制的点

    // 游戏状态和难度
    GameStatus gameStatus;
    GameLevel gameLevel;

private:
    // 游戏地图，存储方块，0表示消失，1-其他数字表示图片标号
    int *gameMap;

    // 游戏提示，存储2个点
    int *hintArray;

    // 判断起点到终点的方块是否可以连接消除
    bool isCanLink(int srcX, int srcY, int dstX, int dstY);

    bool canLinkDirectly(int srcX, int srcY, int dstX, int dstY);
    bool canLinkWithOneCorner(int srcX, int srcY, int dstX, int dstY);
    bool canLinkWithTwoCorner(int srcX, int srcY, int dstX, int dstY);

    // 提示模式还是连接模式判断
    // false 为连接模式
    // true 为提示模式
    bool isFrozenMode;

};

#endif // GAMEMODEL_H
