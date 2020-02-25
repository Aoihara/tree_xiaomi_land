/*
   Copyright (c) 2016, The CyanogenMod Project
   Copyright (c) 2017, The XPerience Project
   Copyright (c) 2018, The LineageOS Project

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#include <sys/sysinfo.h>

#include "property_service.h"

namespace android {
namespace init {

using android::base::GetProperty;
using android::init::property_set;

static std::string board_id;

static void property_override(char const prop[], char const value[]) {
    prop_info *pi;

    pi = (prop_info*) __system_property_find(prop);
    if (pi)
        __system_property_update(pi, value, strlen(value));
    else
        __system_property_add(prop, strlen(prop), value, strlen(value));
}

static void import_kernel_cmdline_land(bool in_qemu,
                           const std::function<void(const std::string&, const std::string&, bool)>& fn) {
    std::string cmdline;
    android::base::ReadFileToString("/proc/cmdline", &cmdline);
    for (const auto& entry : android::base::Split(android::base::Trim(cmdline), " ")) {
        std::vector<std::string> pieces = android::base::Split(entry, "=");
        /* The board_id entry has two equal signs, so accept more than two pieces */
        if (pieces.size() >= 2) { // original -> == 2
            fn(pieces[0], pieces[1], in_qemu);
        }
    }
}

static void parse_cmdline_boardid(const std::string& key,
        const std::string& value, bool for_emulator __attribute__((unused))) {
    if (key.empty())
        return;

    /* Here our value is board_id:board_vol; we only want the first part */
    if (key == "board_id") {
        std::istringstream iss(value);
        std::string token;
        std::getline(iss, token, ':');
        board_id = token;
    }
}

static void variant_properties() {
    std::string product = GetProperty("ro.product.name", "");
    if (product.find("land") == std::string::npos)
        return;

    // Get board_id from cmdline
    import_kernel_cmdline_land(false, parse_cmdline_boardid);

    // Set board id
    property_set("ro.product.wt.boardid", board_id.c_str());

    // Set variant based on board_id
    if (board_id == "S88537AB1") {
        property_override("ro.product.model", "Redmi 3X");
        property_override("ro.product.vendor.model", "Redmi 3X");
    }
}

void vendor_load_properties() {
    variant_properties();
}
}
}
