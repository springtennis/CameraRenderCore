#include "pch.h"
#include "Mp4Recorder.h"

Mp4Recorder::Mp4Recorder(
	const char* filename,
	int width,
	int height,
	int fps)
{
	this->width = width;
	this->height = height;
	this->filename = filename;
	this->fps = fps;
	this->isInitialize = false;
}

bool Mp4Recorder::init()
{
	int ret;

	if (isInitialize) return true;

	// Set Codec
	// First find "h264_nvenc" -> next "libx264rgb"
	codec = avcodec_find_encoder_by_name("h264_nvenc");
	if (!codec)
		goto error;

	c = avcodec_alloc_context3(codec);
	if (!c) goto error;

	pkt = av_packet_alloc();
	if (!pkt) goto error;

	// YouTube recommandation
	//c->bit_rate = 15000000;
	c->width = width;
	c->height = height;
	c->time_base = { 1, fps };
	c->framerate = { fps, 1 };
	c->gop_size = (int)(fps / 2);
	c->max_b_frames = 2;
	c->pix_fmt = AV_PIX_FMT_BGR0;
	av_opt_set(c->priv_data, "preset", "p7", 0);
	//av_opt_set_double(c->priv_data, "cq", 27, 0);

	ret = avcodec_open2(c, codec, NULL);
	if (ret < 0) // try "libx264rgb" codec
	{
		codec = avcodec_find_encoder_by_name("libx264rgb");
		if (!codec)
			goto error;

		c = avcodec_alloc_context3(codec);
		if (!c) goto error;

		pkt = av_packet_alloc();
		if (!pkt) goto error;

		// YouTube recommandation
		//c->bit_rate = 15000000;
		c->width = width;
		c->height = height;
		c->time_base = { 1, fps };
		c->framerate = { fps, 1 };
		c->gop_size = (int)(fps / 2);
		c->max_b_frames = 2;
		c->pix_fmt = AV_PIX_FMT_BGR0;
		av_opt_set(c->priv_data, "preset", "veryslow", 0);

		ret = avcodec_open2(c, codec, NULL);
		if (ret < 0)
			goto error;
	}

	frame = av_frame_alloc();
	if (!frame) goto error;

	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;

	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0) goto error;

	// Set Output format context
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
	if (!ofmt_ctx) goto error;

	ostream = avformat_new_stream(ofmt_ctx, NULL);
	if (!ostream) goto error;

	ostream->codecpar = avcodec_parameters_alloc();
	if (!ostream->codecpar) goto error;

	// Setup values
	ostream->time_base = c->time_base;
	ostream->avg_frame_rate = c->framerate;
	ostream->duration = 1;

	ostream->codecpar->codec_type = c->codec_type;
	ostream->codecpar->codec_id = c->codec_id;
	ostream->codecpar->codec_tag = 0;
	ostream->codecpar->extradata = c->extradata;
	ostream->codecpar->extradata_size = c->extradata_size;
	ostream->codecpar->format = c->pix_fmt;
	ostream->codecpar->bit_rate = c->bit_rate;
	ostream->codecpar->bits_per_coded_sample = c->bits_per_coded_sample;
	ostream->codecpar->bits_per_raw_sample = c->bits_per_raw_sample;
	ostream->codecpar->profile = c->profile;
	ostream->codecpar->level = c->level;
	ostream->codecpar->width = c->width;
	ostream->codecpar->height = c->height;
	ostream->codecpar->sample_aspect_ratio = { 1, 1 };
	ostream->codecpar->field_order = c->field_order;
	ostream->codecpar->color_range = c->color_range;
	ostream->codecpar->color_primaries = c->color_primaries;
	ostream->codecpar->color_trc = c->color_trc;
	ostream->codecpar->color_space = c->colorspace;
	// ostream->codecpar->chroma_location =
	ostream->codecpar->video_delay = 0;

	isInitialize = true;
	return true;

error:
	deInit();
	return false;
}

void Mp4Recorder::deInit()
{
	//av_write_trailer(ofmt_ctx);
	//if (ofmt_ctx) avio_closep(&ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	av_frame_free(&frame);
	av_packet_free(&pkt);
	avcodec_free_context(&c); c = NULL;

	isInitialize = false;
}

bool Mp4Recorder::start()
{
	if (!isInitialize) return false;

	// File open
	if (avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE) < 0)
		return false;

	if (avformat_write_header(ofmt_ctx, NULL) < 0)
		return false;

	frame_idx = 0;
	return true;
}

bool Mp4Recorder::put(void* pR8G8B8A8)
{
	int ret;

	if (!isInitialize) return false;

	ret = av_frame_make_writable(frame);
	if (ret < 0) return false;

	memcpy(
		&frame->data[0][0],
		pR8G8B8A8,
		c->width * c->height * 4);

	frame->pts = frame_idx++;
	if (!encode(false)) end();
}

void Mp4Recorder::end()
{
	if (!isInitialize) return;

	encode(true);
	av_write_trailer(ofmt_ctx);
	avio_closep(&ofmt_ctx->pb);
	isInitialize = false;
}

bool Mp4Recorder::encode(bool isEnd)
{
	int ret;
	AVFrame* target = NULL;
	if (!isEnd) target = frame;

	ret = avcodec_send_frame(c, target);
	if (ret < 0) return false;

	while (ret >= 0)
	{
		ret = avcodec_receive_packet(c, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return true;
		else if (ret < 0)
			return false;

		av_packet_rescale_ts(
			pkt,
			c->time_base,
			ostream->time_base
		);
		pkt->stream_index = ostream->index;
		ret = av_interleaved_write_frame(ofmt_ctx, pkt);
		if (ret < 0)
			return false;
	}

	return true;
}