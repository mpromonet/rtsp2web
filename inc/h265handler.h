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


const int H265_SLICE=1;
const int H265_VPS=32;
const int H265_SPS=33;
const int H265_PPS=34;
const int H265_IDR_W_RADL=19;
const int H265_IDR_N_LP=20;


class H265Handler : public H26xHandler {
public:
    H265Handler(const SessionParams& params) : H26xHandler(params) {}

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
        std::string vps = extractPropConfig("sprop-vps=", sdp);
        if (!vps.empty()) {
            onCfg(vps);
        }
        std::string sps = extractPropConfig("sprop-sps=", sdp);
        if (!sps.empty()) {
            onCfg(sps);
        }
        std::string pps = extractPropConfig("sprop-pps=", sdp);
        if (!pps.empty()) {
            onCfg(pps);
        }
        return true;
    }

private:
    std::string m_vps;
    std::string m_sps;
    std::string m_pps;
};