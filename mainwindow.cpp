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


    jobmanager = Jobmanager(8);

    //
    ui->pushButton_runAll->setEnabled(false);
    ui->pushButton_runNext->setEnabled(false);
    ui->pushButton_showImage->setEnabled(false);
    ui->pushButton_showROI->setEnabled(false);

    setTTF();

    connect(this, &MainWindow::add_log_signal, this, &MainWindow::add_log_even);
    connect(this, &MainWindow::clean_log_signal, this, &MainWindow::clean_log_even);

    connect(&watcher, &QFutureWatcher<QString>::finished,
            this, &MainWindow::runOver);

    ui->groupBox_rubbish->setVisible(false);
    //统计界面初始化
    resetStatisticsData();

}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::showDetROI()
{
    //检测区域
    {
        // 坐标映射

        //
        // QVector<QPointF>Polygon_Points;
        // for(auto j = 0; j < modelROIPointsTransform.size() ; j++)
        // {
        //     Polygon_Points.push_back(QPointF(modelROIPointsTransform[j].x,
        //                                      modelROIPointsTransform[j].y));
        // }
        // myDefectDetROI = new MyGraphicPolygonItem(Polygon_Points,
        //     Qt::blue, NULL,
        //     QString(("defect_det_roi")));

        // myDefectDetROI->m_polygon_pen.setColor(Qt::blue);
        // myDefectDetROI->m_point_pen.setColor(Qt::yellow);
        // display->myGraphicsView->addItem(myDefectDetROI, false);

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








void MainWindow::showResult()
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

        jobmanager.jobworkers[cam_id].getCurrentDefectInfo(save_data_path, save_data_path, is_save_defect_img, defect_scores,
                defect_img_paths, defect_types, defect_locations, defect_areas);

        LOGE("defect_locations size:%d", defect_locations.size());

        for(int id = 0; id < defect_locations.size(); ++id)
        {
            QStringList defect_location_list = QString(defect_locations[id].data()).split(",");
            cv::Rect defect_rect(defect_location_list[0].toInt(), defect_location_list[1].toInt(),
                                 defect_location_list[2].toInt(), defect_location_list[3].toInt());

            int line_wide = 5;
            QGraphicsRectItem *result_item = new QGraphicsRectItem(defect_rect.x / showImgScaleSize, defect_rect.y / showImgScaleSize,
                (defect_rect.width) / showImgScaleSize, (defect_rect.height)  / showImgScaleSize);

            QPen pn(Qt::SolidLine);
            pn.setColor(Qt::red);
            pn.setWidth(line_wide);
            result_item->setPen(pn);
            item_list.push_back(result_item);

            MyTextItem *item_text = new MyTextItem((defect_types[id] + " " + std::to_string(defect_areas[id])).data());


            float scale = std::min((float)display->myGraphicsView->height() / (jobmanager.jobworkers[cam_id].img.rows / showImgScaleSize),
                                   (float)display->myGraphicsView->width() / (jobmanager.jobworkers[cam_id].img.cols / showImgScaleSize));


            item_text->setCustomPos((display->myGraphicsView->width() - (jobmanager.jobworkers[cam_id].img.cols / showImgScaleSize)* scale) / 2
                                    + (defect_rect.x + defect_rect.width) / showImgScaleSize * scale,
                                    (display->myGraphicsView->height() - (jobmanager.jobworkers[cam_id].img.rows / showImgScaleSize)* scale) / 2
                                    + (defect_rect.y + defect_rect.height) / showImgScaleSize * scale);


            item_text->setDefaultTextColor(Qt::red);
            QFont font;
            font.setPointSize(50);
            item_text->setFont(font);

            item_list.push_back(item_text);
        }
        LOGE("finish defect show");


        if(!jobmanager.jobworkers[cam_id].result())
        {
            QGraphicsTextItem *result_string  = new QGraphicsTextItem("NG");
            result_string->setDefaultTextColor(Qt::red);
            QFont font;
            font.setPointSize(qMin(jobmanager.jobworkers[cam_id].img.cols, jobmanager.jobworkers[cam_id].img.rows) / 20 / showImgScaleSize);
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
            font.setPointSize(qMin(jobmanager.jobworkers[cam_id].img.cols, jobmanager.jobworkers[cam_id].img.rows) / 20 / showImgScaleSize);
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

void MainWindow::resetStatisticsData()
{
    //列表清空
    ui->tableWidget_statistics->clear();
    QVector<QString> vec{"相机id", "漏检率", "误检率", "检测总数"};
    QStringList headerLabels = vec.toList();
    ui->tableWidget_statistics->setHorizontalHeaderLabels(headerLabels);
    //添加行
    for(int id = 0; id < jobmanager.jobworkers.size(); ++id)
    {
        ui->tableWidget_statistics->insertRow(0);
    }
    ui->tableWidget_statistics->insertRow(0);

    updataStatisticsData();
}

void MainWindow::updataStatisticsData()
{
    //设置值
    for(int id = 0; id < jobmanager.jobworkers.size(); ++id)
    {

        ui->tableWidget_statistics->setItem(id, 0, new QTableWidgetItem(("Cam" + std::to_string(id + 1)).data())); //设置单元格内容
        ui->tableWidget_statistics->setItem(id, 1, new QTableWidgetItem((to_string_with_precision(jobmanager.jobworkers[id].MissRate() * 100, 3) + "%").data())); //设置单元格内容
        ui->tableWidget_statistics->setItem(id, 2, new QTableWidgetItem((to_string_with_precision(jobmanager.jobworkers[id].FPR() * 100, 3) + "%").data())); //设置单元格内容
        ui->tableWidget_statistics->setItem(id, 3, new QTableWidgetItem(std::to_string(jobmanager.jobworkers[id].Total()).data())); //设置单元格内容
    }

    ui->tableWidget_statistics->setItem(jobmanager.jobworkers.size(), 0, new QTableWidgetItem("总结果")); //设置单元格内容
    ui->tableWidget_statistics->setItem(jobmanager.jobworkers.size(), 1, new QTableWidgetItem((to_string_with_precision(jobmanager.MissRate() * 100, 3) + "%").data())); //设置单元格内容
    ui->tableWidget_statistics->setItem(jobmanager.jobworkers.size(), 2, new QTableWidgetItem((to_string_with_precision(jobmanager.FPR() * 100, 3) + "%").data())); //设置单元格内容
    ui->tableWidget_statistics->setItem(jobmanager.jobworkers.size(), 3, new QTableWidgetItem(std::to_string(jobmanager.Total()).data())); //设置单元格内容

}


void MainWindow::updata_control_value()
{
    is_loading = true;

    //有监督参数
    ui->checkBox_isDefectDet->setChecked(jobmanager.jobworkers[cam_id].isUseDefectDet);
    ui->spinBox_cutSize->setValue(jobmanager.jobworkers[cam_id].cutSize);
    ui->spinBox_overlappingSize->setValue(jobmanager.jobworkers[cam_id].overlappingSize);
    ui->doubleSpinBox_thre->setValue(jobmanager.jobworkers[cam_id].thre);
    ui->spinBox_maskThre->setValue(jobmanager.jobworkers[cam_id].maskThre);
    ui->spinBox_filter_defeat_area->setValue(jobmanager.jobworkers[cam_id].filter_defect_area);



    //无监督参数
    ui->checkBox_isUseAbnormal->setChecked(jobmanager.jobworkers[cam_id].isUseAnomalDet);
    ui->spinBox_AbnormalCutSize ->setValue(jobmanager.jobworkers[cam_id].abnormalCutSize);
    ui->spinBox_AbnormalOverlappingSize->setValue(jobmanager.jobworkers[cam_id].abnormalOverlappingSize);
    ui->doubleSpinBox_AbnormalThre->setValue(jobmanager.jobworkers[cam_id].abnormalThre);
    ui->spinBox_filter_defeat_area_abnormal->setValue(jobmanager.jobworkers[cam_id].filter_defect_area_abnormal);
    ui->spinBox_side_filter_size->setValue(jobmanager.jobworkers[cam_id].side_filter_size);

    is_loading = false;
}

void MainWindow::runOver()
{
    showResult();

    //总结果显示
    if(jobmanager.result())
    {
        ui->label_result->setStyleSheet("QLabel{color:rgb(0,255,0);font: 700 56pt \"Microsoft YaHei UI\";}");
        ui->label_result->setText("OK");
    }
    else
    {
        ui->label_result->setStyleSheet("QLabel{color:rgb(255,0,0);font: 700 56pt \"Microsoft YaHei UI\";}");
        ui->label_result->setText("NG");
    }
    //结果统计
    for(int id = 0; id < jobmanager.jobworkers.size(); ++id)
    {
        jobmanager.jobworkers[id].statistics();

    }
    jobmanager.statistics();

    //统计界面更新
    updataStatisticsData();
}



void MainWindow::on_pushButton_readJsonFile_clicked()
{
    QString strFile;
    strFile = ui->lineEdit_jsonFile->text();
    //参数读取
    for(int id = 0; id < jobmanager.jobworkers.size(); ++id)
    {
        std::string jsonFilePath = strFile.toStdString() + "/cam" + std::to_string(id + 2) + "_DetPara.json";
        jobmanager.jobworkers[id].loadDetToolPara(jsonFilePath);

    }

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
        tmpSrcImgIdList[cam_id] += 1;
        // LOGE("444");
        if(tmpSrcImgIdList[cam_id] > src_image_paths.size() - 1)
        {
            tmpSrcImgIdList[cam_id] = 0;
        }
        // LOGE("555");
        std::string src_image_path = src_image_paths[tmpSrcImgIdList[cam_id]];
        cv::Mat src_image;
        // LOGE("src_image_path:%s", src_image_path.data());
        src_image = cv::imread(src_image_path, -1);
        jobmanager.jobworkers[cam_id].img = src_image;


        display->myGraphicsView->cleanItem();
        // LOGE("888");

        if(jobmanager.jobworkers[cam_id].img.empty())
        {
            LOGE("images is empty.");

            return;
        }
        // LOGE("999");

        display->myGraphicsView->setImage(MatToPixmap(jobmanager.jobworkers[cam_id].img));

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

    if(jobmanager.jobworkers[cam_id].img.empty())
    {
        LOGE("images is empty.");
        // qDebug() << "images is empty.";

        return;
    }



    //显示图片和卡尺位置
    display->myGraphicsView->setImage(MatToPixmap(jobmanager.jobworkers[cam_id].img));

}


void MainWindow::on_pushButton_runNext_clicked()
{
    // LOGE("111");
    std::string src_image_dir = ui->lineEdit_imgDir->text().toStdString();


    for(int id = 0; id < jobmanager.jobworkers.size(); ++id)
    {
        std::string cam_image_dir  = src_image_dir + "/Cam" + std::to_string(id + 1) + "/origin";
        //测试
        std::vector<std::string> src_image_paths;

        // LOGE("222");
        get_files(cam_image_dir, &src_image_paths);
        if(src_image_paths.size() < 1)
        {
            LOGE("no images.");
            return;
        }
        tmpSrcImgIdList[id] += 1;
        // LOGE("333");
        if(tmpSrcImgIdList[id] > src_image_paths.size() - 1)
        {
            tmpSrcImgIdList[id] = 0;
        }
        std::string src_image_path = src_image_paths[tmpSrcImgIdList[id]];
        cv::Mat src_image;
        // LOGE("444");
        src_image = cv::imread(src_image_path, -1);
        jobmanager.jobworkers[id].img = src_image;

    }

    ui->label_id->setText(std::to_string(tmpSrcImgIdList[cam_id]).data());

    on_pushButton_run_clicked();

}


void MainWindow::on_pushButton_runAll_clicked()
{
    // std::string src_image_dir = ui->lineEdit_imgDir->text().toStdString();
    // //测试
    // std:: vector<std::string> src_image_paths;
    // //            qDebug()<<222;
    // get_files(src_image_dir, &src_image_paths);

    // tmpSrcImgIdList[cam_id] = -1;

    // for(int id = 0; id < src_image_paths.size(); ++id)
    // {
    //     on_pushButton_runNext_clicked();
    // }
    // QMessageBox::warning(this, "处理完成。",
    //                      "处理完成。", QMessageBox::StandardButton::Ok);

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
    std::string supervisedModelDir = ui->lineEdit_superviseModel->text().toStdString();
    std::string unsupervisedModelDir = ui->lineEdit_unsuperviseModel->text().toStdString();


    for(int id = 0; id < jobmanager.jobworkers.size(); ++id)
    {
        std::string supervisedModelPath = supervisedModelDir + "/cam" + std::to_string(id + 1) + "/best.wts";

        std::string unsupervisedModelPath = unsupervisedModelDir + "/cam" + std::to_string(id + 1) + "/best.onnx";


        jobmanager.jobworkers[id].initDefectDet(supervisedModelPath, unsupervisedModelPath);

    }

    QMessageBox::warning(this, "打开完成.",
                         "模型文件打开完成。", QMessageBox::StandardButton::Ok);


}


void MainWindow::on_spinBox_maskThre_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    jobmanager.jobworkers[cam_id].maskThre = arg1;
}




void MainWindow::on_spinBox_cutSize_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    jobmanager.jobworkers[cam_id].cutSize = arg1;
}


void MainWindow::on_spinBox_overlappingSize_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    jobmanager.jobworkers[cam_id].overlappingSize = arg1;
}


void MainWindow::on_doubleSpinBox_thre_valueChanged(double arg1)
{
    if(is_loading)
    {
        return;
    }
    jobmanager.jobworkers[cam_id].thre = arg1;
}


void MainWindow::on_spinBox_filter_defeat_area_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    jobmanager.jobworkers[cam_id].filter_defect_area = arg1;
}


void MainWindow::on_spinBox_AbnormalCutSize_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    jobmanager.jobworkers[cam_id].abnormalCutSize = arg1;
}


void MainWindow::on_spinBox_AbnormalOverlappingSize_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    jobmanager.jobworkers[cam_id].abnormalOverlappingSize = arg1;
}


void MainWindow::on_doubleSpinBox_AbnormalThre_valueChanged(double arg1)
{
    if(is_loading)
    {
        return;
    }
    jobmanager.jobworkers[cam_id].abnormalThre = arg1;
}


void MainWindow::on_spinBox_filter_defeat_area_abnormal_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    jobmanager.jobworkers[cam_id].filter_defect_area_abnormal = arg1;
}


void MainWindow::on_spinBox_side_filter_size_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    jobmanager.jobworkers[cam_id].side_filter_size = arg1;
}


void MainWindow::on_pushButton_savePara_clicked()
{
    QString strFile;
    strFile = ui->lineEdit_jsonFile->text();
    //参数读取
    for(int id = 0; id < jobmanager.jobworkers.size(); ++id)
    {
        std::string jsonFilePath = strFile.toStdString() + "/cam" + std::to_string(id + 2) + "_DetPara.json";
        jobmanager.jobworkers[id].saveDetToolPara(jsonFilePath);

    }
    QMessageBox::warning(this, "保存成功.",
                         "json文件保存成功。", QMessageBox::StandardButton::Ok);
}


void MainWindow::on_checkBox_isUseAbnormal_clicked(bool checked)
{
    if(is_loading)
    {
        return;
    }
    jobmanager.jobworkers[cam_id].isUseAnomalDet = checked;
}


void MainWindow::on_checkBox_isDefectDet_clicked(bool checked)
{
    if(is_loading)
    {
        return;
    }
    jobmanager.jobworkers[cam_id].isUseDefectDet = checked;
}


void MainWindow::on_pushButton_run_clicked()
{
    //清空界面
    ui->label_result->setStyleSheet("QLabel{color:rgb(128,128,128); font: 700 36pt \"Microsoft YaHei UI\";}");
    ui->label_result->setText("等待中");

    on_pushButton_showROI_clicked();

    QFuture<void> f = QtConcurrent::run([ = ]()
    {
        for(int id = 0; id < jobmanager.jobworkers.size(); ++id)
        {
            if(jobmanager.jobworkers[id].img.empty())
            {
                return;
            }

            if(cam_id == id)
            {
                bool isshow = ui->checkBox_isshow->isChecked();
                jobmanager.jobworkers[id].run(isshow);

            }
            else
            {
                jobmanager.jobworkers[id].run(false);

            }
        }

    });

    watcher.setFuture(f);
}


void MainWindow::on_pushButton_reset_clicked()
{
    //参数重置
    // tmpSrcImgId = -1;
    for(int id = 0; id < jobmanager.jobworkers.size(); ++id)
    {
        tmpSrcImgIdList[id] = -1;
        jobmanager.jobworkers[id].resetStatistics();
    }
    jobmanager.resetStatistics();

    //界面刷新
    ui->label_id->setText(std::to_string(tmpSrcImgIdList[cam_id]).data());

    ui->label_result->setStyleSheet("QLabel{color:rgb(128,128,128); font: 700 56pt \"Microsoft YaHei UI\";}");
    ui->label_result->setText("--");

    resetStatisticsData();

}


void MainWindow::on_pushButton_runLast_clicked()
{
    //

    // LOGE("111");
    std::string src_image_dir = ui->lineEdit_imgDir->text().toStdString();

    for(int id = 0; id < jobmanager.jobworkers.size(); ++id)
    {
        std::string cam_image_dir  = src_image_dir + "/Cam" + std::to_string(id + 1) + "/origin";
        //测试
        std::vector<std::string> src_image_paths;

        // LOGE("222");
        get_files(cam_image_dir, &src_image_paths);
        if(src_image_paths.size() < 1)
        {
            LOGE("no images.");
            return;
        }
        tmpSrcImgIdList[id] -= 1;
        // LOGE("333");
        if(tmpSrcImgIdList[id] < 0)
        {
            tmpSrcImgIdList[id] = src_image_paths.size() - 1;
        }
        std::string src_image_path = src_image_paths[tmpSrcImgIdList[id]];
        cv::Mat src_image;
        // LOGE("444");
        src_image = cv::imread(src_image_path, -1);
        jobmanager.jobworkers[id].img = src_image;

    }

    ui->label_id->setText(std::to_string(tmpSrcImgIdList[cam_id]).data());


    on_pushButton_run_clicked();

}


void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    cam_id = index;
    updata_control_value();

    //图片及结果显示
    on_pushButton_showROI_clicked();
    //结果显示
    showResult();
}

