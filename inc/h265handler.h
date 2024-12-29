/*
 * SPDX-License-Identifier: Unlicense
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
 * software, either in source code form or as a compiled binary, for any purpose,
 * commercial or non-commercial, and by any means.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#pragma once

#include <string>
#include <iomanip>

#include <json/json.h>

#include "rtspconnectionclient.h"
#include "codechandler.h"


const int H265_SLICE=1;
const int H265_VPS=32;
const int H265_SPS=33;
const int H265_PPS=34;
const int H265_IDR_W_RADL=19;
const int H265_IDR_N_LP=20;


class H265Handler : public CodecHandler {
public:
    H265Handler(const SessionParams& params) : CodecHandler(params) {}

    std::tuple<Json::Value,std::string> onData(unsigned char* buffer, ssize_t size, struct timeval presentationTime) override {
        std::string buf(buffer, buffer + size);
        int nalu = (buffer[4] & 0x7E) >> 1;
        if (nalu == H265_VPS) {
            m_vps = buf;
        } else if (nalu == H265_SPS) {
            m_sps = buf;
        } else if (nalu == H265_PPS) {
            m_pps = buf;
        } else if (nalu == H265_IDR_W_RADL || nalu == H265_IDR_N_LP) {
            buf.insert(0, m_pps);
            buf.insert(0, m_sps);
            buf.insert(0, m_vps);
        }
        if (nalu == H265_IDR_W_RADL || nalu == H265_IDR_N_LP || nalu == H265_SLICE) {
            Json::Value data;
            data["media"] = m_params.m_media;
            data["codec"] = "hev1.1.6.L93.B0";
            data["ts"] = Json::Value::UInt64(1000ULL * 1000 * presentationTime.tv_sec + presentationTime.tv_usec);
            if (nalu == H265_IDR_W_RADL || nalu == H265_IDR_N_LP) {
                data["type"] = "keyframe";
            }
            return std::make_tuple(data, buf);
        } else {
            return std::make_tuple(Json::Value(), "");
        }
    }

    bool onConfig(const char* sdp) override {
        onH265SPropConfig("sprop-vps=", sdp);
        onH265SPropConfig("sprop-sps=", sdp);
        onH265SPropConfig("sprop-pps=", sdp);
        return true;
    }

private:

    bool onH265SPropConfig(const char* pattern, const char* sdp) {
        const char* sprop = strstr(sdp, pattern);
        if (sprop) {
            std::string xps(extractProp(sprop + strlen(pattern)));
            onCfg(xps);
        }
        return true;
    }

    std::string m_vps;
    std::string m_sps;
    std::string m_pps;
};