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

#include "h26xhandler.h"


const int H264_SLICE=1;
const int H264_IDR=5;
const int H264_SPS=7;
const int H264_PPS=8;

class H264Handler : public H26xHandler {
public:
    H264Handler(const SessionParams& params) : H26xHandler(params) {}
    
    std::tuple<Json::Value,std::string> onData(unsigned char* buffer, ssize_t size, struct timeval presentationTime) override {
        std::string buf(buffer, buffer + size);
        int nalu = buffer[4] & 0x1F;
        if (nalu == H264_SPS) {
            m_sps = buf;
        } else if (nalu == H264_PPS) {
            m_pps = buf;
        } else if (nalu == H264_IDR) {
            buf.insert(0, m_pps);
            buf.insert(0, m_sps);
        }
        if (nalu == H264_IDR || nalu == H264_SLICE) {
            Json::Value data;
            data["ts"] = Json::Value::UInt64(1000ULL * 1000 * presentationTime.tv_sec + presentationTime.tv_usec);
            std::stringstream ss;
            for (int i = 5; (i < 8) && (i < m_sps.size()); i++) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(m_sps[i]);
            }
            data["media"] = m_params.m_media;
            data["codec"] = "avc1." + ss.str();
            if (nalu == H264_IDR) {
                data["type"] = "keyframe";
            }
            return std::make_tuple(data, buf);
        } else {
            return std::make_tuple(Json::Value(), "");
        }
    }

    bool onConfig(const char* sdp) override {
        std::string spspps = extractPropConfig("sprop-parameter-sets=", sdp);
        if (!spspps.empty()) {
            std::string sps = spspps.substr(0, spspps.find_first_of(","));
            onCfg(sps);
            std::string pps = spspps.substr(spspps.find_first_of(",") + 1);
            onCfg(pps);
        }
        return true;
    }

private:
    std::string m_sps;
    std::string m_pps;
};


