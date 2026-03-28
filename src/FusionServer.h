#pragma once

#include "zfusion.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>


class FusionServerImpl final : public zfusion::FusionCaptureService::Service {
public:
  FusionServerImpl();
  ~FusionServerImpl() override;

  grpc::Status FusionCapture(
      grpc::ServerContext *context, const zfusion::ZFusionRequest *request,
      grpc::ServerWriter<zfusion::ZFusionResponse> *writer) override;
};

class FusionGrpcServer {
public:
  FusionGrpcServer(const std::string &address = "unix:///tmp/zfusion.sock");
  ~FusionGrpcServer();

  void RunAsync();

  void Wait();

  void Stop();

private:
  std::string ServerAddress;
  FusionServerImpl FusionService;
  std::unique_ptr<grpc::Server> FusionServer;
  std::thread FusionThread;
};
