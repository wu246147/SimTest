#ifndef TOOL_H
#define TOOL_H
#include <opencv2/opencv.hpp>
#include "imgprocesstool.h"
#include "QImage"
#include "QDir"
#include "QPixmap"
#include "QException"

///
/// \brief MatToQImage Mat 转 QImage
/// \param cvImage
/// \return
///
QImage MatToQImage(const cv::Mat &cvImage);

///
/// \brief MatToPixmap  Mat 转 Pixmap
/// \param cvImage
/// \return
///
QPixmap MatToPixmap(const cv::Mat &cvImage);

///
/// \brief createMultipleFolders 创建多层文件夹
/// \param path
/// \return
///
QString createMultipleFolders(const QString path);

///
/// \brief MainWindow::get_files 获取文件夹下所有文件路径
/// \param dir
/// \param file_paths
///
void get_files(std::string dir, std::vector<std::string> *file_paths);


void replaceAll(std::string& str, const std::string& from, const std::string& to);

void getModelROIPointTransform(cv::Mat H, float angle, std::vector<cv::Point2f> modelROIPoints, std::vector<cv::Point2f> &modelROIPointsTransform);

#endif // TOOL_H
