#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setMaskControlVisible(false);
    setNoiseControlVisible(false);
    setTargetControlVisible(false);

    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(start()));

    update();

}

void MainWindow::update()
{
    if (ui->radioButtonNormal->isChecked())
    {
        currentMode = normal;
        setMaskControlVisible(false);
        setNoiseControlVisible(false);
        setTargetControlVisible(false);
    }
    else if (ui->radioButtonMask->isChecked())
    {
        currentMode = mask;
        setMaskControlVisible(true);
        setNoiseControlVisible(false);
        setTargetControlVisible(false);
        QPixmap color(ui->labelColorUp->size());
        color.fill(QColor::fromHsv(2 * ui->sliderHUp->value(), ui->sliderSUp->value(), ui->sliderVUp->value()));
        ui->labelColorUp->setPixmap(color);
        color.fill(QColor::fromHsv(2 * ui->sliderHDown->value(), ui->sliderSDown->value(), ui->sliderVDown->value()));
        ui->labelColorDown->setPixmap(color);
    }
    else if (ui->radioButtonNoise->isChecked())
    {
        currentMode = noise;
        setMaskControlVisible(false);
        setNoiseControlVisible(true);
        setTargetControlVisible(false);
    }
    else if (ui->radioButtonTarget->isChecked())
    {
        currentMode = target;
        setMaskControlVisible(false);
        setNoiseControlVisible(false);
        setTargetControlVisible(true);
    }
    else if (ui->radioButtonResult->isChecked())
    {
        currentMode = result;
        setMaskControlVisible(false);
        setNoiseControlVisible(false);
        setTargetControlVisible(false);
    }
}

void MainWindow::start()
{
    if (video.isOpened())
    {
        video.release();
        ui->pushButton->setText("Start");
        ui->labelView->setPixmap(QPixmap());
    }
    else
    {
        video.open(cv::CAP_DSHOW);
        video.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        video.set(cv::CAP_PROP_FRAME_HEIGHT, 360);
        ui->pushButton->setText("Stop");
        cv::Mat frame;
        while(video.isOpened())
        {
            video >> frame;
            if (!frame.empty()) draw(&frame);
            qApp->processEvents();
        }
    }
}

void MainWindow::draw(cv::Mat *frame)
{
    QPixmap res;
    cv::cvtColor(*frame, *frame, cv::COLOR_BGR2RGB);
    cv::flip(*frame, *frame, 1);
    cv::Mat maskMat = drawMask(frame,
                               ui->sliderHDown->value(),
                               ui->sliderSDown->value(),
                               ui->sliderVDown->value(),
                               ui->sliderHUp->value(),
                               ui->sliderSUp->value(),
                               ui->sliderVUp->value());
    cv::Mat noiseMaskMat = drawNoise(&maskMat,
                                     ui->sliderErodeKernel->value(),
                                     ui->sliderDilationKernel->value());
    cv::Mat gray;
    cv::Mat one = cv::Mat(frame->rows, frame->cols, CV_8UC3, cv::Scalar(255, 255, 255));
    cv::bitwise_and(one, one, gray, noiseMaskMat);
    cv::cvtColor(gray, gray, cv::COLOR_RGB2GRAY);
    cv::Mat circleMat = drawTarget(&gray, ui->sliderP2->value(), ui->sliderMinRadius->value());
    switch(currentMode) {
    case normal: {
        res = convert(frame);
    } break;
    case mask: {
        cv::Mat tmp;
        cv::bitwise_and(one, one, tmp, maskMat);
        res = convert(&tmp);
    } break;
    case noise: {
        cv::Mat tmp;
        cv::bitwise_and(one, one, tmp, noiseMaskMat);
        res = convert(&tmp);
    } break;
    case target: {
        res = convert(&circleMat);
    } break;
    case result: {
        cv::Mat tmp;
        cv::bitwise_and(*frame, circleMat, tmp);
        res = convert(&tmp);
    } break;
    default: break;
    }

    ui->labelView->setPixmap(res);
}

cv::Mat MainWindow::drawMask(cv::Mat *img, int hDown, int sDown, int vDown, int hUp, int sUp, int vUp)
{
    cv::Mat hsv;
    cv::cvtColor(*img, hsv, cv::COLOR_RGB2HSV);
    cv::Mat res;
    cv::Mat mask, mask2;
    if (hDown > hUp)
    {
        cv::inRange(hsv, cv::Scalar(hDown, sDown, vDown), cv::Scalar(179, sUp, vUp), mask);
        cv::inRange(hsv, cv::Scalar(0, sDown, vDown), cv::Scalar(hUp, sUp, vUp), mask2);
        mask = mask + mask2;
    }
    else
    {
        cv::inRange(hsv, cv::Scalar(hDown, sDown, vDown), cv::Scalar(hUp, sUp, vUp), mask);
    }
    return mask;
}

cv::Mat MainWindow::drawNoise(cv::Mat *img, int erodeKernel, int dilationKernel)
{
    cv::Mat kernelErode = cv::Mat::ones(erodeKernel, erodeKernel, CV_8U);
    cv::Mat kernelDilation = cv::Mat::ones(dilationKernel, dilationKernel, CV_8U);
    cv::erode(*img, *img, kernelErode);
    cv::dilate(*img, *img, kernelDilation);
    return *img;
}
cv::Mat MainWindow::drawTarget(cv::Mat *img, int p2, int minR)
{
    cv::Mat result = cv::Mat(img->rows, img->cols, CV_8UC3, cv::Scalar(255, 255, 255));
    std::vector<cv::Vec3f> circles;

    cv::HoughCircles(*img, circles, cv::HOUGH_GRADIENT, 1, img->cols, 100, p2, minR);
    for (size_t i = 0; i < circles.size(); ++i)
    {
        cv::Vec3i c = circles[i];
        cv::Point center = cv::Point(c[0], c[1]);
        cv::circle(result, center, c[2], cv::Scalar(255, 0, 0), 3);
        buffer.push_front(cv::Point(c[0], c[1]));
        if (buffer.size() > 90)
        {
            buffer.pop_back();
        }
    }
    for (int i = 0; i < buffer.size() - 1; i++) {
        cv::line(result, buffer[i], buffer[i + 1], cv::Scalar(255, 0, 0), 3);
    }
    return result;
}
cv::Mat MainWindow::drawResult(cv::Mat *img)
{
    return *img;
}

cv::Mat MainWindow::convert(QImage *arg)
{
    return cv::Mat(arg->height(), arg->width(), CV_8UC3, const_cast<uchar*>(arg->bits()), static_cast<size_t>(arg->bytesPerLine()));
}

QPixmap MainWindow::convert(cv::Mat* arg)
{
    return QPixmap::fromImage(QImage(arg->data, arg->cols, arg->rows, static_cast<int>(arg->step), QImage::Format_RGB888)).scaled(ui->labelView->size());
}

void MainWindow::setMaskControlVisible(bool value)
{
    ui->label->setVisible(value);
    ui->label_2->setVisible(value);
    ui->label_3->setVisible(value);
    ui->label_4->setVisible(value);
    ui->label_5->setVisible(value);
    ui->label_6->setVisible(value);

    ui->sliderHUp->setVisible(value);
    ui->sliderSUp->setVisible(value);
    ui->sliderVUp->setVisible(value);
    ui->sliderHDown->setVisible(value);
    ui->sliderSDown->setVisible(value);
    ui->sliderVDown->setVisible(value);

    ui->labelColorUp->setVisible(value);
    ui->labelColorDown->setVisible(value);
}

void MainWindow::setNoiseControlVisible(bool value)
{
    ui->label_7->setVisible(value);
    ui->label_8->setVisible(value);
    ui->label_9->setVisible(value);
    ui->label_10->setVisible(value);
    ui->sliderErodeKernel->setVisible(value);
    ui->sliderDilationKernel->setVisible(value);
}

void MainWindow::setTargetControlVisible(bool value)
{
    ui->label_9->setVisible(value);
    ui->label_10->setVisible(value);
    ui->sliderP2->setVisible(value);
    ui->sliderMinRadius->setVisible(value);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (video.isOpened())
    {
        event->ignore();
    }
    else
    {
        event->accept();
    }
}

MainWindow::~MainWindow()
{
    if (video.isOpened()) video.release();
    delete ui;
}

