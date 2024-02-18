
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

#include <iostream>

#include "Base64.hh"

#include "rtspconnectionclient.h"
#include "HttpServerRequestHandler.h"

class RTSPCallback : public RTSPConnection::Callback
{
        
    public:
        RTSPCallback(HttpServerRequestHandler& httpServer): m_httpServer(httpServer)   {}
        
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
                m_sps = buf;
            } else if (nalu == 8) {
                m_pps = buf;
            } else if  (nalu == 5) {
                buf.insert(0, m_pps);
                buf.insert(0, m_sps);
            }
            if (nalu == 5 || nalu == 1) {
                m_httpServer.publishBin("/ws", buf.c_str(), buf.size());
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
        HttpServerRequestHandler&   m_httpServer;
        std::string                 m_sps;
        std::string                 m_pps;
};