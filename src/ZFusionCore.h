#ifndef FUSION_LOGIC_H
#define FUSION_LOGIC_H

#pragma once

#include "FusionHelper.h"
#include "HammingDistance.h"
#include "CaptureEngine.h"
#include <string>
#include <opencv2/opencv.hpp>


bool RunZROI(ZPiCap& zcap, int timeout_ms);

std::string RunZFusion(ZPiCap& zcap, int timeout_ms = 10000);



#endif