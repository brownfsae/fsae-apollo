// Copyright (C) Guangzhou FriendlyARM Computer Tech. Co., Ltd.
// (http://www.friendlyarm.com)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, you can access it online at
// http://www.gnu.org/licenses/gpl-2.0.html.

#include "util.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <asm/types.h>
#include <netinet/ether.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int Util::getIPAddress(const char* ifaceName, char* ip, size_t maxLen) {
    int s;
    struct ifconf conf;
    struct ifreq *ifr;
    char buff[BUFSIZ];
    int num;
    int i;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        return -1;
    }

    conf.ifc_len = BUFSIZ;
    conf.ifc_buf = buff;

    ioctl(s, SIOCGIFCONF, &conf);
    num = conf.ifc_len / sizeof(struct ifreq);
    ifr = conf.ifc_req;

    for(i=0;i < num;i++) {
        struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);

        if (ioctl(s, SIOCGIFFLAGS, ifr) < 0) {
            ::close(s);
            return -1;
        }

        if(((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP))
        {
            if (strcmp(ifaceName, ifr->ifr_name) == 0) {
                strncpy(ip, inet_ntoa(sin->sin_addr), maxLen );
                ::close(s);
                return 0;
            }
        }
        ifr++;
    }

    ::close(s);
    return -1;
}

QString Util::seconds_to_DHMS(unsigned long duration)
{
    int seconds = (int) (duration % 60);
    duration /= 60;
    int minutes = (int) (duration % 60);
    duration /= 60;
    int hours = (int) (duration % 24);
    int days = (int) (duration / 24);
    if((hours == 0)&&(days == 0))
        return QString::fromUtf8("%1分%2秒").arg(minutes).arg(seconds);
    if (days == 0)
        return QString::fromUtf8("%1时%2分%3秒").arg(hours).arg(minutes).arg(seconds);
    return QString::fromUtf8("%1天%2时%3分%4秒").arg(days).arg(hours).arg(minutes).arg(seconds);
}

QString Util::seconds_to_DHMS_US(unsigned long duration)
{
    int seconds = (int) (duration % 60);
    duration /= 60;
    int minutes = (int) (duration % 60);
    duration /= 60;
    int hours = (int) (duration % 24);
    int days = (int) (duration / 24);
    if((hours == 0)&&(days == 0))
        return QString::fromUtf8("%1m%2s").arg(minutes).arg(seconds);
    if (days == 0)
        return QString::fromUtf8("%1h%2m%3s").arg(hours).arg(minutes).arg(seconds);
    return QString::fromUtf8("%1d%2h%3m").arg(days).arg(hours).arg(minutes);
}

QString Util::readFile (const QString& filename) {
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QString result = stream.readAll();
        file.close();
        return result;
    } else {
        qDebug() << "open file error: " << filename;
    }
    return "";
}

int Util::GetMemInfoMB() {
    QProcess p;
    p.start("awk", QStringList() << "/MemTotal/ { print $2 }" << "/proc/meminfo");
    p.waitForFinished();
    QString memory = p.readAllStandardOutput();
    int result = memory.toLong() / 1024;
    p.close();
    return result;
}
