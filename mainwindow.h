#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QFontDatabase>

#include <QException>

//图像算法库
#include <opencv2/opencv.hpp>
#include "imgprocesstool.h"
#include "calipertool.h"
#include "mylog.h"
#include "algorithm/qtstreambuf.h"

#include "projectimagetool.h"

#include "yolo11_det.h"
#include "yolov5_det.h"
//异常检测
#include <rvs2d/dl/ad/anomaly_det_multigpu.hpp>
#include <rvs2d/trt/build_model.h>
//交互库
#include "mygraphicrectitem.h"
#include "mygraphicsview.h"
#include "mygraphic_polygon_item.h"
#include "mywidgetview.h"
#include "mygraphicpointitem.h"
#include "mytextitem.h"

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

    ///
    /// \brief runDefectDet
    ///
    void runDefectDet(bool isShow = false);


    ///
    /// \brief runAnormalDefectDet
    ///
    void runAbnormalDefectDet(bool isShow = false);

    void getCaliperToolPara(QString savePath);

    void showDetROI();

    void showCaliperToolROI();


    void setTTF();

    void Append(const QString &text);

    void saveDetToolPara(QString savePath);

    void loadDetToolPara(QString strFile);

    void initSupervisedModel(std::string supervisedModelPath);
    void initUnsupervisedModel(std::string unsupervisedModelPath);

    void initDefectDet(std::string supervisedModelPath, std::string unsupervisedModelPath);

    bool readTClassName();

    bool result();

    void getCurrentDefectInfo(std::string save_data_path, std::string save_data_path_sql, bool is_save_defect_img, std::vector<float> &defect_scores,
                              std::vector<std::string> &defect_img_paths, std::vector<std::string> &defect_types,
                              std::vector<std::string> &defect_locations, std::vector<int> &defect_areas);

    void show_result();

    /***
     * 公有参数
     ***/
public:
    //文件夹路径
    std::string fileDir;
    //运行图片
    cv::Mat img;
    cv::Mat ROIImg;

    // 检测框角度，用于矫正坐标系方向
    double modelROIAngle = 0;
    // ROI
    std::vector<cv::Point2f> modelROIPoints =
    {
        {200, 200},
        {600, 200},
        {600, 600},
        {200, 600}
    };

    std::vector<cv::Point2f> modelROIPointsTransform =
    {
        {200, 200},
        {600, 200},
        {600, 600},
        {200, 600}
    };   //ROI点映射结果


    /// 深度学习参数
    bool isUseDefectDet = false;
    //切割图片大小
    int cutSize = 640;
    //重叠图片大小
    int overlappingSize = 64;
    //结果阈值
    double thre = 0.5;

    // mask阈值
    int maskThre = 50;

    int filter_defect_area = 10;

    // 手动mask区域,目标检测
    std::vector<std::vector<cv::Point2f >> polygonDetMasks;
    std::vector<std::vector<cv::Point2f >> polygonMaskDetTransforms;


    /// 异常检测
    // 是否启用异常检测
    bool isUseAnomalDet = false;
    // 切割图片大小
    int abnormalCutSize = 320;
    // 重叠图片大小
    int abnormalOverlappingSize = 48;
    //结果阈值
    double abnormalThre = 0.9;

    int filter_defect_area_abnormal = 10;
    int side_filter_size = 1;


    // 手动mask区域
    std::vector<std::vector<cv::Point2f >> polygonMasks;

    std::vector<std::vector<cv::Point2f >> polygonMaskTransforms;

    /// 检测模型
    // RaivasDefectDet11::RaivasDefectDet11 defectDet;
    RaivasDefectDet5::RaivasDefectDet5 defectDet;

    std::shared_ptr<rvs2d::dl::ad::MultiGPUInfer> ad_model = nullptr;


    // 结果类，现在先写死，后面加入文件读取
    std::map<int, std::string> TclassNames
    {
        {0, "橘皮"},
        {1, "毛丝"},
        {2, "脱漆"},
        {3, "烤漆不到位"},
        {4, "粉点"},
        {5, "变形"},
        {6, "白点"},
        {7, "磨损刮伤"}
    };

    //目标检测模型类型
    int modelType = 5;
    std::string modelSize = "m";


    //结果参数
    bool defectResult;
    std::vector<int>selectDefectResultID ;
    int resultLen;

    /// 异常检测结果
    bool abnormalDetResult;

    std::vector<float> abnormalDetScores;
    std::vector<float> abnormalDetXs ;
    std::vector<float> abnormalDetYs ;
    std::vector<float> abnormalDetWs ;
    std::vector<float> abnormalDetHs ;

    std::vector<int> abnormalDetAreas ;

    //图像显示窗口
    myWidgetView *display = nullptr;

    MyGraphicRectItem *myRectItem_location = nullptr ;

    MyGraphicRectItem *myRectItem_caliperROI1 = nullptr ;
    MyGraphicRectItem *myRectItem_caliperROI2 = nullptr ;
    MyGraphicRectItem *myRectItem_caliperROI3 = nullptr ;
    MyGraphicRectItem *myRectItem_caliperROI4 = nullptr ;

    MyGraphicPolygonItem *myDefectDetROI = nullptr;

    QGridLayout *display_grid_layout = nullptr;

    int tmpSrcImgId = -1;

    //鼠标拖动事件
    QPoint diff_pos;  // 鼠标和窗口的相对位移
    QPoint window_pos;
    QPoint mouse_pos;

    bool is_loading = false;

    //显示图片缩小系数
    int showImgScaleSize = 1;



    /*
     * 信号槽
     */
    void updata_control_value();

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

    void on_pushButton_initModel_clicked();

    //添加鼠标拖动事件
    void  mousePressEvent(QMouseEvent* event)  override;

    void  mouseMoveEvent(QMouseEvent* event) override;

    void add_log_even(QString message);

    void clean_log_even();

    void on_spinBox_maskThre_valueChanged(int arg1);

    void on_spinBox_cutSize_valueChanged(int arg1);

    void on_spinBox_overlappingSize_valueChanged(int arg1);

    void on_doubleSpinBox_thre_valueChanged(double arg1);

    void on_spinBox_filter_defeat_area_valueChanged(int arg1);

    void on_spinBox_AbnormalCutSize_valueChanged(int arg1);

    void on_spinBox_AbnormalOverlappingSize_valueChanged(int arg1);

    void on_doubleSpinBox_AbnormalThre_valueChanged(double arg1);

    void on_spinBox_filter_defeat_area_abnormal_valueChanged(int arg1);

    void on_spinBox_side_filter_size_valueChanged(int arg1);

    void on_pushButton_savePara_clicked();

    void on_checkBox_isDefectDet_clicked(bool checked);

    void on_checkBox_isUseAbnormal_clicked(bool checked);

    void on_pushButton_run_clicked();

    void on_pushButton_reset_clicked();

    void on_pushButton_runLast_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
