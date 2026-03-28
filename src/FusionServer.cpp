#include "FusionServer.h"
#include <iostream>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using zfusion::ZFusionRequest;
using zfusion::ZFusionResponse;
using zfusion::FusionCaptureService;

FusionServerImpl::FusionServerImpl() {}
FusionServerImpl::~FusionServerImpl() {}

Status
FusionServerImpl::FusionCapture(ServerContext *context,
                                const ZFusionRequest *request,
                                ServerWriter<ZFusionResponse> *writer) {
  std::cout << "Received ZFusionRequest for Session ID: "
            << request->session_id() << std::endl;


  ZFusionResponse response_roi;
  response_roi.set_completion_phase(ZFusionResponse::PHASE_ROI);
  response_roi.set_status_message(
      "ROI Located and Captured in native pipeline.");
  writer->Write(response_roi);


  ZFusionResponse response_fusion;
  response_fusion.set_completion_phase(ZFusionResponse::PHASE_FUSION);
  std::string dummy_bitstream =
      "\x01\x02\x03\x04\x05";
  response_fusion.set_fusion_bitstream(dummy_bitstream);
  response_fusion.set_status_message("Feature Extraction Successful.");
  writer->Write(response_fusion);

  return Status::OK;
}



FusionGrpcServer::FusionGrpcServer(const std::string &address) : ServerAddress(address) {
  ServerBuilder builder;

  builder.AddListeningPort(ServerAddress, grpc::InsecureServerCredentials());
  builder.RegisterService(&FusionService);

  FusionServer = builder.BuildAndStart();
  std::cout << "ZCrypt-FusionEngine gRPC Server listening on "
            << ServerAddress << std::endl;
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
