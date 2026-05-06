cmake_minimum_required(VERSION 3.10)
project(WindowsOrbbecRecorder)

set(CMAKE_CXX_STANDARD 11)

# --- Find OpenCV ---
# User should set OpenCV_DIR to where OpenCVConfig.cmake is located
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# --- Find OrbbecSDK ---
# User should set OrbbecSDK_DIR to where OrbbecSDKConfig.cmake is located
# Usually in the 'lib/cmake/OrbbecSDK' or similar in the Windows SDK
find_package(OrbbecSDK REQUIRED)

add_executable(ob_batch_recorder 
    src/main.cpp 
    src/utils.cpp 
    src/utils_c.c
    src/utils_opencv.cpp
)

target_include_directories(ob_batch_recorder PRIVATE src)
target_link_libraries(ob_batch_recorder PRIVATE ${OpenCV_LIBS} ob::OrbbecSDK)

# Post-build: Copy DLLs to bin directory
if(MSVC)
    add_custom_command(TARGET ob_batch_recorder POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ob::OrbbecSDK>
        $<TARGET_FILE_DIR:ob_batch_recorder>
    )
endif()
