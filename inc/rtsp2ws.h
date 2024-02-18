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
#include "rtspcallback.h"

int NullLogger(const struct mg_connection *, const char *) {
    return 1;
}

class Rtsp2Ws
{
    public:
        Rtsp2Ws(const std::string & url, const std::vector<std::string>& options, int verbose):
            m_httpServer(this->getHttpFunc(), this->getWsFunc(), options, verbose ? NULL : NullLogger) {

            m_thread = std::thread([this, url, verbose](){
                Environment env(m_stop);
                RTSPCallback cb(m_httpServer);
                RTSPConnection rtspClient(env, &cb, url.c_str(), 10, RTSPConnection::RTPOVERTCP, verbose);
                
                env.mainloop();	
            });
        }

        std::map<std::string,HttpServerRequestHandler::httpFunction>& getHttpFunc() {
            if (m_httpfunc.empty()) {
                m_httpfunc["/api/version"] = [this](const struct mg_request_info *, const Json::Value &) -> Json::Value {
                        return Json::Value(VERSION);
                };
                m_httpfunc["/api/help"]    = [this](const struct mg_request_info *, const Json::Value & ) -> Json::Value {
                        Json::Value answer;
                        for (auto it : this->m_httpfunc) {
                                answer.append(it.first);
                        }
                        return answer;
                };
            }
            return m_httpfunc;
        }

        std::map<std::string,HttpServerRequestHandler::wsFunction>& getWsFunc() {
            if (m_wsfunc.empty()) {
                    m_wsfunc["/ws"]  = [this](const struct mg_request_info *req_info, const Json::Value & in) -> Json::Value {
                        return in;
                    };
            }
            return m_wsfunc;
        }


        virtual ~Rtsp2Ws() {
            m_stop = 1;
            m_thread.join();
        }

        const void* getContext() { 
            return m_httpServer.getContext(); 
        }

    private:
        std::map<std::string,HttpServerRequestHandler::httpFunction>  m_httpfunc;
        std::map<std::string,HttpServerRequestHandler::wsFunction>    m_wsfunc;
        HttpServerRequestHandler                                      m_httpServer;
        char                                                          m_stop;
        std::thread                                                   m_thread;
};
