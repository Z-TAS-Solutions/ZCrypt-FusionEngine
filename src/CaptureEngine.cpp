#include "CaptureEngine.h"


cv::Mat& ZPiCap::GetMainFrame() {
	return this->MainFrame;
}


std::string ZPiCap::GenerateStreamURL(StreamMode mode, std::string_view ip, int port)
{
	std::string url;
	url.reserve(48);

	switch (mode)
	{
	case StreamMode::TCP:
		url += ip;
		url += ":";
		url += std::to_string(port);
		break;

	case StreamMode::RTSP:
		url += ip;
		url += ":";
		url += std::to_string(port);
		url += "/cam";
		break;

	default:
		return "";
	}

	return url;
}



bool ZPiCap::SetupGStreamerPipeline8Bit(const std::string& host, int port, StreamMode mode, bool GPUAccel, bool monochrome, cv::VideoCapture& cap)
{
	std::cout << cv::getBuildInformation() << std::endl;

	std::string PipelineCMD;

	if (mode == StreamMode::RTSP) {
		PipelineCMD =
			"rtspsrc location=" + host + " latency=0 ! rtph264depay ! h264parse ! ";
	}
	else if (mode == StreamMode::TCP) {
		PipelineCMD = "tcpclientsrc host=" + host +
			" port=" + std::to_string(port) + " ! h264parse ! ";
	}

	if (GPUAccel) {
		PipelineCMD += "nvh264dec ! nvvidconv ! ";
	}
	else {
		PipelineCMD += "avdec_h264 ! videoconvert ! ";
	}

	if (monochrome) {
		PipelineCMD += "video/x-raw,format=GRAY8 ! ";
	}
	else {
		PipelineCMD += "video/x-raw,format=BGR ! ";
	}

	PipelineCMD += "appsink sync=false drop=true";

	std::cout << "GStreamer: " << PipelineCMD << std::endl;

	cap.open(PipelineCMD, cv::CAP_GSTREAMER);
	if (!cap.isOpened()) {
		std::cerr << "Failed to open GStreamer!" << std::endl;
		return false;
	}

	cap.set(cv::CAP_PROP_CONVERT_RGB, 0);

	std::cout << "Stream connected: " << host << std::endl;

	return true;
}

bool ZPiCap::SetupGStreamerPipeline10Bit(const std::string& host, int port, StreamMode mode, bool GPUAccel, bool monochrome, cv::VideoCapture& cap) 
{ 
	std::string PipelineCMD;

	switch (mode)
  case StreamMode::RTSP: {
		PipelineCMD = "rtspsrc location=" + host + " latency=50 ! ";
		PipelineCMD += "rtph264depay ! h264parse ! ";
		if (GPUAccel) {
			PipelineCMD += "nvh264dec ! ";
			PipelineCMD += "nvvidconv ! ";
		}
		else {
			PipelineCMD += "avdec_h264 ! videoconvert ! ";
		}

  case StreamMode::TCP: {
	  PipelineCMD =
		  "tcpclientsrc host=" + host + " port=" + std::to_string(port) + " !";
	  PipelineCMD += "h264parse ! ";
	  if (GPUAccel) {
		  PipelineCMD += "nvh264dec ! nvvidconv ! ";
	  }
	  else {
		  PipelineCMD += "avdec_h264 ! videoconvert ! ";
	  }
  }
	}

	if (monochrome) {
		PipelineCMD += "video/x-raw,format=GRAY16_LE ! appsink";
	}
	else {
		PipelineCMD += "video/x-raw,format=BGR ! appsink";
	}

	std::cout << "GStreamer: " << PipelineCMD << std::endl;

	cap.open(PipelineCMD, cv::CAP_GSTREAMER);
	if (!cap.isOpened()) {
		std::cerr << "Failed to open GStreamer!" << std::endl;
		return false;
	}

	return true;
}

bool ZPiCap::OpenStream(const std::string& url) 
{

	int backend = cv::CAP_FFMPEG;

	CaptureEngine.open(url, backend);

	if (!CaptureEngine.isOpened()) {
		std::cerr << "Failed to open stream: " << url << std::endl;
		return false;
	}

	LiveFeedStatus = LiveFeedState::READY;

	CaptureEngine.set(cv::CAP_PROP_BUFFERSIZE, 1);

	std::cout << "Stream connected: " << url << std::endl;

	return true;
}

bool ZPiCap::OpenStream(const std::string_view ip, int port, StreamMode mode) 
{
	std::cout << cv::getBuildInformation() << std::endl;
	if (OpenStream(GenerateStreamURL(mode, ip, port))) {
		LiveFeedStatus = LiveFeedState::READY;
		return true;
	}
	else
		return false;
}

bool ZPiCap::OpenGStream8Bit(const std::string& ip, int port, StreamMode mode) 
{

	if (SetupGStreamerPipeline8Bit(ip, port, mode, false, true, CaptureEngine)) {
		LiveFeedStatus = LiveFeedState::READY;
		return true;
	}
	else
		return false;
}

// can't use 10bit yet, have to make the pi output 10 bit manually
bool ZPiCap::OpenGStream10Bit(std::string ip, int port, StreamMode mode) 
{

	if (SetupGStreamerPipeline10Bit(ip, port, mode, false, true, CaptureEngine)) {
		LiveFeedStatus = LiveFeedState::READY;
		return true;
	}
	else
		return false;
}

void ZPiCap::CloseStream() 
{
	if (CaptureEngine.isOpened()) {
		CaptureEngine.release();

		LiveFeedStatus = LiveFeedState::CLOSED;

		std::cout << "Capture closed successfully." << std::endl;
	}
}


bool ZPiCap::Read() {
	if (!CaptureEngine.isOpened()) {
		return false;
	}

	return CaptureEngine.read(MainFrame);
}


bool ZPiCap::ReadTo(cv::Mat& frame) {
	if (!CaptureEngine.isOpened()) {
		return false;
	}

	return CaptureEngine.read(frame);
}


bool ZPiCap::ForceOpenGStream8Bit(const std::string& ip, int port, StreamMode mode) {
	int retryCount = 0;

	while (true) {
		if (SetupGStreamerPipeline8Bit(ip, port, mode, false, true, CaptureEngine)) {

			if (CaptureEngine.isOpened()) {

				cv::Mat testFrame;
				if (CaptureEngine.read(testFrame) && !testFrame.empty()) {
					std::cout << "\n[ZTAS] Connection Established & Verified! Stream is LIVE." << std::endl;
					LiveFeedStatus = LiveFeedState::READY;
					return true;
				}
			}
		}

		retryCount++;
		std::cout << "\r[ZTAS] Connection unstable. Retry #" << retryCount << " (Verifying Pi Signal...)" << std::flush;

		CaptureEngine.release();
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}