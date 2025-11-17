#ifndef JOBWORKER_H
#define JOBWORKER_H
#

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>

//图像算法库
#include "algorithm/tool.h"
#include <opencv2/opencv.hpp>
#include "imgprocesstool.h"
#include "calipertool.h"
#include "projectimagetool.h"

#include "yolo11_det.h"
#include "yolov5_det.h"
//异常检测
#include <rvs2d/dl/ad/anomaly_det_multigpu.hpp>
#include <rvs2d/trt/build_model.h>


#pragma execution_character_set("utf-8")


class Jobworker
{

    /***
     * 算法函数
     ***/
public:
    Jobworker();
    Jobworker(const Jobworker &jobworker);

    void saveDetToolPara(std::string savePath);

    void loadDetToolPara(std::string strFile);

    void initSupervisedModel(std::string supervisedModelPath);
    void initUnsupervisedModel(std::string unsupervisedModelPath);

    void initDefectDet(std::string supervisedModelPath, std::string unsupervisedModelPath);

    bool readTClassName();

    bool readJsonstd(std::string strFile);

    bool result();

    void getCurrentDefectInfo(std::string save_data_path, std::string save_data_path_sql, bool is_save_defect_img, std::vector<float> &defect_scores,
                              std::vector<std::string> &defect_img_paths, std::vector<std::string> &defect_types,
                              std::vector<std::string> &defect_locations, std::vector<int> &defect_areas);

    void run(bool isShow = false);

    ///
    /// \brief runDefectDet
    ///
    void runDefectDet(bool isShow = false);


    ///
    /// \brief runAnormalDefectDet
    ///
    void runAbnormalDefectDet(bool isShow = false);

    float MissRate();

    float FPR();

    int Total();

    void statistics();

    int getStatistics();

    void resetStatistics();

    bool isUsed();

    /***
     * 公有参数
     ***/
public:

    /// 保存参数

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
    int side_filter_size2 = 1;


    // 手动mask区域
    std::vector<std::vector<cv::Point2f >> polygonMasks;



    ///临时参数

    //运行图片
    cv::Mat img;
    cv::Mat ROIImg;

    std::vector<cv::Point2f> modelROIPointsTransform =
    {
        {200, 200},
        {600, 200},
        {600, 600},
        {200, 600}
    };   //ROI点映射结果

    std::vector<std::vector<cv::Point2f >> polygonMaskDetTransforms;

    std::vector<std::vector<cv::Point2f >> polygonMaskTransforms;


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

    //统计结果
    int tp = 0;
    int tn = 0;
    int fp = 0;
    int fn = 0;

    //实际结果
    std::vector<std::vector<cv::Point2f >> labelList;
    bool labelResult = true;



};

#endif // JOBWORKER_H
