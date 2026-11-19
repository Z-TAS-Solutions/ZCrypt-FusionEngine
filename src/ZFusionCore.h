#ifndef FUSION_LOGIC_H
#define FUSION_LOGIC_H

#pragma once

#include "CaptureEngine.h"
#include "FusionHelper.h"
#include "HammingDistance.h"
#include "ZCore.h"

#include <opencv2/opencv.hpp>
#include <string>

class ZFusionCore
{
  private:
    cv::Mat ROIFrame;

  public:
    bool RunZROI(ZPiCap& zcap, int timeout_ms);

    std::string RunZFusion(ZPiCap& zcap, int timeout_ms = 10000);
};

#endif
