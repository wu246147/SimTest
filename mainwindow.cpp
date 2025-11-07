#include "mainwindow.h"
#include "./ui_mainwindow.h"

void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // 防止重复替换
    }
}

void getMaskROI2(std::vector<std::vector<cv::Point2f >> polygons, std::vector<cv::Point2f> modelROIPointsTransform,
                 std::vector<std::vector<cv::Point >> &polygonMaskTransforms)
{

    //添加手动mask区域
    //要根据检测框位置，做出变换
    cv::Mat H;

    std::vector<cv::Point2f> modelROIPointsTransformSort = modelROIPointsTransform;
    //保证点按照左上、右上、右下、左下顺序排列
    ImgAlg::SortPoint(modelROIPointsTransformSort);

    ImgAlg::getPerspectiveTransform(modelROIPointsTransformSort, H);

    std::ostringstream os;
    os << cv::format(H, cv::Formatter::FMT_NUMPY);
    std::string txt = os.str();
    LOGE("H:%s", txt.data());


    //ROIImg 的屏蔽区计算
    // LOGE("111");
    for(int id = 0; id < polygons.size(); ++id)
    {
        //转int型
        std::vector<cv::Point>polygonMaskTransformInt;
        for(int j = 0; j < polygons[id].size(); ++j)
        {
            LOGE("polygons[%d][%d]:(%f,%f)", id, j, polygons[id][j].x, polygons[id][j].y);

            //映射
            cv::Point2f polygonPointTransformTransform;
            ImgAlg::transformPoint(H, polygons[id][j], polygonPointTransformTransform);

            LOGE("polygonPointTransformTransform:(%f,%f)",  polygonPointTransformTransform.x, polygonPointTransformTransform.y);

            polygonMaskTransformInt.push_back(cv::Point(int(polygonPointTransformTransform.x), int(polygonPointTransformTransform.y)));

            LOGE("polygonPointTransformTransform:(%d,%d)",  int(polygonPointTransformTransform.x), int(polygonPointTransformTransform.y));

        }
        // cv::fillPoly(ROIImgMask, polygonMaskTransformInt, cv::Scalar(0));
        polygonMaskTransforms.push_back(polygonMaskTransformInt);

    }
    // LOGE("222");
}

void getModelROIPointTransform(cv::Mat H, float angle, std::vector<cv::Point2f> modelROIPoints, std::vector<cv::Point2f> &modelROIPointsTransform)
{
    try
    {
        for(int id = 0; id < modelROIPoints.size(); ++id)
        {
            cv::Point2f inputPoint;
            cv::Point2f outputPoint;
            inputPoint = modelROIPoints[id];
            int rt = ImgAlg::transformPoint(H, inputPoint, outputPoint);
            modelROIPointsTransform[id] = outputPoint;
        }
    }
    catch(cv::Exception &e)
    {
        // location_result = false;
        LOGE("error:%s", e.what());
    }
    catch(std::exception &e)
    {
        // location_result = false;
        LOGE("error:%s", e.what());
    }
    catch(QException &e)
    {
        // location_result = false;
        LOGE("error:%s", e.what());
    }
    catch(...)
    {
        // location_result = false;
        LOGE("other error.");
    }
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //log初始化
    QDir qdir;
    QString logfile_dir = QString(QDir::currentPath() + "/logfile");
    //    std::cout << "logfile_dir :" << logfile_dir.toStdString().data() << std::endl;
    if(!qdir.exists(logfile_dir))
    {
        qdir.mkdir(logfile_dir);
    }
    logfile::LOGINIT(logfile_dir.toStdString());
    logfile::set_is_write(true);

    //display初始化
    this->display_grid_layout = new QGridLayout(this);
    display = new myWidgetView();
    display->myGraphicsView->setStyleSheet("background-color:rgb(161, 161, 161);border:1px solid black;");
    this->display_grid_layout->addWidget(display);
    ui->widget_2->setLayout(this->display_grid_layout);
    ui->widget_2->layout()->activate();

    //
    ui->pushButton_runAll->setEnabled(false);
    ui->pushButton_runNext->setEnabled(false);
    ui->pushButton_showImage->setEnabled(false);
    ui->pushButton_showROI->setEnabled(false);

    setTTF();

    connect(this, &MainWindow::add_log_signal, this, &MainWindow::add_log_even);
    connect(this, &MainWindow::clean_log_signal, this, &MainWindow::clean_log_even);
}

MainWindow::~MainWindow()
{
    delete ui;
}

///
/// \brief MatToQImage Mat 转 QImage
/// \param cvImage
/// \return
///
QImage MatToQImage(const cv::Mat &cvImage)
{
    std::vector<uchar> imgBuf;
    imencode(".bmp", cvImage, imgBuf);
    QByteArray baImg((char *)imgBuf.data(), static_cast<int>(imgBuf.size()));
    QImage image;
    image.loadFromData(baImg, "bmp");
    return image;
}

///
/// \brief MatToPixmap  Mat 转 Pixmap
/// \param cvImage
/// \return
///
QPixmap MatToPixmap(const cv::Mat &cvImage)
{
    return QPixmap::fromImage(MatToQImage(cvImage));
}

///
/// \brief createMultipleFolders 创建多层文件夹
/// \param path
/// \return
///
QString createMultipleFolders(const QString path)
{
    QString pathReplace = path;
    pathReplace.replace("\\", "/");
    QDir dir(pathReplace);
    if(pathReplace == "" || dir.exists(pathReplace))
    {
        return pathReplace;
    }
    QString parentDir = createMultipleFolders(pathReplace.mid(0, pathReplace.lastIndexOf('/')));
    QString dirName = pathReplace.mid(pathReplace.lastIndexOf('/') + 1);
    QDir parentPath(parentDir);
    if(!dirName.isEmpty())
    {
        parentPath.mkpath(dirName);
    }
    return parentDir + "/" + dirName;
}

///
/// \brief MainWindow::get_files 获取文件夹下所有文件路径
/// \param dir
/// \param file_paths
///
void get_files(std::string dir, std::vector<std::string> *file_paths)
{
    try
    {
        QDir d(dir.data());
        d.setFilter(QDir::Files | QDir::NoSymLinks);
        QFileInfoList list = d.entryInfoList();
        // LOGE("files count:%d.", list.size());

        for(int i = 0; i < list.size(); i++)
        {
            QFileInfo info = list.at(i);
            file_paths->push_back(dir + "/" + info.fileName().toStdString());
        }
    }
    catch(std::exception e)
    {
        LOGE("error:%s", e.what());

    }
}


void MainWindow::showDetROI()
{
    //检测区域
    {
        // 坐标映射

        //
        QVector<QPointF>Polygon_Points;
        for(auto j = 0; j < modelROIPointsTransform.size() ; j++)
        {
            Polygon_Points.push_back(QPointF(modelROIPointsTransform[j].x,
                                             modelROIPointsTransform[j].y));
        }
        myDefectDetROI = new MyGraphicPolygonItem(Polygon_Points,
            Qt::blue, NULL,
            QString(("defect_det_roi")));

        myDefectDetROI->m_polygon_pen.setColor(Qt::blue);
        myDefectDetROI->m_point_pen.setColor(Qt::yellow);
        display->myGraphicsView->addItem(myDefectDetROI, false);

    }
}
void MainWindow::showCaliperToolROI()
{
    {
        // // 卡尺1
        // {
        //     QRectF tool_rect(caliperToolROIs[0].center.x - caliperToolROIs[0].size.width / 2,
        //                      caliperToolROIs[0].center.y - caliperToolROIs[0].size.height / 2,
        //                      caliperToolROIs[0].size.width,
        //                      caliperToolROIs[0].size.height);
        //     this->myRectItem_caliperROI1 = new MyGraphicRectItem(tool_rect, caliperToolROIs[0].angle);
        //     this->myRectItem_caliperROI1->setToolTip("caliperToolRoi 1");
        //     this->myRectItem_caliperROI1->SetColor(Qt::green);
        //     this->myRectItem_caliperROI1->setRotatable(true);
        //     display->myGraphicsView->addItem(myRectItem_caliperROI1, false);
        // }

    }
}

void MainWindow::setTTF()
{
    std::string ttf_path = "./qss/iconfont.ttf";
    if(!QFile::exists(ttf_path.data()))
    {
        return;
    }

    QFontDatabase database;
    int fontId = database.addApplicationFont(ttf_path.data());
    QString fontName = database.applicationFontFamilies(fontId).at(0);


    QString styleSheetPushButton = QString("QPushButton{font: 18pt \"%1\";}").arg(fontName);
    QString styleSheetLabel = QString("QLabel {font: 18pt \"%1\";color: #0078dc;border-bottom: 0px;}").arg(fontName);

    ui->pushButton_min->setStyleSheet(styleSheetPushButton);
    ui->pushButton_max->setStyleSheet(styleSheetPushButton);
    ui->pushButton_close->setStyleSheet(styleSheetPushButton);
    ui->label_log_region->setStyleSheet(styleSheetLabel);


    ui->pushButton_min->setText(QChar(0xe65a));
    ui->pushButton_max->setText(QChar(0xe653));
    ui->pushButton_close->setText(QChar(0xeca0));
    ui->label_log_region->setText(QChar(0xe654));

}

void MainWindow::Append(const QString &text)
{
    emit add_log_signal(text);
}

void MainWindow::add_log_even(QString message)
{
    QString old_message = ui->textEdit_log->toPlainText();
    //    message = message + "\n" + old_message;
    message = message  + QString(old_message.toStdString().substr(0, 5000).data());
    ui->textEdit_log->setText(message);
}

void MainWindow::clean_log_even()
{
    ui->textEdit_log->clear();
}





void MainWindow::runDefectDet(bool isshow)
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

void MainWindow::runAbnormalDefectDet(bool isShow)
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

bool MainWindow::result()
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

void MainWindow::getCurrentDefectInfo(std::string save_data_path, std::string save_data_path_sql, bool is_save_defect_img, std::vector<float> &defect_scores,
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


void MainWindow::show_result()
{
    try
    {
        QGraphicsPolygonItem *tmp_polygon = new QGraphicsPolygonItem(QPolygon(3));
        QGraphicsRectItem *tmp_rect = new QGraphicsRectItem(0, 0, 100, 100);
        QGraphicsTextItem *tmp_string = new QGraphicsTextItem("str");
        MyGraphicRectItem *tmp_my_rect = new MyGraphicRectItem(QRectF(0, 0, 100, 100));
        QGraphicsEllipseItem *tmp_ellip = new QGraphicsEllipseItem();
        display->myGraphicsView->cleanItem(tmp_polygon);
        display->myGraphicsView->cleanItem(tmp_rect);
        display->myGraphicsView->cleanItem(tmp_string);
        display->myGraphicsView->cleanItem(tmp_my_rect);
        display->myGraphicsView->cleanItem(tmp_ellip);
        delete tmp_rect;
        delete tmp_string;
        delete tmp_polygon;
        delete tmp_my_rect;
        delete tmp_ellip;
        std::vector<QGraphicsItem *> item_list;

        std::vector<std::string> defect_img_paths;
        std::vector<std::string> defect_types;
        std::vector<std::string> defect_locations;
        std::vector<float> defect_scores;
        std::vector<int> defect_areas;

        //判断是否卡尺定位成功
        bool is_save_defect_img = false;
        std::string save_data_path = "";
        getCurrentDefectInfo(save_data_path, save_data_path, is_save_defect_img, defect_scores,
                             defect_img_paths, defect_types, defect_locations, defect_areas);

        // std::vector<cv::Rect> tmpDefectDetRectList;
        LOGE("defect_locations size:%d", defect_locations.size());

        for(int id = 0; id < defect_locations.size(); ++id)
        {
            QStringList defect_location_list = QString(defect_locations[id].data()).split(",");
            cv::Rect defect_rect(defect_location_list[0].toInt(), defect_location_list[1].toInt(),
                                 defect_location_list[2].toInt(), defect_location_list[3].toInt());

            // tmpDefectDetRectList.push_back(defect_rect);

            int line_wide = 5;
            QGraphicsRectItem *result_item = new QGraphicsRectItem(defect_rect.x / showImgScaleSize, defect_rect.y / showImgScaleSize,
                (defect_rect.width) / showImgScaleSize, (defect_rect.height)  / showImgScaleSize);

            QPen pn(Qt::SolidLine);
            pn.setColor(Qt::red);
            pn.setWidth(line_wide);
            result_item->setPen(pn);
            item_list.push_back(result_item);

            MyTextItem *item_text = new MyTextItem((defect_types[id] + " " + std::to_string(defect_areas[id])).data());


            // LOGE("displayList[work_id]->myGraphicsView->width():%d", displayList[work_id]->myGraphicsView->width());
            // LOGE("displayList[work_id]->myGraphicsView->height():%d", displayList[work_id]->myGraphicsView->height());

            // LOGE("modelImg[work_id].cols:%d", modelImg[work_id].cols / showImgScaleSize);
            // LOGE("modelImg[work_id].rows:%d", modelImg[work_id].rows / showImgScaleSize);

            float scale = std::min((float)display->myGraphicsView->height() / (img.rows / showImgScaleSize),
                                   (float)display->myGraphicsView->width() / (img.cols / showImgScaleSize));
            // LOGE("displayList[work_id]->myGraphicsView->height() / (modelImg[work_id].rows / showImgScaleSize:%f", (float)displayList[work_id]->myGraphicsView->height() / (modelImg[work_id].rows / showImgScaleSize));
            // LOGE("displayList[work_id]->myGraphicsView->width() / (modelImg[work_id].cols / showImgScaleSize):%f", (float)displayList[work_id]->myGraphicsView->width() / (modelImg[work_id].cols / showImgScaleSize));
            // LOGE("scale%f", scale);


            item_text->setCustomPos((display->myGraphicsView->width() - (img.cols / showImgScaleSize)* scale) / 2
                                    + (defect_rect.x + defect_rect.width) / showImgScaleSize * scale,
                                    (display->myGraphicsView->height() - (img.rows / showImgScaleSize)* scale) / 2
                                    + (defect_rect.y + defect_rect.height) / showImgScaleSize * scale);

            // item_text->setCustomPos(0, 0);
            // item_text->setCustomPos(displayList[work_id]->myGraphicsView->width(), displayList[work_id]->myGraphicsView->height());


            item_text->setDefaultTextColor(Qt::red);
            QFont font;
            font.setPointSize(50);
            item_text->setFont(font);

            item_list.push_back(item_text);
        }
        LOGE("finish defect show");


        if(!result())
        {
            QGraphicsTextItem *result_string  = new QGraphicsTextItem("NG");
            result_string->setDefaultTextColor(Qt::red);
            QFont font;
            font.setPointSize(qMin(img.cols, img.rows) / 20 / showImgScaleSize);
            result_string->setFont(font);
            result_string->setTextWidth(-1);
            result_string->setPos(int(display->myGraphicsView->width() * 8 / 10),
                                  int(display->myGraphicsView->height() * 8.5 / 10));
            item_list.push_back(result_string);
        }
        else
        {
            QGraphicsTextItem *result_string  = new QGraphicsTextItem("OK");
            result_string->setDefaultTextColor(Qt::green);
            QFont font;
            font.setPointSize(qMin(img.cols, img.rows) / 20 / showImgScaleSize);
            result_string->setFont(font);
            result_string->setTextWidth(-1);
            result_string->setPos(int(display->myGraphicsView->width() * 8 / 10),
                                  int(display->myGraphicsView->height() * 8.5 / 10));
            item_list.push_back(result_string);
        }
        for(int i = 0; i < item_list.size(); i++)
        {
            display->myGraphicsView->addItem(item_list[i], false);
        }
        LOGE("finish result show");

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

void MainWindow::saveDetToolPara(QString savePath)
{
    QFile file(savePath);
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




void MainWindow::loadDetToolPara(QString strFile)
{
    QFile file(strFile);
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

void MainWindow::updata_control_value()
{
    is_loading = true;

    //有监督参数
    ui->checkBox_isDefectDet->setChecked(isUseDefectDet);
    ui->spinBox_cutSize->setValue(cutSize);
    ui->spinBox_overlappingSize->setValue(overlappingSize);
    ui->doubleSpinBox_thre->setValue(thre);
    ui->spinBox_maskThre->setValue(maskThre);
    ui->spinBox_filter_defeat_area->setValue(filter_defect_area);



    //无监督参数
    ui->checkBox_isUseAbnormal->setChecked(isUseAnomalDet);
    ui->spinBox_AbnormalCutSize ->setValue(abnormalCutSize);
    ui->spinBox_AbnormalOverlappingSize->setValue(abnormalOverlappingSize);
    ui->doubleSpinBox_AbnormalThre->setValue(abnormalThre);
    ui->spinBox_filter_defeat_area_abnormal->setValue(filter_defect_area_abnormal);
    ui->spinBox_side_filter_size->setValue(side_filter_size);

    is_loading = false;
}

bool MainWindow::readTClassName()
{
    QString path = qApp->applicationDirPath();
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



void MainWindow::initSupervisedModel(std::string supervisedModelPath)
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
void MainWindow::initUnsupervisedModel(std::string unsupervisedModelPath)
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
void MainWindow::initDefectDet(std::string supervisedModelPath, std::string unsupervisedModelPath)
{
    LOGE("start model init.");

    ///目标检测模型初始化
    initSupervisedModel(supervisedModelPath);

    ///异常检测模型初始化
    initUnsupervisedModel(unsupervisedModelPath);
    /// 卡尺工具初始化

    LOGE("finish model init.");

}


void MainWindow::on_pushButton_readJsonFile_clicked()
{
    QString strFile;
    strFile = ui->lineEdit_jsonFile->text();
    //参数读取
    loadDetToolPara(strFile);

    QMessageBox::warning(this, "读取成功.",
                         "json文件读取成功。", QMessageBox::StandardButton::Ok);

    //更新界面
    updata_control_value();


    //使能界面
    ui->pushButton_runAll->setEnabled(true);
    ui->pushButton_runNext->setEnabled(true);
    ui->pushButton_showImage->setEnabled(true);
    ui->pushButton_showROI->setEnabled(true);

}

void MainWindow::on_pushButton_showImage_clicked()
{
    try
    {
        // LOGE("111");

        QString src_image_dir = ui->lineEdit_imgDir->text();
        // LOGE("src_image_dir:%s", src_image_dir.toStdString().data());

        std:: vector<std::string> src_image_paths;
        // LOGE("333");

        get_files(src_image_dir.toStdString(), &src_image_paths);
        // LOGE("images count:%d.", src_image_paths.size());
        if(src_image_paths.size() < 1)
        {
            LOGE("no images.");
            return;
        }
        tmpSrcImgId += 1;
        // LOGE("444");
        if(tmpSrcImgId > src_image_paths.size() - 1)
        {
            tmpSrcImgId = 0;
        }
        // LOGE("555");
        std::string src_image_path = src_image_paths[tmpSrcImgId];
        cv::Mat src_image;
        // LOGE("src_image_path:%s", src_image_path.data());
        src_image = cv::imread(src_image_path, -1);
        img = src_image;
        // LOGE("777");

        // std::string src_image_path = "E:/data/Wuhan_Painting_Inspection/PaintingDet/D1-20250522/front/1/1.jpg";
        // cv::Mat src_image;
        // // LOGE("src_image_path:%s", src_image_path.data());
        // src_image = cv::imread(src_image_path, -1);
        // img = src_image;
        // // LOGE("777");


        display->myGraphicsView->cleanItem();
        // LOGE("888");

        if(img.empty())
        {
            LOGE("images is empty.");

            return;
        }
        // LOGE("999");

        display->myGraphicsView->setImage(MatToPixmap(img));

    }
    catch(cv::Exception &e)
    {
        LOGE("error:%s", e.what());
    }
    catch(std::exception &e)
    {
        LOGE("error:%s", e.what());
    }
    catch(...)
    {
        LOGE("other error");
    }

}

void MainWindow::on_pushButton_showROI_clicked()
{
    // LOGE("111");
    // qDebug() << "111";
    display->myGraphicsView->cleanItem();
    // LOGE("222");
    // qDebug() << "222";

    if(img.empty())
    {
        LOGE("images is empty.");
        // qDebug() << "images is empty.";

        return;
    }



    //显示图片和卡尺位置
    display->myGraphicsView->setImage(MatToPixmap(img));

    // showCaliperToolROI();
    // if(!caliperResult)
    // {
    //     LOGE("caliperResult false.");
    //     return;
    // }
    //显示检测区域
    // showDetROI();
}


void MainWindow::on_pushButton_runNext_clicked()
{
    // LOGE("111");
    std::string src_image_dir = ui->lineEdit_imgDir->text().toStdString();
    //测试
    std::vector<std::string> src_image_paths;

    // LOGE("222");
    get_files(src_image_dir, &src_image_paths);
    if(src_image_paths.size() < 1)
    {
        LOGE("no images.");
        return;
    }
    tmpSrcImgId += 1;
    // LOGE("333");
    if(tmpSrcImgId > src_image_paths.size() - 1)
    {
        tmpSrcImgId = 0;
    }
    ui->label_id->setText(std::to_string(tmpSrcImgId).data());
    std::string src_image_path = src_image_paths[tmpSrcImgId];
    cv::Mat src_image;
    // LOGE("444");
    src_image = cv::imread(src_image_path, -1);
    img = src_image;

    on_pushButton_run_clicked();


    // LOGE("555");
    //保存裁剪图片
    // if(!detRoiResult)
    // {
    //     LOGE("roi location false.");
    //     return;
    // }
    // // LOGE("666");
    // std::string saveDir = ui->lineEdit_saveDir->text().toStdString();
    // // LOGE("saveDir:%s", saveDir.data());

    // createMultipleFolders(saveDir.data());
    // std::string savePath = (saveDir + "/" + QFileInfo(src_image_path.data()).fileName().toStdString());
    // // LOGE("savePath:%s", savePath.data());
    // cv::imwrite(savePath, ROIImg);

    // LOGE("run finish");
}


void MainWindow::on_pushButton_runAll_clicked()
{
    std::string src_image_dir = ui->lineEdit_imgDir->text().toStdString();
    //测试
    std:: vector<std::string> src_image_paths;
    //            qDebug()<<222;
    get_files(src_image_dir, &src_image_paths);
    tmpSrcImgId = -1;
    for(int id = 0; id < src_image_paths.size(); ++id)
    {
        on_pushButton_runNext_clicked();
    }
    QMessageBox::warning(this, "处理完成。",
                         "处理完成。", QMessageBox::StandardButton::Ok);

}


void MainWindow::on_pushButton_min_clicked()
{
    showMinimized();
}


void MainWindow::on_pushButton_max_clicked()
{
    this->windowState() == Qt::WindowNoState ? showMaximized() : showNormal();
}


void MainWindow::on_pushButton_close_clicked()
{
    this->close();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    QPoint p = mouseEvent->pos();
    if(ui->widget_17->geometry().contains(p))
    {
        mouse_pos = event->globalPos();
        window_pos = this->pos();
        diff_pos = mouse_pos - window_pos;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    QPoint p = mouseEvent->pos();
    if(ui->widget_17->geometry().contains(p))
    {
        QPoint pos = event->globalPos();
        this->move(pos - diff_pos);
    }
}



void MainWindow::on_pushButton_initModel_clicked()
{
    std::string supervisedModelPath = ui->lineEdit_superviseModel->text().toStdString();
    std::string unsupervisedModelPath = ui->lineEdit_unsuperviseModel->text().toStdString();

    initDefectDet(supervisedModelPath, unsupervisedModelPath);
    QMessageBox::warning(this, "打开完成.",
                         "模型文件打开完成。", QMessageBox::StandardButton::Ok);


}


void MainWindow::on_spinBox_maskThre_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    maskThre = arg1;
}




void MainWindow::on_spinBox_cutSize_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    cutSize = arg1;
}


void MainWindow::on_spinBox_overlappingSize_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    overlappingSize = arg1;
}


void MainWindow::on_doubleSpinBox_thre_valueChanged(double arg1)
{
    if(is_loading)
    {
        return;
    }
    thre = arg1;
}


void MainWindow::on_spinBox_filter_defeat_area_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    filter_defect_area = arg1;
}


void MainWindow::on_spinBox_AbnormalCutSize_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    abnormalCutSize = arg1;
}


void MainWindow::on_spinBox_AbnormalOverlappingSize_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    abnormalOverlappingSize = arg1;
}


void MainWindow::on_doubleSpinBox_AbnormalThre_valueChanged(double arg1)
{
    if(is_loading)
    {
        return;
    }
    abnormalThre = arg1;
}


void MainWindow::on_spinBox_filter_defeat_area_abnormal_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    filter_defect_area_abnormal = arg1;
}


void MainWindow::on_spinBox_side_filter_size_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    side_filter_size = arg1;
}


void MainWindow::on_pushButton_savePara_clicked()
{
    QString strFile;
    strFile = ui->lineEdit_jsonFile->text();
    //参数读取
    saveDetToolPara(strFile);

    QMessageBox::warning(this, "保存成功.",
                         "json文件保存成功。", QMessageBox::StandardButton::Ok);
}


void MainWindow::on_checkBox_isUseAbnormal_clicked(bool checked)
{
    if(is_loading)
    {
        return;
    }
    isUseAnomalDet = checked;
}


void MainWindow::on_checkBox_isDefectDet_clicked(bool checked)
{
    if(is_loading)
    {
        return;
    }
    isUseDefectDet = checked;
}


void MainWindow::on_pushButton_run_clicked()
{
    if(img.empty())
    {
        return;
    }

    on_pushButton_showROI_clicked();
    bool isshow = ui->checkBox_isshow->isChecked();
    if(isUseDefectDet)
    {
        runDefectDet(isshow);
    }
    if(isUseAnomalDet)
    {
        runAbnormalDefectDet(isshow);
    }
    //结果显示
    show_result();
}


void MainWindow::on_pushButton_reset_clicked()
{
    //参数重置
    tmpSrcImgId = -1;

    //界面刷新
    ui->label_id->setText(std::to_string(tmpSrcImgId).data());

}


void MainWindow::on_pushButton_runLast_clicked()
{
    //

    // LOGE("111");
    std::string src_image_dir = ui->lineEdit_imgDir->text().toStdString();
    //测试
    std::vector<std::string> src_image_paths;

    // LOGE("222");
    get_files(src_image_dir, &src_image_paths);
    if(src_image_paths.size() < 1)
    {
        LOGE("no images.");
        return;
    }
    tmpSrcImgId -= 1;
    // LOGE("333");
    if(tmpSrcImgId < 0)
    {
        tmpSrcImgId = src_image_paths.size() - 1;
    }
    ui->label_id->setText(std::to_string(tmpSrcImgId).data());
    std::string src_image_path = src_image_paths[tmpSrcImgId];
    cv::Mat src_image;
    // LOGE("444");
    src_image = cv::imread(src_image_path, -1);
    img = src_image;

    on_pushButton_run_clicked();

}

