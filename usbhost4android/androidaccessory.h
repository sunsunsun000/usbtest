#ifndef ANDROIDACCESSORY_H
#define ANDROIDACCESSORY_H

#include <functional>
#include <list>
using ReadDataCallBack = std::function<void()>;
struct libusb_device_handle;

class AndroidAccessory
{
public:
    AndroidAccessory();
    virtual ~AndroidAccessory();
    bool isOpened();
    int read(unsigned char *, int length);
    int write(unsigned char *, int length);
    //非线程安全，请在创建该对象的线程上调用或不显示调用，该对象析构时会自动调用
    void close();

private:
    AndroidAccessory(const AndroidAccessory &) = delete;
    bool m_exist{false};
    bool m_isAccessoryMode{false};
    bool m_opened{false};
    u_char endpoint_in{0};
    u_char endpoint_out{0};
    ushort m_usbpid{0};
    int nb_ifaces{0};
    libusb_device_handle *handle{nullptr};
    ReadDataCallBack m_callback;
    void init();
    void refreshUsbList();
    void openUsbDevice();
    bool accessoryMode();
};

#endif // ANDROIDACCESSORY_H
