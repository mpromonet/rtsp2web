/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
 * software, either in source code form or as a compiled binary, for any purpose,
 * commercial or non-commercial, and by any means.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#pragma once

#include <vector>
#include <string>
#include <map>
#include <thread>
#include <iostream>

#include "HttpServerRequestHandler.h"
#include "rtsp2wsstream.h"

int NullLogger(const struct mg_connection *, const char *) {
    return 1;
}

class Rtsp2Ws
{
    public:
        Rtsp2Ws(const std::vector<std::string> & urls, const std::vector<std::string>& options, int verbose)
            : m_httpServer(this->getHttpFunc(), m_wsfunc, options, verbose ? NULL : NullLogger) {
                int idx = 0;
                for (auto & url : urls) {
                    this->addStream("/ws" + std::to_string(idx++) ,url, verbose);
                }
        }

        void addStream(const std::string & wsurl, const std::string & rtspurl, int verbose) {
            m_streams[wsurl] = new Rtsp2WsStream(m_httpServer, wsurl, rtspurl, verbose);
            m_httpServer.addWebSocket(wsurl);
        }

        std::map<std::string,HttpServerRequestHandler::httpFunction>& getHttpFunc() {
            if (m_httpfunc.empty()) {
                m_httpfunc["/api/version"] = [this](const struct mg_request_info *, const Json::Value &) -> Json::Value {
                        return Json::Value(VERSION);
                };
                m_httpfunc["/api/streams"] = [this](const struct mg_request_info *, const Json::Value &) -> Json::Value {
                        Json::Value answer(Json::arrayValue);
                        for (auto & it : this->m_streams) {
                                answer.append(it.first);
                        }
                        return answer;
                };                
                m_httpfunc["/api/help"]    = [this](const struct mg_request_info *, const Json::Value & ) -> Json::Value {
                        Json::Value answer(Json::arrayValue);
                        for (auto it : this->m_httpfunc) {
                                answer.append(it.first);
                        }
                        return answer;
                };
            }
            return m_httpfunc;
        }

        virtual ~Rtsp2Ws() {
            for (auto & it : m_streams) {
                delete it.second;
            }
        }

        const void* getContext() { 
            return m_httpServer.getContext(); 
        }

    private:
        std::map<std::string,HttpServerRequestHandler::httpFunction>  m_httpfunc;
        std::map<std::string,HttpServerRequestHandler::wsFunction>    m_wsfunc;
        HttpServerRequestHandler                                      m_httpServer;
        std::map<std::string,Rtsp2WsStream*>                          m_streams;
};
