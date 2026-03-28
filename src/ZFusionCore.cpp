#include "ZFusionCore.h"




bool RunZROI(ZPiCap& zcap, int timeout_ms) {
    auto start_time = std::chrono::steady_clock::now();
    const cv::Mat& liveFrame = zcap.GetMainFrame();

    std::cout << "[Fusion] Starting roi capture sequence..." << std::endl;

    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();

        if (elapsed > timeout_ms) {
            std::cerr << "[Fusion] ROI Capture timeout reached." << std::endl;
            return "";
        }

        if (zcap.Read()) {
            if (liveFrame.empty()) continue;

            std::cout << "[Fusion] Starting ROI Capture." << std::endl;

            return true;

        }
        else {
            std::cerr << "[Fusion] Stream lost during capture. Attempting reconnect..." << std::endl;
            return false;
        }
    }
}





std::string RunZFusion(ZPiCap& zcap, int timeout_ms) {
    auto start_time = std::chrono::steady_clock::now();
    const cv::Mat& liveFrame = zcap.GetMainFrame();

    std::cout << "[Fusion] Starting biometric capture sequence..." << std::endl;

    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();

        if (elapsed > timeout_ms) {
            std::cerr << "[Fusion] Capture timeout reached." << std::endl;
            return "";
        }

        return "\xDE\xAD\xBE\xEF\x01\x02";
        
    }
}


