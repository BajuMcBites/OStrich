// #include "rndis.h"
// #include "network_card.h"
// #include "printf.h"

// int rndis_get_maximum_frame_size(usb_session *session) {
//     rndis_query_msg_t query;
//     query.MessageType = RNDIS_MSG_QUERY;
//     query.MessageLength = 0x20;
//     query.RequestId = 3;
//     query.Oid = 0x00010106;
//     query.InformationBufferLength = 0x4;
//     query.InformationBufferOffset = 0x14;

//     usb_setup_packet_t setup;
//     setup.bmRequestType = 0x21;
//     setup.bRequest = RNDIS_SEND_ENCAPSULATED_COMMAND;
//     setup.wValue = 0;
//     setup.wIndex = 0;
//     setup.wLength = 0x20;

//     int result;
//     session->out_data = true;
//     if ((result = usb_handle_transfer(session, &setup, (uint8_t *)&query, 0x20))) {
//         printf("Error sending RNDIS query request (err_code = 0x%x)\n", result);
//         return -1;
//     }

//     uint8_t response[64];

//     setup.bmRequestType = 0xA1;
//     setup.bRequest = 0x01;
//     setup.wValue = 0;
//     setup.wIndex = 0;
//     setup.wLength = sizeof(response);

//     session->out_data = false;
//     if ((result = usb_handle_transfer(session, &setup, response, 64))) {
//         printf("Error receiving RNDIS query response (err_code = 0x%x)\n", result);
//         return -1;
//     }

//     rndis_query_complete_t *resp = (rndis_query_complete_t *)response;

//     if (resp->Status != 0 || resp->MessageType != 0x80000004) {
//         printf("RNDIS query failed with status: 0x%08X\n", resp->Status);
//         return -1;
//     }

//     memcpy((void *)&maximum_frame_size, response + sizeof(rndis_query_complete_t),
//            query.InformationBufferLength);

//     return 0;
// }

// int rndis_current_packet_filter(usb_session *session) {
//     rndis_set_message_t set_msg;
//     set_msg.MessageType = 0x00000005;
//     set_msg.MessageLength = 0x20;
//     set_msg.RequestId = 4;
//     set_msg.OID = 0x0001010E;
//     set_msg.InformationBufferLength = 0x4;
//     set_msg.InformationBufferOffset = 0x14;
//     set_msg.data[0] = 0x00000009;

//     usb_setup_packet_t setup;
//     setup.bmRequestType = 0x21;
//     setup.bRequest = RNDIS_SEND_ENCAPSULATED_COMMAND;
//     setup.wValue = 0;
//     setup.wIndex = 0;
//     setup.wLength = set_msg.MessageLength;

//     int result;
//     session->out_data = true;
//     if ((result =
//              usb_handle_transfer(session, &setup, (uint8_t *)&set_msg, set_msg.MessageLength))) {
//         printf("Error sending RNDIS query request (err_code = 0x%x)\n", result);
//         return -1;
//     }

//     rndis_set_complete_t resp;

//     setup.bmRequestType = 0xA1;
//     setup.bRequest = 0x01;
//     setup.wValue = 0;
//     setup.wIndex = 0;
//     setup.wLength = sizeof(resp);

//     session->out_data = false;
//     if ((result = usb_handle_transfer(session, &setup, (uint8_t *)&resp, sizeof(resp)))) {
//         printf("Error receiving RNDIS query response (err_code = 0x%x)\n", result);
//         return -1;
//     }

//     if (resp.MessageType != 0x80000005 || resp.Status != 0) {
//         printf("RNDIS query failed with status: 0x%08X\n", resp.Status);
//         return -1;
//     }

//     return 0;
// }

// int rndis_prepare_bulk_xfer(usb_session *session) {
//     printf("\n\n RNDIS PREPARE BULK XFER \n\n");

//     if (rndis_get_maximum_frame_size(session)) {
//         printf("Error getting maximum frame size\n");
//         return -1;
//     }

//     if (rndis_current_packet_filter(session)) {
//         printf("Error getting current packet filter\n");
//         return -1;
//     }

//     return 0;
// }

// int rndis_init(usb_session *session) {
//     rndis_init_msg_t init_msg;

//     printf("\n\n RNDIS INIT \n\n");

//     session->mps = 8;

//     printf("seesion->mps = 0x%x\n", session->mps);
//     printf("session->device_address = 0x%x\n", session->device_address);
//     printf("session->out_ep_num = 0x%x\n", session->out_ep_num);
//     printf("session->in_ep_num = 0x%x\n", session->in_ep_num);
//     printf("\n");

//     init_msg.MessageType = RNDIS_MSG_INIT;
//     init_msg.MessageLength = 0x18;
//     init_msg.RequestId = 1;
//     init_msg.MajorVersion = 1;
//     init_msg.MinorVersion = 0;
//     init_msg.MaxTransferSize = 0x4000;

//     usb_setup_packet_t setup;
//     setup.bmRequestType = 0x21;  // Host-to-Device, Class, Interface
//     setup.bRequest = RNDIS_SEND_ENCAPSULATED_COMMAND;
//     setup.wValue = 0;
//     setup.wIndex = 0;
//     setup.wLength = 0x18;

//     session->out_data = true;

//     printf("\n\n`RNDIS INIT TRANSFER BEING\n\n");

//     if (usb_handle_transfer(session, &setup, (uint8_t *)&init_msg, 0x18)) {
//         printf("Error sending RNDIS INIT setup request\n");
//         return -1;
//     }

//     printf("\n \n \nwaiting for device to respond \n \n \n");

//     uint8_t __attribute__((aligned(4))) response[64];

//     printf("response = 0x%08x\n", response);

//     for (size_t i = 0; i < 64; i++) response[i] = 0x69;

//     usb_setup_packet_t setup_response;
//     setup_response.bmRequestType = 0xa1;
//     setup_response.bRequest = RNDIS_GET_ENCAPSULATED_RESPONSE;
//     setup_response.wValue = 0;
//     setup_response.wIndex = 0;
//     setup_response.wLength = 64;
//     session->out_data = false;
//     if (usb_handle_transfer(session, &setup_response, response,
//                             sizeof(RNDIS_Initialize_Complete_t))) {
//         printf("Error receiving RNDIS INIT response\n");
//         return -1;
//     }

//     // Parse the response
//     RNDIS_Initialize_Complete_t *resp = (RNDIS_Initialize_Complete_t *)response;
//     if (resp->Status != 0 && resp->MessageType == 0x80000002) {
//         printf("RNDIS initialization failed with status: 0x%08X\n", resp->Status);
//         return -1;
//     }

//     printf("received RNDIS INIT response\n");

//     return 0;
// }

// int rndis_get_mac_address(usb_session *session) {
//     rndis_query_msg_t query;

//     query.MessageType = RNDIS_MSG_QUERY;
//     query.MessageLength = sizeof(query);
//     query.RequestId = 2;
//     query.Oid = OID_802_3_PERMANENT_ADDRESS;
//     query.InformationBufferLength = 0;
//     query.InformationBufferOffset = 0;

//     usb_setup_packet_t setup;
//     setup.bmRequestType = 0x21;
//     setup.bRequest = RNDIS_SEND_ENCAPSULATED_COMMAND;
//     setup.wValue = 0;
//     setup.wIndex = 0;
//     setup.wLength = sizeof(query);

//     int result;
//     session->out_data = true;
//     if ((result = usb_handle_transfer(session, &setup, (uint8_t *)&query, sizeof(query)))) {
//         printf("Error sending RNDIS query request (err_code = 0x%x)\n", result);
//         return -1;
//     }

//     uint8_t response[32];

//     setup.bmRequestType = 0xA1;
//     setup.bRequest = 0x01;
//     setup.wValue = 0;
//     setup.wIndex = 0;
//     setup.wLength = sizeof(response);

//     session->out_data = false;
//     if ((result = usb_handle_transfer(session, &setup, response, 32))) {
//         printf("Error receiving RNDIS query response (err_code = 0x%x)\n", result);
//         return -1;
//     }

//     rndis_query_complete_t *resp = (rndis_query_complete_t *)response;

//     if (resp->Status != 0 || resp->MessageType != 0x80000004) {
//         printf("RNDIS query failed with status: 0x%08X\n", resp->Status);
//         return -1;
//     }

//     memcpy(mac_address, response + sizeof(rndis_query_complete_t), 6);

//     printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac_address[0], mac_address[1],
//            mac_address[2], mac_address[3], mac_address[4], mac_address[5]);

//     return 0;
// }
