#include "HammingDistance.h"

int MatchHammingD(const std::vector<uint8_t>& src1, const std::vector<uint8_t>& src2) {
    int UnmatchedCount = 0;

    size_t len = std::min(src1.size(), src2.size());
    
    for (size_t i = 0; i < len; ++i) {
        if (src1[i] != src2[i]) {
            ++UnmatchedCount;
        }
    }
    return UnmatchedCount;
}

int MatchHammingD(const BiometricTemplate& TemplateA, const BiometricTemplate& TemplateB) {
    int UnmatchedCount = 0;

    for (size_t i = 0; i < TEMPLATE_SIZE; ++i) {
        if (TemplateA.Data[i] != TemplateB.Data[i]) {
            ++UnmatchedCount;
        }
    }
    return UnmatchedCount;
} 