#include "androidaccessory.h"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <thread>

const auto buffSize = 1 * 1024 * 1024;
int main(int argc, char *argv[])
{
    AndroidAccessory androidAccessory;
    std::thread read;
    if (androidAccessory.isOpened())
    {
        read = std::thread([&] {
            char *data = new char[buffSize];
            while (androidAccessory.isOpened())
            {
                int r = androidAccessory.read((unsigned char *)data, buffSize);
                if (r > 0)
                {
                    if (r < 20)
                    {
                        data[r] = 0;
                        printf("read %s\n", data);
                    }
                    else
                    {
                        printf("read %d byte\n", r);
                    }
                }
                else
                {
                    break;
                }
            }
            delete[] data;
        });
    };

    char *data = new char[buffSize];
    int a;
    while (androidAccessory.isOpened())
    {
        std::cin >> a;
        int r;
        if (a == 1)
        {
            sprintf(data, "%s", "你好，android");
            std::string s = data;
            r = androidAccessory.write((unsigned char *)data, s.length());
        }
        else if (a == 1)
        {
            sprintf(data, "%s", "你好，android aaaa");
            std::string s = data;
            r = androidAccessory.write((unsigned char *)data, s.length());
        }
        else if (a == 10)
        {
            r = -1;
        }
        else
        {
            //            for (;;){
            r = androidAccessory.write((unsigned char *)data, buffSize);
            //            }
        }
        if (r < 0)
        {
            androidAccessory.close();
        }
    }
    delete[] data;
    if (read.joinable())
    {
        read.join();
    }
    return 0;
}
