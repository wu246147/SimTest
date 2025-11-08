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
#include <QThread>
#include <QtConcurrent>
#include<QMetaObject>

#include "mylog.h"
#include "algorithm/qtstreambuf.h"


//交互库
#include "mygraphicrectitem.h"
#include "mygraphicsview.h"
#include "mygraphic_polygon_item.h"
#include "mywidgetview.h"
#include "mygraphicpointitem.h"
#include "mytextitem.h"

#include "jobmanager.h"

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


    void getCaliperToolPara(QString savePath);

    void showDetROI();

    void showCaliperToolROI();


    void setTTF();

    void Append(const QString &text);


    void showResult(bool isShowBox);

    void resetStatisticsData();

    void updataStatisticsData();

    void resetCamResult();

    void updataCamResult();

    /***
     * 公有参数
     ***/
public:
    //文件夹路径
    std::string fileDir;

    int cam_id = 0;

    Jobmanager jobmanager;

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
    std::vector<int> tmpSrcImgIdList = std::vector<int>(8, -1);

    //鼠标拖动事件
    QPoint diff_pos;  // 鼠标和窗口的相对位移
    QPoint window_pos;
    QPoint mouse_pos;

    bool is_loading = false;

    //显示图片缩小系数
    int showImgScaleSize = 1;

    QFutureWatcher<void> watcher;

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

    void runOver();

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

    void on_comboBox_currentIndexChanged(int index);

    void on_checkBox_showBox_clicked(bool checked);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
