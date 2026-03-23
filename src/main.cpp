#include <iostream>
#include "FusionHelper.h"
#include "HammingDistance.h"

int main() {
    BiometricTemplate TemplateA1, TemplateB1;
    GenerateTemplatePair(TemplateA1, TemplateB1, 0.95);

    BiometricTemplate TemplateA2, TemplateB2;
    std::mt19937 MTEngine(42);
    GenerateTemplatePair2(TemplateA2, TemplateB2, 0.95, MTEngine);

    int distance1 = MatchHammingD(TemplateA1, TemplateB1);
    int distance2 = MatchHammingD(TemplateA1, TemplateB1);

    int Score1 = GetHammingDSCore(distance1);
    int Score2 = GetHammingDSCore(distance2);


    std::cout << "Hamming distance score 1: " << Score1 << "%" << std::endl;
    std::cout << "Hamming distance score 2: " << Score2 << "%" << std::endl;




    return 0;
}