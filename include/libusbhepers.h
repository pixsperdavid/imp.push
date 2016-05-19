#pragma once

#include <stdio.h>

#ifdef _WIN32
#pragma warning(disable:4200)
#include <windows.h>
#endif

#include "libusb.h"

const int ABLETON_VENDOR_ID = 0x2982;
const int PUSH2_PRODUCT_ID = 0x1967;
const int PUSH2_BULK_EP_OUT = 0x01;
const int PUSH2_TRANSFER_TIMEOUT = 1000;

const uint8_t PUSH2_DISPLAY_FRAME_HEADER[] = 
{ 0xFF, 0xCC, 0xAA, 0x88,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00 };

const int PUSH2_DISPLAY_WIDTH = 960;
const int PUSH2_DISPLAY_HEIGHT = 160;
const int PUSH2_DISPLAY_LINE_SIZE = 2048;
const int PUSH2_DISPLAY_LINE_GUTTER_SIZE = 128;
const int PUSH2_DISPLAY_MESSAGE_BUFFER_SIZE = 16384;
const int PUSH2_DISPLAY_IMAGE_BUFFER_SIZE = 2048 * 160;
const int PUSH2_DISPLAY_MESSAGES_PER_IMAGE = (2048 * 160) / 16384;

const int PUSH2_DISPLAY_SHAPING_PATTERN = 0xFFE7F3E7;

const int PUSH2_DISPLAY_FRAMERATE = 60;


libusb_device_handle* push2_open_device()
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

void push2_close_device(libusb_device_handle* device_handle)
{
	libusb_release_interface(device_handle, 0);
	libusb_close(device_handle);
}

void callback(struct libusb_transfer* transfer)
{
	int i = 0;
}

void push2_send_frame(libusb_device_handle* device_handle, uint8_t* buffer)
{
	/*struct libusb_transfer* frame_header_transfer;

	if ((frame_header_transfer = libusb_alloc_transfer(0)) == NULL)
	{
		printf("error: could not allocate frame header transfer handle\n");
	}
	else
	{
		libusb_fill_bulk_transfer(
			frame_header_transfer,
			device_handle,
			PUSH2_BULK_EP_OUT,
			PUSH2_DISPLAY_FRAME_HEADER,
			sizeof(PUSH2_DISPLAY_FRAME_HEADER),
			NULL,
			NULL,
			PUSH2_TRANSFER_TIMEOUT);

		
	}*/

	int actual_length;
	libusb_bulk_transfer(
		device_handle,
		PUSH2_BULK_EP_OUT,
		PUSH2_DISPLAY_FRAME_HEADER,
		sizeof(PUSH2_DISPLAY_FRAME_HEADER),
		&actual_length,
		PUSH2_TRANSFER_TIMEOUT);

	for(int i = 0; i < PUSH2_DISPLAY_MESSAGES_PER_IMAGE; ++i)
	{
		/*struct libusb_transfer* pixel_data_transfer;

		if ((pixel_data_transfer = libusb_alloc_transfer(0)) == NULL)
		{
			printf("error: could not allocate transfer handle\n");
		}
		else
		{
			libusb_fill_bulk_transfer(
				pixel_data_transfer,
				device_handle,
				PUSH2_BULK_EP_OUT,
				buffer + (i * PUSH2_DISPLAY_MESSAGE_BUFFER_SIZE),
				PUSH2_DISPLAY_MESSAGE_BUFFER_SIZE,
				callback,
				NULL,
				PUSH2_TRANSFER_TIMEOUT);

			libusb_submit_transfer(pixel_data_transfer);
		}*/

		int actual_length;
		libusb_bulk_transfer(
			device_handle,
			PUSH2_BULK_EP_OUT,
			buffer + (i * PUSH2_DISPLAY_MESSAGE_BUFFER_SIZE),
			PUSH2_DISPLAY_MESSAGE_BUFFER_SIZE,
			&actual_length,
			PUSH2_TRANSFER_TIMEOUT);
		
	}
}