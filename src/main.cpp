#include <iostream>
#include "FusionHelper.h"
#include "HammingDistance.h"

int main() {

    auto [TemplateA, TemplateB] = GenerateTemplatePair(0.95);
    int distance = MatchHammingD(TemplateA, TemplateB);
    std::cout << "Hamming distance: " << distance << std::endl;



    return 0;
}