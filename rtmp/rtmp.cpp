/*
 * rtmp.c
 *
 *  Created on: Sep 30, 2016
 *      Author: hoang
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// for open h264 raw file.
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include "rtmp.h"

using namespace std;

static srs_rtmp_t rtmp;
static bool rtmp_ready = 0;
pthread_mutex_t lock;

char server[50];
//int rtmp_init_client(int client)
int rtmp_init_client(char* client)
{
//	if (client == 0) {
//		rtmp = srs_rtmp_create(RTMP_SERVER_URL_LUMI);
//		sprintf(server, RTMP_SERVER_URL_LUMI);
//	} else if (client == 1) {
//		rtmp = srs_rtmp_create(RTMP_SERVER_URL_A_DUC);
//		sprintf(server, RTMP_SERVER_URL_A_DUC);
//	} else if (client == 2) {
//		rtmp = srs_rtmp_create(RTMP_SERVER_URL_SCTV);
//		sprintf(server, RTMP_SERVER_URL_SCTV);
//	} else if (client == 3) {
//		rtmp = srs_rtmp_create(RTMP_SERVER_URL_QUANGANH);
//		sprintf(server, RTMP_SERVER_URL_QUANGANH);
//	} else if (client == 4) {
//		rtmp = srs_rtmp_create(RTMP_SERVER_URL_MAYAO);
//		sprintf(server, RTMP_SERVER_URL_MAYAO);
//	} else if (client == 5) {
//		rtmp = srs_rtmp_create(RTMP_SERVER_URL_LUMI_FORWARD);
//		sprintf(server, RTMP_SERVER_URL_LUMI_FORWARD);
//	}

	string key = client;
//	string url = "rtmp://192.168.20.3:1935/live?doPublish=" + key + "/stream";
//	string url = "rtmp://112.197.2.156:1935/live?doPublish=" + key + "/stream";
//	string url = "rtmp://192.168.0.125:1935/live?doPublish=" + key + "/stream";
//	string url = "rtmp://192.168.100.12:1935/livestream?doPublish=" + key + "/B0D59D246169";
//	string url = "rtmp://112.197.2.156:1935/live/tokenTest123456789";

//SCTV
//	string url = "rtmp://112.197.10.222:1935/lumi_test/tokenTest123456789";
	string url = "rtmp://112.197.10.222:1935/lumi_test/" + key;
//Lumi
//	string url = "rtmp://192.168.100.12:1935/live/tokenTest123456789";
//	string url = "rtmp://192.168.1.107:1935/live/" + key;
//May ao
//	string url = "rtmp://192.168.1.10:1935/live/stream";
//Forward port
//	string url = "rtmp://123.16.43.20:1935/live/tokenTest123456789";
//A Duc
//	string url = RTMP_SERVER_URL_A_DUC;

//VIETTEL
//	string url = "rtmp://192.168.10.114:1935/live/stream";


//	rtmp = srs_rtmp_create(client);
	rtmp = srs_rtmp_create(url.c_str());
	sprintf(server, client);

    if (rtmp == NULL) {
    	srs_human_trace("srs_rtmp_create failed.");
    	return -1;
    }
    else {
    	srs_human_trace("srs_rtmp_create success \"%s\"", url.c_str());
//    	srs_human_trace("srs_rtmp_create success.");
    }

    if (srs_rtmp_handshake(rtmp) != 0) {
        srs_human_trace("simple handshake failed.");
        return -1;
    }
    srs_human_trace("simple handshake success");

    if (srs_rtmp_connect_app(rtmp) != 0) {
        srs_human_trace("connect vhost/app failed.");
        return -1;
    }
    srs_human_trace("connect vhost/app success");

    if (srs_rtmp_publish_stream(rtmp) != 0) {
        srs_human_trace("publish stream failed.");
        return -1;
    }
    srs_human_trace("publish stream success");

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    rtmp_ready = 1;
	return 0;
}

int rtmp_send_h264_raw_stream(char* data, int size, u_int32_t dts, u_int32_t pts) {
	//Check if rtmp client has inited
	if ((rtmp == NULL) || (rtmp_ready == 0))
		return -1;

	pthread_mutex_lock(&lock);
    // send out the h264 packet over RTMP
    int ret = srs_h264_write_raw_frames(rtmp, data, size, dts, pts);
    if (ret != 0) {
        if (srs_h264_is_dvbsp_error(ret)) {
            srs_human_trace("ignore drop video error, code=%d", ret);
        } else if (srs_h264_is_duplicated_sps_error(ret)) {
//            srs_human_trace("ignore duplicated sps, code=%d", ret);
        } else if (srs_h264_is_duplicated_pps_error(ret)) {
//            srs_human_trace("ignore duplicated pps, code=%d", ret);
        } else {
            srs_human_trace("send h264 raw data failed. ret=%d", ret);
            rtmp_ready = 0;
            return -2;
        }
    }
//    srs_human_trace("write video frame success buf 0x%p, size = %d", data, size);
    pthread_mutex_unlock(&lock);

	return 0;
}

/******************************************************************************
* write an audio raw frame to srs.
* not similar to h.264 video, the audio never aggregated, always
* encoded one frame by one, so this api is used to write a frame.
*
* @param sound_format Format of SoundData. The following values are defined:
*               0 = Linear PCM, platform endian
*               1 = ADPCM
*               2 = MP3
*               3 = Linear PCM, little endian
*               4 = Nellymoser 16 kHz mono
*               5 = Nellymoser 8 kHz mono
*               6 = Nellymoser
*               7 = G.711 A-law logarithmic PCM
*               8 = G.711 mu-law logarithmic PCM
*               9 = reserved
*               10 = AAC
*               11 = Speex
*               14 = MP3 8 kHz
*               15 = Device-specific sound
*               Formats 7, 8, 14, and 15 are reserved.
*               AAC is supported in Flash Player 9,0,115,0 and higher.
*               Speex is supported in Flash Player 10 and higher.
* @param sound_rate Sampling rate. The following values are defined:
*               0 = 5.5 kHz
*               1 = 11 kHz
*               2 = 22 kHz
*               3 = 44 kHz
* @param sound_size Size of each audio sample. This parameter only pertains to
*               uncompressed formats. Compressed formats always decode
*               to 16 bits internally.
*               0 = 8-bit samples
*               1 = 16-bit samples
* @param sound_type Mono or stereo sound
*               0 = Mono sound
*               1 = Stereo sound
******************************************************************************/
int rtmp_send_audio_raw_stream(char* data, int size, u_int32_t timestamp) {
	//Check if rtmp client has inited
	if (rtmp == NULL)
		return -1;

    // 0 = Linear PCM, platform endian
    // 1 = ADPCM
    // 2 = MP3
    // 7 = G.711 A-law logarithmic PCM
    // 8 = G.711 mu-law logarithmic PCM
    // 10 = AAC
    // 11 = Speex
    char sound_format = 0;
    // 2 = 22 kHz
    // 3 = 44 kHz
    char sound_rate = 2;
    // 1 = 16-bit samples
    char sound_size = 1;
    // 0 = Mono sound
    // 1 = Stereo sound
    char sound_type = 0;

    pthread_mutex_lock(&lock);
    if (srs_audio_write_raw_frame(rtmp, sound_format, sound_rate, sound_size,
    		sound_type, data, size, timestamp) != 0) {
        srs_human_trace("send audio raw data failed.");
        return -1;
    }
//    srs_human_trace("write audio frame success buf 0x%p, size = %d", data, size);
    pthread_mutex_unlock(&lock);

	return 0;
}

int rtmp_destroy_client() {
	if ((rtmp != NULL) && (rtmp_ready == 1))
		srs_rtmp_destroy(rtmp);
	pthread_mutex_destroy(&lock);
	rtmp_ready = 0;

	return 0;
}

