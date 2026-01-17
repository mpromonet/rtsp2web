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
#include <cstdio>
#include <memory>

#include "HttpServerRequestHandler.h"
#include "rtsp2wsstream.h"

inline int logger(const struct mg_connection *conn, const char *message) 
{
    const struct mg_request_info *req_info = mg_get_request_info(conn);

	fprintf(stderr, "[%s:%d] %s\n"
                    , req_info ? req_info->remote_addr : ""
                    , req_info ? req_info->remote_port: 0
                    , message);
	return 0;
}

class HttpServer
{
    public:
        HttpServer(const Json::Value & config, const std::vector<std::string>& options, const std::string & rtptransport, int verbose)
            : m_httpServer(this->getHttpFunc(), m_wsfunc, options, verbose ? logger : nullptr) {
                Json::Value urls(config["urls"]);
                for (auto & url : urls.getMemberNames()) {
                    this->addStream("/"+url, urls[url]["video"].asString(), rtptransport, verbose);
                }
        }

        HttpServer(const HttpServer&) = delete;
        HttpServer& operator=(const HttpServer&) = delete;
        HttpServer(HttpServer&&) noexcept = default;
        HttpServer& operator=(HttpServer&&) noexcept = default;
        ~HttpServer() = default;

        const void* getContext() const { 
            return m_httpServer.getContext(); 
        }

    private:
        void addStream(const std::string & wsurl, const std::string & rtspurl, const std::string & rtptransport, int verbose) {
            m_streams[wsurl] = std::make_unique<Rtsp2WsStream>(m_httpServer, wsurl, rtspurl, rtptransport, verbose);
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
                    for (const auto & it : m_httpfunc) {
                                answer.append(it.first);
                        }
                        return answer;
                };
            }
            return m_httpfunc;
        }

    private:
        std::map<std::string,HttpServerRequestHandler::httpFunction>  m_httpfunc;
        std::map<std::string,HttpServerRequestHandler::wsFunction>    m_wsfunc;
        HttpServerRequestHandler                                      m_httpServer;
        std::map<std::string, std::unique_ptr<Rtsp2WsStream>>         m_streams;
};
