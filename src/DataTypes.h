#ifndef FUSION_TYPES_H
#define FUSION_TYPES_H

#pragma once

constexpr int TEMPLATE_SIZE = 1024;


struct BiometricTemplate {
    uint8_t Data[TEMPLATE_SIZE];
    uint32_t GroupingIndex;

    BiometricTemplate()
        : GroupingIndex(0) {
        std::memset(Data, 0, TEMPLATE_SIZE);
    }

    explicit BiometricTemplate(uint32_t GIndex)
        : GroupingIndex(GIndex) {
        std::memset(Data, 0, TEMPLATE_SIZE);
    }

    BiometricTemplate(const uint8_t* InputData, uint32_t GIndex)
        : GroupingIndex(GIndex) {
        std::memcpy(Data, InputData, TEMPLATE_SIZE);
    }

    BiometricTemplate(const BiometricTemplate& Template)
        : GroupingIndex(Template.GroupingIndex) {
        std::memcpy(Data, Template.Data, TEMPLATE_SIZE);
    }

};


#endif