#include <QDebug>
#include <QSound>
#include <QAction>
#include <QMessageBox>
#include <QPainter>
#include <QLine>
#include <QFont>
#include "main_game_window.h"
#include "ui_main_game_window.h"

// --------- 全局变量 --------- //
const int kIconSize = 50;
const int kTopMargin = 120;
const int kLeftMargin = 60;

const QString kIconReleasedStyle = "";
const QString kIconClickedStyle = "background-color: rgba(255, 255, 12, 161)";
const QString kIconHintStyle = "background-color: rgba(255, 0, 0, 255)";


const int kGameTimeTotal = 5 * 60 * 1000;   // 总时间
const int kGameTimerInterval = 50;         // 进度条刷新时间间隔
const int kLinkTimerDelay = 300;
const int kGameTimeGive = 10000;

int flag = 1;
// -------------------------- //

// 游戏主界面
MainGameWindow::MainGameWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainGameWindow),
    preIcon(NULL),
    curIcon(NULL)
{
    ui->setupUi(this);
    // 重载eventfilter安装到当前window（其实如果不适用ui文件的话直接可以在window的paintevent里面画）
    ui->centralWidget->installEventFilter(this);

    ui->label->setText("时间：");
    ui->label_2->setText("得分：");
    ui->label_3->setText("0");
    ui->hintBtn->setText("提示");
    ui->stop->setText("暂停");
    ui->robot_btn->setText("自动完成");
    ui->reset->setText("重新开始");
    // 设置标题大小
    QFont ft;
    ft.setPointSize(14);
    ui->label->setFont(ft);
    ui->label_2->setFont(ft);
    ui->label_3->setFont(ft);
    ui->timeBar->setFont(ft);
    ui->menuBar->setFont(ft);
    ui->menuGame->setFont(ft);
    ui->hintBtn->setFont(ft);
    ui->stop->setFont(ft);
    ui->robot_btn->setFont(ft);
    ui->reset->setFont(ft);

    ui->label->setFixedSize(60,50);
    ui->label_2->setFixedSize(60,50);
    ui->label_3->setFixedSize(60,50);
    ui->hintBtn->setFixedSize(60,40);
    ui->stop->setFixedSize(60,40);
    ui->robot_btn->setFixedSize(120,40);
    ui->reset->setFixedSize(120,40);
    ui->timeBar->setFixedSize(600,40);

    ui->label->move(10,0);
    ui->timeBar->move(80,5);
    ui->hintBtn->move(690,5);
    ui->stop->move(760,5);
    ui->robot_btn->move(830,5);
    ui->reset->move(960,5);
    ui->label_2->move(1090,0);
    ui->label_3->move(1170,0);

    ui->hintBtn->setStyleSheet("background-color: rgb(0, 255, 0);");
    ui->stop->setStyleSheet("background-color: rgb(0, 255, 0);");
    ui->robot_btn->setStyleSheet("background-color: rgb(0, 255, 0);");
    ui->reset->setStyleSheet("background-color: rgb(0, 255, 0);");
    //    setFixedSize(kLeftMargin * 2 + (kIconMargin + kIconSize) * MAX_COL, kTopMargin + (kIconMargin + kIconSize) * MAX_ROW);
    // 选关信号槽
    connect(ui->actionBasic, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));
    connect(ui->actionMedium, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));
    connect(ui->actionHard, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));
    connect(ui->reset, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));


    // 初始化游戏
    initGame(BASIC);
    gameTimer->stop();

}

MainGameWindow::~MainGameWindow()
{
    if (game)
        delete game;

    delete ui;
}

void MainGameWindow::initGame(GameLevel level)
{
    // 启动游戏

    game = new GameModel;
    game->startGame(level);
    grade = 0;
    // 添加button
    for(int i = 0; i < MAX_ROW * MAX_COL; i++)
    {
        imageButton[i] = new IconButton(this);
        imageButton[i]->setGeometry(kLeftMargin + (i % MAX_COL) * kIconSize, kTopMargin + (i / MAX_COL) * kIconSize, kIconSize, kIconSize);
        // 设置索引
        imageButton[i]->xID = i % MAX_COL;
        imageButton[i]->yID = i / MAX_COL;

        imageButton[i]->show();

        if (game->getGameMap()[i])
        {
            // 有方块就设置图片
            QPixmap iconPix;
            QString fileString;
            fileString.sprintf(":/res/image/%d.png", game->getGameMap()[i]);
            iconPix.load(fileString);
            QIcon icon(iconPix);
            imageButton[i]->setIcon(icon);
            imageButton[i]->setIconSize(QSize(kIconSize, kIconSize));

            // 添加按下的信号槽
            connect(imageButton[i], SIGNAL(pressed()), this, SLOT(onIconButtonPressed()));
        }
        else
            imageButton[i]->hide();
    }

    // 进度条
    ui->timeBar->setMaximum(kGameTimeTotal);
    ui->timeBar->setMinimum(0);
    ui->timeBar->setValue(kGameTimeTotal);

    // 游戏计时器
    gameTimer = new QTimer(this);
    connect(gameTimer, SIGNAL(timeout()), this, SLOT(gameTimerEvent()));
    gameTimer->start(kGameTimerInterval);

    // 连接状态值
    isLinking = false;

    // 播放背景音乐(QMediaPlayer只能播放绝对路径文件),确保res文件在程序执行文件目录里而不是开发目录
    audioPlayer = new QMediaPlayer(this);
    QString curDir = QCoreApplication::applicationDirPath(); // 这个api获取路径在不同系统下不一样,mac 下需要截取路径
    QStringList sections = curDir.split(QRegExp("[/]"));
    QString musicPath;

    for (int i = 0; i < sections.size() - 3; i++)
        musicPath += sections[i] + "/";

    audioPlayer->setMedia(QUrl::fromLocalFile(musicPath + "res/sound/backgrand.mp3"));
    audioPlayer->play();
}

void MainGameWindow::onIconButtonPressed()
{
    // 如果当前有方块在连接，不能点击方块
    if (isLinking)
    {
        // 播放音效
        QSound::play(":/res/sound/release.wav");
        return;
    }

    // 记录当前点击的icon
    curIcon = dynamic_cast<IconButton *>(sender());

    if(!preIcon)
    {
        // 播放音效
        QSound::play(":/res/sound/select.wav");

        // 如果单击一个icon
        curIcon->setStyleSheet(kIconClickedStyle);
        preIcon = curIcon;
    }
    else
    {
        if(curIcon != preIcon)
        {
            // 如果不是同一个button就都标记,尝试连接
            curIcon->setStyleSheet(kIconClickedStyle);
            if(game->linkTwoTiles(preIcon->xID, preIcon->yID, curIcon->xID, curIcon->yID))
            {
                // 锁住当前状态
                isLinking = true;

                grade += 10;
                str = QString::number(grade);
                ui->label_3->setText(str);

                // 播放音效
                QSound::play(":/res/sound/pair.wav");

                // 重绘
                update();

                // 延迟后实现连接效果
                QTimer::singleShot(kLinkTimerDelay, this, SLOT(handleLinkEffect()));
                kGameGived = ui->timeBar->value();
                kGameGived += kGameTimeGive;

                // 每次检查一下是否僵局
                if (game->isFrozen())
                    QMessageBox::information(this, "oops", "dead game");

                // 检查是否胜利
                if (game->isWin())
                    QMessageBox::information(this, "great", "you win");

                int *hints = game->getHint();
            }
            else
            {
                // 播放音效
                QSound::play(":/res/sound/release.wav");

                // 消除失败，恢复
                preIcon->setStyleSheet(kIconReleasedStyle);
                curIcon->setStyleSheet(kIconReleasedStyle);

                // 指针置空，用于下次点击判断
                preIcon = NULL;
                curIcon = NULL;
            }
        }
        else if(curIcon == preIcon)
        {
            // 播放音效
            QSound::play(":/res/sound/release.wav");

            preIcon->setStyleSheet(kIconReleasedStyle);
            curIcon->setStyleSheet(kIconReleasedStyle);
            preIcon = NULL;
            curIcon = NULL;
        }
    }
}

void MainGameWindow::handleLinkEffect()
{
    // 消除成功，隐藏掉，并析构
    game->paintPoints.clear();
    preIcon->hide();
    curIcon->hide();

    preIcon = NULL;
    curIcon = NULL;

    // 重绘
    update();

    // 恢复状态
    isLinking = false;
}

bool MainGameWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 重绘时会调用，可以手动调用
    if (event->type() == QEvent::Paint)
    {
        // 定义画板并关联
        QPainter painter(ui->centralWidget);
        // 定义画笔
        QPen pen;
        // 颜色随机
        QColor color(rand() % 256, rand() % 256, rand() % 256);
        pen.setColor(color);
        pen.setWidth(5);
        painter.setPen(pen);

        QString str;
        for (int i = 0; i < game->paintPoints.size(); i++)
        {
            PaintPoint p = game->paintPoints[i];
            str += "x:" + QString::number(p.x) + "y:" + QString::number(p.y) + "->";
        }
        // qDebug() << str;

        // 连接各点画线
        for (int i = 0; i < int(game->paintPoints.size()) - 1; i++)
        {
            PaintPoint p1 = game->paintPoints[i];
            PaintPoint p2 = game->paintPoints[i + 1];

            // 拿到各button的坐标,注意边缘点坐标
            QPoint btn_pos1;
            QPoint btn_pos2;

            // p1
            if (p1.x == -1)
            {
                btn_pos1 = imageButton[p1.y * MAX_COL + 0]->pos();
                btn_pos1 = QPoint(btn_pos1.x() - kIconSize, btn_pos1.y());
            }
            else if (p1.x == MAX_COL)
            {
                btn_pos1 = imageButton[p1.y * MAX_COL + MAX_COL - 1]->pos();
                btn_pos1 = QPoint(btn_pos1.x() + kIconSize, btn_pos1.y()/2);
            }
            else if (p1.y == -1)
            {
                btn_pos1 = imageButton[0 + p1.x]->pos();
                btn_pos1 = QPoint(btn_pos1.x(), btn_pos1.y() - kIconSize);
            }
            else if (p1.y == MAX_ROW)
            {
                btn_pos1 = imageButton[(MAX_ROW - 1) * MAX_COL + p1.x]->pos();
                btn_pos1 = QPoint(btn_pos1.x(), btn_pos1.y() + kIconSize);
            }
            else
                btn_pos1 = imageButton[p1.y * MAX_COL + p1.x]->pos();

            // p2
            if (p2.x == -1)
            {
                btn_pos2 = imageButton[p2.y * MAX_COL + 0]->pos();
                btn_pos2 = QPoint(btn_pos2.x() - kIconSize, btn_pos2.y());
            }
            else if (p2.x == MAX_COL)
            {
                btn_pos2 = imageButton[p2.y * MAX_COL + MAX_COL - 1]->pos();
                btn_pos2 = QPoint(btn_pos2.x() + kIconSize, btn_pos2.y());
            }
            else if (p2.y == -1)
            {
                btn_pos2 = imageButton[0 + p2.x]->pos();
                btn_pos2 = QPoint(btn_pos2.x(), btn_pos2.y() - kIconSize);
            }
            else if (p2.y == MAX_ROW)
            {
                btn_pos2 = imageButton[(MAX_ROW - 1) * MAX_COL + p2.x]->pos();
                btn_pos2 = QPoint(btn_pos2.x(), btn_pos2.y() + kIconSize);
            }
            else
                btn_pos2 = imageButton[p2.y * MAX_COL + p2.x]->pos();
            // 中心点
            QPoint pos1(btn_pos1.x() + kIconSize / 2, btn_pos1.y() - kIconSize / 16);
            QPoint pos2(btn_pos2.x() + kIconSize / 2, btn_pos2.y() - kIconSize / 16);

            painter.drawLine(pos1, pos2);
        }

        return true;
    }
    else
        return QMainWindow::eventFilter(watched, event);
}

void MainGameWindow::gameTimerEvent()
{
    // 进度条计时效果
    if(ui->timeBar->value() == 0)
    {
        gameTimer->stop();
        QMessageBox::information(this, "game over", "play again>_<");
    }
    else
    {
        ui->timeBar->setValue(ui->timeBar->value() - kGameTimerInterval);
    }

}

// 提示
void MainGameWindow::on_hintBtn_clicked()
{
    // 初始时不能获得提示
    for (int i = 0; i < 4;i++)
        if (game->getHint()[i] == -1)
            return;

    int srcX = game->getHint()[0];
    int srcY = game->getHint()[1];
    int dstX = game->getHint()[2];
    int dstY = game->getHint()[3];

    IconButton *srcIcon = imageButton[srcY * MAX_COL + srcX];
    IconButton *dstIcon = imageButton[dstY * MAX_COL + dstX];
    srcIcon->setStyleSheet(kIconHintStyle);
    dstIcon->setStyleSheet(kIconHintStyle);
    grade -= 20;
    str = QString::number(grade);
    ui->label_3->setText(str);


}

void MainGameWindow::on_robot_btn_clicked()
{
    // 初始时不能自动玩
    for (int i = 0; i < 4;i++)
        if (game->getHint()[i] == -1)
            return;

    while (game->gameStatus == PLAYING)
    {
        // 连接生成提示

        int srcX = game->getHint()[0];
        int srcY = game->getHint()[1];
        int dstX = game->getHint()[2];
        int dstY = game->getHint()[3];

        if(game->linkTwoTiles(srcX, srcY, dstX, dstY))
        {
            // 播放音效
            // QSound::play(":/res/sound/pair.wav");

            // 消除成功，隐藏掉
            IconButton *icon1 = imageButton[srcY * MAX_COL + srcX];
            IconButton *icon2 = imageButton[dstY * MAX_COL + dstX];

            icon1->hide();
            icon2->hide();

            game->paintPoints.clear();

            // 重绘
            update();

            // 检查是否胜利
            if (game->isWin())
                QMessageBox::information(this, "great", "you win");

            // 每次检查一下是否僵局
            if (game->isFrozen() && game->gameStatus == PLAYING)
                QMessageBox::information(this, "oops", "dead game");

            int *hints = game->getHint();
        }
    }
}

void MainGameWindow::createGameWithLevel()
{
    // 先析构之前的
    if (game)
    {
        delete game;
        for (int i = 0;i < MAX_ROW * MAX_COL; i++)
        {
            if (imageButton[i])
               delete imageButton[i];
        }
    }

    // 停止音乐
    audioPlayer->stop();

    // 重绘
    update();
    lev = (QAction *)dynamic_cast<QAction *>(sender());
    QAction *actionSender = (QAction *)dynamic_cast<QAction *>(sender());
    if (actionSender == ui->actionBasic)
    {
        initGame(BASIC);
    }
    else if (actionSender == ui->actionMedium)
    {
        initGame(MEDIUM);
    }
    else if (actionSender == ui->actionHard)
    {
        initGame(HARD);
    }

}

void MainGameWindow::on_reset_clicked()
{
    if (game)
    {
        delete game;
        for (int i = 0;i < MAX_ROW * MAX_COL; i++)
        {
            if (imageButton[i])
               delete imageButton[i];
        }
    }

    // 停止音乐
    audioPlayer->stop();

    // 重绘
    update();
    if (lev == ui->actionBasic)
    {
        initGame(BASIC);
    }
    else if (lev == ui->actionMedium)
    {
        initGame(MEDIUM);
    }
    else if (lev == ui->actionHard)
    {
        initGame(HARD);
    }
}

void MainGameWindow::on_stop_clicked()
{
    flag++;
    if(flag%2==1)
    {
        gameTimer->start();
    }
    if(flag%2==0)
    {
        gameTimer->stop();
    }
}
