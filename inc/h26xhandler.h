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

#include "Base64.hh"

#include "codechandler.h"

class H26xHandler: public CodecHandler {
public:
    H26xHandler(const SessionParams& params) : CodecHandler(params) {}

protected:

    void onCfg(const std::string& prop) {
        unsigned int length = 0;
        unsigned char * decoded = base64Decode(prop.c_str(), length);
        if (decoded) {
            std::string cfg;
            cfg.insert(cfg.end(), H26X_marker, H26X_marker+sizeof(H26X_marker));
            cfg.insert(cfg.end(), decoded, decoded+length);
            onData((unsigned char*)cfg.c_str(), cfg.size(), timeval());
            delete[]decoded;
        }
    }

};


