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

inline void GenerateTemplatePair(BiometricTemplate& TemplateA, BiometricTemplate& TemplateB, double Accuracy) {
    if (Accuracy < 0.0) Accuracy = 0.0;
    if (Accuracy > 1.0) Accuracy = 1.0;

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    for (int i = 0; i < TEMPLATE_SIZE; ++i) {
        uint8_t bit = static_cast<uint8_t>(std::rand() % 256);
        TemplateA.Data[i] = bit;

        if ((std::rand() / static_cast<double>(RAND_MAX)) < Accuracy) {
            TemplateB.Data[i] = bit;
        }
        else {
            TemplateB.Data[i] = static_cast<uint8_t>(std::rand() % 256);
        }
    }

    return;
}

inline void GenerateTemplatePair2(BiometricTemplate& TemplateA, BiometricTemplate& TemplateB, float Accuracy) {
    static std::mt19937 RNGEngine{ std::random_device{}() };
    std::uniform_int_distribution<int> ByteDist(0, 255);

    for (int i = 0; i < TEMPLATE_SIZE; ++i) {
        TemplateA.Data[i] = static_cast<uint8_t>(ByteDist(RNGEngine));
        TemplateB.Data[i] = TemplateA.Data[i];
    }

    int BitCount = TEMPLATE_SIZE * 8;
    int UpdateCount = static_cast<int>(BitCount * (1.0f - Accuracy));
    std::uniform_int_distribution<int> BitDist(0, BitCount - 1);

    for (int i = 0; i < UpdateCount; ++i) {
        int TargetBit = BitDist(RNGEngine);
        int ByteIndex = TargetBit / 8;
        int BitIndex = TargetBit % 8;

        TemplateB.Data[ByteIndex] ^= static_cast<uint8_t>(1u << BitIndex);
    }
}


#endif

