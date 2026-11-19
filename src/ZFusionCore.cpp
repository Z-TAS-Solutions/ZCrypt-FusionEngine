#include "ZFusionCore.h"

bool ZFusionCore::RunZROI(ZPiCap& zcap, int timeout_ms)
{
    auto           StartTime = std::chrono::steady_clock::now();
    const cv::Mat& liveFrame = zcap.GetMainFrame();

    std::cout << "[Fusion] Starting roi capture sequence..." << std::endl;

    while (true) {
        auto CurrentTime = std::chrono::steady_clock::now();
        auto Elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(CurrentTime - StartTime).count();

        if (Elapsed > timeout_ms) {
            std::cerr << "[Fusion] ROI Capture timeout reached." << std::endl;
            return "";
        }

        if (zcap.Read()) {
            if (liveFrame.empty())
                continue;

            std::cout << "[Fusion] Starting ROI Capture." << std::endl;

            auto ValleyPoints = ZCore::ValleyExRadial(liveFrame);

            if (ValleyPoints.size() >= 5) {
                cv::Point2f P1 = ValleyPoints[1];
                cv::Point2f P2 = ValleyPoints[2];
                cv::Point2f P4 = ValleyPoints[4];

                float DistToP1 = (float)cv::norm(P4 - P1);
                float DistToP2 = (float)cv::norm(P4 - P2);

                int Handedness = (DistToP1 < DistToP2) ? -1 : 1;

                std::vector<cv::Point2f> ROIPoints = ZCore::ROIGen(P1, P2, Handedness);

                ROIFrame = ZCore::ExtractPalmROI(liveFrame, ROIPoints, 140);
                ROIFrame = ZCore::CropBorder(ROIFrame, 12);

                return true;
            }

        } else {
            std::cerr << "[Fusion] Stream lost during capture. Attempting reconnect..."
                      << std::endl;
            return false;
        }
    }
}

std::string ZFusionCore::RunZFusion(ZPiCap& zcap, int timeout_ms)
{
    auto           StartTime = std::chrono::steady_clock::now();
    const cv::Mat& LiveFrame = zcap.GetMainFrame();

    std::cout << "[Fusion] Starting Fusion Sequence..." << std::endl;

    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(current_time - StartTime).count();

        if (elapsed > timeout_ms) {
            std::cerr << "[Fusion] Capture timeout reached." << std::endl;
            return "";
        }

        cv::Mat FrangiFrame, Lxx, Lxy, Lyy;
        FrangiFrame = ZCore::PreProcessTest(ROIFrame, Lxx, Lxy, Lyy);

        cv::Mat LiveTemplate, LiveTemplateMask;
        ZCore::GenerateTemplate(FrangiFrame, Lxx, Lxy, Lyy, LiveTemplate, LiveTemplateMask);

        auto Bitstream = ZCore::PackTemplate(LiveTemplate, LiveTemplateMask);

        std::string str(Bitstream.begin(), Bitstream.end());

        return str;
    }
}
