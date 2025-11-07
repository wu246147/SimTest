#include "jobworker.h"
#include "mylog.h"


Jobworker::Jobworker() {}

Jobworker::Jobworker(const Jobworker &jobworker)
{
    // 检测框角度，用于矫正坐标系方向
    modelROIAngle = jobworker.modelROIAngle;
    // ROI
    modelROIPoints = jobworker.modelROIPoints;


    /// 深度学习参数
    isUseDefectDet = jobworker.isUseDefectDet;
    cutSize = jobworker.cutSize;
    overlappingSize = jobworker.overlappingSize;
    thre = jobworker.thre;
    maskThre = jobworker.maskThre;
    filter_defect_area = jobworker.filter_defect_area;
    polygonDetMasks = jobworker.polygonDetMasks;

    /// 异常检测参数
    isUseAnomalDet = jobworker.isUseAnomalDet;
    abnormalCutSize = jobworker.abnormalCutSize;
    abnormalOverlappingSize = jobworker.abnormalOverlappingSize;
    abnormalThre = jobworker.abnormalThre;
    filter_defect_area_abnormal = jobworker.filter_defect_area_abnormal;
    side_filter_size = jobworker.side_filter_size;
    polygonMasks = jobworker.polygonMasks;


}

void Jobworker::runDefectDet(bool isshow)
{
    try
    {
        ROIImg = img;
        cv::Mat showImage;
        if(isshow)
        {
            showImage = ROIImg.clone();
        }
        defectResult = true;
        resultLen = 0;
        selectDefectResultID.clear();


        if(ROIImg.empty())
        {
            defectResult = false;
            return;
        }

        auto start = std::chrono::high_resolution_clock::now();

        //保证用显卡1
        cudaSetDevice(0);
        //运行

        defectDet.runModel(ROIImg, resultLen);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        // std::cout << "Defect total:" << duration << "ms" << std::endl;
        // std::cout << "-----------------------------------" << std::endl;
        LOGE("Defect total: %d ms", duration);
        LOGE("-----------------------------------");


        // 获取屏蔽区域
        //后处理
        //ROIImg 的mask计算
        cv::Mat ROIImgMask;
        std::vector<std::vector<cv::Point >> polygonMaskTransforms2;
        if(true)
        {
            // cv::threshold(ROIImg,ROIImgMask,maskThre,255,cv::THRESH_BINARY);
            imageScaleMask(ROIImg, ROIImgMask, maskThre);
        }
        else
        {
            ROIImgMask = cv::Mat::ones(ROIImg.size(), CV_8UC1) * 255;
        }

        // for(int id = 0; id < polygonDetMasks.size(); ++id)
        // {
        //     for(int id2 = 0; id2 < polygonDetMasks[id].size(); ++id2)
        //     {
        //         LOGE("polygonDetMasks[%d][%d]:(%f,%f)", id, id2, polygonDetMasks[id][id2].x, polygonDetMasks[id][id2].y);
        //     }
        // }

        // for(int id2 = 0; id2 < modelROIPoints.size(); ++id2)
        // {
        //     LOGE("modelROIPoints[%d]:(%f,%f)",  id2, modelROIPoints[id2].x, modelROIPoints[id2].y);
        // }

        //这里因为卡尺定位可能造成xy方向发生变化，这里需要对坐标系方向进行矫正
        //主要是针对检测框和屏蔽框
        cv::Mat H;
        ImgAlg::getTransform(0, 0, modelROIAngle, cv::Point2f(0, 0), H);

        cv::Mat H33 = cv::Mat::eye(3, 3, CV_64F);
        H33.at<double>(0, 0) = H.at<double>(0, 0);
        H33.at<double>(0, 1) = H.at<double>(0, 1);
        H33.at<double>(0, 2) = H.at<double>(0, 2);
        H33.at<double>(1, 0) = H.at<double>(1, 0);
        H33.at<double>(1, 1) = H.at<double>(1, 1);
        H33.at<double>(1, 2) = H.at<double>(1, 2);

        getModelROIPointTransform(H33, modelROIAngle, modelROIPoints, modelROIPointsTransform);

        polygonMaskDetTransforms.clear();
        for(int id = 0; id < polygonDetMasks.size(); ++id)
        {
            std::vector<cv::Point2f >polygonMaskDetTransform = std::vector<cv::Point2f >(polygonDetMasks[id].size(), cv::Point2f(0, 0));
            getModelROIPointTransform(H33, modelROIAngle, polygonDetMasks[id], polygonMaskDetTransform);
            polygonMaskDetTransforms.push_back(polygonMaskDetTransform);
        }

        // for(int id = 0; id < polygonMaskDetTransforms.size(); ++id)
        // {
        //     for(int id2 = 0; id2 < polygonMaskDetTransforms[id].size(); ++id2)
        //     {
        //         LOGE("polygonMaskDetTransforms[%d][%d]:(%f,%f)", id, id2, polygonMaskDetTransforms[id][id2].x, polygonMaskDetTransforms[id][id2].y);
        //     }
        // }

        // for(int id2 = 0; id2 < modelROIPointsTransform.size(); ++id2)
        // {
        //     LOGE("modelROIPointsTransform[%d]:(%f,%f)",  id2, modelROIPointsTransform[id2].x, modelROIPointsTransform[id2].y);
        // }

        //仿射变换
        getMaskROI(polygonMaskDetTransforms, modelROIPointsTransform, polygonMaskTransforms2);

        if(isshow)
        {

            for(int id = 0; id < polygonMaskTransforms2.size(); ++id)
            {

                cv::fillPoly(showImage, polygonMaskTransforms2[id], cv::Scalar(0, 0, 255));

                // for(int id2 = 0; id2 < polygonMaskTransforms2[id].size(); ++id2)
                // {
                //     LOGE("polygonMaskTransforms2[%d][%d]:(%d,%d)", id, id2, polygonMaskTransforms2[id][id2].x, polygonMaskTransforms2[id][id2].y);
                // }

            }
            cv::namedWindow("showImage", cv::WINDOW_NORMAL);
            cv::imshow("showImage", showImage);
            cv::waitKey(0);
        }
        for(int id = 0; id < polygonMaskTransforms2.size(); ++id)
        {

            cv::fillPoly(ROIImgMask, polygonMaskTransforms2[id], cv::Scalar(0));

        }
        //阈值过滤
        for(int id = 0; id < resultLen; ++id)
        {
            int locateOCRClassId ;
            float locateOCRScore;
            float locateOCRX ;
            float locateOCRY ;
            float locateOCRW ;
            float locateOCRH ;
            int line_wide = 5;
            // std::cout << "getOCRAndDepthResult:" << id << std::endl;

            defectDet.getDefectDetResult(id, &locateOCRClassId, &locateOCRScore, &locateOCRX, &locateOCRY,
                                         &locateOCRW, &locateOCRH);


            //阈值过滤
            if(locateOCRScore > thre)
            {

                locateOCRX = std::max(0, int(locateOCRX));
                locateOCRY = std::max(0, int(locateOCRY));
                double locateOCRX2 = std::min(int(locateOCRX + locateOCRW), ROIImgMask.size().width);
                double locateOCRY2 = std::min(int(locateOCRY + locateOCRH), ROIImgMask.size().height);

                //阈值过滤
                cv::Rect roi(locateOCRX, locateOCRY, locateOCRX2 - locateOCRX, locateOCRY2 - locateOCRY);

                cv::Mat ROIImgMaskPart = ROIImgMask(roi);
                cv::Mat ROIImgMaskPartInv ;
                cv::bitwise_not(ROIImgMaskPart, ROIImgMaskPartInv);

                //判断是否存在屏蔽区域,并且面积大于过滤面积
                if(cv::countNonZero(ROIImgMaskPartInv) == 0 && roi.area() > filter_defect_area)

                {
                    selectDefectResultID.push_back(id);
                }

            }
            if(isshow)
            {
                cv::rectangle(showImage, cv::Rect(locateOCRX, locateOCRY, locateOCRW, locateOCRH), cv::Scalar(0, 0, 255), 2);
            }

        }

        if(isshow)
        {
            cv::namedWindow("showImg", cv::WINDOW_NORMAL);
            cv::imshow("showImg", showImage);
            cv::waitKey(0);

            // cv::imwrite("DefectDetResult.jpg", showImage);

        }

        if(selectDefectResultID.size() > 0)
        {
            defectResult = false;
        }

    }
    catch(cv::Exception &e)
    {
        defectResult = false;
        LOGE("error:%s", e.what());

    }
    catch(std::exception &e)
    {
        defectResult = false;
        LOGE("error:%s", e.what());
    }
    catch(QException &e)
    {
        defectResult = false;
        LOGE("error:%s", e.what());

    }
    catch(...)
    {
        defectResult = false;
        LOGE("other error.");
    }
}

void Jobworker::runAbnormalDefectDet(bool isShow)
{
    try
    {

        ROIImg = img;

        abnormalDetResult = true;
        abnormalDetScores.clear();
        abnormalDetXs.clear();
        abnormalDetYs.clear();
        abnormalDetWs.clear();
        abnormalDetHs.clear();

        abnormalDetAreas.clear();

        if(ROIImg.empty())
        {
            defectResult = false;
            return;
        }

        auto start = std::chrono::high_resolution_clock::now();

        // 图像映射,卡尺检测那里已经完成了

        // 图像切割
        std::vector<cv::Mat> cutImgs;
        std::vector<cv::Point> startPoints;
        ImgAlg::getCutImage(ROIImg, abnormalCutSize, (float)abnormalOverlappingSize / abnormalCutSize, cutImgs, startPoints);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        LOGE("getCutImage total: %lld ms", duration);
        LOGE("-----------------------------------");


        // if(isShow)
        // {
        //     cv::imwrite("ROI.bmp", ROIImg);
        // }
        LOGE_HIGH("getCutImage count: %d", startPoints.size());

        if(cutImgs.size() < 0)
        {
            LOGE("No cutImgs");
            abnormalDetResult = false;
            return;
        }
        start = std::chrono::high_resolution_clock::now();

        //运行
        auto results = ad_model->commitsOnce(cutImgs);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        // std::cout << "Abnormal Defect total:" << duration << "ms" << std::endl;
        // std::cout << "-----------------------------------" << std::endl;
        LOGE("Defect total: %lld ms", duration);
        LOGE("-----------------------------------");


        start = std::chrono::high_resolution_clock::now();

        // 获取屏蔽区域
        // cv::Mat ROIImgMask;
        // getMaskROI(polygonMaskTransforms, ROIImgMask);

        //ROIImg 的mask计算
        cv::Mat ROIImgMask;
        std::vector<std::vector<cv::Point >> polygonMaskTransforms2;
        if(true)
        {
            // cv::threshold(ROIImg,ROIImgMask,maskThre,255,cv::THRESH_BINARY);
            imageScaleMask(ROIImg, ROIImgMask, maskThre);
        }
        else
        {
            ROIImgMask = cv::Mat::ones(ROIImg.size(), CV_8UC1) * 255;
        }

        for(int id = 0; id < polygonMasks.size(); ++id)
        {
            for(int id2 = 0; id2 < polygonMasks[id].size(); ++id2)
            {
                LOGE("polygonMasks[%d][%d]:(%f,%f)", id, id2, polygonMasks[id][id2].x, polygonMasks[id][id2].y);
            }
        }

        for(int id2 = 0; id2 < modelROIPoints.size(); ++id2)
        {
            LOGE("modelROIPoints[%d]:(%f,%f)",  id2, modelROIPoints[id2].x, modelROIPoints[id2].y);
        }

        //这里因为卡尺定位可能造成xy方向发生变化，这里需要对坐标系方向进行矫正
        //主要是针对检测框和屏蔽框
        cv::Mat H;
        ImgAlg::getTransform(0, 0, modelROIAngle, cv::Point2f(0, 0), H);

        cv::Mat H33 = cv::Mat::eye(3, 3, CV_64F);
        H33.at<double>(0, 0) = H.at<double>(0, 0);
        H33.at<double>(0, 1) = H.at<double>(0, 1);
        H33.at<double>(0, 2) = H.at<double>(0, 2);
        H33.at<double>(1, 0) = H.at<double>(1, 0);
        H33.at<double>(1, 1) = H.at<double>(1, 1);
        H33.at<double>(1, 2) = H.at<double>(1, 2);

        getModelROIPointTransform(H33, modelROIAngle, modelROIPoints, modelROIPointsTransform);

        for(int id = 0; id < polygonMaskTransforms.size(); ++id)
        {
            for(int id2 = 0; id2 < polygonMaskTransforms[id].size(); ++id2)
            {
                LOGE("polygonMaskTransforms[%d][%d]:(%f,%f)", id, id2, polygonMaskTransforms[id][id2].x, polygonMaskTransforms[id][id2].y);
            }
        }

        for(int id2 = 0; id2 < modelROIPointsTransform.size(); ++id2)
        {
            LOGE("modelROIPointsTransform[%d]:(%f,%f)",  id2, modelROIPointsTransform[id2].x, modelROIPointsTransform[id2].y);
        }



        polygonMaskTransforms.clear();
        for(int id = 0; id < polygonMasks.size(); ++id)
        {
            std::vector<cv::Point2f >polygonMaskTransform = std::vector<cv::Point2f >(polygonMasks[id].size(), cv::Point2f(0, 0));
            getModelROIPointTransform(H33, modelROIAngle, polygonMasks[id], polygonMaskTransform);
            polygonMaskTransforms.push_back(polygonMaskTransform);
        }

        //仿射变换
        getMaskROI(polygonMaskTransforms, modelROIPointsTransform, polygonMaskTransforms2);
        for(int id = 0; id < polygonMaskTransforms2.size(); ++id)
        {

            cv::fillPoly(ROIImgMask, polygonMaskTransforms2[id], cv::Scalar(0));
            for(int id2 = 0; id2 < polygonMaskTransforms2[id].size(); ++id2)
            {
                LOGE("polygonMaskTransforms2[%d][%d]:(%d,%d)", id, id2, polygonMaskTransforms2[id][id2].x, polygonMaskTransforms2[id][id2].y);
            }
        }


        // 屏蔽边缘位置
        drawSideRect(side_filter_size, ROIImgMask.size().width, ROIImgMask.size().height, ROIImgMask);

        if(isShow)
        {
            cv::namedWindow("ROIImgMask", cv::WINDOW_NORMAL);
            cv::imshow("ROIImgMask", ROIImgMask);
            // cv::namedWindow("ROIImgMask2", cv::WINDOW_NORMAL);
            // cv::imshow("ROIImgMask2", ROIImgMask2);
            cv::waitKey(0);
        }
        // LOGE("333");

        //结果过滤及映射
        cv::Mat resultMask = cv::Mat::zeros(ROIImg.size(), CV_8U);

        for(int id = 0; id < results.size(); ++id)
        {
            auto res = results[id];
            cv::Mat anomaly_map, anomaly_mask, anomaly_mask_thre;

            if(res.anomaly_map_ptr)    // GPU
            {
                anomaly_map = cv::Mat(res.anomaly_map_ptr->height, res.anomaly_map_ptr->width,
                                      CV_32FC1, res.anomaly_map_ptr->data);
            }
            else
            {
                anomaly_map = res.anomaly_map;
            }
            cv::threshold(anomaly_map, anomaly_mask_thre, abnormalThre, 255, cv::THRESH_BINARY);
            anomaly_mask_thre.convertTo(anomaly_mask_thre, CV_8UC1);

            //面积过滤
            std::vector<std::vector<cv::Point >> contours;
            cv::findContours(anomaly_mask_thre, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            std::vector<std::vector<cv::Point >> selectContour;

            for(int contour_id = 0; contour_id < contours.size(); ++contour_id)
            {
                int area = cv::contourArea(contours[contour_id]);
                if(area > filter_defect_area_abnormal)
                {
                    selectContour.push_back(contours[contour_id]);
                }
            }

            anomaly_mask = cv::Mat::zeros(anomaly_mask_thre.size(), anomaly_mask_thre.type());

            cv::drawContours(anomaly_mask, selectContour, -1, cv::Scalar(255), -1);

            // 复制到结果图片
            cv::Point startPoint = startPoints[id];
            cv::Point endPoint;
            endPoint.x = std::min(startPoint.x + abnormalCutSize, resultMask.cols);
            endPoint.y = std::min(startPoint.y + abnormalCutSize, resultMask.rows);


            auto roi = cv::Rect(startPoint.x, startPoint.y, endPoint.x - startPoint.x, endPoint.y - startPoint.y);
            anomaly_mask.copyTo(resultMask(roi), anomaly_mask);

        }
        // LOGE("444");

        if(isShow)
        {
            cv::namedWindow("resultMask", cv::WINDOW_NORMAL);
            cv::imshow("resultMask", resultMask);
            cv::waitKey(0);
        }
        //mask过滤
        cv::bitwise_and(resultMask, ROIImgMask, resultMask);
        // LOGE("555");

        //把断开的区域连接
        closeROIProcess(resultMask);
        //结果显示
        if(isShow)
        {
            cv::imwrite("ROIImg.bmp", ROIImg);

            cv::namedWindow("ROIImg", cv::WINDOW_NORMAL);
            cv::imshow("ROIImg", ROIImg);

            cv::namedWindow("resultMask", cv::WINDOW_NORMAL);
            cv::imshow("resultMask", resultMask);
            cv::waitKey(0);
        }

        //结果转为框记录保存
        std::vector<std::vector<cv::Point >> contours;
        cv::findContours(resultMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for(int id = 0; id < contours.size(); ++id)
        {
            cv::Rect rect = cv::boundingRect(contours[id]);

            abnormalDetScores.push_back(abnormalThre);
            abnormalDetXs.push_back(rect.x);
            abnormalDetYs.push_back(rect.y);
            abnormalDetWs.push_back(rect.width);
            abnormalDetHs.push_back(rect.height);

            // LOGE("abnormal contour area:%d", int(cv::contourArea(contours[id])));

            abnormalDetAreas.push_back(int(cv::contourArea(contours[id])));
        }

        if(abnormalDetScores.size() > 0)
        {
            abnormalDetResult = false;
        }

        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        LOGE("Post total: %d ms", duration);
        LOGE("-----------------------------------");


    }
    catch(cv::Exception &e)
    {
        abnormalDetResult = false;
        LOGE("error:%s", e.what());

    }
    catch(std::exception &e)
    {
        abnormalDetResult = false;
        LOGE("error:%s", e.what());

    }
    catch(QException &e)
    {
        abnormalDetResult = false;
        LOGE("error:%s", e.what());

    }
    catch(...)
    {
        abnormalDetResult = false;
        LOGE("other error.");
    }

}

bool Jobworker::result()
{
    bool rt = true;
    if(isUseDefectDet)
    {
        if(!defectResult)
        {
            rt = false;
        }
    }
    if(isUseAnomalDet)
    {
        if(!abnormalDetResult)
        {
            rt = false;
        }
    }
    return rt;
}

void Jobworker::getCurrentDefectInfo(std::string save_data_path, std::string save_data_path_sql, bool is_save_defect_img, std::vector<float> &defect_scores,
                                     std::vector<std::string> &defect_img_paths, std::vector<std::string> &defect_types,
                                     std::vector<std::string> &defect_locations, std::vector<int> &defect_areas)
{
    try
    {
        LOGE("Defect Result size:%d", selectDefectResultID.size());

        if(isUseDefectDet)
        {
            for(int id = 0; id < selectDefectResultID.size(); ++id)
            {
                long long produceTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                std::string fileName = std::to_string(produceTime) + "_defect_det_" + std::to_string(id) + ".png";
                //图片切割
                int locateOCRClassId ;
                float locateOCRScore;
                float locateOCRX ;
                float locateOCRY ;
                float locateOCRW ;
                float locateOCRH ;
                int expend_size = 50;
                // std::cout << "getOCRAndDepthResult:" << id << std::endl;

                defectDet.getDefectDetResult(selectDefectResultID[id],
                                             &locateOCRClassId, &locateOCRScore, &locateOCRX, &locateOCRY,
                                             &locateOCRW, &locateOCRH);



                cv::RotatedRect resultRectOrigin(cv::Point2f(locateOCRX + locateOCRW / 2, locateOCRY + locateOCRH / 2),
                                                 cv::Size2f(locateOCRW, locateOCRH), 0);
                cv::RotatedRect resultRectTransform;

                resultRectTransform = resultRectOrigin;

                locateOCRX = std::max(0, int(locateOCRX));
                locateOCRY = std::max(0, int(locateOCRY));
                double locateOCRX2 = std::min(int(locateOCRX + locateOCRW), ROIImg.size().width);
                double locateOCRY2 = std::min(int(locateOCRY + locateOCRH), ROIImg.size().height);

                //阈值过滤
                cv::Rect filter_roi(locateOCRX, locateOCRY, locateOCRX2 - locateOCRX, locateOCRY2 - locateOCRY);

                // std::vector<cv::Point2f> modelROIPointsTransformSort = modelROIPointsTransform;

                // //保证点按照左上、右上、右下、左下顺序排列
                // ImgAlg::SortPoint(modelROIPointsTransformSort);

                // cv::Mat H, H_Inv;
                // ImgAlg::getPerspectiveTransform(modelROIPointsTransformSort, H);
                // H_Inv = H.inv();
                // ImgAlg::transformROI(H_Inv, resultRectOrigin, resultRectTransform);



                cv::Rect roi(resultRectTransform.boundingRect());

                int defect_area = filter_roi.area();
                // float xL = roi.x;
                // float xR = roi.x + roi.width;
                // float yL = roi.y;
                // float yR = roi.y + roi.height;

                float xL = std::max((0), roi.x - expend_size);
                float xR = std::min((img.cols), roi.x + roi.width + expend_size);
                float yL = std::max((0), roi.y - expend_size);
                float yR = std::min((img.rows), roi.y + roi.height + expend_size);
                roi = cv::Rect(xL, yL, (xR - xL), (yR - yL));

                if(roi.width <= 0 || roi.height <= 0)
                {
                    //结果框为空
                    continue;
                }

                char defect_location[256];
                sprintf(defect_location, "%d,%d,%d,%d", int(xL), int(yL), int(xR - xL), int(yR - yL));
                // LOGE_HIGH("defect_location:%s", defect_location);

                // if(is_save_defect_img)
                // {
                //     // 保证图片不为空
                //     if(img.empty())
                //     {
                //         continue;
                //     }
                //     cv::Mat defectImg = img(roi).clone();
                //     // cv::imwrite(save_data_path + "\\" + fileName, defectImg);
                //     QtConcurrent::run(this, &JobWorker::saveImage, save_data_path + "\\" + fileName, defectImg);
                //     // saveImage(save_data_path + "\\" + fileName, defectImg);

                // }
                //sql 保存数据整理

                std::string defect_img_path = save_data_path_sql + "\\" + fileName;
                replaceAll(defect_img_path, "\\", "\\\\");
                replaceAll(defect_img_path, "/", "\\\\");




                defect_img_paths.push_back(defect_img_path);
                // defect_types.push_back(classNames[locateOCRClassId]);
                defect_types.push_back(TclassNames[locateOCRClassId]);
                defect_locations.push_back(defect_location);
                defect_scores.push_back(locateOCRScore);
                defect_areas.push_back(defect_area);

            }

            if(isUseAnomalDet)
            {
                for(int id = 0; id < abnormalDetScores.size(); ++id)
                {
                    long long produceTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                    std::string fileName = std::to_string(produceTime) + "_ab_defect_det_" + std::to_string(id) + ".png";

                    float locateOCRScore;
                    float locateOCRX ;
                    float locateOCRY ;
                    float locateOCRW ;
                    float locateOCRH ;
                    int expend_size = 50;
                    locateOCRScore = abnormalDetScores[id];
                    locateOCRX = abnormalDetXs[id];
                    locateOCRY = abnormalDetYs[id];
                    locateOCRW = abnormalDetWs[id];
                    locateOCRH = abnormalDetHs[id];

                    cv::RotatedRect resultRectOrigin(cv::Point2f(locateOCRX + locateOCRW / 2, locateOCRY + locateOCRH / 2),
                                                     cv::Size2f(locateOCRW, locateOCRH), 0);
                    cv::RotatedRect resultRectTransform;

                    resultRectTransform = resultRectOrigin;

                    // std::vector<cv::Point2f> modelROIPointsTransformSort = modelROIPointsTransform;


                    // //保证点按照左上、右上、右下、左下顺序排列
                    // ImgAlg::SortPoint(modelROIPointsTransformSort);

                    // cv::Mat H, H_Inv;
                    // ImgAlg::getPerspectiveTransform(modelROIPointsTransformSort, H);

                    // H_Inv = H.inv();
                    // ImgAlg::transformROI(H_Inv, resultRectOrigin, resultRectTransform);

                    cv::Rect roi(resultRectTransform.boundingRect());

                    int defect_area = roi.area();

                    float xL = std::max((0), roi.x - expend_size);
                    float xR = std::min((img.cols), roi.x + roi.width + expend_size);
                    float yL = std::max((0), roi.y - expend_size);
                    float yR = std::min((img.rows), roi.y + roi.height + expend_size);
                    roi = cv::Rect(xL, yL, (xR - xL), (yR - yL));

                    if(roi.width <= 0 || roi.height <= 0)
                    {
                        //结果框为空
                        continue;
                    }
                    // if(is_save_defect_img)
                    // {
                    //     // 保证图片不为空
                    //     if(img.empty())
                    //     {
                    //         continue;
                    //     }
                    //     cv::Mat defectImg = img(roi).clone();
                    //     // cv::imwrite(save_data_path + "\\" + fileName, defectImg);
                    //     QtConcurrent::run(this, &JobWorker::saveImage, save_data_path + "\\" + fileName, defectImg);
                    //     // saveImage(save_data_path + "\\" + fileName, defectImg);

                    // }
                    //sql 保存数据整理
                    char defect_location[256];
                    sprintf(defect_location, "%d,%d,%d,%d", int(xL), int(yL), int(xR - xL), int(yR - yL));

                    std::string defect_img_path = save_data_path_sql + "\\" + fileName;
                    replaceAll(defect_img_path, "\\", "\\\\");
                    replaceAll(defect_img_path, "/", "\\\\");

                    defect_img_paths.push_back(defect_img_path);
                    defect_types.push_back("其他");
                    defect_locations.push_back(defect_location);
                    defect_scores.push_back(locateOCRScore);

                    defect_areas.push_back(abnormalDetAreas[id]);

                }
            }


        }

    }
    catch(cv::Exception &e)
    {
        LOGE("error:%s", e.what());
    }
    catch(std::exception &e)
    {
        LOGE("error:%s", e.what());

    }
    catch(QException &e)
    {
        LOGE("error:%s", e.what());

    }
    catch(...)
    {
        LOGE("other error.");
    }

}

void Jobworker::run(bool isShow)
{
    if(isUseDefectDet)
    {
        runDefectDet(isShow);
    }
    if(isUseAnomalDet)
    {
        runAbnormalDefectDet(isShow);
    }
}

void Jobworker::saveDetToolPara(std::string savePath)
{
    QFile file(savePath.data());
    if(!file.open(QIODevice::WriteOnly))
    {
        LOGE("could not save DetToolPara");
        return;
    }

    QJsonDocument document;
    QJsonObject mJsonObject ;
    QJsonObject modelROIPointsObject;
    QJsonObject objectDetParaObject;
    QJsonObject abnormalDetParaObject;

    ///检测框
    QJsonArray modelROIPointsArray;
    for(int id = 0; id < 4; ++id)
    {
        QJsonObject modelROIPointObject ;

        modelROIPointObject.insert("modelROIPointX", modelROIPoints[id].x);
        modelROIPointObject.insert("modelROIPointY", modelROIPoints[id].y);

        modelROIPointsArray.append(modelROIPointObject);
    }
    modelROIPointsObject.insert("modelROIPointsArray", modelROIPointsArray);
    modelROIPointsObject.insert("modelROIAngle", modelROIAngle);

    ///目标检测
    //屏蔽框
    QJsonArray polygonDetMasksArray;
    for(int id = 0; id < polygonDetMasks.size(); ++id)
    {
        QJsonArray polygonDetMaskArray;

        for(int id2 = 0; id2 < polygonDetMasks[id].size(); ++id2)
        {
            QJsonObject polygonDetMaskPointObject ;

            polygonDetMaskPointObject.insert("polygonDetMaskPointObjectX", polygonDetMasks[id][id2].x);
            polygonDetMaskPointObject.insert("polygonDetMaskPointObjectY", polygonDetMasks[id][id2].y);

            polygonDetMaskArray.append(polygonDetMaskPointObject);
        }
        polygonDetMasksArray.append(polygonDetMaskArray);
    }
    objectDetParaObject.insert("polygonDetMasksArray", polygonDetMasksArray);
    //其他参数
    objectDetParaObject.insert("isUseDefectDet", isUseDefectDet);
    objectDetParaObject.insert("cutSize", cutSize);
    objectDetParaObject.insert("overlappingSize", overlappingSize);
    objectDetParaObject.insert("thre", thre);
    objectDetParaObject.insert("filter_defect_area", filter_defect_area);

    objectDetParaObject.insert("maskThre", maskThre);

    ///异常检测
    //屏蔽框
    QJsonArray polygonMasksArray;
    for(int id = 0; id < polygonMasks.size(); ++id)
    {
        QJsonArray polygonMaskArray;

        for(int id2 = 0; id2 < polygonMasks[id].size(); ++id2)
        {
            QJsonObject polygonMaskPointObject ;

            polygonMaskPointObject.insert("polygonMaskPointObjectX", polygonMasks[id][id2].x);
            polygonMaskPointObject.insert("polygonMaskPointObjectY", polygonMasks[id][id2].y);

            polygonMaskArray.append(polygonMaskPointObject);
        }
        polygonMasksArray.append(polygonMaskArray);
    }
    abnormalDetParaObject.insert("polygonMasksArray", polygonMasksArray);
    //其他参数
    abnormalDetParaObject.insert("isUseAnomalDet", isUseAnomalDet);
    abnormalDetParaObject.insert("abnormalCutSize", abnormalCutSize);
    abnormalDetParaObject.insert("abnormalOverlappingSize", abnormalOverlappingSize);
    abnormalDetParaObject.insert("abnormalThre", abnormalThre);
    abnormalDetParaObject.insert("filter_defect_area_abnormal", filter_defect_area_abnormal);
    abnormalDetParaObject.insert("side_filter_size", side_filter_size);


    //
    mJsonObject.insert("modelROIPointsObject", modelROIPointsObject);
    mJsonObject.insert("objectDetParaObject", objectDetParaObject);
    mJsonObject.insert("abnormalDetParaObject", abnormalDetParaObject);

    document.setObject(mJsonObject);

    file.write(document.toJson());

    file.close();

}




void Jobworker::loadDetToolPara(std::string strFile)
{
    QFile file(strFile.data());
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // LOGE("could't open projects json");
        return;
    }


    QString value = file.readAll();
    file.close();

    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);
    if(!(parseJsonErr.error == QJsonParseError::NoError))
    {
        // LOGE(parseJsonErr.errorString().toStdString().data());

        return;
    }
    QJsonObject mJsonObject = document.object();

    // for(int id2 = 0; id2 < modelROIPoints.size(); ++id2)
    // {
    //     LOGE("modelROIPoints[%d]:(%f,%f)",  id2, modelROIPoints[id2].x, modelROIPoints[id2].y);
    // }
    ///检测框
    QJsonObject modelROIPointsObject = mJsonObject["modelROIPointsObject"].toObject();
    QJsonArray modelROIPointsArray = modelROIPointsObject["modelROIPointsArray"].toArray();
    modelROIPoints[0].x = modelROIPointsArray[0].toObject()["modelROIPointX"].toDouble();
    modelROIPoints[0].y = modelROIPointsArray[0].toObject()["modelROIPointY"].toDouble();
    modelROIPoints[1].x = modelROIPointsArray[1].toObject()["modelROIPointX"].toDouble();
    modelROIPoints[1].y = modelROIPointsArray[1].toObject()["modelROIPointY"].toDouble();
    modelROIPoints[2].x = modelROIPointsArray[2].toObject()["modelROIPointX"].toDouble();
    modelROIPoints[2].y = modelROIPointsArray[2].toObject()["modelROIPointY"].toDouble();
    modelROIPoints[3].x = modelROIPointsArray[3].toObject()["modelROIPointX"].toDouble();
    modelROIPoints[3].y = modelROIPointsArray[3].toObject()["modelROIPointY"].toDouble();

    modelROIAngle = modelROIPointsObject["modelROIAngle"].toDouble();
    // for(int id2 = 0; id2 < modelROIPoints.size(); ++id2)
    // {
    //     LOGE("modelROIPoints[%d]:(%f,%f)",  id2, modelROIPoints[id2].x, modelROIPoints[id2].y);
    // }
    ///目标检测
    QJsonObject objectDetParaObject = mJsonObject["objectDetParaObject"].toObject();
    //屏蔽框
    QJsonArray polygonDetMasksArray = objectDetParaObject["polygonDetMasksArray"].toArray();
    polygonDetMasks.clear();
    for(int id = 0; id < polygonDetMasksArray.count(); ++id)
    {
        std::vector<cv::Point2f>polygonDetMask;
        QJsonArray polygonDetMaskArray = polygonDetMasksArray[id].toArray();
        for(int id2 = 0; id2 < polygonDetMaskArray.count(); ++id2)
        {
            cv::Point2f polygonDetPoint;
            polygonDetPoint.x = polygonDetMaskArray[id2].toObject()["polygonDetMaskPointObjectX"].toDouble();
            polygonDetPoint.y = polygonDetMaskArray[id2].toObject()["polygonDetMaskPointObjectY"].toDouble();
            polygonDetMask.push_back(polygonDetPoint);
        }

        polygonDetMasks.push_back(polygonDetMask);
    }

    // for(int id = 0; id < polygonDetMasks.size(); ++id)
    // {
    //     for(int id2 = 0; id2 < polygonDetMasks[id].size(); ++id2)
    //     {
    //         LOGE("polygonDetMasks[%d][%d]:(%f,%f)", id, id2, polygonDetMasks[id][id2].x, polygonDetMasks[id][id2].y);
    //     }
    // }

    //其他参数
    isUseDefectDet = objectDetParaObject["isUseDefectDet"].toBool();
    cutSize = objectDetParaObject["cutSize"].toInt();
    overlappingSize = objectDetParaObject["overlappingSize"].toInt();
    thre = objectDetParaObject["thre"].toDouble();
    filter_defect_area = objectDetParaObject["filter_defect_area"].toInt();
    maskThre = objectDetParaObject["maskThre"].toInt();

    ///异常检测
    QJsonObject abnormalDetParaObject = mJsonObject["abnormalDetParaObject"].toObject();
    //屏蔽框
    QJsonArray polygonMasksArray = abnormalDetParaObject["polygonMasksArray"].toArray();
    polygonMasks.clear();
    for(int id = 0; id < polygonMasksArray.count(); ++id)
    {
        std::vector<cv::Point2f>polygonMask;
        QJsonArray polygonMaskArray = polygonMasksArray[id].toArray();
        for(int id2 = 0; id2 < polygonMaskArray.count(); ++id2)
        {
            cv::Point2f polygonPoint;
            polygonPoint.x = polygonMaskArray[id2].toObject()["polygonMaskPointObjectX"].toDouble();
            polygonPoint.y = polygonMaskArray[id2].toObject()["polygonMaskPointObjectY"].toDouble();
            polygonMask.push_back(polygonPoint);
        }

        polygonMasks.push_back(polygonMask);
    }


    //其他参数
    isUseAnomalDet = abnormalDetParaObject["isUseAnomalDet"].toBool();
    abnormalCutSize = abnormalDetParaObject["abnormalCutSize"].toInt();
    abnormalOverlappingSize = abnormalDetParaObject["abnormalOverlappingSize"].toInt();
    abnormalThre = abnormalDetParaObject["abnormalThre"].toDouble();
    filter_defect_area_abnormal = abnormalDetParaObject["filter_defect_area_abnormal"].toInt();
    side_filter_size = abnormalDetParaObject["side_filter_size"].toInt();


}
bool Jobworker::readTClassName()
{
    // QString path = qApp->applicationDirPath();
    QString strFile;


    strFile = QString("model/TClassName.json");
    QFile file(strFile);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "could't open projects json";
        return false;
    }

    QString value = file.readAll();
    file.close();
    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);
    if(!(parseJsonErr.error == QJsonParseError::NoError))
    {
        qDebug() << parseJsonErr.errorString();
        return false;
    }
    QJsonObject mJsonObject = document.object();

    TclassNames.clear();
    foreach(QString key, mJsonObject.keys())
    {
        TclassNames[key.toInt()] = mJsonObject.value(key).toString().toStdString();
    }
    return true;
}



void Jobworker::initSupervisedModel(std::string supervisedModelPath)
{
    //字典初始化
    // bool rt = readModelInfo();
    // if(!rt)
    // {
    //     QString strFile;
    //     strFile = QString("model/%1/cam%2_modelInfo.json").arg(m_model_type).arg(cam_id);
    //     LOGE("open model info file failed\n");
    //     // messageboxSignal(QString("模型信息文件：\"%1\" 不存在。").arg(strFile), 2);
    //     return ;
    // }
    //字典初始化
    bool rt = readTClassName();
    //保证用显卡1
    cudaSetDevice(0);
    //模型读取
    QString pathOnnxFile = supervisedModelPath.data(); //QString("model/%1/cam%2_Det_model.wts").arg(m_model_type).arg(cam_id);
    replaceAll(supervisedModelPath, ".wts", ".engine");
    QString pathTRTFile = supervisedModelPath.data(); // QString("model/%1/cam%2_Det_model.engine").arg(m_model_type).arg(cam_id);

    if(!QFile::exists(pathTRTFile))
    {
        if(QFile::exists(pathOnnxFile))
        {

            defectDet.wtsToEngine(pathOnnxFile.toStdString(), pathTRTFile.toStdString(), modelSize, TclassNames.size());
        }
        else
        {
            LOGE("Not Exist defect det model");
            // messageboxSignal(QString("模型文件：\"%1\" 无法查找。").arg(pathTRTFile), 2);
            return;
        }
    }
    LOGE("start init Supervised Model");

    defectDet.initModel(pathTRTFile.toStdString().data());
    defectDet.setPara(cutSize, overlappingSize, 0.25);

    LOGE("finish init Supervised Model");


}
void Jobworker::initUnsupervisedModel(std::string unsupervisedModelPath)
{

    if(ad_model != nullptr)
    {
        ad_model.reset();
        ad_model = nullptr;
    }
    int max_batch_size = 32;
    std::string fp_str = "fp16";

    QString pathAnomalOnnxFile = unsupervisedModelPath.data(); // QString("model/%1/cam%2_anomalDet_model.onnx").arg(m_model_type).arg(cam_id);
    replaceAll(unsupervisedModelPath, ".onnx", "_fp16_b32.engine");

    QString pathAnomalTRTFile = unsupervisedModelPath.data(); //QString("model/%1/cam%2_anomalDet_model_fp16_b32.engine").arg(m_model_type).arg(cam_id);

    if(QFileInfo(pathAnomalOnnxFile).exists())
    {
        if(!rvs2d::trt::build_model(pathAnomalOnnxFile.toStdString(), pathAnomalTRTFile.toStdString(), max_batch_size, false))
        {
            LOGE("build model failed\n");
            // messageboxSignal(QString("模型文件：\"%1\" 不存在。").arg(pathAnomalTRTFile), 2);
            return ;
        }
        LOGE("start init Unsupervised Model");

        ad_model = rvs2d::dl::ad::createMultiGpuInfer(pathAnomalTRTFile.toStdString());

        LOGE("finish init Unsupervised Model");
    }
    else
    {
        LOGE("not exist Unsupervised Model %s", pathAnomalOnnxFile.toStdString().data());

    }



}
void Jobworker::initDefectDet(std::string supervisedModelPath, std::string unsupervisedModelPath)
{
    LOGE("start model init.");

    ///目标检测模型初始化
    initSupervisedModel(supervisedModelPath);

    ///异常检测模型初始化
    initUnsupervisedModel(unsupervisedModelPath);
    /// 卡尺工具初始化

    LOGE("finish model init.");

}


