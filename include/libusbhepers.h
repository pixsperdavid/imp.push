#pragma once

#include <stdio.h>

#ifdef _WIN32
#pragma warning(disable:4200)
#include <windows.h>
#endif

#include "libusb.h"

const int ABLETON_VENDOR_ID = 0x2982;
const int PUSH2_PRODUCT_ID = 0x1967;

const uint8_t PUSH2_DISPLAY_FRAME_HEADER[] = 
{ 0xFF, 0xCC, 0xAA, 0x88,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00 };

const int PUSH2_DISPLAY_WIDTH = 960;
const int PUSH2_DISPLAY_HEIGHT = 160;
const int PUSH2_DISPLAY_LINE_SIZE = 2048;
const int PUSH2_DISPLAY_LINE_GUTTER = 128;
const int PUSH2_DISPLAY_MESSAGE_BUFFER = 16384;
const int PUSH2_DISPLAY_IMAGE_BUFFER = 2048 * 160;
const int PUSH2_DISPLAY_MESSAGES_PER_IMAGE = (2048 * 160) / 16384;

const int PUSH2_DISPLAY_SHAPING_PATTERN = 0xFFE7F3E7;

const int PUSH2_DISPLAY_FRAMERATE = 60;


libusb_device_handle* open_push2_device()
{
	int result;

	if ((result = libusb_init(NULL)) < 0)
	{
		printf("error: [%d] could not initilialize usblib\n", result);
		return NULL;
	}

	libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_ERROR);

	libusb_device** devices;
	ssize_t count;
	count = libusb_get_device_list(NULL, &devices);
	if (count < 0)
	{
		printf("error: [%ld] could not get usb device list\n", count);
		return NULL;
	}

	libusb_device* device;
	libusb_device_handle* device_handle = NULL;

	char ErrorMsg[128];

	// set message in case we get to the end of the list w/o finding a device
	sprintf(ErrorMsg, "error: Ableton Push 2 device not found\n");

	for (int i = 0; (device = devices[i]) != NULL; i++)
	{
		struct libusb_device_descriptor descriptor;
		if ((result = libusb_get_device_descriptor(device, &descriptor)) < 0)
		{
			sprintf(ErrorMsg,
				"error: [%d] could not get usb device descriptor\n", result);
			continue;
		}

		if (descriptor.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE
			&& descriptor.idVendor == ABLETON_VENDOR_ID
			&& descriptor.idProduct == PUSH2_PRODUCT_ID)
		{
			if ((result = libusb_open(device, &device_handle)) < 0)
			{
				sprintf(ErrorMsg,
					"error: [%d] could not open Ableton Push 2 device\n", result);
			}
			else if ((result = libusb_claim_interface(device_handle, 0)) < 0)
			{
				sprintf(ErrorMsg,
					"error: [%d] could not claim interface 0 of Push 2 device\n", result);
				libusb_close(device_handle);
				device_handle = NULL;
			}
			else
			{
				break; // successfully opened
			}
		}
	}

	if (device_handle == NULL)
	{
		printf(ErrorMsg);
	}

	libusb_free_device_list(devices, 1);
	return device_handle;
}

void close_push2_device(libusb_device_handle* device_handle)
{
	libusb_release_interface(device_handle, 0);
	libusb_close(device_handle);
}