// Copyright (c) Orbbec Inc. All Rights Reserved.
// Licensed under the MIT License.

#include <libobsensor/ObSensor.hpp>

#include "utils.hpp"
#include "utils_opencv.hpp"
#include <iostream>
#include <iomanip>
#include <mutex>
#include <thread>
#include <atomic>
#include <map>
#include <chrono>
#include <string>
#include <vector>
#include <ctime>
#include <signal.h>
#include <fstream>
#include <cstdlib>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/statvfs.h>
#endif

std::atomic<bool> g_running{true};
void signal_handler(int) { g_running = false; }

void make_dir(const std::string& path) {
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0755);
#endif
}

std::string get_timestamp_string() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

void check_disk_space(const std::string& path) {
#ifndef _WIN32
    struct statvfs fiData;
    if((statvfs(path.c_str(), &fiData)) >= 0) {
        long long free_space = (long long)fiData.f_bsize * fiData.f_bfree;
        double gb = free_space / (1024.0 * 1024.0 * 1024.0);
        if (gb < 10.0) {
            std::cout << "\033[1;31m[CRITICAL] Low disk space! Only " << std::fixed << std::setprecision(2) << gb << " GB remaining.\033[0m" << std::endl;
        } else {
            std::cout << "[INFO] Disk space: " << std::fixed << std::setprecision(2) << gb << " GB available." << std::endl;
        }
    }
#else
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceExA(path.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
        double gb = freeBytesAvailable.QuadPart / (1024.0 * 1024.0 * 1024.0);
        if (gb < 10.0) {
            std::cout << "[CRITICAL] Low disk space! Only " << std::fixed << std::setprecision(2) << gb << " GB remaining." << std::endl;
        } else {
            std::cout << "[INFO] Disk space: " << std::fixed << std::setprecision(2) << gb << " GB available." << std::endl;
        }
    }
#endif
}

int main(int argc, char** argv) try {
    signal(SIGINT, signal_handler);

    int duration_s = 60;
    int max_batches = 0;
    std::string output_dir = "recordings";
    std::string post_process_cmd = "";
    bool simulate = false;
    bool no_gui = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--duration") duration_s = std::stoi(argv[++i]);
        else if (arg == "--count") max_batches = std::stoi(argv[++i]);
        else if (arg == "--output") output_dir = argv[++i];
        else if (arg == "--post-process") post_process_cmd = argv[++i];
        else if (arg == "--simulate") simulate = true;
        else if (arg == "--no-gui") no_gui = true;
        else if (arg == "--help") {
            std::cout << "Usage: ob_batch_recorder [--duration sec] [--count n] [--output dir] [--post-process cmd] [--simulate] [--no-gui]" << std::endl;
            return 0;
        }
    }

    make_dir(output_dir);
    std::cout << "====================================================" << std::endl;
    std::cout << "   ORBBEC BATCH RECORDER (Windows Optimized)        " << std::endl;
    if (simulate) std::cout << "   *** SIMULATION MODE: Writing dummy files ***     " << std::endl;
    std::cout << "====================================================" << std::endl;
    std::cout << " Config: " << duration_s << "s per file | " << (max_batches==0?"Infinite":std::to_string(max_batches)) << " files" << std::endl;
    check_disk_space(".");

    std::shared_ptr<ob::Context> context;
    std::shared_ptr<ob::Device> device;
    std::shared_ptr<ob::Pipeline> pipe;
    std::mutex frameMutex;
    std::shared_ptr<const ob::FrameSet> renderFrameSet;

    if (!simulate) {
        context = std::make_shared<ob::Context>();
        auto deviceList = context->queryDeviceList();
        if(deviceList->getCount() < 1) {
            std::cout << "\n[ERROR] No camera found. Connect Orbbec camera or use --simulate to test." << std::endl;
            return EXIT_FAILURE;
        }
        device = deviceList->getDevice(0);
        pipe = std::make_shared<ob::Pipeline>(device);
        std::shared_ptr<ob::Config> config = std::make_shared<ob::Config>();
        
        // Depth Y16 30FPS
        try {
            auto profiles = device->getSensor(OB_SENSOR_DEPTH)->getStreamProfileList();
            for(uint32_t i=0; i<profiles->getCount(); i++) {
                auto p = profiles->getProfile(i)->as<ob::VideoStreamProfile>();
                if(p->format() == OB_FORMAT_Y16 && p->fps() == 30) {
                    config->enableStream(p); 
                    break;
                }
            }
        } catch(...) {}

        // Color 30FPS
        try {
            auto profiles = device->getSensor(OB_SENSOR_COLOR)->getStreamProfileList();
            for(uint32_t i=0; i<profiles->getCount(); i++) {
                auto p = profiles->getProfile(i)->as<ob::VideoStreamProfile>();
                if(p->fps() == 30) {
                    config->enableStream(p);
                    break;
                }
            }
        } catch(...) {}

        pipe->start(config, [&](std::shared_ptr<ob::FrameSet> frameSet) {
            if (!no_gui) {
                std::lock_guard<std::mutex> lock(frameMutex);
                renderFrameSet = frameSet;
            }
        });
    }

    std::unique_ptr<ob_smpl::CVWindow> win;
    if (!no_gui) {
        win = std::unique_ptr<ob_smpl::CVWindow>(new ob_smpl::CVWindow("Batch Recorder Feed", 1280, 720, ob_smpl::ARRANGE_GRID));
        win->setKeyPressedCallback([&](int key) {
            if (key == 'q' || key == 'Q') {
                g_running = false;
                win->close();
            }
        });
    }

    int current_batch = 0;
    while (g_running && (max_batches == 0 || current_batch < max_batches)) {
        current_batch++;
        std::string filename = output_dir + "/rec_" + get_timestamp_string() + "_B" + std::to_string(current_batch) + ".bag";
        std::cout << "\n>>> [BATCH " << current_batch << "] Starting: " << filename << std::endl;

        std::shared_ptr<ob::RecordDevice> recordDevice;
        if (!simulate) {
            recordDevice = std::make_shared<ob::RecordDevice>(device, filename);
        } else {
            std::ofstream dummy(filename);
            dummy << "Simulation of " << duration_s << " seconds recording." << std::endl;
            dummy.close();
        }

        auto start = std::chrono::steady_clock::now();
        while (g_running) {
            if (!no_gui && win) {
                if (!win->run()) {
                    g_running = false;
                    break;
                }
            } else {
                // If no GUI, we need to handle termination via Ctrl+C
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
            if (elapsed >= duration_s) break;
            
            if (!simulate && !no_gui && win) {
                std::lock_guard<std::mutex> lock(frameMutex);
                if (renderFrameSet) {
                    win->pushFramesToView(renderFrameSet);
                }
            }
            
            std::cout << "    Progress: " << elapsed << "s / " << duration_s << "s recording..." << "\r" << std::flush;
            if (no_gui) std::this_thread::sleep_for(std::chrono::milliseconds(100));
            else std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if (!simulate) recordDevice = nullptr; 
        std::cout << "    [COMPLETED] Batch " << current_batch << " saved.                     " << std::endl;
        
        if (!post_process_cmd.empty()) {
            std::string cmd = post_process_cmd + " " + filename;
            std::cout << "    [EXEC] " << cmd << std::endl;
            // Run in background on Windows using start or just system
            #ifdef _WIN32
            std::string win_cmd = "start /B " + cmd;
            std::system(win_cmd.c_str());
            #else
            std::string unix_cmd = cmd + " &";
            std::system(unix_cmd.c_str());
            #endif
        }

        check_disk_space(".");
        if (!g_running) break;
    }

    if (!simulate && pipe) pipe->stop();
    std::cout << "\n====================================================" << std::endl;
    std::cout << " Finished. Check the '" << output_dir << "' folder." << std::endl;
    std::cout << "====================================================" << std::endl;
    return 0;
}
catch(ob::Error &e) { std::cerr << "\nSDK ERROR: " << e.what() << std::endl; return 1; }
catch(const std::exception& e) { std::cerr << "\nSYSTEM ERROR: " << e.what() << std::endl; return 1; }

