#pragma once

#include "zfusion.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>

#include "ZFusionCore.h"

#include <memory>
#include <string>
#include <thread>


class FusionServerImpl final : public zfusion::FusionCaptureService::Service {
public:
	explicit FusionServerImpl(ZPiCap& CaptureEngine);
    ~FusionServerImpl() override;

	grpc::Status FusionCapture(
		grpc::ServerContext* context,
		const zfusion::ZFusionRequest* request,
		grpc::ServerWriter<zfusion::ZFusionResponse>* writer
	) override;

private:
	ZPiCap& ZPiCapture;
};

class FusionGrpcServer {
public:
	FusionGrpcServer(ZPiCap& CaptureEngine, const std::string& address = "unix:///tmp/zfusion.sock");
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
