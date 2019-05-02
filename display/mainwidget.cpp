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

#include "mainwidget.h"
#include "util.h"
#include "sys/sysinfo.h"
#include <fcntl.h> 
#include <stdio.h> 
#include <unistd.h> 
#include "boardtype_friendlyelec.h"

int fd1;

TMainWidget::TMainWidget(QWidget *parent, bool transparency, const QString& surl) :
    QWidget(parent),bg(QPixmap(":/backgrounds/thompsonscreenhappiness.png")),transparent(transparency),sourceCodeUrl(surl)
{
    const QString qwsDisplay = QString(qgetenv("QWS_DISPLAY"));
    isUsingTFT28LCD = qwsDisplay.contains("/dev/fb-st7789s");
    tft28LCDThread = NULL;
    for (unsigned int i=0; i<sizeof(progresses)/sizeof(int); i++) {
        progresses[i]=0;
    }
    if (isUsingTFT28LCD) {
        tft28LCDThread =new TFT28LCDThread(this, this);
        tft28LCDThread->start();
    }

    mpKeepAliveTimer = new QTimer();
    mpKeepAliveTimer->setSingleShot(false);
    QObject::connect(mpKeepAliveTimer, SIGNAL(timeout()), this, SLOT(onKeepAlive()));
    mpKeepAliveTimer->start(50);

    quitButton = new QPushButton("<< Quit", this);	
    connect(quitButton, SIGNAL(clicked()), this, SLOT(qtdemoButtonClicked()));	

     qtdemoButton = new QPushButton("Start Qt Demo >>", this);	
    connect(qtdemoButton, SIGNAL(clicked()), this, SLOT(qtdemoButtonClicked()));

    gettimeofday(&startTime,NULL);

       
    // FIFO file path 
    char * myfifo = "/tmp/myfifo2"; 
    mkfifo(myfifo, 0666);
    fd1 = open(myfifo,O_RDONLY); 
}

void TMainWidget::qtdemoButtonClicked() {
    QPushButton* btn = (QPushButton*) sender();
    if (btn == qtdemoButton) {
        system("/opt/QtE-Demo/run-qtexample.sh&");
        exit(0);
    } else if (btn == quitButton) {
        QMessageBox::information(this, "Message", "Please press Alt-F2 to login from tty2.");
        exit(0);
    }
}

void TMainWidget::resizeEvent(QResizeEvent*) {
    const int buttonWidth = width()/4;
    const int buttonHeight = height()/12;
    qtdemoButton->setGeometry(width()-buttonWidth-10,height()-5-buttonHeight,buttonWidth, buttonHeight);
    quitButton->setGeometry(10,height()-5-buttonHeight,buttonWidth,buttonHeight);

    if (width() < 800) {
        qtdemoButton->hide();
        quitButton->hide();
    } else {
        qtdemoButton->hide();
        quitButton->hide();
    }
}

static inline double time_diff(struct timeval _tstart,struct timeval _tend) {
  double t1 = 0.;
  double t2 = 0.;

  t1 = ((double)_tstart.tv_sec * 1000 + (double)_tstart.tv_usec/1000.0) ;
  t2 = ((double)_tend.tv_sec * 1000 + (double)_tend.tv_usec/1000.0) ;

  return t2-t1;
}

void TMainWidget::onKeepAlive() {
    static char ipStr[50];
    memset(ipStr, 0, sizeof(ipStr));
    int ret = Util::getIPAddress("eth0", ipStr, 49);
    if (ret == 0) {
        eth0IP = QString(ipStr);
    } else {
        eth0IP = "0.0.0.0";
    }

    memset(ipStr, 0, sizeof(ipStr));
    ret = Util::getIPAddress("wlan0", ipStr, 49);
    if (ret == 0) {
        wlan0IP = QString(ipStr);
    } else {
        wlan0IP = "0.0.0.0";
    }

    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        qint32 totalmem=(qint32)(sys_info.totalram/1048576);
        qint32 freemem=(qint32)(sys_info.freeram/1048576); // divide by 1024*1024 = 1048576
        // float f = ((sys_info.totalram-sys_info.freeram)*1.0/sys_info.totalram)*100;
        // memInfo = QString("%1%,F%2MB").arg(int(f)).arg(freemem);
         memInfo = QString("%1/%2 MB").arg(totalmem-freemem).arg(totalmem);
         usageInfo = QString("%1 Bytes").arg(sys_info.totalram-sys_info.freeram);
    }

    BoardHardwareInfo* retBoardInfo;
    int boardId;
    boardId = getBoardType(&retBoardInfo);
    if (boardId >= 0) {
        if ((boardId >= S5P4418_BASE && boardId <= S5P4418_MAX)
            || (boardId >= S5P6818_BASE && boardId <= S5P6818_MAX)
            ) {
            QString templ_filename("/sys/class/hwmon/hwmon0/device/temp_label");
        QString tempm_filename("/sys/class/hwmon/hwmon0/device/temp_max");
        QFile f1(templ_filename);
        QFile f2(tempm_filename);
        if (f1.exists()) {
            currentCPUTemp = Util::readFile(templ_filename).simplified();
        }
        if (f2.exists()) {
            maxCPUTemp = Util::readFile(tempm_filename).simplified();
        }

    } else if (boardId >= ALLWINNER_BASE && boardId <= ALLWINNER_MAX) {
        QString str;
        bool ok=false;
        QString templ_filename("/sys/class/thermal/thermal_zone0/temp");
        QFile f3(templ_filename);
        if (f3.exists()) {
            float _currentCPUTemp = Util::readFile(templ_filename).simplified().toInt(&ok);
            if (ok) {
                if (_currentCPUTemp > 1000) {
                    _currentCPUTemp = _currentCPUTemp / 1000;
                }
                currentCPUTemp = str.sprintf("%.1f",_currentCPUTemp);
                maxCPUTemp = currentCPUTemp;
            }
        }
    }

    // Temp Parsing

    QString str;
    bool ok=false;
    QString templ_filename("/sys/class/thermal/thermal_zone0/temp");
    QFile f3(templ_filename);
    if (f3.exists()) {
        float _currentCPUTemp = Util::readFile(templ_filename).simplified().toInt(&ok);
        if (ok) {
            if (_currentCPUTemp > 1000) {
                _currentCPUTemp = _currentCPUTemp / 1000;
            }
            currentCPUTemp = str.sprintf("%.1f",_currentCPUTemp);
            maxCPUTemp = currentCPUTemp;
        }
    }
	
	bool ok1=false;
        float _currentCPUTemp = currentCPUTemp.toInt(&ok1);
	if (_currentCPUTemp>1000.0 && ok1) {
	    QString str;
	    currentCPUTemp = str.sprintf("%.1f",_currentCPUTemp/1000.0);
        }
	bool ok2=false;
	float _maxCPUTemp = maxCPUTemp.toInt(&ok2);
        if (_maxCPUTemp>1000.0 && ok2) {
            QString str;
            maxCPUTemp = str.sprintf("%.1f",_maxCPUTemp/1000.0);
        }
    }

    QString contents = Util::readFile("/proc/loadavg").simplified();
    QStringList values = contents.split(" ");
    int vCount = 3;
    loadAvg = "";
    foreach (const QString &v, values) {
        QString str = v.simplified();
        if (!str.isEmpty()) {
            if (!loadAvg.isEmpty()) {
                loadAvg += "/";
            }
            bool ok = false;
            float f = str.toFloat(&ok);
            if (ok) {
                QString toprint;
                loadAvg += toprint.sprintf("%.1f",f);
            }
            vCount--;
            if (vCount <= 0) {
                break;
            }
        }
    }


    QString fileName = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq";
    QFile f5(fileName);
    freqStr = "";
    if (f5.exists()) {
        QString str = Util::readFile(fileName).simplified();
        bool ok = false;
        int freq = str.toInt(&ok,10);
        
        if (ok) {
            QString str;
            if (freq > 1000000) {
                freqStr = str.sprintf("%.1fG",freq*1.0/1000000);
            } else if (freq > 1000) {
                freqStr = str.sprintf("%dM",freq/1000);
            } else {
                freqStr = str.sprintf("%d",freq);
            }
        }
    }

    struct timeval endTime;
    gettimeofday(&endTime,NULL);
    double global_time = time_diff(startTime,endTime); 
    timeSinceStart = "";
    QString timestr;
    timeSinceStart = timestr.sprintf("%.2fms", global_time);


    // First open in read only and read 
    char str1[32], str2[32];  
    size_t bytes_read = read(fd1, str1, 32); 
    if (bytes_read <= 6) {
	printf("Got garbage string: %s\n", str1);
	return;
    }

    QString value = QString(&str1[5]);

    printf("Value: %s\n", value);

    if (strncmp(str1, "oilp", 4) == 0) {
	oilP = value;
    } else if (strncmp(str1, "ctmp", 4) == 0) {
	ctmP = value;
    } else if (strncmp(str1, "vbat", 4) == 0) {
	vbaT = value;
    } else if (strncmp(str1, "lamb", 4) == 0) {
	lamB = value;
    } else if (strncmp(str1, "lspd", 4) == 0) {
	lspD = value;
    } else if (strncmp(str1, "rspd", 4) == 0) {
	rspD = value;
    } else if (strncmp(str1, "rmp_", 4) == 0) {
	rpM_ = value;
    } else if (strncmp(str1, "accx", 4) == 0) {
	accX = value;
    } else if (strncmp(str1, "accy", 4) == 0) {
	accY = value;
    } else if (strncmp(str1, "accz", 4) == 0) {
	accZ = value;
    } else {
	printf("Unknown string: %s\n", str1);
	return;
    }

    /*
    printf("%s\n", str1); 
    // Print the read string and close 
    if (
    char *sub = str1+5;
    int len = 9-5;
    printf("%.*s\n",len,sub);

    std::string string = str1;
    std::string delimiter = ":";
    printf("%s\n", string); 
    std::string token = string.substr(0, string.find(delimiter));
    printf("%s\n", token); 
    cTemp = "";
    QString str;
    printf("%s\n", token); 
    cTemp = str.sprintf("%s",token);
    */
    update();
}

void TMainWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    int space = 3;
    int itemHeight = 20;

    if (!transparent) {
        p.fillRect(0,0,width(),height(),QBrush(QColor(0,0,0)));
        p.drawPixmap(0, 0, width(), height(), bg);
    }

    QString ip = eth0IP;
    if (ip == "0.0.0.0") {
        ip = wlan0IP;
    }

    p.setPen(QPen(QColor(255,255,255)));
    p.drawText(space,itemHeight*23,width()-space*2,itemHeight,Qt::AlignLeft | Qt::AlignVCenter,QString("CPU: %1/T%2").arg(freqStr).arg(currentCPUTemp));
    p.drawText(space*50,itemHeight*23,width()-space*2,itemHeight,Qt::AlignLeft | Qt::AlignVCenter,QString("Memory: %1").arg(memInfo));
    p.drawText(space*120,itemHeight*23,width()-space*2,itemHeight,Qt::AlignLeft | Qt::AlignVCenter,QString("LoadAvg: %1").arg(loadAvg));
    p.drawText(space*180,itemHeight*23,width()-space*2,itemHeight,Qt::AlignLeft | Qt::AlignVCenter,QString("IP: %1").arg(ip));
    

    // p.drawText(0,itemHeight*8,width()-space*9,itemHeight + 30,Qt::AlignRight | Qt::AlignVCenter,QString("Memory: %1").arg(usageInfo));
    p.drawText(5,itemHeight*23,width()-space*9,itemHeight,Qt::AlignRight | Qt::AlignVCenter,QString("t=%1").arg(timeSinceStart));

//// 6 Main Number items on dash
    p.setFont(QFont("Arial",35));
    int blockHeight = 40;
    int sideBorder = 30;
    int fieldWidth = 130;

    p.drawText(sideBorder,blockHeight*2,fieldWidth,blockHeight,Qt::AlignCenter | Qt::AlignVCenter,rpM_);
    p.drawText(sideBorder,blockHeight*5,fieldWidth,blockHeight,Qt::AlignCenter | Qt::AlignVCenter,ctmP);
    p.drawText(sideBorder,blockHeight*8,fieldWidth,blockHeight,Qt::AlignCenter | Qt::AlignVCenter,QString(lamB));
    
    p.drawText(sideBorder,blockHeight*2,width()-fieldWidth,blockHeight,Qt::AlignRight | Qt::AlignVCenter,oilP);
    p.drawText(sideBorder,blockHeight*5,width()-fieldWidth,blockHeight,Qt::AlignRight | Qt::AlignVCenter,vbaT);
    p.drawText(sideBorder,blockHeight*8,width()-fieldWidth,blockHeight,Qt::AlignRight | Qt::AlignVCenter,QString("100"));



    if (1) {
        const int keyCount = sizeof(progresses)/sizeof(int);
        const int maxProgressBarWidth = width()-20;
        const int space = 5;
        const int progressBarX = 10;
        const int progressBarHeight = 20;

        int progressBarY = height()-progressBarHeight*keyCount-space*(keyCount-1);
        for (unsigned int i=0; i<sizeof(progresses)/sizeof(int); i++) {
            int progressBarWidth = int(maxProgressBarWidth * (progresses[i]/100.0));
            QRect rect(progressBarX, progressBarY, progressBarWidth, progressBarHeight);
            if (i==0) {
                p.setBrush(QColor(255,0,0));
            } else if (i==1) {
                p.setBrush(QColor(0,255,0));
            } else if (i==2) {
                p.setBrush(QColor(0,0,255));
            } else {
                p.setBrush(QColor(255,0,255));
            }
            p.drawRect(rect);
            progressBarY += progressBarHeight + space;
        }
    }
}



void TMainWidget::customEvent(QEvent *e)
{
    if (e->type() == TFT28LCDKeyEvent::TFT28LCDKEY_EVENT_TYPE) {
        struct timeval endTime;
        gettimeofday(&endTime,NULL);
        if (time_diff(startTime,endTime) < 1000) {
            // ignore first event
            QWidget::customEvent(e);
            return ;
        }
        TFT28LCDKeyEvent* ee = (TFT28LCDKeyEvent*)e;
        if (ee->key == TFT28LCDKeyEvent::KEY1) {
            progresses[0] += 10;
            if (progresses[0] > 100) {
                progresses[0] = 10;
            }
            update();
        } else if (ee->key == TFT28LCDKeyEvent::KEY2) {
            progresses[1] += 10;
            if (progresses[1] > 100) {
                progresses[1] = 10;
            }
            update();
        } else if (ee->key == TFT28LCDKeyEvent::KEY3) {
            progresses[2] += 10;
            if (progresses[2] > 100) {
                progresses[2] = 10;
            }
            update();
        }
    } else {
        QWidget::customEvent(e);
    }
}
