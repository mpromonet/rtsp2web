
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

#include "Base64.hh"

#include "rtspconnectionclient.h"
#include "HttpServerRequestHandler.h"

class RTSPCallback : public RTSPConnection::Callback
{
        
    public:
        RTSPCallback(HttpServerRequestHandler& httpServer, const std::string& uri): m_httpServer(httpServer), m_uri(uri)   {}

        virtual bool    onNewSession(const char* id, const char* media, const char* codec, const char* sdp) {
            std::cout << id << " " << media << "/" <<  codec << " " << sdp << std::endl;

            bool ret = false;
            if (strcmp(media, "video") == 0) {
                if (strcmp(codec, "H264") == 0) {
                    m_medias[id] = media;
                    m_codecs[id] = codec;
                    ret = this->onH264Config(id, sdp);
                } else if (strcmp(codec, "H265") == 0) {
                    m_medias[id] = media;
                    m_codecs[id] = codec;
                    ret =  this->onH265Config(id, sdp);
                } else if (strcmp(codec, "JPEG") == 0) {
                    m_medias[id] = media;
                    m_codecs[id] = codec;
                    ret = true;
                } else {
                    std::cout << codec << " not supported" << std::endl;
                }
            } else if (strcmp(media, "audio") == 0) {
                if (strcmp(codec, "L16") == 0) {
                    m_medias[id] = media;
                    m_codecs[id] = codec;
                    ret = true;
                } else if (strcmp(codec, "MPA") == 0) {
                    m_medias[id] = media;
                    m_codecs[id] = codec;
                    ret = true;
                } else if (strcmp(codec, "OPUS") == 0) {
                    m_medias[id] = media;
                    m_codecs[id] = codec;
                    ret = true;
                } else {
                    std::cout << codec << " not supported" << std::endl;
                }
            } else {
                std::cout << media << " not supported" << std::endl;
            }
            return ret;
        }
        
        virtual bool    onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
            std::string codec = m_codecs[id];
            if (codec == "H264") {
                this->onH264Data(id, buffer, size, presentationTime);
            } else if (codec == "H265") {
                this->onH265Data(id, buffer, size, presentationTime);
            } else if (codec == "JPEG") {
                this->onDefaultData(id, "jpeg", buffer, size, presentationTime);
            } else if (codec == "L16") {
                this->onDefaultData(id, "pcm-s16", buffer, size, presentationTime);
            } else if (codec == "MPA") {
                this->onDefaultData(id, "mp3", buffer, size, presentationTime);
            } else if (codec == "OPUS") {
                this->onDefaultData(id, "opus", buffer, size, presentationTime);
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

        Json::Value toJSON() {
            Json::Value data;
            for (auto const& x : m_medias) {
                data[x.first] = x.second + "/" + m_codecs[x.first];
            }
            return data;
        }

    private:
        void onDefaultData(const char* id, const std::string& codec, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
            Json::Value data;
            data["media"] = m_medias[id];
            data["codec"] = codec;
            data["ts"] = Json::Value::UInt64(1000ULL*1000*presentationTime.tv_sec+presentationTime.tv_usec);
            m_httpServer.publishJSON(m_uri, data);                    
            m_httpServer.publishBin(m_uri, (const char*)buffer, size);
        }

        void onH264Data(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
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
                Json::Value data;
                data["ts"] = Json::Value::UInt64(1000ULL*1000*presentationTime.tv_sec+presentationTime.tv_usec);
                std::stringstream ss;
                for (int i = 5; (i < 8) && (i < m_sps.size()); i++) {
                    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(m_sps[i]);
                }
                data["media"] = m_medias[id];
                data["codec"] = "avc1." + ss.str();   
                if (nalu == 5) {
                    data["type"] = "keyframe";
                }             
                m_httpServer.publishJSON(m_uri, data);                    
                m_httpServer.publishBin(m_uri, buf.c_str(), buf.size());
            }
        }

        void onH265Data(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
            std::string buf(buffer, buffer+size);
            int nalu = (buffer[4] & 0x7E)>>1;
            if (nalu == 32) {
                m_vps = buf;
            } else if (nalu == 33) {
                m_sps = buf;
            } else if (nalu == 34) {
                m_pps = buf;
            } else if  (nalu == 19 || nalu == 20) {
                buf.insert(0, m_pps);
                buf.insert(0, m_sps);
                buf.insert(0, m_vps);
            }
            if (nalu == 19 || nalu ==20 || nalu == 1) {
                Json::Value data;
                data["media"] = m_medias[id];
                data["codec"] = "hev1.1.6.L93.B0";
                data["ts"] = Json::Value::UInt64(1000ULL*1000*presentationTime.tv_sec+presentationTime.tv_usec);
                if (nalu == 19 || nalu == 20) {
                    data["type"] = "keyframe";
                }
                m_httpServer.publishJSON(m_uri, data);
                m_httpServer.publishBin(m_uri, buf.c_str(), buf.size());
            }
        }

        bool onH264Config(const char* id, const char* sdp) {
            const char* pattern="sprop-parameter-sets=";
            const char* sprop=strstr(sdp, pattern);
            if (sprop)
            {
                std::string sdpstr(extractProp(sprop+strlen(pattern)));
                
                std::string sps=sdpstr.substr(0, sdpstr.find_first_of(","));
                onCfg(id, sps);

                std::string pps=sdpstr.substr(sdpstr.find_first_of(",")+1);
                onCfg(id, pps);
            } 
            return true;                       
        }

        bool onH265Config(const char* id, const char* sdp) {
            const char* pattern="sprop-vps=";
            const char* sprop=strstr(sdp, pattern);
            if (sprop) {
                std::string vps(extractProp(sprop+strlen(pattern)));
                onCfg(id, vps);
            }
            pattern="sprop-sps=";
            sprop=strstr(sdp, pattern);
            if (sprop) {
                std::string sps(extractProp(sprop+strlen(pattern)));
                onCfg(id, sps);
            }
            pattern="sprop-pps=";
            sprop=strstr(sdp, pattern);
            if (sprop) {
                std::string pps(extractProp(sprop+strlen(pattern)));
                onCfg(id, pps);
            }
            return true;
        }

        std::string extractProp(const char* spropvalue) {
            std::string sdpstr(spropvalue);
            size_t pos = sdpstr.find_first_of(" ;\r\n");
            if (pos != std::string::npos) {
                sdpstr.erase(pos);
            }            
            return sdpstr;
        }

        void onCfg(const char* id, const std::string& prop) {
            unsigned int length = 0;
            unsigned char * decoded = base64Decode(prop.c_str(), length);
            if (decoded) {
                std::string cfg;
                cfg.insert(cfg.end(), H26X_marker, H26X_marker+sizeof(H26X_marker));
                cfg.insert(cfg.end(), decoded, decoded+length);
                onData(id, (unsigned char*)cfg.c_str(), cfg.size(), timeval());
                delete[]decoded;
            }
        }

    private:
        HttpServerRequestHandler&               m_httpServer;
        std::string                             m_uri;
        std::map<std::string,std::string>       m_medias;
        std::map<std::string,std::string>       m_codecs;
        std::string                             m_vps;
        std::string                             m_sps;
        std::string                             m_pps;
};