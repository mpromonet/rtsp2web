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
            m_stop(0),
            m_env(m_stop),
            m_cb(httpServer, wsurl),
            m_rtspClient(m_env, &m_cb, rtspurl.c_str(), 10, rtptransport, verbose),
            m_thread(std::thread([this]() {
                m_env.mainloop();	
            })) {
                httpServer.addWebSocket(wsurl, this);
        }

        Json::Value toJSON() {
            return m_cb.toJSON();
        }

        virtual ~Rtsp2WsStream() {
            m_stop = 1;
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

    private:
        HttpServerRequestHandler &                                    m_httpServer;
        const std::string                                             m_wsurl;
        char                                                          m_stop;    
        Environment                                                   m_env;
        RTSPCallback                                                  m_cb;
        RTSPConnection                                                m_rtspClient;
        std::thread                                                   m_thread;
};