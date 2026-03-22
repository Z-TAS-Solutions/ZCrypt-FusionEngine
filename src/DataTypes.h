#ifndef FUSION_TYPES_H
#define FUSION_TYPES_H

#pragma once

constexpr int TEMPLATE_SIZE = 1024;


struct BiometricTemplate {
    uint8_t data[TEMPLATE_SIZE];
    uint32_t GroupingIndex;

    BiometricTemplate()
        : GroupingIndex(0) {
        std::memset(data, 0, TEMPLATE_SIZE);
    }

    explicit BiometricTemplate(uint32_t GIndex)
        : GroupingIndex(index) {
        std::memset(data, 0, TEMPLATE_SIZE);
    }

    BiometricTemplate(const uint8_t* InputData, uint32_t GIndex)
        : GroupingIndex(index) {
        std::memcpy(data, inputData, TEMPLATE_SIZE);
    }

    BiometricTemplate(const BiometricTemplate& Template)
        : GroupingIndex(other.GroupingIndex) {
        std::memcpy(data, other.data, TEMPLATE_SIZE);
    }

};


#endif