#pragma once
#include "framework.h"

class Mp4Recorder
{
public:
	Mp4Recorder(
		const char* fname,
		int width,
		int height,
		int fps);

	~Mp4Recorder()
	{
		deInit();
	}

	bool init();

	bool start();
	bool put(void* pR8G8B8A8);
	void end();

private:
	const char* filename;
	int width;
	int height;
	int fps;
	bool isInitialize;

	// Context
	AVFormatContext* ofmt_ctx;
	AVStream* ostream;

	// Codec
	const AVCodec* codec;
	AVCodecContext* c;
	AVFrame* frame;
	AVPacket* pkt;
	int frame_idx;

	bool encode(bool isEnd);
	void deInit();
};