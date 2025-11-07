cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
set(CMAKE_CUDA_ARCHITECTURES 70 75 80)
endif(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)

project(SimTest LANGUAGES CXX VERSION 2.2.0)

# # 项目版本
# configure_file(
#     "${CMAKE_SOURCE_DIR}/version.rc.in"
#     "${CMAKE_BINARY_DIR}/version.rc"
# )

add_compile_definitions(NOMINMAX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_definitions(-std=c++11)
add_definitions(-DAPI_EXPORTS)
option(CUDA_USE_STATIC_CUDA_RUNTIME OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#默认输出所有动态库函数
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

#qt
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
set(Qt5_DIR   "/home/dell/Qt5.14.2/5.14.2/gcc_64/lib/cmake/Qt5")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
endif()
find_package(Qt5 COMPONENTS Widgets Xml Concurrent Network Sql Charts REQUIRED)


#Threads
Set(THREADS PREFER PTHREAD FLAG ON)
find_package(Threads)

#cuda
find_package(CUDA REQUIRED)
message(STATUS " libraries: ${CUDA_LIBRARIES}")
message(STATUS " include path: ${CUDA_INCLUDE_DIRS}")
include_directories(${CUDA_INCLUDE_DIRS})
###
enable_language(CUDA) # add this line, then no need to setup cuda path in vs
###

# tensorrt
set(TRT_DIR "E:/env/dll/tensorRT/TensorRT-8.6.1.6")
set(TRT_INCLUDE_DIRS ${TRT_DIR}\\include) #8
set(TRT_LIB_DIRS ${TRT_DIR}\\lib) #9
include_directories(${TRT_INCLUDE_DIRS})
#添加trt路径，后面直接调用nvinfer。否则需要添加它的库
link_directories(${TRT_DIR}/lib)

# set(TRT_LIB "E:/env/dll/tensorRT/TensorRT-8.6.1.6/lib/nvinfer.lib")
# link_libraries(${TRT_LIB})

#统一打包
set(LIB_DIR "${PROJECT_SOURCE_DIR}/lib/x64-win64")

#opencv
include_directories(${PROJECT_SOURCE_DIR}/include)
file(GLOB OPENCV_LIBS "${LIB_DIR}/opencv/*.lib")

# message(${PROJECT_SOURCE_DIR}/../Painting_Defect_Detect/include)

#log display alg（det ad）
include_directories(${PROJECT_SOURCE_DIR}/include/algorithm)
file(GLOB ALGORITHM_LIBS "${LIB_DIR}/algorithm/*.lib")


set(PROJECT_SOURCES
        Image.qrc
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        algorithm/qtstreambuf.h
        algorithm/qtstreambuf.cpp
        mytextitem.cpp
        mytextitem.h
)

add_executable(SimTest
    ${PROJECT_SOURCES}
)


target_link_libraries(SimTest Qt5::Widgets Qt5::Xml Qt5::Concurrent Qt5::Network Qt5::Sql Qt5::Charts)
target_link_libraries(SimTest  ${OPENCV_LIBS} ${ALGORITHM_LIBS} )
target_link_libraries(SimTest nvinfer)
target_link_libraries(SimTest ${CUDA_LIBRARIES})
target_link_libraries(SimTest Threads::Threads)

