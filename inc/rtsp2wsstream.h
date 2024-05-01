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
#include <thread>

#include "rtspcallback.h"
#include "WebsocketHandler.h"

class Rtsp2WsStream : public WebsocketHandler
{
    public:
        Rtsp2WsStream(HttpServerRequestHandler &httpServer, const std::string & wsurl, const std::string & rtspurl, int rtptransport, int verbose) :
            WebsocketHandler(httpServer.getCallbacks()),
            m_httpServer(httpServer),
            m_wsurl(wsurl),
            m_env(),
            m_cb(httpServer, wsurl),
            m_rtspClient(m_env, &m_cb, rtspurl.c_str(), 10, rtptransport, verbose),
            m_thread(std::thread([this, wsurl]() {
#ifndef _WIN32
                pthread_setname_np(m_thread.native_handle(), wsurl.c_str());
#endif                
                m_env.mainloop();	
            })) {
                httpServer.addWebSocket(wsurl, this);
        }

        Json::Value toJSON() {
            Json::Value json(m_cb.toJSON());
            json["connections"] = this->getConnections();
            return json;
        }

        virtual ~Rtsp2WsStream() {
            m_rtspClient.stop();
            m_env.stop();
            m_thread.join();
            m_httpServer.removeWebSocket(m_wsurl);
        }

    private:
        virtual bool handleConnection(CivetServer *server, const struct mg_connection *conn) override {
            if (this->getNbConnections() == 0) {
                m_rtspClient.start();
            }
            return WebsocketHandler::handleConnection(server, conn);
        }

        virtual void  handleClose(CivetServer *server, const struct mg_connection *conn) override {
            WebsocketHandler::handleClose(server, conn);
            if (this->getNbConnections() == 0) {
                m_rtspClient.stop();
            }
        }

    private:
        HttpServerRequestHandler &                                    m_httpServer;
        const std::string                                             m_wsurl;
        Environment                                                   m_env;
        RTSPCallback                                                  m_cb;
        RTSPConnection                                                m_rtspClient;
        std::thread                                                   m_thread;
};