#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDateTime>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    int size = 10;  //Set a fixed size grid

    //Store all the pixels to be drawn on screen into QRect pixels
    for(uint8_t row=0; row < 32; row++)
    {
        for(uint8_t col=0; col < 64; col++)
        {
            pixels.append(QRect(row*size, col*size, size, size));
        }
    }

    //Assign key values to Key array
    keyArray[Qt::Key_1] = 0;
    keyArray[Qt::Key_2] = 1;
    keyArray[Qt::Key_3] = 2;
    keyArray[Qt::Key_4] = 3;
    keyArray[Qt::Key_Q] = 4;
    keyArray[Qt::Key_W] = 5;
    keyArray[Qt::Key_E] = 6;
    keyArray[Qt::Key_R] = 7;
    keyArray[Qt::Key_A] = 8;
    keyArray[Qt::Key_S] = 9;
    keyArray[Qt::Key_D] = 0xA;
    keyArray[Qt::Key_F] = 0xB;
    keyArray[Qt::Key_Z] = 0xC;
    keyArray[Qt::Key_X] = 0xD;
    keyArray[Qt::Key_C] = 0xE;
    keyArray[Qt::Key_V] = 0xF;

    //Connect File Menu bar to open File browser dialog, to locate the program file
    connect(ui->actionBrowse, &QAction::triggered, this, &MainWindow::browse);

    mTimer = new QTimer(this);

}

MainWindow::~MainWindow()
{
    killTimer(timerId); //kill timer pointer before destroying the class
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QPointF center = QPointF(0,0);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.translate(center);
    painter.rotate(90);
    painter.scale(1, -1);


    for(uint8_t row=0; row < 32; ++row)
    {
        for(uint8_t col=0; col < 64; ++col)
        {
            if(cpu.gfx[(row * 64) + col] != 0)
            {
                painter.fillRect(pixels.at((row * 64) + col), QBrush(Qt::white));
            }
            else
            {
                painter.fillRect(pixels.at((row * 64) + col), QBrush(Qt::black));
            }
        }
    }

}

void MainWindow::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);    //Silent unused warning for event
    QDateTime _DT;
    quint64 current_time = _DT.currentMSecsSinceEpoch();
    static quint64 target_time;
    if ((current_time >= target_time))
    {
        target_time = current_time + 2;

        //Execute cpu cycle
        cpu.execute();
        if(cpu.drawFlag)
        {
            update();   //Call paintEvent and draw on screen
            cpu.drawFlag = false;   //Disable draw flag till next pixel changes
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *key)
{
    //Clear the priviously pressed key
    for(uint8_t i=0; i<16; ++i)
        cpu.key[i] = 0;

    //Store new key pressed
    cpu.key[keyArray[key->key()]] = 1;
}

void MainWindow::browse()
{
    //Open File browser and get the program file location
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
    {
        cpu.initialize();   //init the CPU or reset the cpu
        cpu.loadProgram(fileName.toStdString());    //load the given program file
        timerId = startTimer(1);    //start timer to call timer Event
    }
}

