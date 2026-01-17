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
#include <thread>

#include "WebsocketHandler.h"

#include "rtspconnectionclient.h"
#include "HttpServerRequestHandler.h"
#include "session.h"
#include "codechandler.h"
#include "h264handler.h"
#include "h265handler.h"

std::map<std::string,std::string> getopts(int timeout, const std::string & rtptransport) {
    std::map<std::string,std::string> opts;
    opts["timeout"] = std::to_string(timeout);
    opts["rtptransport"] = rtptransport;
    return opts;
}

template <typename T>
class WsStream : public WebsocketHandler, public T::Callback
{
    public:
        WsStream(HttpServerRequestHandler &httpServer, const std::string & wsurl, const std::string & rtspurl, const std::string & rtptransport, int verbose) :
            WebsocketHandler(httpServer.getCallbacks()),
            m_httpServer(httpServer),
            m_wsurl(wsurl),
            m_env(),
            m_rtspClient(m_env, this, rtspurl.c_str(), getopts(10, rtptransport), verbose),
            m_thread(std::thread([this, wsurl]() {
#ifndef _WIN32
                pthread_setname_np(m_thread.native_handle(), wsurl.c_str());
#endif                
                m_env.mainloop();	
            })) {
                httpServer.addWebSocket(wsurl, this);
        }

        Json::Value toJSON() {
            Json::Value json;
            for (const auto& [key, value] : m_handler) {
                json[key] = value->m_params.m_media + "/" + value->m_params.m_codec;
            }            
            json["connections"] = getConnections();
            return json;
        }

        virtual ~WsStream() {
            m_rtspClient.stop();
            m_env.stop();
            m_thread.join();
            m_httpServer.removeWebSocket(m_wsurl);
        }

    private:
        bool handleConnection(CivetServer *server, const struct mg_connection *conn) override {
            if (this->getNbConnections() == 0) {
                m_rtspClient.start();
            }
            return WebsocketHandler::handleConnection(server, conn);
        }

        void  handleClose(CivetServer *server, const struct mg_connection *conn) override {
            WebsocketHandler::handleClose(server, conn);
            if (this->getNbConnections() == 0) {
                m_rtspClient.stop();
            }
        }


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
            if (m_handler.find(id) != m_handler.end()) {
                std::tuple<Json::Value,std::string> data = m_handler[id]->onData(buffer, size, presentationTime);
                if (!std::get<1>(data).empty()) {
                    publish(std::get<0>(data), std::get<1>(data)); 
                }
            }
            return true;
        }
        
        void    onError(T& connection, const char* message) override {
            connection.start(10);
        }
        
        void    onConnectionTimeout(T& connection) override {
            connection.start();
        }
        
        void    onDataTimeout(T& connection)  override {
            connection.start();
        }	

        void    onCloseSession(const char* id) override {
            m_handler.erase(id);
        }

    private:
        void publish(const Json::Value & data, const std::string & buf) const {
            m_httpServer.publishJSON(m_wsurl, data);
            m_httpServer.publishBin(m_wsurl, buf.c_str(), buf.size());
        }

    private:
        HttpServerRequestHandler &                                    m_httpServer;
        const std::string                                             m_wsurl;
        Environment                                                   m_env;
        T                                                             m_rtspClient;
        std::thread                                                   m_thread;
        std::map<std::string,std::unique_ptr<CodecHandler>>           m_handler;

};