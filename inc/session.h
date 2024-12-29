

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

#include <string>

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