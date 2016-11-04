//
// Created by johann on 22.10.16.
//

#ifndef TWFTP_CONFIG_H
#define TWFTP_CONFIG_H

#include <string>

class config {
    std::string _downloadDir;
public:
    config();
    std::string getDownloadDir() const;
};


#endif //TWFTP_CONFIG_H
