#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include <sys/stat.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>

void SaveFrameToFile(int fd, AVFrame *pFrame, int width, int height, long int timecode)
{
int  y;

	write(fd, &timecode, sizeof(long int));
	for(y = 0;y < height;y++) 
	{
		write(fd, pFrame->data[0] + (y * pFrame->linesize[0]), width * 3);
	}
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame)
{
FILE *pFile;
char szFilename[4096];
int  y;

	sprintf(szFilename, "frame_%05d.ppm", iFrame);
	pFile = fopen(szFilename, "wb");
	if(pFile != NULL)
	{
		fprintf(pFile, "P6\n%d %d\n255\n", width, height);
		for(y = 0;y < height;y++) 
		{
			fwrite(pFrame->data[0] + (y * pFrame->linesize[0]), 1, width * 3, pFile);
		}
		fclose(pFile);
	}
}

void SaveFrameAsPNG(char *filename_preface, AVFrame *pFrame, int width, int height, int iFrame)
{
char	filename[4096];
int		x, y;

	if(access(filename_preface, 0) != 0)
	{
		int success = mkdir(filename_preface, 0777);
	}
	cv::Mat out;
	cv::Mat local_mat(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
	unsigned char *ptr = local_mat.ptr();
	for(y = 0;y < height;y++) 
	{
		unsigned char *data = (unsigned char *)(pFrame->data[0] + (y * pFrame->linesize[0]));
		for(x = 0;x < (width * 3);x++)
		{
			*ptr++ = *data++;
		}
	}
	sprintf(filename, "%s/frame_%06d.png", filename_preface, iFrame);
	cvtColor(local_mat, out, cv::COLOR_RGB2BGR);
	cv::imwrite(filename, out);
}

int	extract_video(void *update_win, int *progress, char *in_filename, char *out_filename, int in_sequence)
{
void			update_extracting_message(void *update_win, int type);
AVFormatContext *pFormatCtx = NULL;
int				 i, videoStream;
AVCodecContext	*pCodecCtx = NULL;
AVCodec			*pCodec = NULL;
AVFrame			*pFrame = NULL;
AVFrame			*pFrameRGB = NULL;
AVPacket		packet;
int				frameFinished;
int				numBytes;
uint8_t			*buffer = NULL;

	int rr = 0;
	AVDictionary *optionsDict = NULL;
	struct SwsContext *sws_ctx = NULL;
	av_register_all();
	if(avformat_open_input(&pFormatCtx, in_filename, NULL, NULL) == 0)	 
	{
		if(avformat_find_stream_info(pFormatCtx, NULL) >= 0)
		{
			// Find the first video stream
			videoStream = -1;
			for(i = 0;((i < pFormatCtx->nb_streams) && (videoStream == -1));i++) 
			{
				if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) 
				{ 
					videoStream = i;
				}
			}
			if(videoStream != -1)
			{
				// Get a pointer to the codec context for the video stream
				pCodecCtx = pFormatCtx->streams[videoStream]->codec;
				double videoFPS = av_q2d(pFormatCtx->streams[videoStream]->r_frame_rate);
				AVRational units = pFormatCtx->streams[videoStream]->time_base;
				long int start_time = pFormatCtx->streams[videoStream]->start_time;
				long int duration = pFormatCtx->streams[videoStream]->duration;
				long int nb_frames = pFormatCtx->streams[videoStream]->nb_frames;
				AVRational avg = pFormatCtx->streams[videoStream]->avg_frame_rate;

				// Find the decoder for the video stream
				pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
				if(pCodec != NULL) 
				{
					if(avcodec_open2(pCodecCtx, pCodec, &optionsDict) >= 0)
					{
						// Allocate video frame
						pFrame = av_frame_alloc();
						// Allocate an AVFrame structure
						pFrameRGB = av_frame_alloc();
						if(pFrameRGB != NULL)
						{
							// Determine required buffer size and allocate buffer
							numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
							buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

							sws_ctx = sws_getContext(
								pCodecCtx->width,
								pCodecCtx->height,
								pCodecCtx->pix_fmt,
								pCodecCtx->width,
								pCodecCtx->height,
								AV_PIX_FMT_RGB24,
								SWS_BILINEAR,
								NULL,
								NULL,
								NULL);

							// Assign appropriate parts of buffer to image planes in pFrameRGB.  Note that pFrameRGB is an AVFrame, but AVFrame is a superset of AVPicture
							avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
							if(in_sequence == 0)
							{
								int fd = open(out_filename, O_CREAT | O_TRUNC | O_WRONLY, 0777);
								int depth = 3;
								int ufps = (int)videoFPS;
								write(fd, &pCodecCtx->width, sizeof(int));
								write(fd, &pCodecCtx->height, sizeof(int));
								write(fd, &depth, sizeof(int));
								write(fd, &ufps, sizeof(int));

								if(fd > -1)
								{
									i = 0;
									long int timecode = 0;
									while(av_read_frame(pFormatCtx, &packet) >= 0)
									{
										// Is this a packet from the video stream?
										if(packet.stream_index == videoStream)
										{
											// Decode video frame
											avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
									
											// Did we get a video frame?
											if(frameFinished)
											{
												// Convert the image from its native format to RGB
												sws_scale(sws_ctx, 
													(uint8_t const * const *)pFrame->data,
													pFrame->linesize,
													0,
													pCodecCtx->height,
													pFrameRGB->data,
													pFrameRGB->linesize);
												// Save the frame to disk
												timecode = (long int)(((double)packet.pts * (double)((double)units.num / (double)units.den)) * 1000.0);
												SaveFrameToFile(fd, pFrameRGB, pCodecCtx->width, pCodecCtx->height, timecode);
												i++;
												update_extracting_message(update_win, 1);
											}
										}
										// Free the packet that was allocated by av_read_frame
										av_free_packet(&packet);
									}
									close(fd);
								}
							}
							else
							{
								i = 0;
								long int timecode = 0;
								while(av_read_frame(pFormatCtx, &packet) >= 0)
								{
									// Is this a packet from the video stream?
									if(packet.stream_index == videoStream)
									{
										// Decode video frame
										avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
									
										// Did we get a video frame?
										if(frameFinished)
										{
											// Convert the image from its native format to RGB
											sws_scale(sws_ctx, 
												(uint8_t const * const *)pFrame->data,
												pFrame->linesize,
												0,
												pCodecCtx->height,
												pFrameRGB->data,
												pFrameRGB->linesize);
											// Save the frame to disk
											timecode = (long int)(((double)packet.pts * (double)((double)units.num / (double)units.den)) * 1000.0);
											SaveFrameAsPNG(out_filename, pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
											i++;
											update_extracting_message(update_win, 1);
										}
									}
									// Free the packet that was allocated by av_read_frame
									av_free_packet(&packet);
								}
							}
							av_free(buffer);av_free(pFrameRGB); // Free the RGB image
							av_free(pFrame); // Free the YUV frame
							avcodec_close(pCodecCtx); // Close the codec
							avformat_close_input(&pFormatCtx); // Close the video file
						}
						else
						{	
							rr = -6;
						}
					}
					else
					{
						rr = -5;
					}
				}
				else
				{
					rr = -4;
				}
			}
			else
			{
				rr = -3;
			}
		}
		else
		{
			rr = -2;
		}
	}
	else
	{
		rr = -1;
	}
	return(rr);
}
