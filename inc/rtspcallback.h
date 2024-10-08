
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

const int H264_SLICE=1;
const int H264_IDR=5;
const int H264_SPS=7;
const int H264_PPS=8;

const int H265_SLICE=1;
const int H265_VPS=32;
const int H265_SPS=33;
const int H265_PPS=34;
const int H265_IDR_W_RADL=19;
const int H265_IDR_N_LP=20;


class RTSPCallback : public RTSPConnection::Callback
{
    class SessionParams
    {
        public:
            SessionParams(const std::string& media="", const std::string& codec="", unsigned int rtpfrequency=0, unsigned int channels=0)
                : m_media(media), m_codec(codec), m_rtpfrequency(rtpfrequency), m_channels(channels) {
            }

            std::string m_media;
            std::string m_codec;
            unsigned int m_rtpfrequency;
            unsigned int m_channels;
    };

    public:
        RTSPCallback(HttpServerRequestHandler& httpServer, const std::string& uri): m_httpServer(httpServer), m_uri(uri)   {}

        virtual bool    onNewSession(const char* id, const char* media, const char* codec, const char* sdp, unsigned int rtpfrequency, unsigned int channels) {
            std::cout << id << " " << media << "/" <<  codec << " " << rtpfrequency << "/" << channels << std::endl;

            bool ret = false;
            if (strcmp(media, "video") == 0) {
                if (strcmp(codec, "H264") == 0) {
                    m_sessions[id] = SessionParams(media, codec, rtpfrequency, channels);
                    ret = this->onH264Config(id, sdp);
                } else if (strcmp(codec, "H265") == 0) {
                    m_sessions[id] = SessionParams(media, codec, rtpfrequency, channels);
                    ret =  this->onH265Config(id, sdp);
                } else if (strcmp(codec, "JPEG") == 0) {
                    m_sessions[id] = SessionParams(media, codec, rtpfrequency, channels);
                    ret = true;
                } else {
                    std::cout << codec << " not supported" << std::endl;
                }
            } else if (strcmp(media, "audio") == 0) {
                if (strcmp(codec, "MPEG4-GENERIC") == 0) {
                    m_sessions[id] = SessionParams(media, codec, rtpfrequency, channels);
                    ret = true;
                } else if (strcmp(codec, "MPA") == 0) {
                    m_sessions[id] = SessionParams(media, codec, rtpfrequency, channels);
                    ret = true;
                } else if (strcmp(codec, "OPUS") == 0) {
                    m_sessions[id] = SessionParams(media, codec, rtpfrequency, channels);
                    ret = true;
                } else if (strcmp(codec, "PCMU") == 0) {
                    m_sessions[id] = SessionParams(media, codec, rtpfrequency, channels);
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
            std::string codec = m_sessions[id].m_codec;
            if (codec == "H264") {
                this->onH264Data(id, buffer, size, presentationTime);
            } else if (codec == "H265") {
                this->onH265Data(id, buffer, size, presentationTime);
            } else if (codec == "JPEG") {
                this->onDefaultData(id, "jpeg", buffer, size, presentationTime);
            } else if (codec == "MPEG4-GENERIC") {
                this->onDefaultData(id, "mp4a.40.2", buffer, size, presentationTime);
            } else if (codec == "MPA") {
                this->onDefaultData(id, "mp3", buffer, size, presentationTime);
            } else if (codec == "OPUS") {
                this->onDefaultData(id, "opus", buffer, size, presentationTime);
            } else if (codec == "PCMU") {
                this->onDefaultData(id, "ulaw", buffer, size, presentationTime);
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
            for (auto const& x : m_sessions) {
                data[x.first] = x.second.m_media + "/" + x.second.m_codec;
            }
            return data;
        }

    private:
        void publish(const Json::Value & data, const std::string & buf) {
            m_httpServer.publishJSON(m_uri, data);
            m_httpServer.publishBin(m_uri, buf.c_str(), buf.size());
        }

        void onDefaultData(const char* id, const std::string& codec, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
            Json::Value data;
            data["media"] = m_sessions[id].m_media;
            data["codec"] = codec;
            data["freq"] = m_sessions[id].m_rtpfrequency;
            data["channels"] = m_sessions[id].m_channels;
            data["ts"] = Json::Value::UInt64(1000ULL*1000*presentationTime.tv_sec+presentationTime.tv_usec);
            std::string buf(buffer, buffer+size);
            publish(data, buf);                    
        }

        void onH264Data(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
            std::string buf(buffer, buffer+size);
            int nalu = buffer[4] & 0x1F;
            if (nalu == H264_SPS) {
                m_sps = buf;
            } else if (nalu == H264_PPS) {
                m_pps = buf;
            } else if  (nalu == H264_IDR) {
                buf.insert(0, m_pps);
                buf.insert(0, m_sps);
            }
            if (nalu == H264_IDR || nalu == H264_SLICE) {
                Json::Value data;
                data["ts"] = Json::Value::UInt64(1000ULL*1000*presentationTime.tv_sec+presentationTime.tv_usec);
                std::stringstream ss;
                for (int i = 5; (i < 8) && (i < m_sps.size()); i++) {
                    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(m_sps[i]);
                }
                data["media"] = m_sessions[id].m_media;
                data["codec"] = "avc1." + ss.str();   
                if (nalu == H264_IDR) {
                    data["type"] = "keyframe";
                }             
                publish(data, buf);                    
            }
        }

        void onH265Data(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
            std::string buf(buffer, buffer+size);
            int nalu = (buffer[4] & 0x7E)>>1;
            if (nalu == H265_VPS) {
                m_vps = buf;
            } else if (nalu == H265_SPS) {
                m_sps = buf;
            } else if (nalu == H265_PPS) {
                m_pps = buf;
            } else if  (nalu == H265_IDR_W_RADL || nalu == H265_IDR_N_LP) {
                buf.insert(0, m_pps);
                buf.insert(0, m_sps);
                buf.insert(0, m_vps);
            }
            if (nalu == H265_IDR_W_RADL || nalu == H265_IDR_N_LP || nalu == H265_SLICE) {
                Json::Value data;
                data["media"] = m_sessions[id].m_media;
                data["codec"] = "hev1.1.6.L93.B0";
                data["ts"] = Json::Value::UInt64(1000ULL*1000*presentationTime.tv_sec+presentationTime.tv_usec);
                if (nalu == H265_IDR_W_RADL || nalu == H265_IDR_N_LP) {
                    data["type"] = "keyframe";
                }
                publish(data, buf);                    
            }
        }

        bool onH264Config(const char* id, const char* sdp) {
            const char* pattern="sprop-parameter-sets=";
            const char* sprop=strstr(sdp, pattern);
            if (sprop) {
                std::string sdpstr(extractProp(sprop+strlen(pattern)));
                
                std::string sps=sdpstr.substr(0, sdpstr.find_first_of(","));
                onCfg(id, sps);

                std::string pps=sdpstr.substr(sdpstr.find_first_of(",")+1);
                onCfg(id, pps);
            } 
            return true;                       
        }

        bool onH265SPropConfig(const char* pattern, const char* id, const char* sdp) {
            const char* sprop=strstr(sdp, pattern);
            if (sprop) {
                std::string vps(extractProp(sprop+strlen(pattern)));
                onCfg(id, vps);
            }
            return true;
        }

        bool onH265Config(const char* id, const char* sdp) {
            onH265SPropConfig("sprop-vps=", id, sdp);
            onH265SPropConfig("sprop-sps=", id, sdp);
            onH265SPropConfig("sprop-pps=", id, sdp);
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
        std::map<std::string,SessionParams>     m_sessions;
        std::string                             m_vps;
        std::string                             m_sps;
        std::string                             m_pps;
};