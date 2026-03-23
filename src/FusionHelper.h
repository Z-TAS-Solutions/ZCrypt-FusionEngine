#ifndef FUSION_HELPER_H
#define FUSION_HELPER_H

#pragma once

#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

#include "DataTypes.h"

#include <cstdlib>
#include <ctime>

std::pair<BiometricTemplate, BiometricTemplate>
GenerateTemplatePair(double accuracy, uint32_t GIndexA = 0, uint32_t GIndexB = 1) {
    if (accuracy < 0.0) accuracy = 0.0;
    if (accuracy > 1.0) accuracy = 1.0;

    BiometricTemplate t1(GIndexA);
    BiometricTemplate t2(GIndexB);

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    for (int i = 0; i < TEMPLATE_SIZE; ++i) {
        uint8_t bit = static_cast<uint8_t>(std::rand() % 256);
        t1.Data[i] = bit;

        if ((std::rand() / static_cast<double>(RAND_MAX)) < accuracy) {
            t2.Data[i] = bit;
        }
        else {
            t2.Data[i] = static_cast<uint8_t>(std::rand() % 256);
        }
    }

    return { t1, t2 };
}



#endif

