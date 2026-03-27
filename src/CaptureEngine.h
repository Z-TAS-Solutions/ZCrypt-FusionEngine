#ifndef ZCap_H
#define ZCap_H

#pragma once

#include <opencv2/opencv.hpp>

enum StreamMode {
	RTSP,
	TCP
};

enum CaptureMode {
	Default,
	GStream
};

enum LiveFeedState {
	CLOSED,
	READY,
	LIVE
};

class ZPiCap
{
public:
	bool SetupGStreamerPipeline8Bit(const std::string& host, int port, StreamMode mode, bool GPUAccel, bool monochrome, cv::VideoCapture& cap);

	bool SetupGStreamerPipeline10Bit(const std::string& host, int port, StreamMode mode, bool GPUAccel, bool monochrome, cv::VideoCapture& cap);

	std::string GenerateStreamURL(StreamMode mode, std::string_view ip, int port);

	bool OpenStream(const std::string& url);

	bool OpenStream(const std::string_view ip, int port, StreamMode mode);

	bool OpenGStream8Bit(const std::string& ip, int port, StreamMode mode);

	bool OpenGStream10Bit(std::string ip, int port, StreamMode mode);

	void ZPiCap::CloseStream();

private:
	
	cv::VideoCapture CaptureEngine;

	cv::Mat MainFrame;

	LiveFeedState LiveFeedStatus = LiveFeedState::CLOSED;
};


#endif