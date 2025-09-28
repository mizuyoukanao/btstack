//==============================================================================================
/**
 * @file	btkeyLib.c
 * @brief	Bluetoothアダプタをワイヤレスコントローラに偽装する
 * @date	2021.06.20
 */
 //==============================================================================================
#define BTSTACK_FILE__ "btkeyLib.c"
#define __EnabledAmiibo true
#define __EnabledRumble true

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <windows.h>
#include "btstack.h"
#include "btstack_stdin.h"

static const char hid_device_name[] = "Pro Controller";

static btstack_packet_callback_registration_t hci_event_callback_registration;
static uint16_t hid_cid;
uint16_t pairing_state;
bool paired = false;
uint8_t tim = 0;

#if __EnabledAmiibo
bool packet_amiibo_flag = false;
#endif

static uint8_t hid_service_buffer[900];
static uint8_t device_id_sdp_service_buffer[400];
static uint8_t l2cap_sdp_service_buffer[600];

#if __EnabledRumble
bool rumble_flag = false;
#endif

//----------------------------------------------------------
// 送信パケット
//----------------------------------------------------------
static const uint8_t reportMap[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x06, 0x01, 0xFF,  //   Usage Page (Vendor Defined 0xFF01)

    0x85, 0x21,  //   Report ID (33)
    0x09, 0x21,  //   Usage (0x21)
    0x75, 0x08,  //   Report Size (8)
    0x95, 0x30,  //   Report Count (48)
    0x81, 0x02,  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                 //   Position)

    0x85, 0x30,  //   Report ID (48)
    0x09, 0x30,  //   Usage (0x30)
    0x75, 0x08,  //   Report Size (8)
    0x95, 0x30,  //   Report Count (48)
    0x81, 0x02,  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                 //   Position)

    0x85, 0x31,        //   Report ID (49)
    0x09, 0x31,        //   Usage (0x31)
    0x75, 0x08,        //   Report Size (8)
    0x96, 0x69, 0x01,  //   Report Count (361)
    0x81, 0x02,  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                 //   Position)

    0x85, 0x32,        //   Report ID (50)
    0x09, 0x32,        //   Usage (0x32)
    0x75, 0x08,        //   Report Size (8)
    0x96, 0x69, 0x01,  //   Report Count (361)
    0x81, 0x02,  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                 //   Position)

    0x85, 0x33,        //   Report ID (51)
    0x09, 0x33,        //   Usage (0x33)
    0x75, 0x08,        //   Report Size (8)
    0x96, 0x69, 0x01,  //   Report Count (361)
    0x81, 0x02,  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                 //   Position)

    0x85, 0x3F,  //   Report ID (63)
    0x05, 0x09,  //   Usage Page (Button)
    0x19, 0x01,  //   Usage Minimum (0x01)
    0x29, 0x10,  //   Usage Maximum (0x10)
    0x15, 0x00,  //   Logical Minimum (0)
    0x25, 0x01,  //   Logical Maximum (1)
    0x75, 0x01,  //   Report Size (1)
    0x95, 0x10,  //   Report Count (16)
    0x81, 0x02,  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                 //   Position)
    0x05, 0x01,  //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x39,  //   Usage (Hat switch)
    0x15, 0x00,  //   Logical Minimum (0)
    0x25, 0x07,  //   Logical Maximum (7)
    0x75, 0x04,  //   Report Size (4)
    0x95, 0x01,  //   Report Count (1)
    0x81, 0x42,  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null
                 //   State)
    0x05, 0x09,  //   Usage Page (Button)
    0x75, 0x04,  //   Report Size (4)
    0x95, 0x01,  //   Report Count (1)
    0x81, 0x01,  //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No
                 //   Null Position)
    0x05, 0x01,  //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,  //   Usage (X)
    0x09, 0x31,  //   Usage (Y)
    0x09, 0x33,  //   Usage (Rx)
    0x09, 0x34,  //   Usage (Ry)
    0x16, 0x00, 0x00,              //   Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00,  //   Logical Maximum (65534)
    0x75, 0x10,                    //   Report Size (16)
    0x95, 0x04,                    //   Report Count (4)
    0x81, 0x02,  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                 //   Position)
    0x06, 0x01, 0xFF,  //   Usage Page (Vendor Defined 0xFF01)

    0x85, 0x01,  //   Report ID (1)
    0x09, 0x01,  //   Usage (0x01)
    0x75, 0x08,  //   Report Size (8)
    0x95, 0x30,  //   Report Count (48)
    0x91, 0x02,  //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                 //   Null Position,Non-volatile)

    0x85, 0x10,  //   Report ID (16)
    0x09, 0x10,  //   Usage (0x10)
    0x75, 0x08,  //   Report Size (8)
    0x95, 0x09,  //   Report Count (9)
    0x91, 0x02,  //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                 //   Null Position,Non-volatile)

    0x85, 0x11,  //   Report ID (17)
    0x09, 0x11,  //   Usage (0x11)
    0x75, 0x08,  //   Report Size (8)
    0x95, 0x30,  //   Report Count (48)
    0x91, 0x02,  //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                 //   Null Position,Non-volatile)

    0x85, 0x12,  //   Report ID (18)
    0x09, 0x12,  //   Usage (0x12)
    0x75, 0x08,  //   Report Size (8)
    0x95, 0x30,  //   Report Count (48)
    0x91, 0x02,  //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                 //   Null Position,Non-volatile)
    0xC0,        // End Collection
};
static uint8_t reply00[] = { 0xa1, 0x30, 0x00, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply01_1[] = { 0xa1, 0x21, 0x04, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x81, 0x01, 0x01, 0xD4, 0xF0, 0x57, 0x8D, 0x74, 0x23, 0x00, 0x25, 0x08, 0x50, 0x72, 0x6F, 0x20, 0x43, 0x6F, 0x6E, 0x74, 0x72, 0x6F, 0x6C, 0x6C, 0x65, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00 };
static uint8_t reply01_2[] = { 0xa1, 0x21, 0x04, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x81, 0x01, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t reply01_3[] = { 0xa1, 0x21, 0x04, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x81, 0x01, 0x03 };
static uint8_t reply03[] = { 0xa1, 0x21, 0x05, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply02[] = { 0xa1, 0x21, 0x00, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x82, 0x02, 0x03, 0x48, 0x03, 0x02, 0xD4, 0xF0, 0x57, 0x8D, 0x74, 0x23, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply04[] = { 0xa1, 0x21, 0x06, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x83, 0x04, 0x2C, 0x01, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply08[] = { 0xa1, 0x21, 0x02, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x80, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply1010[] = { 0xa1, 0x21, 0x04, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x90, 0x10, 0x10, 0x80, 0x00, 0x00, 0x18, 0x00, 0x00 };
static uint8_t reply1020[] = { 0xa1, 0x21, 0x57, 0x8E, 0x00, 0x00, 0x00, 0x9A, 0x77, 0x8C, 0x00, 0x00, 0x00, 0x90, 0x90, 0x10, 0x20, 0x60, 0x00, 0x00, 0x18, 0x8c, 0x00, 0x31, 0x00, 0xDC, 0x00, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0xEF, 0xFF, 0xEB, 0xFF, 0xD2, 0xFF, 0x3B, 0x34, 0x3B, 0x34, 0x3B, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply103D[] = { 0xa1, 0x21, 0x05, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x90, 0x10, 0x3D, 0x60, 0x00, 0x00, 0x19, 0x07, 0xD6, 0x65, 0xF7, 0x97, 0x7F, 0x81, 0x06, 0x5c, 0x1f, 0x78, 0x77, 0x4e, 0x56, 0x5f, 0xb8, 0xd5, 0x65, 0xFF, 0x2D, 0x2D, 0x2D, 0xE6, 0xE6, 0xE6, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply1050[] = { 0xa1, 0x21, 0x04, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x90, 0x10, 0x50, 0x60, 0x00, 0x00, 0x18, 0x00, 0x95, 0xd9, 0xe6, 0x00, 0x33, 0xff, 0xd9, 0x00, 0x3e, 0xb3, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply1060[] = { 0xa1, 0x21, 0x03, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x90, 0x10, 0x00, 0x60, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply1080[] = { 0xa1, 0x21, 0x04, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x90, 0x10, 0x80, 0x60, 0x00, 0x00, 0x18, 0x50, 0xFD, 0x00, 0x00, 0xC6, 0x0f, 0x0f, 0x30, 0x61, 0xAE, 0x90, 0xD9, 0xD4, 0x14, 0x54, 0x41, 0x15, 0x54, 0xC7, 0x79, 0x9C, 0x33, 0x36, 0x63, 0x00, 0x00 };
static uint8_t reply1098[] = { 0xa1, 0x21, 0x04, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x90, 0x10, 0x98, 0x60, 0x00, 0x00, 0x12, 0x0f, 0x30, 0x61, 0xAE, 0x90, 0xd9, 0xd4, 0x14, 0x54, 0x41, 0x15, 0x54, 0xc7, 0x79, 0x9c, 0x33, 0x36, 0x63 };
static uint8_t reply3001[] = { 0xa1, 0x21, 0x04, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x80, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply3333[] = { 0xa1, 0x21, 0x03, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply4001[] = { 0xa1, 0x21, 0x04, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply4801[] = { 0xa1, 0x21, 0x04, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x80, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply0331[] = { 0xa1, 0x21, 0x00, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply1101[312] = { 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x1B, 0x04 };
static uint8_t reply11011[312] = { 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x1B, 0x01 };
static uint8_t replyFF[312] = { 0xFF };
static uint8_t reply2201[] = { 0xa1, 0x21, 0x00, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x80, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t reply110201[312] = { 0x2A, 0x00, 0x05, 0x00, 0x00, 0x09, 0x31, 0x09, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x00, 0x07 };
static uint8_t reply110204[312] = { 0x2A, 0x00, 0x05, 0x00, 0x00, 0x09, 0x31 };
static uint8_t reply2121[] = { 0xA1, 0x21, 0x00, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0xA0, 0x21, 0x01, 0x00, 0xFF, 0x00, 0x08, 0x00, 0x1B, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC8 };

uint8_t spilen = 0;

typedef struct {
    uint8_t* data;
    uint32_t size;
}
swpacket;

static swpacket packet_list[16] = {
    {reply00, sizeof(reply00)},
    {reply02, sizeof(reply02)},
    {reply08, sizeof(reply08)},
    {reply1060, sizeof(reply1060)},
    {reply1050, sizeof(reply1050)},
    {reply03, sizeof(reply03)},
    {reply04, sizeof(reply04)},
    {reply1080, sizeof(reply1080)},
    {reply1098, sizeof(reply1098)},
    {reply1010, sizeof(reply1010)},
    {reply103D, sizeof(reply103D)},
    {reply1020, sizeof(reply1020)},
    {reply4001, sizeof(reply4001)},
    {reply4801, sizeof(reply4801)},
    {reply3001, sizeof(reply3001)},
    {reply2121, sizeof(reply2121)}
};

//----------------------------------------------------------
// CRC8
//----------------------------------------------------------

#if __EnabledAmiibo
static const uint8_t CRC_TABLE[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
    0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
    0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
    0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
    0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
    0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
    0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
    0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
    0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
    0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
    0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
    0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

uint8_t crc8ccitt(const void* data, size_t size) {
    uint8_t val = 0;

    uint8_t* pos = (uint8_t*)data;
    uint8_t* end = pos + size;

    while (pos < end) {
        val = CRC_TABLE[val ^ *pos];
        pos++;
    }

    return val;
}
#endif

//----------------------------------------------------------
// Amiibo関連フラグ
//----------------------------------------------------------
#if __EnabledAmiibo
static uint8_t amiibo_all_data[540];
static uint8_t amiibo_data1[245];
static uint8_t amiibo_data2[295];
static uint8_t amiibo_UID[7];
static uint8_t nfc_packet1[] = { 0x3a, 0x00, 0x07, 0x01, 0x00, 0x01, 0x31, 0x02, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x07 };
static uint8_t nfc_packet2[] = { 0x00, 0x00, 0x00, 0x00, 0x7D, 0xFD, 0xF0, 0x79, 0x36, 0x51, 0xAB, 0xD7, 0x46, 0x6E, 0x39, 0xC1, 0x91, 0xBA, 0xBE, 0xB8, 0x56, 0xCE, 0xED, 0xF1, 0xCE, 0x44, 0xCC, 0x75, 0xEA, 0xFB, 0x27, 0x09, 0x4D, 0x08, 0x7A, 0xE8, 0x03, 0x00, 0x3B, 0x3C, 0x77, 0x78, 0x86, 0x00, 0x00 };
static uint8_t nfc_packet3[] = { 0x3a, 0x00, 0x07, 0x02, 0x00, 0x09, 0x27 };
static uint8_t nfc_packet4[] = { 0x2a, 0x00, 0x05, 0x00, 0x00, 0x09, 0x31, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x00, 0x07 };
static uint8_t amiibo_flg1[312];
static uint8_t amiibo_flg2[312];
static uint8_t amiibo_flg3[312];
static uint8_t reportplusamiibo[363];
static uint8_t crcflg1, crcflg2, crcflg3, crcdata1;
#endif

//----------------------------------------------------------
// DLL関数
// amiiboデータ送信
//----------------------------------------------------------
#if __EnabledAmiibo
void WINAPI send_amiibo(char* string)
{
    // TODO: forループ用の変数を減らす
    int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;
    FILE* fp;
    errno_t error = fopen_s(&fp, string, "rb");
    if (error != 0) {
        return;
    }
    fread(amiibo_all_data, sizeof(uint8_t), 540, fp);
    fclose(fp);

    amiibo_UID[0] = amiibo_all_data[0];
    amiibo_UID[1] = amiibo_all_data[1];
    amiibo_UID[2] = amiibo_all_data[2];
    amiibo_UID[3] = amiibo_all_data[4];
    amiibo_UID[4] = amiibo_all_data[5];
    amiibo_UID[5] = amiibo_all_data[6];
    amiibo_UID[6] = amiibo_all_data[7];
    reply110201[16] = amiibo_all_data[0];
    reply110201[17] = amiibo_all_data[1];
    reply110201[18] = amiibo_all_data[2];
    reply110201[19] = amiibo_all_data[4];
    reply110201[20] = amiibo_all_data[5];
    reply110201[21] = amiibo_all_data[6];
    reply110201[22] = amiibo_all_data[7];

    for (i = 0; i < 245; i++)
    {
        amiibo_data1[i] = amiibo_all_data[i];
    }

    for (k = 0, j = 245; j < 540; k++, j++)
    {
        amiibo_data2[k] = amiibo_all_data[j];
    }

    for (a = 0; a < 15; a++)
    {
        amiibo_flg1[a] = nfc_packet1[a];
    }

    for (b = 15, c = 0; c < 7; b++, c++)
    {
        amiibo_flg1[b] = amiibo_UID[c];
    }

    for (d = 22, e = 0; e < 45; d++, e++)
    {
        amiibo_flg1[d] = nfc_packet2[e];
    }

    for (f = 67, g = 0; g < 245; f++, g++)
    {
        amiibo_flg1[f] = amiibo_data1[g];
    }

    for (h = 0; h < 7; h++)
    {
        amiibo_flg2[h] = nfc_packet3[h];
    }

    for (l = 7, m = 0; m < 295; l++, m++)
    {
        amiibo_flg2[l] = amiibo_data2[m];
    }

    for (n = 0; n < 16; n++)
    {
        amiibo_flg3[n] = nfc_packet4[n];
    }

    for (o = 16, p = 0; p < 7; o++, p++)
    {
        amiibo_flg3[o] = amiibo_UID[p];
    }

    crcflg1 = crc8ccitt(amiibo_flg1, sizeof(amiibo_flg1));
    crcflg2 = crc8ccitt(amiibo_flg2, sizeof(amiibo_flg2));
    crcflg3 = crc8ccitt(amiibo_flg3, sizeof(amiibo_flg3));
    crcdata1 = crc8ccitt(reply110201, sizeof(reply110201));

    //デバッグ用
    //printf("success");
}
#endif

//----------------------------------------------------------
// ボタン押下フラグ
//----------------------------------------------------------
static uint32_t button_flg = 0;
static uint32_t stick_r_flg = 0x800800;
static uint32_t stick_l_flg = 0x800800;
static int16_t accel_x_flg = 100;
static int16_t accel_y_flg = 100;
static int16_t accel_z_flg = 100;
static int16_t gyro1_flg = 0x0000;
static int16_t gyro2_flg = 0x0000;
static int16_t gyro3_flg = 0x0000;
#if __EnabledRumble
static uint32_t rumble_bflag = 0;
#endif

typedef struct {
    uint8_t timer;
    uint8_t battery_connection_info;
    uint8_t buttons1;
    uint8_t buttons2;
    uint8_t buttons3;
    uint8_t ldata1;
    uint8_t ldata2;
    uint8_t ldata3;
    uint8_t rdata1;
    uint8_t rdata2;
    uint8_t rdata3;
    uint8_t accelx;
    uint8_t accelx2;
    uint8_t accely;
    uint8_t accely2;
    uint8_t accelz;
    uint8_t accelz2;
    uint8_t gyro1;
    uint8_t gyro1_2;
    uint8_t gyro2;
    uint8_t gyro2_2;
    uint8_t gyro3;
    uint8_t gyro3_2;
}
gamepad_report_t;

static gamepad_report_t joy;

//----------------------------------------------------------
// ボタン押下時間を管理する
//----------------------------------------------------------
//clock_t button_clk[24];
//clock_t stick_r_clk;
//clock_t stick_l_clk;
clock_t start;
clock_t end;
double send_delay = 1/60;

//----------------------------------------------------------
// DLL関数
// switchと接続されているかどうかを確認する
//----------------------------------------------------------
bool WINAPI gamepad_paired()
{
    return paired;
}

//----------------------------------------------------------
// DLL関数
// ボタン押下フラグ送信
//----------------------------------------------------------
void WINAPI send_button(uint32_t button_status, uint32_t press_time)
{
    UNUSED(press_time);
    //log_info("send_button: %08x %d", button_status, press_time);
    //if (press_time == 0) press_time = 0xFFFF;
    button_flg = button_status;
    //for (int i = 0; i < 24; i++)
    //{
    //    if (button_status & (1 << i))
    //    {
    //        button_clk[i] = clock() + press_time;
    //    }
    //}
}

//----------------------------------------------------------
// DLL関数
// 右スティック状態送信
//----------------------------------------------------------
void WINAPI send_stick_r(uint32_t stick_horizontal, uint32_t stick_vertical, uint32_t press_time)
{
    UNUSED(press_time);
    //if (press_time == 0) press_time = 0xFFFF;
    stick_r_flg = stick_horizontal | (stick_vertical << 12);
    //stick_r_clk = clock() + press_time;
}

//----------------------------------------------------------
// DLL関数
// 左スティック状態送信
//----------------------------------------------------------
void WINAPI send_stick_l(uint32_t stick_horizontal, uint32_t stick_vertical, uint32_t press_time)
{
    UNUSED(press_time);
    //if (press_time == 0) press_time = 0xFFFF;
    stick_l_flg = stick_horizontal | (stick_vertical << 12);
    //stick_l_clk = clock() + press_time;
}

//----------------------------------------------------------
// DLL関数
// ジャイロ状態送信
//----------------------------------------------------------
void WINAPI send_gyro(int16_t gyro1, int16_t gyro2, int16_t gyro3)
{
    gyro1_flg = gyro1;
    gyro2_flg = gyro2;
    gyro3_flg = gyro3;
    //printf("gyro1:%d, gyro2:%d, gyro3:%d\n", gyro1, gyro2, gyro3);
    //gyro1_flg = gyro1;
    //gyro2_flg = gyro2;
    //gyro3_flg = gyro3;
    //joy.gyro1 = gyro1;
    //joy.gyro1_2 = gyro1_2;
    //joy.gyro2 = gyro2;
    //joy.gyro2_2 = gyro2_2;
    //joy.gyro3 = gyro3;
    //joy.gyro3_2 = gyro3_2;
    //printf("gyro1:%d,%d\n", gyro1, gyro1_2);
    //printf("gyro2:%d,%d\n", gyro2, gyro2_2);
    //printf("gyro3:%d,%d\n", gyro3, gyro3_2);
}

//----------------------------------------------------------
// DLL関数
// 加速度状態送信
//----------------------------------------------------------
void WINAPI send_accel(int16_t accel_x, int16_t accel_y, int16_t accel_z)
{
    accel_x_flg = accel_x;
    accel_y_flg = accel_y;
    accel_z_flg = accel_z;
    //printf("accelx:%d, accely:%d, accelz:%d\n", accel_x, accel_y, accel_z);
}

//----------------------------------------------------------
// DLL関数
// カラー変更
//----------------------------------------------------------
void WINAPI send_padcolor(uint32_t pad_color, uint32_t button_color, uint32_t leftgrip_color, uint32_t rightgrip_color)
{
    // コントローラ本体カラー
    reply1050[0x15] = (pad_color >> 0x0) & 0xFF; // B
    reply1050[0x16] = (pad_color >> 0x08) & 0xFF; // G
    reply1050[0x17] = (pad_color >> 0x10) & 0xFF; // R
    
    // ボタンカラー
    reply1050[0x18] = (button_color >> 0x0) & 0xFF; // B
    reply1050[0x19] = (button_color >> 0x08) & 0xFF; // G
    reply1050[0x1a] = (button_color >> 0x10) & 0xFF; // R
    
    // 左グリップカラー
    reply1050[0x1b] = (leftgrip_color >> 0x0) & 0xFF; // B
    reply1050[0x1c] = (leftgrip_color >> 0x08) & 0xFF; // G
    reply1050[0x1d] = (leftgrip_color >> 0x10) & 0xFF; // R
    
    // 右グリップカラー
    reply1050[0x1e] = (rightgrip_color >> 0x0) & 0xFF; // B
    reply1050[0x1f] = (rightgrip_color >> 0x08) & 0xFF; // G
    reply1050[0x20] = (rightgrip_color >> 0x10) & 0xFF; // R

}

//----------------------------------------------------------
// DLL関数
// 振動検知
//----------------------------------------------------------
#if __EnabledRumble
bool WINAPI get_rumble()
{
    return rumble_flag;
}
void WINAPI rumble_register(uint32_t key)
{
    rumble_bflag = key;
}
#endif

//----------------------------------------------------------
// DLL関数
// 通信切断
//----------------------------------------------------------
//void WINAPI shutdown_gamepad()
//{
    //hci_power_control(HCI_POWER_OFF);
    //btstack_stdin_reset();
    //exit(0);
//}

//----------------------------------------------------------
// 保持されているボタンフラグをSwitch本体へ送信する
//----------------------------------------------------------
static void send_report_joystick(void)
{

    double proc_time = (start - end) / CLOCKS_PER_SEC;

    //for (int i = 0; i < 24; i++)
    //{
    //    if ((button_flg & (1 << i)) && (button_clk[i] <= clock()))
    //    {
    //        button_flg &= ~(1 << i);
    //    }
    //}

    //if (stick_r_clk <= clock())
    //{
    //    stick_r_flg = 0x800800;
    //}

    //if (stick_l_clk <= clock())
    //{
    //    stick_l_flg = 0x800800;
    //}

    joy.battery_connection_info = 0x8E;

    joy.buttons1 = (button_flg >> 0x00) & 0xFF;
    joy.buttons2 = (button_flg >> 0x08) & 0xFF;
    joy.buttons3 = (button_flg >> 0x10) & 0xFF;

    if (rumble_flag == true && rumble_bflag != 0)
    {
        joy.buttons1 |= (rumble_bflag) & 0xFF;
        joy.buttons2 |= (rumble_bflag >> 8) & 0xFF;
        joy.buttons3 |= (rumble_bflag >> 16) & 0xFF;
    }
    joy.ldata1 = (stick_l_flg >> 0x00) & 0xFF;
    joy.ldata2 = (stick_l_flg >> 0x08) & 0xFF;
    joy.ldata3 = (stick_l_flg >> 0x10) & 0xFF;
    
    joy.rdata1 = (stick_r_flg >> 0x00) & 0xFF;
    joy.rdata2 = (stick_r_flg >> 0x08) & 0xFF;
    joy.rdata3 = (stick_r_flg >> 0x10) & 0xFF;

    joy.gyro1 = gyro1_flg & 0xFF;
    joy.gyro1_2 = (gyro1_flg >> 8);
    joy.gyro2 = gyro2_flg & 0xFF;
    joy.gyro2_2 = (gyro2_flg >> 8);
    joy.gyro3 = gyro3_flg & 0xFF;
    joy.gyro3_2 = (gyro3_flg >> 8);
    //joy.gyro2_2 = gyro2_flg & 0xFF;
    //joy.gyro2 = (gyro2_flg >> 8);
    //joy.gyro3_2 = gyro3_flg & 0xFF;
    //joy.gyro3 = (gyro3_flg >> 8);

    joy.accelx = accel_x_flg & 0xFF;
    joy.accelx2 = (accel_x_flg >> 8);
    joy.accely = accel_y_flg & 0xFF;
    joy.accely2 = (accel_y_flg >> 8);
    joy.accelz = accel_z_flg & 0xFF;
    joy.accelz2 = (accel_z_flg >> 8);

    joy.timer++;

    uint8_t report[] = {
        0xA1,
        0x30,
        joy.timer,
        joy.battery_connection_info,
        joy.buttons1,
        joy.buttons2,
        joy.buttons3,
        joy.ldata1,
        joy.ldata2,
        joy.ldata3,
        joy.rdata1,
        joy.rdata2,
        joy.rdata3,
        0x00,
        joy.accelx,
        joy.accelx2,
        joy.accely,
        joy.accely2,
        joy.accelz,
        joy.accelz2,
        joy.gyro1,
        joy.gyro1_2,
        joy.gyro2,
        joy.gyro2_2,
        joy.gyro3,
        joy.gyro3_2,
        joy.accelx,
        joy.accelx2,
        joy.accely,
        joy.accely2,
        joy.accelz,
        joy.accelz2,
        joy.gyro1,
        joy.gyro1_2,
        joy.gyro2,
        joy.gyro2_2,
        joy.gyro3,
        joy.gyro3_2,
        joy.accelx,
        joy.accelx2,
        joy.accely,
        joy.accely2,
        joy.accelz,
        joy.accelz2,
        joy.gyro1,
        joy.gyro1_2,
        joy.gyro2,
        joy.gyro2_2,
        joy.gyro3,
        joy.gyro3_2
        };

    start = clock();
    if (proc_time < send_delay) {
        Sleep((send_delay - proc_time) * 1000);
        hid_device_send_interrupt_message(hid_cid, report, sizeof(report));
    } else {
        hid_device_send_interrupt_message(hid_cid, report, sizeof(report));
    }
    end = clock();

}

//----------------------------------------------------------
// 保持されているボタンフラグ+amiiboのデータをSwitch本体へ送信する
//----------------------------------------------------------
#if __EnabledAmiibo
static void send_report_joystick_amiibo(uint8_t* flg, uint8_t crcdata)
{
    //for (int i = 0; i < 24; i++)
    //{
    //    if ((button_flg & (1 << i)) && (button_clk[i] <= clock()))
    //    {
    //        button_flg &= ~(1 << i);
    //    }
    //}

    //if (stick_r_clk <= clock())
    //{
    //    stick_r_flg = 0x800800;
    //}

    //if (stick_l_clk <= clock())
    //{
    //    stick_l_flg = 0x800800;
    //}
                //デバッグ用
    int a, b, c;//, pr;
    joy.battery_connection_info = 0x8E;

    joy.buttons1 = (button_flg >> 0x00) & 0xFF;
    joy.buttons2 = (button_flg >> 0x08) & 0xFF;
    joy.buttons3 = (button_flg >> 0x10) & 0xFF;
    if (rumble_flag == true && rumble_bflag != 0)
    {
        joy.buttons1 |= (rumble_bflag) & 0xFF;
        joy.buttons2 |= (rumble_bflag >> 8) & 0xFF;
        joy.buttons3 |= (rumble_bflag >> 16) & 0xFF;
    }
    joy.ldata1 = (stick_l_flg >> 0x00) & 0xFF;
    joy.ldata2 = (stick_l_flg >> 0x08) & 0xFF;
    joy.ldata3 = (stick_l_flg >> 0x10) & 0xFF;

    joy.rdata1 = (stick_r_flg >> 0x00) & 0xFF;
    joy.rdata2 = (stick_r_flg >> 0x08) & 0xFF;
    joy.rdata3 = (stick_r_flg >> 0x10) & 0xFF;

    joy.gyro1 = gyro1_flg & 0xFF;
    joy.gyro1_2 = (gyro1_flg >> 8);
    joy.gyro2 = gyro2_flg & 0xFF;
    joy.gyro2_2 = (gyro2_flg >> 8);
    joy.gyro3 = gyro3_flg & 0xFF;
    joy.gyro3_2 = (gyro3_flg >> 8);
    //joy.gyro2_2 = gyro2_flg & 0xFF;
    //joy.gyro2 = (gyro2_flg >> 8);
    //joy.gyro3_2 = gyro3_flg & 0xFF;
    //joy.gyro3 = (gyro3_flg >> 8);

    joy.timer++;

    uint8_t report[] = {
        0xA1,
        0x31,
        joy.timer,
        joy.battery_connection_info,
        joy.buttons1,
        joy.buttons2,
        joy.buttons3,
        joy.ldata1,
        joy.ldata2,
        joy.ldata3,
        joy.rdata1,
        joy.rdata2,
        joy.rdata3,
        0x00,
        joy.accelx,
        joy.accelx2,
        joy.accely,
        joy.accely2,
        joy.accelz,
        joy.accelz2,
        joy.gyro1,
        joy.gyro1_2,
        joy.gyro2,
        joy.gyro2_2,
        joy.gyro3,
        joy.gyro3_2,
        joy.accelx,
        joy.accelx2,
        joy.accely,
        joy.accely2,
        joy.accelz,
        joy.accelz2,
        joy.gyro1,
        joy.gyro1_2,
        joy.gyro2,
        joy.gyro2_2,
        joy.gyro3,
        joy.gyro3_2,
        joy.accelx,
        joy.accelx2,
        joy.accely,
        joy.accely2,
        joy.accelz,
        joy.accelz2,
        joy.gyro1,
        joy.gyro1_2,
        joy.gyro2,
        joy.gyro2_2,
        joy.gyro3,
        joy.gyro3_2
    };

    for (a = 0; a < 50; a++)
    {
        reportplusamiibo[a] = report[a];
    }

    for (b = 50, c = 0; c < 312; b++, c++)
    {
        reportplusamiibo[b] = flg[c];
    }

    reportplusamiibo[362] = crcdata;
    //デバッグ用
    //for (pr = 0; pr < 363; ++pr) {
        //printf("%x", reportplusamiibo[pr]);
    //}
    hid_device_send_interrupt_message(hid_cid, reportplusamiibo, sizeof(reportplusamiibo));
}
#endif

//----------------------------------------------------------
// Switch本体から受け取ったパケットの処理
//----------------------------------------------------------
static void hid_report_data_callback(uint16_t cid, hid_report_type_t report_type, uint16_t report_id, int report_size, uint8_t * report)
{
    //デバッグ用
    //for (int i = 0; i < report_size; ++i) {
    //    printf("%x,", report[i]);
    //}

    tim = report[0];

    if (report[9] == 1) {
        switch (report[10]) {
            case 1:
                // 0101
                pairing_state = 30;
                break;
            case 2:
                // 0102
                pairing_state = 31;
                break;
            case 3:
                // 0103
                pairing_state = 32;
                break;
            default:
                break;
        }
    }

    if(report[9] == 2)
    {
#if __EnabledAmiibo
        if (report[10] == 4)
        {
            if (packet_amiibo_flag == false)
            {
                pairing_state = 24;
            }
        } else if (report[10] == 1)
        {
            packet_amiibo_flag = true;
            pairing_state = 19;
        } else if (report[10] == 2)
        {
            packet_amiibo_flag = false;
            pairing_state = 24;
        } else if (report[10] == 6)
        {
            packet_amiibo_flag = true;
            pairing_state = 20;
        } else {
        // 02
        pairing_state = 1;
        }
#else
        pairing_state = 1;
#endif
    }
    if(report[9] == 8)
    {
        // 08
        pairing_state = 2;
    }
    if(report[9] == 16 && report[10] == 0 && report[11] == 96)
    {
        // 1060
        pairing_state = 3;
    }
    if(report[9] == 16 && report[10] == 80 && report[11] == 96)
    {
        // 1050
        pairing_state = 4;
    }
    if(report[9] == 3)
    {
#if __EnabledAmiibo
        if (report[10] == 49)
        {
            //0331
            pairing_state = 16;
        }
        else if (report[10] == 48 && pairing_state == 22)
        {
            packet_amiibo_flag = false;
            //hid_device_send_interrupt_message(hid_cid, reply03, sizeof(reply03));
            pairing_state = 28;
        }
        else// if (report[10] == 48)
        {
            //if (pairing_state != 0)
            //{
                //03
                pairing_state = 5;
            //}
        }
#else
        //03
        pairing_state = 5;
#endif
    }
    if(report[9] == 4)
    {
        // 04
        pairing_state = 6;
    }
    if(report[9] == 16 && report[10] == 128 && report[11] == 96)
    {
        // 1080
        //printf("calibration\n");
        pairing_state = 7;
    }
    if(report[9] == 16 && report[10] == 152 && report[11] == 96)
    {
        // 1098
        //printf("stickdeviceparameter\n");
        //spilen = report[14];
        //printf("spilen=%x\n", spilen);
        pairing_state = 8;
    }
    if(report[9] == 16 && report[10] == 16 && report[11] == 128)
    {
        // 1010
        pairing_state = 9;
    }
    if(report[9] == 16 && report[10] == 61 && report[11] == 96)
    {
        // 103D
        pairing_state = 10;
    }
    if(report[9] == 16 && report[10] == 32 && report[11] == 96)
    {
        // 1020
        pairing_state = 11;
    }
    if(report[9] == 64)// && report[10] == 1)
    {
        // 4001
        pairing_state = 12;
    }
    if(report[9] == 72 && report[10] == 1)
    {
        // 4801
        pairing_state = 13;
    }
    if(report_id == 1 && report[9] == 48 && report[10] == 1)
    {
        // 3001
        //for (int i = 0; i < report_size; ++i) {
        //printf("%x,", report[i]);
        //}
        pairing_state = 14;
    }
    if(report[9] == 33 && report[10] == 33)
    {
#if __EnabledAmiibo
        if(report[12] == 4)
        {
            pairing_state = 23;
        }
        else
        {
            //3333
            pairing_state = 15;
        }
#else
        //3333
        pairing_state = 15;
#endif
    }
    if(report[9] == 64 && report[10] == 2)
    {
        // 4001
        pairing_state = 12;
    }

#if __EnabledAmiibo
    if(report_size == 48 && report[9] == 34 && report[10] == 1)
    {
        //2201
        //for (int i = 0; i < report_size; ++i) {
        //    printf("%x,", report[i]);
        //}
        //printf("\n");
        if (pairing_state > 15) {
            pairing_state = 18;
        } else {
            pairing_state = 29;
        }
    }
#endif

#if __EnabledRumble
    if (report_id == 16 && report[1] != 0 && report[2] != 1 && report[5] != 0 && report[6] != 1)
    {
        rumble_flag = true;
    } else {
        rumble_flag = false;
    }
#endif

    if (report[9] == 16 && report[10] == 40 && report[11] == 128)
    {
        //printf("sixaxis found\n");
    }

    //デバッグ用
    //printf("\n");

    hid_device_request_can_send_now_event(hid_cid);
}

//----------------------------------------------------------
// デバイスイベント処理
//----------------------------------------------------------
static void nintendo_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t * packet, uint16_t packet_size)
{
    uint8_t status;
    if(packet_type == HCI_EVENT_PACKET)
    {
        switch (packet[0])
        {
            case BTSTACK_EVENT_STATE:
            {
                if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
                break;
            }
            case HCI_EVENT_HID_META:
            {
                switch (hci_event_hid_meta_get_subevent_code(packet))
                {
                    case HID_SUBEVENT_CONNECTION_OPENED:
                    {
                        status = hid_subevent_connection_opened_get_status(packet);
                        if (status)
                        {
                            hid_cid = 0;
                            return;
                        }
                        hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);

                        hid_device_request_can_send_now_event(hid_cid);
                        break;
                    }
                    case HID_SUBEVENT_CONNECTION_CLOSED:
                    {
                        hid_cid = 0;
                        pairing_state = 0;
                        break;
                    }
                    case HID_SUBEVENT_CAN_SEND_NOW:
                    {
                        uint8_t buffer[64];
                        bool isop = false;
                        //joy.timer = tim+1;
                        if (pairing_state == 0)
                        {
                            send_report_joystick();
                            isop = true;
                        }
                        else if (pairing_state == 16)
                        {
                            //hid_device_send_interrupt_message(hid_cid, reply0331, sizeof(reply0331));
                            memcpy(buffer, reply0331, sizeof(reply0331));
                            pairing_state = 17;
                        }
                        else if (pairing_state == 17)
                        {
                            send_report_joystick_amiibo(replyFF, 0x6F);
                            isop = true;
                        }
                        else if (pairing_state == 18)
                        {
                            //printf("reply2201 1\n");
                            //hid_device_send_interrupt_message(hid_cid, reply2201, sizeof(reply2201));
                            memcpy(buffer, reply2201, sizeof(reply2201));
                            pairing_state = 22;
                        }
                        else if (pairing_state == 19)
                        {
                            send_report_joystick_amiibo(reply110201, crcdata1);
                            isop = true;
                        }
                        else if (pairing_state == 20)
                        {
                            send_report_joystick_amiibo(amiibo_flg1, crcflg1);
                            isop = true;
                            pairing_state = 25;
                        }
                        else if (pairing_state == 21)
                        {
                            //hid_device_send_interrupt_message(hid_cid, reply2121, sizeof(reply2121));
                            memcpy(buffer, reply2121, sizeof(reply2121));
                            pairing_state = 23;
                        }
                        else if (pairing_state == 22)
                        {
                            send_report_joystick_amiibo(reply11011, 0x13);
                            isop = true;
                        }
                        else if (pairing_state == 23)
                        {
                            //hid_device_send_interrupt_message(hid_cid, reply2121, sizeof(reply2121));
                            memcpy(buffer, reply2121, sizeof(reply2121));
                            pairing_state = 27;
                        }
                        else if (pairing_state == 24)
                        {
                            send_report_joystick_amiibo(reply110204, 0x0F);
                            isop = true;
                        }
                        else if (pairing_state == 25)
                        {
                            send_report_joystick_amiibo(amiibo_flg2, crcflg2);
                            isop = true;
                            pairing_state = 26;
                        }
                        else if (pairing_state == 26)
                        {
                            send_report_joystick_amiibo(amiibo_flg3, crcflg3);
                            isop = true;
                            pairing_state = 19;
                        }
                        else if (pairing_state == 27)
                        {
                            send_report_joystick_amiibo(reply1101, 0x01);
                            isop = true;
                        }
                        else if (pairing_state == 28) 
                        {
                            //hid_device_send_interrupt_message(hid_cid, reply03, sizeof(reply03));
                            memcpy(buffer, reply03, sizeof(reply03));
                            pairing_state = 0;
                        }
                        else if (pairing_state == 29) 
                        {
                            //printf("reply2201 2\n");
                            //hid_device_send_interrupt_message(hid_cid, reply2201, sizeof(reply2201));
                            memcpy(buffer, reply2201, sizeof(reply2201));
                            pairing_state = 0;
                        }
                        else if (pairing_state == 30)
                        {
                            //hid_device_send_interrupt_message(hid_cid, reply01_1, sizeof(reply01_1));
                            memcpy(buffer, reply01_1, sizeof(reply01_1));
                        }
                        else if (pairing_state == 31)
                        {
                            //hid_device_send_interrupt_message(hid_cid, reply01_2, sizeof(reply01_2));
                            memcpy(buffer, reply01_2, sizeof(reply01_2));
                        }
                        else if (pairing_state == 32)
                        {
                            //hid_device_send_interrupt_message(hid_cid, reply01_3, sizeof(reply01_3));
                            memcpy(buffer, reply01_3, sizeof(reply01_3));
                        }
                        else if (15 >= pairing_state)
                        {
                            //printf("pairing_state:%d\n", pairing_state);
                            //if (pairing_state == 8)
                            //{
                            //    uint8_t buffer[sizeof(reply1098)] = {0};
                            //    memcpy(buffer, reply1098, sizeof(reply1098));
                            //    //memset(&buffer[21], 0xFF, 0x12);
                            //    buffer[20] = spilen;
                            //    hid_device_send_interrupt_message(hid_cid, buffer, sizeof(buffer));
                            //    for (int i = 0; i < sizeof(buffer);i++) {
                            //        printf("%x,", buffer[i]);
                            //    }
                            //    printf("\n");
                            //} else {
                            //hid_device_send_interrupt_message(hid_cid, packet_list[pairing_state].data, packet_list[pairing_state].size);
                            //}
                            memcpy(buffer, packet_list[pairing_state].data, packet_list[pairing_state].size);
                            if (pairing_state == 14)
                            {
                                pairing_state = 0;
                                joy.timer = tim+1;
                                paired = true;
                            }
                        }

                        if (isop == false)
                        {
                            buffer[2] = ++tim;
                            hid_device_send_interrupt_message(hid_cid, buffer, sizeof(buffer));
                        }
                        //printf("pairing_state:%d\n", pairing_state);
                        hid_device_request_can_send_now_event(hid_cid);
                        break;
                    }
                }
                break;
            }
        }
    }
}

//----------------------------------------------------------
// Bluetoothデバイス情報のセットアップ
//----------------------------------------------------------
void l2cap_create_sdp_record(void)
{
    uint8_t * attribute;
    de_create_sequence(l2cap_sdp_service_buffer);
    
    de_add_number(l2cap_sdp_service_buffer, DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_SERVICE_RECORD_HANDLE);
    de_add_number(l2cap_sdp_service_buffer, DE_UINT, DE_SIZE_32, 0x0000);
    
    de_add_number(l2cap_sdp_service_buffer, DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_SERVICE_CLASS_ID_LIST);
    attribute = de_push_sequence(l2cap_sdp_service_buffer);
    {
        de_add_number(attribute,  DE_UUID, DE_SIZE_16, 0x1000);
    }
    de_pop_sequence(l2cap_sdp_service_buffer, attribute);
    de_add_number(l2cap_sdp_service_buffer,  DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_PROTOCOL_DESCRIPTOR_LIST);
    attribute = de_push_sequence(l2cap_sdp_service_buffer);
    {
        uint8_t * l2cpProtocol = de_push_sequence(attribute);
        {
            de_add_number(l2cpProtocol,  DE_UUID, DE_SIZE_16, BLUETOOTH_PROTOCOL_L2CAP);
            de_add_number(l2cpProtocol,  DE_UINT, DE_SIZE_16, 0x01);
        }
        de_pop_sequence(attribute, l2cpProtocol);
        
        uint8_t * hidProtocol = de_push_sequence(attribute);
        {
            de_add_number(hidProtocol,  DE_UUID, DE_SIZE_16, 0x01);
        }
        de_pop_sequence(attribute, hidProtocol);
    }
    de_pop_sequence(l2cap_sdp_service_buffer, attribute);
    de_add_number(l2cap_sdp_service_buffer,  DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_BROWSE_GROUP_LIST);
    attribute = de_push_sequence(l2cap_sdp_service_buffer);
    {
        de_add_number(attribute,  DE_UUID, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_PUBLIC_BROWSE_ROOT );
    }
    de_pop_sequence(l2cap_sdp_service_buffer, attribute);
    de_add_number(l2cap_sdp_service_buffer,  DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
    attribute = de_push_sequence(l2cap_sdp_service_buffer);
    {
        de_add_number(attribute, DE_UINT, DE_SIZE_16, 0x656e);
        de_add_number(attribute, DE_UINT, DE_SIZE_16, 0x006a);
        de_add_number(attribute, DE_UINT, DE_SIZE_16, 0x0100);
    }
    de_pop_sequence(l2cap_sdp_service_buffer, attribute);
    de_add_number(l2cap_sdp_service_buffer,  DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
    attribute = de_push_sequence(l2cap_sdp_service_buffer);
    {
        uint8_t * hidProfile = de_push_sequence(attribute);
        {
            de_add_number(hidProfile,  DE_UUID, DE_SIZE_16, BLUETOOTH_PROTOCOL_L2CAP);
            de_add_number(hidProfile,  DE_UINT, DE_SIZE_16, 0x0100);
        }
        de_pop_sequence(attribute, hidProfile);
    }
    de_pop_sequence(l2cap_sdp_service_buffer, attribute);
    const char wgp2[] = "Wireless Gamepad";
    de_add_number(l2cap_sdp_service_buffer,  DE_UINT, DE_SIZE_16, 0x0100);
    de_add_data(l2cap_sdp_service_buffer, DE_STRING, strlen(wgp2), (uint8_t *)wgp2);
    const char gp2[] = "Gamepad";
    de_add_number(l2cap_sdp_service_buffer,  DE_UINT, DE_SIZE_16, 0x0101);
    de_add_data(l2cap_sdp_service_buffer, DE_STRING, strlen(gp2), (uint8_t *)gp2);
    de_add_number(l2cap_sdp_service_buffer,  DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_SPECIFICATION_ID);
    attribute = de_push_sequence(l2cap_sdp_service_buffer);
    {
        de_add_number(attribute,  DE_UINT, DE_SIZE_16, 0x0100);
    }
    de_pop_sequence(l2cap_sdp_service_buffer, attribute);
    
    sdp_register_service(l2cap_sdp_service_buffer);
}

//----------------------------------------------------------
// 起動処理
//----------------------------------------------------------
int btstack_main(int argc, const char * argv[])
{
    gap_discoverable_control(1);
    gap_set_class_of_device(0x2508);
    gap_set_local_name(hid_device_name);
    gap_set_default_link_policy_settings( LM_LINK_POLICY_ENABLE_ROLE_SWITCH | LM_LINK_POLICY_ENABLE_SNIFF_MODE );
    gap_set_allow_role_switch(true);
    l2cap_init();
    sm_init();
    sdp_init();
    memset(hid_service_buffer, 0, sizeof(hid_service_buffer));
    uint8_t hid_virtual_cable = 1;
    uint8_t hid_remote_wake = 1;
    uint8_t hid_reconnect_initiate = 1;
    uint8_t hid_normally_connectable = 0;
    uint8_t hid_boot_device = 1;

    hid_sdp_record_t hid_params = {
        // hid sevice subclass 2508 Gamepad, hid counntry code 33 US
        0x2508, 33, 
        hid_virtual_cable, hid_remote_wake, 
        hid_reconnect_initiate, hid_normally_connectable,
        0, 
        0x0640, 0x0320, 0x0c80,
        reportMap,
        sizeof(reportMap), 
        hid_device_name
    };
    hid_create_sdp_record(hid_service_buffer, sdp_create_service_record_handle(), &hid_params);
    sdp_register_service(hid_service_buffer);
    device_id_create_sdp_record(device_id_sdp_service_buffer, sdp_create_service_record_handle(), DEVICE_ID_VENDOR_ID_SOURCE_USB, 0x057e, 0x2009, 1);
    sdp_register_service(device_id_sdp_service_buffer);
    hid_device_init(hid_boot_device, sizeof(reportMap), reportMap);
    hci_event_callback_registration.callback = &nintendo_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
    hci_register_sco_packet_handler(&nintendo_packet_handler);
    //hci_dump_open(NULL, HCI_DUMP_STDOUT); 

    l2cap_create_sdp_record();

    hid_device_register_packet_handler(&nintendo_packet_handler);
    hid_device_register_report_data_callback(&hid_report_data_callback);

    hci_power_control(HCI_POWER_ON);

    return 0;
}