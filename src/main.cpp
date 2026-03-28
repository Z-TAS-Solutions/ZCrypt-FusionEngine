#include <iostream>
#include "FusionHelper.h"
#include "HammingDistance.h"
#include "CaptureEngine.h"
#include "FusionServer.h"


#ifdef _WIN32
#define SERVER_ADDRESS "127.0.0.1:50051"
#else
#define SERVER_ADDRESS "unix:///tmp/ztas.sock"
#define PLAIN_PATH "/tmp/ztas.sock"
#endif

#define CAPTURE_ADDRESS "192.168.1.229"
#define CAPTURE_PORT 8888





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


    ZPiCap CaptureEngine;
    CaptureEngine.ForceOpenGStream8Bit(CAPTURE_ADDRESS, CAPTURE_PORT, StreamMode::TCP);


#ifndef _WIN32
    if (std::filesystem::exists(PLAIN_PATH)) {
        std::filesystem::remove(PLAIN_PATH);
    }
#endif

    std::cout << "Starting GStreamer capture ..." << std::endl;

    FusionGrpcServer ZCapIPC(CaptureEngine, SERVER_ADDRESS);

    ZCapIPC.Wait();

    return 0;
}