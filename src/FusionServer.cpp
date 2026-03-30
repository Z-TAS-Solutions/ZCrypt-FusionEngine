#include "FusionServer.h"
#include <iostream>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using zfusion::ZFusionRequest;
using zfusion::ZFusionResponse;



FusionServerImpl::FusionServerImpl(ZPiCap& CaptureEngine) : ZPiCapture(CaptureEngine) {}

FusionServerImpl::~FusionServerImpl() {}

Status FusionServerImpl::FusionCapture(ServerContext* context,
    const ZFusionRequest* request,
    ServerWriter<ZFusionResponse>* writer) {

    std::cout << "Received ZFusionRequest: " << request->session_id() << std::endl;

    bool Result = RunZROI(this->ZPiCapture, 10000);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (!Result) {
        return Status(grpc::StatusCode::DEADLINE_EXCEEDED, "Palm not detected.");
    }

    ZFusionResponse ROIResponse;
    ROIResponse.set_completion_phase(zfusion::ZFusionResponse_PHASE_PHASE_ROI);
    ROIResponse.set_status_message(Result ? "ROI_SUCCESS" : "ROI_FAIL");
    writer->Write(ROIResponse);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::string FusionPayload = "";

    ZFusionResponse FusionResponse;
    FusionResponse.set_completion_phase(zfusion::ZFusionResponse_PHASE_PHASE_FUSION);
    FusionResponse.set_fusion_bitstream(FusionPayload);
    FusionResponse.set_status_message("FUSION_SUCCESS");
    writer->Write(FusionResponse);

    return Status::OK;
}


FusionGrpcServer::FusionGrpcServer(ZPiCap& CaptureEngine, const std::string& address)
    : ServerAddress(address),
    FusionService(CaptureEngine)
{
    ServerBuilder builder;

    builder.AddListeningPort(ServerAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(&FusionService);

    FusionServer = builder.BuildAndStart();
    std::cout << "ZCrypt-FusionEngine gRPC Server listening on " << ServerAddress << std::endl;
}

FusionGrpcServer::~FusionGrpcServer() { Stop(); }

void FusionGrpcServer::RunAsync() {
    FusionThread = std::thread([this]() { FusionServer->Wait(); });
}

void FusionGrpcServer::Wait() {
    if (FusionServer) {
        FusionServer->Wait();
    }
}

void FusionGrpcServer::Stop() {
    if (FusionServer) {
        FusionServer->Shutdown();
    }
    if (FusionThread.joinable()) {
        FusionThread.join();
    }
}