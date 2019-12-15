#include "androidaccessory.h"
#include <unistd.h>
#include <libusb-1.0/libusb.h>

#define usb_interface interface

using namespace std;
constexpr ushort ACCESSORY_VID = 0x18d1;
constexpr ushort ACCESSORY_PID1 = 0x2d01;
constexpr ushort ACCESSORY_PID2 = 0x2d00;
constexpr auto timeout = 5000;

static string MANUFACTURER = "quandoo";
static string MODEL = "Android2AndroidAccessory";
static string DESCRIPTION = "showcasing android2android USB communication";
static string VERSION = "1.0";
static string URI = "https://github.com/";
static string SERIAL = "123456";

AndroidAccessory::AndroidAccessory()
{
    int r = libusb_init(nullptr);
    if (r >= 0)
    {
        init();
    }
}

AndroidAccessory::~AndroidAccessory()
{
    close();
}

bool AndroidAccessory::isOpened()
{
    return m_opened;
}

int AndroidAccessory::read(unsigned char *data, int length)
{
    if (m_opened)
    {
        int actual_length;
        int r = libusb_bulk_transfer(handle, endpoint_in, data, length, &actual_length, 0);
        if (r < 0)
        {
            return r;
        }
        return actual_length;
    }
    return -1;
}

int AndroidAccessory::write(unsigned char *data, int length)
{
    if (m_opened)
    {
        int actual_length;
        int r = libusb_bulk_transfer(handle, endpoint_out, data, length, &actual_length, timeout);
        if (r < 0)
        {
            return r;
        }
        return actual_length;
    }
    return -1;
}

void AndroidAccessory::close()
{
    m_opened = false;
    if (handle)
    {
        for (int iface = 0; iface < nb_ifaces; iface++)
        {
            printf("Releasing interface %d...\n", iface);
            libusb_release_interface(handle, iface);
        }
        libusb_exit(nullptr);
        handle = nullptr;
    }
}

void AndroidAccessory::init()
{
    refreshUsbList();
    openUsbDevice();
}

void AndroidAccessory::refreshUsbList()
{
    m_exist = false;
    m_isAccessoryMode = false;
    m_opened = false;

    libusb_device **list;
    ssize_t cnt = libusb_get_device_list(nullptr, &list);
    ssize_t i = 0;
    if (cnt < 0)
        return;
    for (i = 0; i < cnt; i++)
    {
        libusb_device *device = list[i];
        libusb_device_descriptor dev_desc;
        libusb_get_device_descriptor(device, &dev_desc);
        if (dev_desc.idVendor == ACCESSORY_VID)
        {
            m_exist = true;
            if (dev_desc.idProduct == ACCESSORY_PID1 || dev_desc.idProduct == ACCESSORY_PID2)
            {
                m_isAccessoryMode = true;
            }
            else
            {
                m_usbpid = dev_desc.idProduct;
                m_exist = true;
            }
            break;
        }
    }
    libusb_free_device_list(list, 1);
}

void AndroidAccessory::openUsbDevice()
{
    if (!m_isAccessoryMode)
    {
        if (m_exist)
        {
            handle = libusb_open_device_with_vid_pid(nullptr, ACCESSORY_VID, m_usbpid);
            if (!handle)
            {
                return;
            }
            libusb_set_auto_detach_kernel_driver(handle, 1);
            bool changeMode = accessoryMode();
            if (handle)
            {
                libusb_close(handle);
            }
            if (changeMode)
            {
                usleep(1000 * 1000);
                refreshUsbList();
            }
        }
    }
    if (m_isAccessoryMode)
    {
        handle = libusb_open_device_with_vid_pid(nullptr, ACCESSORY_VID, ACCESSORY_PID1);
        if (!handle)
        {
            handle = libusb_open_device_with_vid_pid(nullptr, ACCESSORY_VID, ACCESSORY_PID2);
        }
        if (!handle)
        {
            return;
        }

        libusb_config_descriptor *conf_desc;
        const libusb_endpoint_descriptor *endpoint;
        libusb_get_active_config_descriptor(libusb_get_device(handle), &conf_desc);
        int i, j, k, r;
        int iface;
        nb_ifaces = conf_desc->bNumInterfaces;
        for (i = 0; i < nb_ifaces; i++)
        {
            for (j = 0; j < conf_desc->usb_interface[i].num_altsetting; j++)
            {
                for (k = 0; k < conf_desc->usb_interface[i].altsetting[j].bNumEndpoints; k++)
                {
                    struct libusb_ss_endpoint_companion_descriptor *ep_comp = nullptr;
                    endpoint = &conf_desc->usb_interface[i].altsetting[j].endpoint[k];
                    printf("       endpoint[%d].address: %02X\n", k, endpoint->bEndpointAddress);
                    if ((endpoint->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) & (LIBUSB_TRANSFER_TYPE_BULK | LIBUSB_TRANSFER_TYPE_INTERRUPT))
                    {
                        if (endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN)
                        {
                            if (!endpoint_in)
                                endpoint_in = endpoint->bEndpointAddress;
                        }
                        else
                        {
                            if (!endpoint_out)
                                endpoint_out = endpoint->bEndpointAddress;
                        }
                    }
                    printf("           max packet size: %04X\n", endpoint->wMaxPacketSize);
                    printf("          polling interval: %02X\n", endpoint->bInterval);
                    libusb_get_ss_endpoint_companion_descriptor(nullptr, endpoint, &ep_comp);
                    if (ep_comp)
                    {
                        printf("                 max burst: %02X   (USB 3.0)\n", ep_comp->bMaxBurst);
                        printf("        bytes per interval: %04X (USB 3.0)\n", ep_comp->wBytesPerInterval);
                        libusb_free_ss_endpoint_companion_descriptor(ep_comp);
                    }
                }
            }
        }
        libusb_free_config_descriptor(conf_desc);

        libusb_set_auto_detach_kernel_driver(handle, 1);
        for (iface = 0; iface < nb_ifaces; iface++)
        {
            printf("\nClaiming interface %d...\n", iface);
            r = libusb_claim_interface(handle, iface);
            m_opened = (r == LIBUSB_SUCCESS);
            if (m_opened)
            {

                return;
            }
        }
    }
    else
    {
        printf("m_isAccessoryMode is false\n");
    }
}

bool AndroidAccessory::accessoryMode()
{
    ushort devVersion;
    int response;
    response = libusb_control_transfer(handle, 0xc0, 51, 0, 0, (unsigned char *)&devVersion, 2, timeout);
    if (response < 0)
    {
        return false;
    }
    if (devVersion != 1 && devVersion != 2)
    {

        return false;
    }
    usleep(1000);
    unsigned char strs[512];

    sprintf((char *)strs, "%s", MANUFACTURER.c_str());
    response = libusb_control_transfer(handle, 0x40, 52, 0, 0, strs, MANUFACTURER.length(), timeout);
    if (response < 0)
    {
        return false;
    }

    sprintf((char *)strs, "%s", MODEL.c_str());
    response = libusb_control_transfer(handle, 0x40, 52, 0, 1, strs, MODEL.length(), timeout);
    if (response < 0)
    {
        return false;
    }

    sprintf((char *)strs, "%s", DESCRIPTION.c_str());
    response = libusb_control_transfer(handle, 0x40, 52, 0, 2, strs, DESCRIPTION.length(), timeout);
    if (response < 0)
    {
        return false;
    }

    sprintf((char *)strs, "%s", VERSION.c_str());
    response = libusb_control_transfer(handle, 0x40, 52, 0, 3, strs, VERSION.length(), timeout);
    if (response < 0)
    {
        return false;
    }

    sprintf((char *)strs, "%s", URI.c_str());
    response = libusb_control_transfer(handle, 0x40, 52, 0, 4, strs, URI.length(), timeout);
    if (response < 0)
    {
        return false;
    }

    sprintf((char *)strs, "%s", SERIAL.c_str());
    response = libusb_control_transfer(handle, 0x40, 52, 0, 5, strs, SERIAL.length(), 0);
    if (response < 0)
    {
        return false;
    }
    response = libusb_control_transfer(handle, 0x40, 53, 0, 0, nullptr, 0, 0);
    if (response < 0)
    {
        return false;
    }
    return true;
}
