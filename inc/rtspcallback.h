
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

#include <iostream>
#include <iomanip>
#include <map>

#include "rtspconnectionclient.h"
#include "HttpServerRequestHandler.h"
#include "session.h"
#include "codechandler.h"
#include "h264handler.h"
#include "h265handler.h"


class RTSPCallback : public RTSPConnection::Callback
{
    public:
        RTSPCallback(HttpServerRequestHandler& httpServer, const std::string& uri): m_httpServer(httpServer), m_uri(uri)   {}

        bool  onNewSession(const char* id, const char* media, const char* codec, const char* sdp, unsigned int rtpfrequency, unsigned int channels) override { 
            std::cout << id << " " << media << "/" <<  codec << " " << rtpfrequency << "/" << channels << std::endl;

            bool ret = false;
            if (strcmp(media, "video") == 0) {
                if (strcmp(codec, "H264") == 0) {
                    m_handler[id] = std::make_unique<H264Handler>(SessionParams(media, codec, rtpfrequency, channels));
                    ret = m_handler[id]->onConfig(sdp);
                } else if (strcmp(codec, "H265") == 0) {
                    m_handler[id] = std::make_unique<H265Handler>(SessionParams(media, codec, rtpfrequency, channels));
                    ret =  m_handler[id]->onConfig(sdp);
                } else if (strcmp(codec, "JPEG") == 0) {
                    m_handler[id] = std::make_unique<CodecHandler>(SessionParams(media, "jpeg", rtpfrequency, channels));
                    ret = true;
                } else {
                    std::cout << codec << " not supported" << std::endl;
                }
            } else if (strcmp(media, "audio") == 0) {
                if (strcmp(codec, "MPEG4-GENERIC") == 0) {
                    m_handler[id] = std::make_unique<CodecHandler>(SessionParams(media, "mp4a.40.2", rtpfrequency, channels));
                    ret = true;
                } else if (strcmp(codec, "MPA") == 0) {
                    m_handler[id] = std::make_unique<CodecHandler>(SessionParams(media, "mp3", rtpfrequency, channels));
                    ret = true;
                } else if (strcmp(codec, "OPUS") == 0) {
                    m_handler[id] = std::make_unique<CodecHandler>(SessionParams(media, "opus", rtpfrequency, channels));
                    ret = true;
                } else if (strcmp(codec, "PCMU") == 0) {
                    m_handler[id] = std::make_unique<CodecHandler>(SessionParams(media, "ulaw", rtpfrequency, channels));
                    ret = true;
                } else {
                    std::cout << codec << " not supported" << std::endl;
                }
            } else {
                std::cout << media << " not supported" << std::endl;
            }
            return ret;
        }
        
        bool    onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) override {
            auto it = m_handler.find(id);
            if (it != m_handler.end()) {
                std::tuple<Json::Value,std::string> data = it->second->onData(buffer, size, presentationTime);
                if (!std::get<1>(data).empty()) {
                    publish(std::get<0>(data), std::get<1>(data)); 
                }
            }
            return true;
        }
        
        void    onError(RTSPConnection& connection, const char* message) override {
            connection.start(10);
        }
        
        void    onConnectionTimeout(RTSPConnection& connection) override {
            connection.start();
        }
        
        void    onDataTimeout(RTSPConnection& connection) override {
            connection.start();
        }	

        void    onCloseSession(const char* id) override {
            m_handler.erase(id);
        }

        Json::Value toJSON() const {
            Json::Value data;
            for (auto const& x : m_handler) {
                data[x.first] = x.second->m_params.m_media + "/" + x.second->m_params.m_codec;
            }
            return data;
        }

    private:
        void publish(const Json::Value & data, const std::string & buf) const {
            m_httpServer.publishJSON(m_uri, data);
            m_httpServer.publishBin(m_uri, buf.c_str(), buf.size());
        }

    private:
        HttpServerRequestHandler&                                m_httpServer;
        std::string                                              m_uri;
        std::map<std::string,std::unique_ptr<CodecHandler>>      m_handler;
};