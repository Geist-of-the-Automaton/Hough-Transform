#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qsa = new QScrollArea(this);
    QVBoxLayout *layout = new QVBoxLayout(qsa);
    qsa->setWidgetResizable(true);
    viewport = new QLabel(qsa);
    viewport->setLayout(layout);
    setCentralWidget(qsa);
    qmb = new QMenuBar(this);
    QMenu *menu = qmb->addMenu("File");
    QAction *action1 = menu->addAction("Load Image");
    connect(action1, &QAction::triggered, this, [=]() { this->loadImg(0); });
    QAction *action2 = menu->addAction("Set Mode");
    connect(action2, &QAction::triggered, this, [=]() { this->setMode(); });
    QAction *action3 = menu->addAction("Set View");
    connect(action3, &QAction::triggered, this, [=]() { this->setView(); });
    QAction *action4 = menu->addAction("Set Zoom");
    connect(action4, &QAction::triggered, this, [=]() { this->setZoom(); });
    QAction *action5 = menu->addAction("Set Blur");
    connect(action5, &QAction::triggered, this, [=]() { this->setBlur(); });
    qsa->setWidget(viewport);
    accX = accY = 0;
    colorMode = rgb;
    blurStr = three;
    index = 0;
    viewport->setPixmap(QPixmap::fromImage(process[0]));
    setWindowTitle("Auden Childress - Hough Transform (CSCE 590 Assignment 4)");
    ui->menubar->setCornerWidget(qmb, Qt::TopLeftCorner);
}

MainWindow::~MainWindow()
{
    hide();
    delete viewport;
    delete qsa;
    delete qmb;
    delete ui;
}

void MainWindow::setView() {
    if (process[0].isNull())
        return;
    QInputDialog resPrompt(this);
    QStringList items;
    for (string s : names)
        items.push_back(s.c_str());
    resPrompt.setOptions(QInputDialog::UseListViewForComboBoxItems);
    resPrompt.setComboBoxItems(items);
    resPrompt.setTextValue(items.at(index));
    resPrompt.setWindowTitle("Set View");
    resPrompt.exec();
    index = items.indexOf(resPrompt.textValue());
    viewport->setPixmap(QPixmap::fromImage(process[index].scaledToHeight(static_cast<int>(static_cast<double>(process[index].height()) * zoom))));
    viewport->repaint();
    statusBar()->showMessage(names[index].c_str(), 2000);

}

void MainWindow::loadImg(int loadFlag) {
    if (loadFlag == 0) {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Import"), "/", tr("Image Files (*.png *.jpg *.bmp)"));
        if (fileName == "")
            return;
        // load original
        process[0] = QImage(fileName);
    }
    QProgressDialog qpd(this, Qt::WindowType::FramelessWindowHint);
    qpd.setCancelButton(nullptr);
    qpd.setMaximum(6);
    qpd.setLabelText("Creating Greyscale");
    qpd.setValue(0);
    qpd.show();
    QCoreApplication::processEvents();
    // Process greyscale or particular channel
    process[1] = process[0];
    for (int j = 0; j < process[1].height(); ++j) {
        QRgb *line = reinterpret_cast<QRgb *>(process[1].scanLine(j));
        for (int i = 0; i < process[1].width(); ++i)
            line[i] = greyscale(line[i]).rgba();
    }
    qpd.setValue(1);
    qpd.setLabelText("Applying Blur");
    QCoreApplication::processEvents();
    // Process blur via guassian
    process[2] = process[1];
    if (blurStr != none) {
        int offset = blurStr == three ? 1 : 2;
        for (int i = 0; i < process[2].width(); ++i)
            for (int j = 0; j < process[2].height(); ++j) {
                double value = 0;
                int X = i - offset, Y = j - offset;
                for (int x = X; x <= i + offset; ++x)
                    for (int y = Y; y <= j + offset; ++y) {
                        if (blurStr == three)
                            value += guassian3[x - X][y - Y] * static_cast<double>((x < 0 || y < 0 || x >= process[2].width() || y >= process[2].height()) ? process[1].pixelColor(i, j).red() : process[1].pixelColor(x, y).red());
                        else
                            value += guassian3[x - X][y - Y] * static_cast<double>((x < 0 || y < 0 || x >= process[2].width() || y >= process[2].height()) ? process[1].pixelColor(i, j).red() : process[1].pixelColor(x, y).red());
                    }
                int color = max(0, min(255, static_cast<int>(value + 0.5)));
                process[2].setPixelColor(i, j, QColor(color, color, color));
            }
    }
    qpd.setValue(2);
    qpd.setLabelText("Processing Edge Detection");
    QCoreApplication::processEvents();
    // Process edge detection via sobel
    process[3] = process[2];
    long threshold = 0;
    int maxVal = 0, minVal = 255;
    int zeroCnt = 0;
    for (int i = 0; i < process[3].width(); ++i)
        for (int j = 0; j < process[3].height(); ++j) {
            int color = 0;
            int X = i - 1, Y = j - 1;
            for (int x = X; x <= i + 1; ++x)
                for (int y = Y; y <= j + 1; ++y) {
                    int val = (x < 0 || y < 0 || x >= process[3].width() || y >= process[3].height()) ? process[2].pixelColor(i, j).red() : process[2].pixelColor(x, y).red();
                    color += edge[x - X][y - Y] * val;
                }
            color = max(0, min(255, color));
            process[3].setPixelColor(i, j, QColor(color, color, color));
            threshold += color;
            minVal = min(minVal, color);
            maxVal = max(maxVal, color);
            if (color == 0 || color == 255)
                ++zeroCnt;
        }
    qpd.setValue(3);
    qpd.setLabelText("Thresholding");
    QCoreApplication::processEvents();
    // Threshold image
    process[4] = process[3];
    threshold /= (process[4].width() * process[4].height() - zeroCnt);
    threshold = (threshold + (maxVal + minVal) / 2) / 2;
    for (int j = 0; j < process[4].height(); ++j)
        for (int i = 0; i < process[4].width(); ++i) {
            QColor qc = process[4].pixelColor(i, j);
            int val = qc.red() <= threshold ? 0 : 255;
            process[4].setPixelColor(i, j, QColor(val, val, val));
        }
    qpd.setValue(4);
    qpd.setLabelText("Creating Accumulator");
    QCoreApplication::processEvents();
    // Hough transform
    double sub = sqrt(2.0) * (static_cast<double>(process[0].height() > process[0].width() ? process[0].height() : process[0].width()));
    accX = static_cast<int>(0.5 + sqrt(2.0) * (static_cast<double>(process[0].height() > process[0].width() ? process[0].height() : process[0].width())));
    sub /= 2.0;
    accY = 180;
    int **accumalator = new int*[accY];
    for (int j = 0; j < accY; ++j) {
        accumalator[j] = new int[accX];
        for (int i = 0; i < accX; ++i)
            accumalator[j][i] = 0;
    }
    double cx = static_cast<double>(process[0].width()) / 2.0, cy = static_cast<double>(process[0].height()) / 2.0;
    maxVal = 0;
    for (int j = 0; j < process[0].height(); ++j) {
        double y = static_cast<double>(j) - cy;
        for (int i = 0; i < process[0].width(); ++i) {
            double x = static_cast<double>(i) - cx;
            if (process[4].pixelColor(i, j).red() == 255)
                for (double theta = 0.0; theta < 180.0; theta += 1.0) {
                    double rad = theta * deg2rad;
                    int offset = static_cast<int>(x * cos(rad) + y * sin(rad) + sub);
                    ++accumalator[static_cast<int>(theta)][offset];
                    maxVal = max(maxVal, accumalator[static_cast<int>(theta)][offset]);
                }
        }
    }
    process[5] = QImage(accX, accY, QImage::Format_ARGB32_Premultiplied);
    process[5].fill(0xFF000000);
    double mult = 255 / static_cast<double>(maxVal + 1);
    for (int i = 0; i < accX; ++i)
        for (int j = 0; j < accY; ++j) {
            int color = static_cast<int>(mult * static_cast<double>(accumalator[j][i]));
            process[5].setPixelColor(i, j, QColor(color, color, color));
        }
    qpd.setValue(5);
    qpd.setLabelText("Calculating and Drawing Lines");
    QCoreApplication::processEvents();
    // Weird aliasing at 45deg and 135deg
    // Get lines
    process[6] = process[1];
    QPainter qp(&process[6]);
    qp.setPen(QPen(QBrush(0xFFFF0000), max(1.0, static_cast<double>(process[0].width()) / 600.0)));
    double acx = static_cast<double>(accX) / 2.0, acy = static_cast<double>(accX) / 2.0;
    threshold = max(threshold / mult, threshold * mult);
    for (int j = 0; j < accY; ++j) {
        int ystart = max(j - 4, 0), yend = min(j + 4, accY - 1);
        for (int i = 0; i < accX; ++i)
            if (accumalator[j][i] > threshold) {
                bool flag = true;
                int xstart = max(i - 4, 0), xend = min(i + 4, accX - 1);
                maxVal = accumalator[j][i];
                for (int y = ystart; y <= yend; ++y)
                    for (int x = xstart; x <= xend; ++x)
                        if (accumalator[y][x] >= maxVal && !(x == i && y == j)) {
                            y = yend;
                            flag = false;
                            break;
                        }
                if (flag) {
                    double val = static_cast<double>(j) * deg2rad;
                    double Cos = cos(val), Sin = sin(val);
                    val = static_cast<double>(i) - acx;
                    if (j < 45 || j > 135) {
                        int x1 = (val - (-cy * Sin)) / Cos + cx;
                        int x2 = (val - (cy * Sin)) / Cos + cx;
                        qp.drawLine(x1, 0, x2, 2 * cy);
                    }
                    else {
                        int y1 = (val - (-cx * Cos)) / Sin + cy;
                        int y2 = (val - (cx * Cos)) / Sin + cy;
                        qp.drawLine(0, y1, 2 * cx, y2);
                    }
                }
            }
    }
    qp.end();
    for (int j = 0; j < accY; ++j)
        delete [] accumalator[j];
    delete [] accumalator;
    // draw lines
    if (loadFlag == 0) {
        index = 0;
        zoom = 1.0;
    }
    qpd.setValue(6);
    qpd.setLabelText("Finishing");
    QCoreApplication::processEvents();
    viewport->setPixmap(QPixmap::fromImage(process[index].scaledToHeight(static_cast<int>(static_cast<double>(process[index].height()) * zoom))));
    viewport->repaint();
}

void MainWindow::setMode() {
    if (process[0].isNull())
        return;
    QInputDialog resPrompt(this);
    QStringList items;
    items.push_back("RGB");
    items.push_back("Red Channel");
    items.push_back("Green Channel");
    items.push_back("Blue Channel");
    resPrompt.setOptions(QInputDialog::UseListViewForComboBoxItems);
    resPrompt.setComboBoxItems(items);
    resPrompt.setTextValue(items.at(static_cast<int>(mode(colorMode))));
    resPrompt.setWindowTitle("Set Mode");
    resPrompt.exec();
    mode temp = static_cast<mode>(items.indexOf(resPrompt.textValue()));
    if (temp != colorMode) {
        colorMode = temp;
        loadImg(1);
    }
}

void MainWindow::setZoom() {
    if (process[0].isNull())
        return;
    bool ok = false;
    double ret = QInputDialog::getDouble(this, "Set Zoom", "Set zoom from 0.2x to 3.0x", zoom, 0.2, 3.0, 1, &ok);
    if (ok)
        zoom = ret;
    viewport->setPixmap(QPixmap::fromImage(process[index].scaledToHeight(static_cast<int>(static_cast<double>(process[index].height()) * zoom))));
    viewport->repaint();
}

QColor MainWindow::greyscale(QColor qc) {
    int n;
    QColor ret;
    switch (colorMode) {
    case rgb:
        n = (qc.red() + qc.green() + qc.blue()) / 3;
        ret = QColor(n, n, n);
        break;
    case red:
        ret = QColor(qc.red(), qc.red(), qc.red());
        break;
    case green:
        ret = QColor(qc.green(), qc.green(), qc.green());
        break;
    case blue:
        ret = QColor(qc.blue(), qc.blue(), qc.blue());
        break;
    }
    return ret;
}

void MainWindow::setBlur() {
    if (process[0].isNull())
        return;
    QInputDialog resPrompt(this);
    QStringList items;
    items.push_back("None");
    items.push_back("Guassian 3");
    items.push_back("Guassian 5");
    resPrompt.setOptions(QInputDialog::UseListViewForComboBoxItems);
    resPrompt.setComboBoxItems(items);
    resPrompt.setTextValue(items.at(static_cast<int>(blur(blurStr))));
    resPrompt.setWindowTitle("Set Mode");
    resPrompt.exec();
    blur temp = static_cast<blur>(items.indexOf(resPrompt.textValue()));
    if (temp != blurStr) {
        blurStr = temp;
        loadImg(1);
    }
}

QColor MainWindow::getProper(QImage qi, QPoint loc, QPoint org) {
    if (loc.x() < 0 || loc.y() < 0 || loc.x() >= qi.width() || loc.y() >= qi.height())
        return qi.pixelColor(org);
    return qi.pixelColor(loc);
}
