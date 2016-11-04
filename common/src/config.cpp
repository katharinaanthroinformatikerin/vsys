//
// Created by johann on 22.10.16.
//

#include <stdlib.h>
#include <string>
#include "include/config.h"

config::config() {
    _downloadDir = (std::string)getenv("HOME") + "/Downloads";
}

std::string config::getDownloadDir() const {
    return _downloadDir;
}
