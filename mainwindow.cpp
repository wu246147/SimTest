#include "mainwindow.h"
#include "./ui_mainwindow.h"

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

void MainWindow::runCaliper(bool isShow = false)
{
    try
    {
        ;
        cv::Mat showImg;
        caliperResult = true;
        detRoiResult = true;
        caliperResults.clear();
        caliperLocationOriginPoints.clear();
        auto start = std::chrono::high_resolution_clock::now();
        ///图片预处理
        cv::Mat gray;
        std::vector<cv::Mat> channels;
        std::vector<cv::RotatedRect>ROIList;                    //定位卡尺

        split(img, channels);
        gray = channels[0];
        // cv::cvtColor(img, gray, COLOR_BGR2GRAY);
        cv::Mat reductImg;
        cv::resize(gray, reductImg, img.size() / 8, 0, 0, cv::INTER_NEAREST);
        cv::Mat medianImg;
        cv::medianBlur(reductImg, medianImg, 7);

        for(int id = 0; id < caliperToolROIs.size(); ++id)
        {
            cv::RotatedRect caliperROISmall =  cv::RotatedRect(caliperToolROIs[id].center / 8,
                                               cv::Size2f(caliperToolROIs[id].size.width / 8, caliperToolROIs[id].size.height / 8),
                                               caliperToolROIs[id].angle);

            ROIList.push_back(caliperROISmall);
        }
        /// 卡尺边缘定位
        CaliperLocation(medianImg, ROIList, caliperPolarityList, caliperSelectPositionList,
                        caliperThreList, locationAngle, locationPoint, caliperResults, caliperLocationOriginPoints);
        /// 结果显示
        if(isShow)
        {
            showImg = img.clone();
            for(int id = 0; id < ROIList.size(); ++id)
            {
                if(caliperResults[id])
                {
                    auto caliperLocationOriginPoint = caliperLocationOriginPoints[id] * 8;
                    cv::circle(showImg, caliperLocationOriginPoint, 32, cv::Scalar(0, 0, 255), 16);
                    // std::cout << "caliperLocationOriginPoint:(" << caliperLocationOriginPoint.x << ","
                    // << caliperLocationOriginPoint.y << ")" << std::endl;
                }
            }
            if(std::count(caliperResults.begin(), caliperResults.end(), false) == 0)
            {
                // std::cout << "locationAngle:" << locationAngle << std::endl;
                auto originLocationPoint = locationPoint * 8;

                // std::cout << "originLocationPoint:(" << originLocationPoint.x << "," << originLocationPoint.y << ")" << std::endl;

                cv::circle(showImg, originLocationPoint, 32, cv::Scalar(0, 255, 0), 16);
            }
            else
            {
                caliperResult = false;
                detRoiResult = false;

                return;
            }
            cv::namedWindow("showImg", cv::WINDOW_NORMAL);
            cv::imshow("showImg", showImg);
            cv::waitKey(0);
        }
        /// ROI矫正
        modelROITransform.size = modelROI.size;
        // anormalROICorrect.center = anormalROI.center + locationPoint * 8;
        modelROITransform.angle = modelROI.angle + locationAngle;
        cv::Mat H;
        getTransform((locationPoint * 8).x, (locationPoint * 8).y, locationAngle, cv::Point2f(0, 0), H);
        cv::Point2f abnormalROICorrectCenter;
        transformPoint(H, modelROI.center, abnormalROICorrectCenter);
        modelROITransform.center = abnormalROICorrectCenter;

        if(isShow)
        {
            cv::Point2f vertices[4];
            modelROITransform.points(vertices);
            for(int i = 0; i < 4; i++)
            {
                line(showImg, vertices[i], vertices[(i + 1) % 4], cv::Scalar(0, 255, 0), 4);
            }
            cv::namedWindow("showImg", cv::WINDOW_NORMAL);
            cv::imshow("showImg", showImg);
            cv::waitKey(0);
        }
        /// 提取图片

        // 判断roi是否超出图像范围，如果是，则有可能检测区域不再视野内，直接判为ng
        cv::Rect boundingRect = modelROITransform.boundingRect();

        if(boundingRect.x < 0 || boundingRect.y < 0
                || boundingRect.br().x >= img.cols || boundingRect.br().y >= img.rows)
        {
            // LOGE("modelROITransform out of image");
            detRoiResult = false;
            return;
        }
        //截取图片
        transformImgFromRotateRect(modelROITransform, img, ROIImg);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        // LOGE("Caliper location image total use time: %d ms", duration);
        // LOGE("-----------------------------------");

    }
    catch(cv::Exception &e)
    {
        caliperResult = false;
        detRoiResult = false;
        LOGE_HIGH(e.what());
    }
    catch(std::exception &e)
    {
        caliperResult = false;
        detRoiResult = false;

        LOGE_HIGH(e.what());
    }
    catch(...)
    {
        caliperResult = false;
        detRoiResult = false;

        LOGE_HIGH("other error.");
    }
}

void MainWindow::showDetROI()
{
    //检测区域
    {
        if(caliperResult)
        {
            QRectF tool_rect(modelROITransform.center.x - modelROITransform.size.width / 2,
                             modelROITransform.center.y - modelROITransform.size.height / 2,
                             modelROITransform.size.width,
                             modelROITransform.size.height);
            this->myRectItem_location = new MyGraphicRectItem(tool_rect, modelROITransform.angle);
            this->myRectItem_location->setToolTip("defect_det_roi");
            this->myRectItem_location->SetColor(Qt::blue);
            this->myRectItem_location->setRotatable(true);
            display->myGraphicsView->addItem(myRectItem_location, false);
        }
    }
}
void MainWindow::showCaliperToolROI()
{
    {
        // 卡尺1
        {
            QRectF tool_rect(caliperToolROIs[0].center.x - caliperToolROIs[0].size.width / 2,
                             caliperToolROIs[0].center.y - caliperToolROIs[0].size.height / 2,
                             caliperToolROIs[0].size.width,
                             caliperToolROIs[0].size.height);
            this->myRectItem_caliperROI1 = new MyGraphicRectItem(tool_rect, caliperToolROIs[0].angle);
            this->myRectItem_caliperROI1->setToolTip("caliperToolRoi 1");
            this->myRectItem_caliperROI1->SetColor(Qt::green);
            this->myRectItem_caliperROI1->setRotatable(true);
            display->myGraphicsView->addItem(myRectItem_caliperROI1, false);
        }
        // 卡尺2
        {
            QRectF tool_rect(caliperToolROIs[1].center.x - caliperToolROIs[1].size.width / 2,
                             caliperToolROIs[1].center.y - caliperToolROIs[1].size.height / 2,
                             caliperToolROIs[1].size.width,
                             caliperToolROIs[1].size.height);
            this->myRectItem_caliperROI2 = new MyGraphicRectItem(tool_rect, caliperToolROIs[1].angle);
            this->myRectItem_caliperROI2->setToolTip("caliperToolRoi 2");
            this->myRectItem_caliperROI2->SetColor(Qt::green);
            this->myRectItem_caliperROI2->setRotatable(true);
            display->myGraphicsView->addItem(myRectItem_caliperROI2, false);
        }
        // 卡尺3
        {
            QRectF tool_rect(caliperToolROIs[2].center.x - caliperToolROIs[2].size.width / 2,
                             caliperToolROIs[2].center.y - caliperToolROIs[2].size.height / 2,
                             caliperToolROIs[2].size.width,
                             caliperToolROIs[2].size.height);
            this->myRectItem_caliperROI3 = new MyGraphicRectItem(tool_rect, caliperToolROIs[2].angle);
            this->myRectItem_caliperROI3->setToolTip("caliperToolRoi 3");
            this->myRectItem_caliperROI3->SetColor(Qt::green);
            this->myRectItem_caliperROI3->setRotatable(true);
            display->myGraphicsView->addItem(myRectItem_caliperROI3, false);
        }
        // 卡尺4
        {
            QRectF tool_rect(caliperToolROIs[3].center.x - caliperToolROIs[3].size.width / 2,
                             caliperToolROIs[3].center.y - caliperToolROIs[3].size.height / 2,
                             caliperToolROIs[3].size.width,
                             caliperToolROIs[3].size.height);
            this->myRectItem_caliperROI4 = new MyGraphicRectItem(tool_rect, caliperToolROIs[3].angle);
            this->myRectItem_caliperROI4->setToolTip("caliperToolRoi 4");
            this->myRectItem_caliperROI4->SetColor(Qt::green);
            this->myRectItem_caliperROI4->setRotatable(true);
            display->myGraphicsView->addItem(myRectItem_caliperROI4, false);
        }
    }
}

void MainWindow::setTTF()
{
    std::string ttf_path = "./qss/iconfont.ttf";
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

void MainWindow::on_pushButton_readJsonFile_clicked()
{
    QString strFile;
    strFile = ui->lineEdit_jsonFile->text();
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

    QJsonArray modelROIJosn = mJsonObject["modelROI"].toArray();

    modelROI.center.x = modelROIJosn[0].toDouble();
    modelROI.center.y = modelROIJosn[1].toDouble();
    modelROI.size.width = modelROIJosn[2].toDouble();
    modelROI.size.height = modelROIJosn[3].toDouble();
    modelROI.angle = modelROIJosn[4].toDouble();

    QJsonArray caliperToolROISJson = mJsonObject["caliperToolROIS"].toArray();
    for(int id = 0; id < 4; ++id)
    {
        QJsonObject caliperToolROIJson = caliperToolROISJson[id].toObject();

        caliperToolROIs[id].center.x   = caliperToolROIJson["caliperToolROIX"].toDouble();
        caliperToolROIs[id].center.y   = caliperToolROIJson["caliperToolROIY"].toDouble();
        caliperToolROIs[id].size.width = caliperToolROIJson["caliperToolROIW"].toDouble();
        caliperToolROIs[id].size.height = caliperToolROIJson["caliperToolROIH"].toDouble();
        caliperToolROIs[id].angle  = caliperToolROIJson["caliperToolROIA"].toDouble();

        caliperPolarityList[id] = caliperToolROIJson["caliperToolROIP"].toInt();
        caliperSelectPositionList[id] = caliperToolROIJson["caliperToolROIS"].toInt();
        caliperThreList[id] = caliperToolROIJson["caliperToolROIT"].toInt();

    }

    QMessageBox::warning(this, "读取成功.",
                         "json文件读取成功。", QMessageBox::StandardButton::Ok);

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
    // LOGE("333");
    // qDebug() << "333";

    //运行卡尺定位
    runCaliper(false);
    //显示定位结果

    //显示图片和卡尺位置
    display->myGraphicsView->setImage(MatToPixmap(img));

    showCaliperToolROI();
    if(!caliperResult)
    {
        LOGE("caliperResult false.");
        return;
    }
    //显示检测区域
    showDetROI();
}


void MainWindow::on_pushButton_runNext_clicked()
{
    // LOGE("111");
    std::string src_image_dir = ui->lineEdit_imgDir->text().toStdString();
    //测试
    std:: vector<std::string> src_image_paths;

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

    on_pushButton_showROI_clicked();
    // LOGE("555");
    //保存裁剪图片
    if(!detRoiResult)
    {
        LOGE("roi location false.");
        return;
    }
    // LOGE("666");
    std::string saveDir = ui->lineEdit_saveDir->text().toStdString();
    // LOGE("saveDir:%s", saveDir.data());

    createMultipleFolders(saveDir.data());
    std::string savePath = (saveDir + "/" + QFileInfo(src_image_path.data()).fileName().toStdString());
    // LOGE("savePath:%s", savePath.data());
    cv::imwrite(savePath, ROIImg);

    LOGE("run finish");
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


