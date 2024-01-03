#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <chip8.h>
#include <QTimer>
#include <QKeyEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QVector<QRect> pixels;

protected:
    void paintEvent(QPaintEvent *event);
    void timerEvent(QTimerEvent *event);
    void keyPressEvent(QKeyEvent *key);

public slots:
    void browse();  //file browser function

private:
    Ui::MainWindow *ui;
    chip8 cpu;
    QTimer *mTimer;
    int timerId;
    uint8_t keyArray[100];
};
#endif // MAINWINDOW_H
