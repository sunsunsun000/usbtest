package com.example.usbslave;

import android.content.Context;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelFileDescriptor;

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

public abstract class AccessoryCommunicator {

    private UsbManager usbManager;
    private Context context;
    private Handler sendHandler;
    private ParcelFileDescriptor fileDescriptor;
    private FileInputStream inStream;
    private FileOutputStream outStream;
    private boolean running;
    private byte[] readBuffer = new byte[16 * 1024];

    public AccessoryCommunicator(final Context context) {
        this.context = context;

        usbManager = (UsbManager) this.context.getSystemService(Context.USB_SERVICE);

        final UsbAccessory[] accessoryList = usbManager.getAccessoryList();
        if (accessoryList == null || accessoryList.length == 0) {
            onError("no accessory found");
        } else {
            openAccessory(accessoryList[0]);
        }
    }

    public void send(byte[] payload) {
        if (sendHandler != null) {
            Message msg = sendHandler.obtainMessage();
            msg.obj = payload;
            sendHandler.sendMessage(msg);
        }
    }

    private void receive(final byte[] payload) {
        onReceive(payload);
    }

    public abstract void onReceive(final byte[] payload);

    public abstract void onError(String msg);

    public abstract void onConnected();

    public abstract void onDisconnected();


    private class CommunicationThread extends Thread {
        @Override
        public void run() {
            running = true;

            while (running) {
                try {
                    int len = inStream.read(readBuffer);
                    if(inStream != null && len > 0 && running) {
                        byte[] msg = new byte[len];
                        System.arraycopy(readBuffer, 0, msg, 0, len);
                        receive(msg);
                    }
                } catch (final Exception e) {
                    onError("USB Receive Failed " + e.toString() + "\n");
                    closeAccessory();
                }
            }
        }
    }

    private void openAccessory(UsbAccessory accessory) {
        fileDescriptor = usbManager.openAccessory(accessory);
        if (fileDescriptor != null) {

            FileDescriptor fd = fileDescriptor.getFileDescriptor();
            inStream = new FileInputStream(fd);
            outStream = new FileOutputStream(fd);

            new CommunicationThread().start();

            sendHandler = new Handler() {
                public void handleMessage(Message msg) {
                    try {
                        outStream.write((byte[]) msg.obj);
                    } catch (final Exception e) {
                        onError("USB Send Failed " + e.toString() + "\n");
                    }
                }
            };

            onConnected();
        } else {
            onError("could not connect");
        }
    }

    public void closeAccessory() {
        running = false;

        try {
            if (fileDescriptor != null) {
                fileDescriptor.close();
            }
        } catch (IOException e) {
        } finally {
            fileDescriptor = null;
        }

        onDisconnected();
    }

}
