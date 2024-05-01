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

#include <vector>
#include <string>
#include <map>
#include <thread>
#include <iostream>

#include "HttpServerRequestHandler.h"
#include "rtsp2wsstream.h"

int logger(const struct mg_connection *conn, const char *message) 
{
    const struct mg_request_info *req_info = mg_get_request_info(conn);

	fprintf(stderr, "[%s:%d] %s\n"
                    , req_info ? req_info->remote_addr : ""
                    , req_info ? req_info->remote_port: 0
                    , message);
	return 0;
}

class Rtsp2Ws
{
    public:
        Rtsp2Ws(const Json::Value & config, const std::vector<std::string>& options, const std::string & rtptransport, int verbose)
            : m_httpServer(this->getHttpFunc(), m_wsfunc, options, verbose ? logger : NULL) {
                Json::Value urls(config["urls"]);
                for (auto & url : urls.getMemberNames()) {
                    this->addStream("/"+url, urls[url]["video"].asString(), rtptransport, verbose);
                }
        }

        void addStream(const std::string & wsurl, const std::string & rtspurl, const std::string & rtptransport, int verbose) {
            m_streams[wsurl] = new Rtsp2WsStream(m_httpServer, wsurl, rtspurl, rtptransport, verbose);
        }

        std::map<std::string,HttpServerRequestHandler::httpFunction>& getHttpFunc() {
            if (m_httpfunc.empty()) {
                m_httpfunc["/api/version"] = [this](const struct mg_request_info *, const Json::Value &) -> Json::Value {
                        return Json::Value(VERSION);
                };
                m_httpfunc["/api/streams"] = [this](const struct mg_request_info *, const Json::Value &) -> Json::Value {
                        Json::Value answer(Json::objectValue);
                        for (auto & it : m_streams) {
                                answer[it.first] = it.second->toJSON();
                        }
                        return answer;
                };                
                m_httpfunc["/api/help"]    = [this](const struct mg_request_info *, const Json::Value & ) -> Json::Value {
                        Json::Value answer(Json::arrayValue);
                        for (auto it : m_httpfunc) {
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
