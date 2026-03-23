#ifndef HAMMING_DISTANCE_H
#define HAMMING_DISTANCE_H

#pragma once
#include <vector>
#include <cstdint>
#include "DataTypes.h"



int MatchHammingD(const std::vector<uint8_t>& src1, const std::vector<uint8_t>& src2);

int MatchHammingD(const BiometricTemplate& TemplateA, const BiometricTemplate& TemplateB);

inline int GetHammingDSCore(int distance) 
{
	return ((TEMPLATE_SIZE - distance) * 100) / TEMPLATE_SIZE;
}

#endif