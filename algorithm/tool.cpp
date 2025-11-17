#include "tool.h"
#include "mylog.h"


bool appendCsvLine(const QString &filePath,
                   const QStringList &cells,
                   QChar sep)
{
    QStringList escaped;
    escaped.reserve(cells.size());

    for(QString cell : cells)
    {
        // // 是否需要引号包围
        // if(cell.contains(sep) || cell.contains(u'"') || cell.contains(u'\n') || cell.contains(u'\r'))
        // {
        //     cell.replace(u'"', u"\"\"");          // 内部 " 变 ""
        //     cell = u'"' + cell + u'"';
        // }
        escaped << cell;
    }

    QFile f(filePath);
    if(!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        return false;
    }

    QTextStream ts(&f);
    ts.setCodec("UTF-8");
    ts << escaped.join(sep) << Qt::endl;
    return true;
}

template <typename T>
std::string to_string_with_precision(const T a_value, const int n)
{
    int nn = n ;
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out << std::setprecision(nn) << a_value;
    return out.str();
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



void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // 防止重复替换
    }
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


