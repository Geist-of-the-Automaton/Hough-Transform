#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPaintEvent>
#include <QScrollArea>
#include <QLabel>
#include <QPixmap>
#include <QFileDialog>
#include <QMenuBar>
#include <QImage>
#include <QBoxLayout>
#include <QInputDialog>
#include <QPainter>
#include <QProgressDialog>
#include <iostream>
#include <string>

using Qt::Key_Left;
using Qt::Key_Right;
using std::cout;
using std::endl;
using std::string;
using std::min;
using std::max;

enum mode {rgb, red, green, blue};
enum blur {none, three, five};

const string names[7] = {"Orginal", "Greyscale", "Blurred", "Edge detected", "Thresholded", "Accumulator", "Greyscale with lines"};

const QSize qs(800,800);

const double deg2rad = 3.14159265359 / 180.0;

const int edge[3][3] = {{-1, -1, -1},
                        {-1,  8, -1},
                        {-1, -1, -1}};

const double guassian3[3][3] = {{1.0/16.0, 1.0/8.0, 1.0/16.0},
                                {1.0/8.0 , 1.0/4.0, 1.0/8.0 },
                                {1.0/16.0, 1.0/8.0, 1.0/16.0}};

const double guassian5[5][5] = {{2.0/159.0,  4.0/159.0,  5.0/159.0,  4.0/159.0, 2.0/159.0},
                               { 4.0/159.0,  9.0/159.0, 12.0/159.0,  9.0/159.0, 4.0/159.0},
                               { 5.0/159.0, 12.0/159.0, 15.0/159.0, 12.0/159.0, 5.0/159.0},
                               { 4.0/159.0,  9.0/159.0, 12.0/159.0,  9.0/159.0, 4.0/159.0},
                               { 2.0/159.0,  4.0/159.0,  5.0/159.0,  4.0/159.0, 2.0/159.0}};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void loadImg(int loadFlag);
    void setMode();
    void setView();
    void setBlur();
    void setZoom();

private:
    QColor getProper(QImage qi, QPoint loc, QPoint org);
    QColor greyscale(QColor qc);

    Ui::MainWindow *ui;
    QImage process[7];  // original, greyscale, blurred, detected, thresholded, accumalator, original with lines
    int accX, accY;
    QScrollArea *qsa;
    QLabel *viewport;
    QMenuBar *qmb;
    mode colorMode;
    int index;
    double zoom;
    blur blurStr;

};
#endif // MAINWINDOW_H
