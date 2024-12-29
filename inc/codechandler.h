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

#include "Base64.hh"

#include "session.h"

class CodecHandler {
public:
    CodecHandler() = default;

    CodecHandler(const SessionParams& params) : m_params(params) {}

    virtual ~CodecHandler() = default;

    virtual std::tuple<Json::Value,std::string> onData(unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
        Json::Value data;
        data["media"] = m_params.m_media;
        data["codec"] = m_params.m_codec;
        data["freq"] = m_params.m_rtpfrequency;
        data["channels"] = m_params.m_channels;
        data["ts"] = Json::Value::UInt64(1000ULL*1000*presentationTime.tv_sec+presentationTime.tv_usec);
        std::string buf(buffer, buffer+size);        
        return std::make_tuple(data, buf);
    }

    virtual bool onConfig(const char* sdp) { 
        return true; 
    }

    std::string extractProp(const char* spropvalue) {
        std::string sdpstr(spropvalue);
        size_t pos = sdpstr.find_first_of(" ;\r\n");
        if (pos != std::string::npos) {
            sdpstr.erase(pos);
        }            
        return sdpstr;
    }    

    void onCfg(const std::string& prop) {
        unsigned int length = 0;
        unsigned char * decoded = base64Decode(prop.c_str(), length);
        if (decoded) {
            std::string cfg;
            cfg.insert(cfg.end(), H26X_marker, H26X_marker+sizeof(H26X_marker));
            cfg.insert(cfg.end(), decoded, decoded+length);
            onData((unsigned char*)cfg.c_str(), cfg.size(), timeval());
            delete[]decoded;
        }
    }

public:
    SessionParams m_params; 
};


