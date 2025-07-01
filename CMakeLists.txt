cmake_minimum_required(VERSION 3.16)

project(TrainDataPara VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#qt
find_package(Qt5 COMPONENTS Widgets Xml Concurrent Network Sql Charts REQUIRED)

#统一打包
set(LIB_DIR "${PROJECT_SOURCE_DIR}/../lib/x64-win64")
#opencv
include_directories(${PROJECT_SOURCE_DIR}/../include)
file(GLOB OPENCV_LIBS "${LIB_DIR}/opencv/*.lib")

#log display alg（det ad）
include_directories(${PROJECT_SOURCE_DIR}/../include/algorithm)
file(GLOB ALGORITHM_LIBS "${LIB_DIR}/algorithm/*.lib")


set(PROJECT_SOURCES
        Image.qrc
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

add_executable(TrainDataPara
    ${PROJECT_SOURCES}
)
target_link_libraries(TrainDataPara Qt5::Widgets Qt5::Xml Qt5::Concurrent Qt5::Network Qt5::Sql Qt5::Charts)

target_link_libraries(TrainDataPara ${OPENCV_LIBS} ${ALGORITHM_LIBS})
