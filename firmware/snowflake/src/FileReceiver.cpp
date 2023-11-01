#include "FileReceiver.h"
#include "YModem.h"
#include "application.h"
#include <fcntl.h>

#define SERIAL Serial

class SerialYModem : public YModem {
public:
    SerialYModem() : YModem() {
        SERIAL.begin(9600);
    }

    virtual ~SerialYModem() {
        SERIAL.end();
    }

    virtual int read(uint8_t* data, int timeout) override {
        uint32_t start = millis();
        do {
            if (SERIAL.available() > 0) {
                *data = (uint8_t)SERIAL.read();
                return 0;
            }
            delay(1);
        } while (millis() - start < (size_t)timeout);
        return -1;
    }

    virtual int write(uint8_t data, int timeout) override {
        uint32_t start = millis();
        do {
            if (SERIAL.availableForWrite() > 0) {
                SERIAL.write(data);
            }
            delay(1);
        } while (millis() - start < (size_t)timeout);
        return -1;
    }
};

void fileReceiver() {
    SerialYModem ymodem;

    while (1) {
        int fd = -1;
        char fileName[256] = {};
        size_t fileSize = 0;
        int ret = ymodem.receiveFile([&](YModem::YModemEvent event, void* param) {
            switch (event) {
                case YModem::EVENT_RECEIVED_INIT_PACKET: {
                    YModem::EventInitPacket* initPacket = (YModem::EventInitPacket*)param;
                    // Log.info("init packet, name: %s, size: %ld", initPacket->fileName, initPacket->fileSize);
                    snprintf(fileName, 256 - 1, "%s", initPacket->fileName);
                    fileSize = initPacket->fileSize;
                    remove(fileName);
                    fd = open(fileName, O_CREAT | O_WRONLY | O_TRUNC);
                    if (fd < 0) {
                        Log.error("open file failed, fileName: %s, fd: %d", fileName, fd);
                        return -1;
                    }
                    break;
                }
                case YModem::EVENT_RECEIVED_DATA_PACKET: {
                    YModem::EventDataPacket* dataPacket = (YModem::EventDataPacket*)param;
                    if (fd < 0 || write(fd, dataPacket->data, dataPacket->size) != dataPacket->size) {
                        Log.error("file operation failed");
                        return -1;
                    }
                    break;
                }
            }
            return 0;
        });
        close(fd);

        if (ret == 0) {
            struct stat fileStat = {};
            if (stat(fileName, &fileStat) != 0) {
                Log.info("Error while getting file size");
            }

            if ((size_t)fileStat.st_size == fileSize) {
                Log.info("YModem receive file O.K.");
            } else {
                Log.error("ERROR: File size does not match");
            }
        } else {
            Log.error("ERROR: YModem receive file failed!");
        }
    }
}

