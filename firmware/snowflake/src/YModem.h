/*
    YModem protocol example:
    Receiver: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< C
    Sender:   SOH 00 FF [55 53…6E 00]" "[32…30 00]’’ NUL[96] CRC CRC >>
    Receiver: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ACK
    Receiver: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< C
    Sender:   STX 01 FE data[1024] CRC CRC>>>>>>>>>>>>>>>>>>>>>>>>
    Receiver: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ACK
    Sender:   STX 02 FD data[1024] CRC CRC>>>>>>>>>>>>>>>>>>>>>>>
    Receiver: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ACK
    Sender:   STX 03 FC data[1024] CRC CRC>>>>>>>>>>>>>>>>>>>>>>>
    Receiver: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ACK
    Sender:   STX 04 FB data[1024] CRC CRC>>>>>>>>>>>>>>>>>>>>>>>
    Receiver: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ACK
    Sender:   SOH 05 FA data[100] 1A[28] CRC CRC>>>>>>>>>>>>>>>>>>
    Receiver: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ACK
    Sender:   EOT >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    Receiver: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< NAK
    Sender:   EOT>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    Receiver: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ACK
    Receiver: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< C
    Sender:   SOH 00 FF NUL[128] CRCCRC >>>>>>>>>>>>>>>>>>>>>>
    Receiver: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ACK
 */

#pragma once

#include "application.h"

#include <functional>
#include <string.h>

#define YMODEM_DEBUG 0

#if YMODEM_DEBUG
#define YMODEM_LOG_INFO(fmt, ...) Log.printf(fmt "\r\n", ##__VA_ARGS__)
#define YMODEM_LOG_PRINTF(fmt, ...) Log.printf(fmt, ##__VA_ARGS__)
#else
#define YMODEM_LOG_INFO(fmt, ...)
#define YMODEM_LOG_PRINTF(fmt, ...)
#endif // YMODEM_DEBUG

class YModem {
public:

    static constexpr int YMODEM_PACKET_SEQNO_INDEX = 1;
    static constexpr int YMODEM_PACKET_SEQNO_COMP_INDEX = 2;

    static constexpr int YMODEM_PACKET_HEADER = 3;
    static constexpr int YMODEM_PACKET_TRAILER = 2;
    static constexpr int YMODEM_PACKET_OVERHEAD = (YMODEM_PACKET_HEADER + YMODEM_PACKET_TRAILER);
    static constexpr int YMODEM_PACKET_128B_SIZE = 128;
    static constexpr int YMODEM_PACKET_1K_SIZE = 1024;

    static constexpr int WAIT_BEGIN_TIMEOUT = 30 * 1000; // 30s to wait for the first packet
    static constexpr int PACKET_TIMEOUT = 30 * 1000; // 5s to wait for the next packet
    static constexpr int SYNC_INTERVAL = 1 * 1000; // 1s to send a sync character 'C'
    static constexpr int INVALID_DATA_RETRY = 10; // retry times when receiving invalid data


    enum ControlCharacters {
        SOH     = 0x01,     // start of 128-byte data packet
        STX     = 0x02,     // start of 1024-byte data packet
        EOT     = 0x04,     // end of transmission
        ACK     = 0x06,     // acknowledge
        NAK     = 0x15,     // negative acknowledge
        CA      = 0x18,     // two of these in succession aborts transfer
        CRC16   = 0x43,     // 'C' == 0x43, request 16-bit CRC
        ABORT1  = 0x41,     // 'A' == 0x41, abort by user
        ABORT2  = 0x61      // 'a' == 0x61, abort by user
    };

    enum ErrorCode {
        ERROR_OK = 0,
        ERROR_TIMEOUT = -1,
        ERROR_PACKET = -2,
        ERROR_INVALID_DATA = -3,
        ERROR_USER_ABORT = -4,
    };

    typedef enum {
        EVENT_RECEIVED_INIT_PACKET,
        EVENT_RECEIVED_DATA_PACKET,
    } YModemEvent;

    typedef struct {
        char* fileName;
        size_t fileSize;
    } EventInitPacket;

    typedef struct {
        char* data;
        size_t size;
    } EventDataPacket;

    using YModemEventCb = std::function<int(YModemEvent event, void* param)>;

public:
    YModem() {
        packetBuffer_ = std::unique_ptr<char>(new char[YMODEM_PACKET_1K_SIZE]);
    }
    ~YModem() {
    }

    virtual int read(uint8_t* data, int timeout) = 0;
    virtual int write(uint8_t data, int timeout) = 0;

    int receiveFile(YModemEventCb userCb = nullptr) {
        userCb_ = userCb;

        size_t packetNumber = 0;
#if 0  // TODO: figure out why std::unique_ptr causes secure fault as soon as SerialYModem destructed
        // ERROR: cause secure fault
        std::unique_ptr<char> receivedPacket(new char[YMODEM_PACKET_1K_SIZE]);
        char* receivedPacketPtr = receivedPacket.get();
#else
        static char receivedPacket[YMODEM_PACKET_1K_SIZE];
        char* receivedPacketPtr = receivedPacket;
#endif
        // Stage 1: establish connection and receive the first packet
        uint32_t startTime = millis();
        bool receivedInitialPacket = false;
        do {
            write('C', SYNC_INTERVAL);
            int ret = receivePacket(packetNumber, 1000, receivedPacketPtr, YMODEM_PACKET_128B_SIZE);
            if (ret == YMODEM_PACKET_128B_SIZE && receivedPacketPtr[0] == SOH) {
                receivedInitialPacket = true;
                break;
            }
        } while (millis() - startTime < WAIT_BEGIN_TIMEOUT);

        if (!receivedInitialPacket) {
            return ERROR_TIMEOUT;
        }

        // Get file name and size from the initial packet, emit the event
        char fileName[YMODEM_PACKET_128B_SIZE] = {};
        size_t fileSize = 0;

        parseInitPacket(receivedPacketPtr, fileName, &fileSize);

        YMODEM_LOG_INFO("fileName: %s, size: %d", fileName, (int)fileSize);

        // Emit the event with file information
        EventInitPacket initPacket;
        initPacket.fileName = fileName;
        initPacket.fileSize = fileSize;
        if (userCb_(EVENT_RECEIVED_INIT_PACKET, &initPacket)) {
            sendAbortPacket();
            return ERROR_USER_ABORT;
        } else {
            write(ACK, PACKET_TIMEOUT);
            write('C', PACKET_TIMEOUT);
        }
        packetNumber++;

        // Stage 3: receive the file
        size_t receivedFileSize = 0;
        while (true) {
            int ret = receivePacket(packetNumber, PACKET_TIMEOUT, receivedPacketPtr, YMODEM_PACKET_1K_SIZE);
            if (ret < 0) {
                return ret;
            }

            // EOT packet
            if (receivedPacketPtr[0] == EOT) {
                if (confirmEOT()) {
                    break;
                } else {
                    return ERROR_INVALID_DATA;
                }
            } else {
                if (receivedPacketPtr[0] == SOH && \
                    receivedPacketPtr[1] == 0x00 && \
                    receivedPacketPtr[2] == 0xFF) {
                    // End packet, no need to notify user for this empty packet
                    // format: | SOH | 00 | FF | 00 | 00 | 00 |
                    write(ACK, PACKET_TIMEOUT);
                    continue;
                } else if (receivedPacketPtr[0] == SOH || receivedPacketPtr[0] == STX) {
                    // Data packet received, process it
                    EventDataPacket dataPacket;
                    dataPacket.data = receivedPacketPtr + 3; // Skip header
                    dataPacket.size = (receivedPacketPtr[0] == SOH) ? YMODEM_PACKET_128B_SIZE : YMODEM_PACKET_1K_SIZE;
                    // Last data packet, trim the padding bytes
                    if (fileSize - receivedFileSize < YMODEM_PACKET_1K_SIZE) {
                        dataPacket.size = fileSize - receivedFileSize;
                    }
                    if (userCb_(EVENT_RECEIVED_DATA_PACKET, &dataPacket)) {
                        sendAbortPacket();
                        return ERROR_USER_ABORT;
                    } else {
                        write(ACK, PACKET_TIMEOUT);
                    }
                    packetNumber++;
                    receivedFileSize += dataPacket.size;
                }
            }
        }

        return 0;
    }

private:
    int receivePacket(uint8_t packetNumber, int timeout, void* data, size_t size) {
        size_t packetSize = 0;
        uint8_t dataChar = 0;

        if (read(&dataChar, timeout) < 0) {
            return ERROR_TIMEOUT;
        }

        // Receive SOH/STX packet
        uint8_t *p = static_cast<uint8_t*>(data);
        p[0] = dataChar;

        switch (dataChar) {
            case SOH:
                packetSize = YMODEM_PACKET_128B_SIZE;
                break;
            case STX:
                packetSize = YMODEM_PACKET_1K_SIZE;
                break;
            case EOT:
                return 0;
            case CA:
                read(&dataChar, timeout); // drop the second CA
                return ERROR_USER_ABORT;
            case ABORT1:
            case ABORT2:
                return ERROR_USER_ABORT;
            default:
                return ERROR_INVALID_DATA;
        }

        for (size_t i = 1; i < packetSize + YMODEM_PACKET_OVERHEAD; i++) {
            if (read(&dataChar, timeout) < 0) {
                return ERROR_TIMEOUT;
            }
            p[i] = dataChar;
        }

        // Check packet number
        if ((p[YMODEM_PACKET_SEQNO_INDEX] != packetNumber) || \
            (p[YMODEM_PACKET_SEQNO_INDEX] != ((p[YMODEM_PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))) {
            YMODEM_LOG_INFO("ERROR: packet number! expected num: %d, received num: %d", packetNumber, p[YMODEM_PACKET_SEQNO_INDEX]);
            return ERROR_PACKET;
        }

        // Check CRC16
        uint16_t receivedCrc = static_cast<uint16_t>((p[packetSize + YMODEM_PACKET_HEADER] << 8) | p[packetSize + YMODEM_PACKET_HEADER + 1]);
        uint16_t calculatedCrc = crc16(p + YMODEM_PACKET_HEADER, packetSize);
        if (receivedCrc != calculatedCrc) {
            YMODEM_LOG_INFO("ERROR: CRC! receivedCrc: 0x%x, calculatedCrc: 0x%x", receivedCrc, calculatedCrc);
            return ERROR_PACKET;
        }

        return static_cast<int>(packetSize);
    }

    void parseInitPacket(const char* packet, char* fileName, size_t* fileSize) {
        // The initial packet format is: "FILENAME<NULL>SIZE<NULL>"
        char* p = const_cast<char*>(packet + YMODEM_PACKET_HEADER);
        size_t len = strnlen(p, YMODEM_PACKET_128B_SIZE - YMODEM_PACKET_OVERHEAD - 1);

        // Copy the file name
        strncpy(fileName, p, len);
        fileName[len] = '\0';

        // Move to the size field
        p += len + 1;
        len = strnlen(p, YMODEM_PACKET_128B_SIZE);

        // Parse the file size
        *fileSize = 0;
        for (size_t i = 0; i < len; i++) {
            *fileSize = *fileSize * 10 + (p[i] - '0');
        }
    }

    bool confirmEOT() {
        write(NAK, PACKET_TIMEOUT);
        uint8_t data = 0;
        read(&data, PACKET_TIMEOUT);
        if (data == EOT) {
            write(ACK, PACKET_TIMEOUT);
            return true;
        } else {
            return false;
        }
    }

    void sendAbortPacket() {
        write(CA, PACKET_TIMEOUT);
        write(CA, PACKET_TIMEOUT);
    }

    uint16_t crc16(const uint8_t* data, uint32_t size, uint16_t* lastCrc = NULL) {
        uint16_t crc = (lastCrc == NULL) ? 0xFFFF : *lastCrc;
        for (uint32_t i = 0; i < size; i++) {
            crc = (uint8_t)(crc >> 8) | (crc << 8);
            crc ^= data[i];
            crc ^= (uint8_t)(crc & 0xFF) >> 4;
            crc ^= (crc << 8) << 4;
            crc ^= ((crc & 0xFF) << 4) << 1;
        }
        return crc;
    }

private:
    YModemEventCb    userCb_;
    std::unique_ptr<char> packetBuffer_;
    size_t           packetSize_;
};
