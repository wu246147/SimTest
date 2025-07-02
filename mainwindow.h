#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QFontDatabase>

//图像算法库
#include <opencv2/opencv.hpp>
#include "imgprocesstool.h"
#include "calipertool.h"
#include "mylog.h"
#include "algorithm/qtstreambuf.h"
//交互库
#include "mygraphicrectitem.h"
#include "mygraphicsview.h"
#include "mygraphic_polygon_item.h"
#include "mywidgetview.h"
#include "mygraphicpointitem.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    /***
     * 算法函数
     ***/
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void runCaliper(bool isShow);

    void getCaliperToolPara(QString savePath);

    void showDetROI();

    void showCaliperToolROI();


    void setTTF();

    void Append(const QString &text);

    /***
     * 公有参数
     ***/
public:
    //文件夹路径
    std::string fileDir;
    //运行图片
    cv::Mat img;
    cv::Mat ROIImg;
    // ROI
    cv::RotatedRect modelROI = cv::RotatedRect(cv::Point2d(100, 100), cv::Size(100, 100), 0);


    /// 卡尺定位参数
    std::vector<cv::RotatedRect> caliperToolROIs = std::vector<cv::RotatedRect>(4, cv::RotatedRect(cv::Point2d(100, 100), cv::Size(100, 100), 0));
    std::vector<int>caliperPolarityList = std::vector<int>(4, 0);            //卡尺极性
    std::vector<int>caliperSelectPositionList = std::vector<int>(4, 0);      //边缘位置
    std::vector<uchar>caliperThreList = std::vector<uchar>(4, 5);              //边缘阈值

    //结果
    bool caliperResult;
    bool detRoiResult;

    double locationAngle;                                   //定位角度结果
    cv::Point2f locationPoint;                              //定位位置结果
    std::vector<bool> caliperResults;                       //卡尺检测结果
    std::vector<cv::Point2f> caliperLocationOriginPoints;   //卡尺定位结果
    cv::RotatedRect modelROITransform;                      //ROI映射结果

    //图像显示窗口
    myWidgetView *display = nullptr;

    MyGraphicRectItem *myRectItem_location = nullptr ;

    MyGraphicRectItem *myRectItem_caliperROI1 = nullptr ;
    MyGraphicRectItem *myRectItem_caliperROI2 = nullptr ;
    MyGraphicRectItem *myRectItem_caliperROI3 = nullptr ;
    MyGraphicRectItem *myRectItem_caliperROI4 = nullptr ;

    QGridLayout *display_grid_layout = nullptr;

    int tmpSrcImgId = -1;

    //鼠标拖动事件
    QPoint diff_pos;  // 鼠标和窗口的相对位移
    QPoint window_pos;
    QPoint mouse_pos;

    /*
     * 信号槽
     */
signals:
    ///
    /// \brief add_log_signal 添加日志信号
    /// \param message 添加日志
    ///
    void add_log_signal(QString message);

    ///
    /// \brief clean_log_signal 清空日志信号
    ///
    void clean_log_signal();

    /***
     * 事件
     ***/
private slots:
    void on_pushButton_readJsonFile_clicked();

    void on_pushButton_showROI_clicked();

    void on_pushButton_runNext_clicked();

    void on_pushButton_runAll_clicked();

    /***
     * 私有参数
     ***/
    void on_pushButton_showImage_clicked();

    void on_pushButton_min_clicked();

    void on_pushButton_max_clicked();

    void on_pushButton_close_clicked();

    //添加鼠标拖动事件
    void  mousePressEvent(QMouseEvent* event)  override;

    void  mouseMoveEvent(QMouseEvent* event) override;

    void add_log_even(QString message);

    void clean_log_even();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
