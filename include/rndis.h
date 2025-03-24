#ifndef _RNDIS_H
#define _RNDIS_H

#define RNDIS_MSG_QUERY 0x00000004
#define RNDIS_MSG_INIT 0x00000002
#define RNDIS_MSG_INIT_C 0x80000002
#define RNDIS_SEND_ENCAPSULATED_COMMAND 0x00
#define RNDIS_GET_ENCAPSULATED_RESPONSE 0x01

typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t Oid;
    uint32_t InformationBufferLength;
    uint32_t InformationBufferOffset;
    uint32_t DeviceVcHandle;
} __attribute__((packed, aligned(4))) rndis_query_msg_t;

typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t MajorVersion;
    uint32_t MinorVersion;
    uint32_t MaxTransferSize;
} __attribute__((packed, aligned(4))) rndis_init_msg_t;

typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t Status;
    uint32_t InformationBufferLength;
    uint32_t InformationBufferOffset;
} __attribute__((packed, aligned(4))) rndis_query_complete_t;

typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t Status;
    uint32_t MajorVersion;
    uint32_t MinorVersion;
    uint32_t DeviceFlags;
    uint32_t Medium;
    uint32_t MaxPacketsPerTransfer;
    uint32_t MaxTransferSize;
    uint32_t PacketAlignmentFactor;
    uint32_t AFListOffset;
    uint32_t AFListSize;
} __attribute__((packed, aligned(4))) RNDIS_Initialize_Complete_t;

typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t Status;
} __attribute__((packed, aligned(4))) rndis_set_complete_t;

typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t OID;
    uint32_t InformationBufferLength;
    uint32_t InformationBufferOffset;
    uint32_t _reserved = 0x00000000;
    uint32_t data[5];
} __attribute__((packed, aligned(4))) rndis_set_message_t;

#endif