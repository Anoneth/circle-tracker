#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum MODE {
        normal = 1,
        mask,
        noise,
        target,
        result
    };


private slots:
    void update();
    void start();

private:
    Ui::MainWindow *ui;

    void draw(cv::Mat* frame);

    cv::Mat drawMask(cv::Mat* img, int hDown, int sDown, int vDown, int hUp, int sUp, int vUp);
    cv::Mat drawNoise(cv::Mat* img, int erodeKernel, int dilationKernel);
    cv::Mat drawTarget(cv::Mat* img, int p2, int minR);
    cv::Mat drawResult(cv::Mat* img);

    QPixmap convert(cv::Mat* arg);
    cv::Mat convert(QImage* arg);

    void setMaskControlVisible(bool value);
    void setNoiseControlVisible(bool value);
    void setTargetControlVisible(bool value);

    void closeEvent(QCloseEvent *event);

    MODE currentMode;

    QList<cv::Point> buffer;

    cv::VideoCapture video;

};
#endif // MAINWINDOW_H
