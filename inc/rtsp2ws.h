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

#include "Base64.hh"

#include "HttpServerRequestHandler.h"
#include "rtspconnectionclient.h"

int NullLogger(const struct mg_connection *, const char *) {
    return 1;
}

class Rtsp2Ws
{
    public:

        class RTSPCallback : public RTSPConnection::Callback
        {
                
            public:
                RTSPCallback(Rtsp2Ws * rtsp2ws): m_rtsp2ws(rtsp2ws)   {}
                
                virtual bool    onNewSession(const char* id, const char* media, const char* codec, const char* sdp) {
                    std::cout << id << " " << media << "/" <<  codec << std::endl;

                    bool ret = false;
                    if (strcmp(codec, "H264") == 0) {
                        const char* pattern="sprop-parameter-sets=";
                        const char* sprop=strstr(sdp, pattern);
                        if (sprop)
                        {
                            std::string sdpstr(sprop+strlen(pattern));
                            size_t pos = sdpstr.find_first_of(" ;\r\n");
                            if (pos != std::string::npos)
                            {
                                sdpstr.erase(pos);
                            }
                            
                            std::string sps=sdpstr.substr(0, sdpstr.find_first_of(","));
                            unsigned int length = 0;
                            unsigned char * sps_decoded = base64Decode(sps.c_str(), length);
                            if (sps_decoded) {
                                std::string cfg;
                                cfg.insert(cfg.end(), H26X_marker, H26X_marker+sizeof(H26X_marker));
                                cfg.insert(cfg.end(), sps_decoded, sps_decoded+length);
                                onData(id, (unsigned char*)cfg.c_str(), cfg.size(), timeval());
                                delete[]sps_decoded;
                            }
                            std::string pps=sdpstr.substr(sdpstr.find_first_of(",")+1);
                            unsigned char * pps_decoded = base64Decode(pps.c_str(), length);
                            if (pps_decoded) {
                                std::string cfg;
                                cfg.insert(cfg.end(), H26X_marker, H26X_marker+sizeof(H26X_marker));
                                cfg.insert(cfg.end(), pps_decoded, pps_decoded+length);
                                onData(id, (unsigned char*)cfg.c_str(), cfg.size(), timeval());
                                delete[]pps_decoded;
                            }
                        }                        
                        ret = true;
                    }

                    return ret;
                }
                
                virtual bool    onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
                    std::string buf(buffer, buffer+size);
                    int nalu = buffer[4] & 0x1F;
                    if (nalu == 7) {
                        m_rtsp2ws->m_sps = buf;
                    } else if (nalu == 8) {
                        m_rtsp2ws->m_pps = buf;
                    } else if  (nalu == 5) {
                        buf.insert(0, m_rtsp2ws->m_pps);
                        buf.insert(0, m_rtsp2ws->m_sps);
                    }
                    if (nalu == 5 || nalu == 1) {
                        m_rtsp2ws->m_httpServer.publishBin("/ws", buf.c_str(), buf.size());
                    }
                    return true;
                }
                
                virtual void    onError(RTSPConnection& connection, const char* message) {
                    connection.start(10);
                }
                
                virtual void    onConnectionTimeout(RTSPConnection& connection) {
                    connection.start();
                }
                
                virtual void    onDataTimeout(RTSPConnection& connection)       {
                    connection.start();
                }		
            private:
                Rtsp2Ws * m_rtsp2ws;
        };

        Rtsp2Ws(const std::string & url, const std::vector<std::string>& options, int verbose):
            m_httpServer(this->getHttpFunc(), this->getWsFunc(), options, verbose ? NULL : NullLogger) {


            m_thread = std::thread([this, url, verbose](){
                Environment env(m_stop);
                RTSPCallback cb(this);
                RTSPConnection rtspClient(env, &cb, url.c_str(), 10, RTSPConnection::RTPOVERTCP, verbose);
                
                env.mainloop();	
            });

        }

        std::map<std::string,HttpServerRequestHandler::httpFunction>& getHttpFunc() {
            if (m_httpfunc.empty()) {
                m_httpfunc["/api/version"] = [this](const struct mg_request_info *, const Json::Value &) -> Json::Value {
                        return Json::Value(VERSION);
                };
                m_httpfunc["/api/help"]           = [this](const struct mg_request_info *, const Json::Value & ) -> Json::Value {
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
        const void* getContext() { return m_httpServer.getContext(); }

    private:
        std::map<std::string,HttpServerRequestHandler::httpFunction>  m_httpfunc;
        std::map<std::string,HttpServerRequestHandler::wsFunction>    m_wsfunc;
        HttpServerRequestHandler                                      m_httpServer;
        char                                                          m_stop;
        std::thread                                                   m_thread;
        std::string                                                   m_sps;
        std::string                                                   m_pps;
};
