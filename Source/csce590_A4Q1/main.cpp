#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QStringList>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/opencv.hpp>

using Qt::Key_Left;
using Qt::Key_Right;
using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::to_string;
using std::ofstream;
using std::pair;
using cv::Point2f;
using cv::Point3f;
using cv::imread;
using cv::Mat;
using cv::cvtColor;
using cv::COLOR_BGR2GRAY;
using cv::findChessboardCorners;
using cv::Size;
using cv::CALIB_CB_ADAPTIVE_THRESH;
using cv::CALIB_CB_FAST_CHECK;
using cv::CALIB_CB_NORMALIZE_IMAGE;
using cv::cornerSubPix;
using cv::TermCriteria;
using cv::drawChessboardCorners;
using cv::COLOR_BGR2RGB;
using cv::calibrateCamera;
using cv::imshow;
using cv::imwrite;

const Size boardSize(7, 7);
vector <vector <Point2f> > boardCorners2d;
vector <vector <Point3f> > boardCorners3d;
vector <pair <int, Mat> > images;

int main(int argc, char *argv[]) {
    // load images and make greyscale copies for calibration
    string path = QDir::currentPath().toStdString() + "/boardImages/";
    QDir dir(path.c_str());
    if (!dir.exists())
        QCoreApplication::exit(1);
    QStringList fileNames = dir.entryList({"*.jpg", "*.png", "*.bmp"});
    if (fileNames.empty())
        QCoreApplication::exit(2);
    vector <Mat> imgs;
    for (QString fileName : fileNames) {
        imgs.push_back(imread(path + fileName.toStdString()));
        cout << "Reading in image " << fileName.toStdString() << endl;
    }
    int flags = CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_FAST_CHECK | CALIB_CB_NORMALIZE_IMAGE;
    TermCriteria crit(TermCriteria::EPS | TermCriteria::COUNT, 30, 0.1);
    string of(" of " + to_string(imgs.size()));
    for (size_t i = 0; i < imgs.size(); ++i) {
        vector <Point2f> corners;
        Mat greyCopy;
        cvtColor(imgs[i], greyCopy, COLOR_BGR2GRAY);
        cout << "Processing image " << (i + 1) << of << endl;
        bool ok = findChessboardCorners(greyCopy, boardSize, corners, flags);
        if (ok) {
            cornerSubPix(greyCopy, corners, Size(11,11), Size(-1,-1), crit);
            drawChessboardCorners(imgs[i], boardSize, corners, ok);
            boardCorners2d.push_back(corners);
            images.push_back(pair <int, Mat> (i + 1, imgs[i]));
        }
    }
    if (images.empty())
        QCoreApplication::exit(3);
    // from https://docs.opencv.org/master/d9/d0c/group__calib3d.html
    // as cameraMatrix, distCoeffs, rvecs, and tvecs
    Mat retMatrices[4];
    vector <Point3f> worldView;
    for (int i = 0; i < boardSize.width; ++i)
        for (int j = 0; j < boardSize.height; ++j)
            worldView.push_back(Point3f(j, i, 0.0));
    for (size_t i = 0; i < images.size(); ++i)
        boardCorners3d.push_back(worldView);
    calibrateCamera(boardCorners3d, boardCorners2d, Size(imgs[0].rows, imgs[0].cols), retMatrices[0], retMatrices[1], retMatrices[2], retMatrices[3]);
    ofstream fs;
    path += "processed/";
    dir.setPath(path.c_str());
    if (!dir.exists())
        dir.mkdir(path.c_str());
    fs.open((path + "log.txt").c_str());
    if (fs.is_open())
        fs << endl << "Print matrices: Camera Matrix, Dist Coefficients, Rotation Vectors, Translation Vectors" << endl << endl;
    cout << endl << "Print matrices: Camera Matrix, Dist Coefficients, Rotation Vectors, Translation Vectors" << endl << endl;
    for (Mat m : retMatrices) {
        if (fs.is_open())
            fs << m << endl << endl;
        cout << m << endl << endl;
    }
    if (fs.is_open())
        fs.close();
    for (size_t i = 0; i < images.size(); ++i)
        imwrite((path + to_string(images[i].first) + "_processed.png").c_str(), images[i].second);
}
