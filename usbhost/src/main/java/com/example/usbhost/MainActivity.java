package com.example.usbhost;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

public class MainActivity extends AppCompatActivity {

    private TextView contentTextView;
    private EditText input;
    private Button mButton;

    private final AtomicBoolean keepThreadAlive = new AtomicBoolean(true);
    private final List<String> sendBuffer = new ArrayList<>();

    private UsbManager mUsbManager;
    private UsbDevice mdevice;
    private UsbDeviceConnection connection;


    private static final int USB_TIMEOUT_IN_MS = 1000;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        contentTextView = findViewById(R.id.content_text);
        input = findViewById(R.id.input_edittext);
        mButton = findViewById(R.id.send_button);
        mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        mButton.setText("Tv");
        mButton.setOnClickListener(view -> {
            final String inputString = input.getText().toString();
            if (inputString.length() == 0) {
                return;
            }
            sendString(inputString);
            printString(getString(R.string.local_prompt) + inputString);
            input.setText("");
        });
    }

    @Override
    protected void onResume() {
        super.onResume();

        final HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();

        if (deviceList == null || deviceList.size() == 0) {
            printString("设备未连接");
            return;
        }

        if (searchForUsbAccessory(deviceList)) {
            printString("设备未连接");
            return;
        }

        for (UsbDevice device : deviceList.values()) {
            if (initAccessory(device)) {
                mdevice = device;
                new Thread(new CommunicationRunnable()).start();
                return;
            }
        }
    }

    private boolean searchForUsbAccessory(final HashMap<String, UsbDevice> deviceList) {
        for (UsbDevice device : deviceList.values()) {
            if (isUsbAccessory(device)) {
                return true;
            }
        }

        return false;
    }

    private boolean isUsbAccessory(final UsbDevice device) {

        boolean value = (device.getProductId() == 0x2d00) || (device.getProductId() == 0x2d01);
        return value;
    }

    private boolean initAccessory(final UsbDevice device) {
        final UsbDeviceConnection connection = mUsbManager.openDevice(device);

        if (connection == null) {
            return false;
        }

        initStringControlTransfer(connection, 0, "quandoo"); // MANUFACTURER
        initStringControlTransfer(connection, 1, "Android2AndroidAccessory"); // MODEL
        initStringControlTransfer(connection, 2, "showcasing android2android USB communication"); // DESCRIPTION
        initStringControlTransfer(connection, 3, "0.1"); // VERSION
        initStringControlTransfer(connection, 4, "http://quandoo.de"); // URI
        initStringControlTransfer(connection, 5, "42"); // SERIAL

        connection.controlTransfer(0x40, 53, 0, 0, new byte[]{}, 0, USB_TIMEOUT_IN_MS);

        connection.close();

        return true;
    }

    private void initStringControlTransfer(final UsbDeviceConnection deviceConnection,
                                           final int index,
                                           final String string) {
        deviceConnection.controlTransfer(0x40, 52, 0, index, string.getBytes(), string.length(), USB_TIMEOUT_IN_MS);
    }

    private class CommunicationRunnable implements Runnable {

        @Override
        public void run() {
            final UsbManager usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);

            UsbEndpoint endpointIn = null;
            UsbEndpoint endpointOut = null;

            UsbInterface usbInterface = mdevice.getInterface(0);
            for (int i = 0; i < usbInterface.getEndpointCount(); i++) {

                final UsbEndpoint endpoint = usbInterface.getEndpoint(i);
                if (endpoint.getDirection() == UsbConstants.USB_DIR_IN) {
                    endpointIn = endpoint;
                }
                if (endpoint.getDirection() == UsbConstants.USB_DIR_OUT) {
                    endpointOut = endpoint;
                }

            }

            if (endpointIn == null) {
                printString("Input Endpoint not found");
                return;
            }

            if (endpointOut == null) {
                printString("Output Endpoint not found");
                return;
            }

            connection = usbManager.openDevice(mdevice);

            if (connection == null) {
                printString("Could not open device");
                return;
            }

            final boolean claimResult = connection.claimInterface(usbInterface, true);

            if (!claimResult) {
                printString("Could not claim device");
            } else {
                final byte buff[] = new byte[16*1024];
                printString("Claimed interface - ready to communicate");

                while (keepThreadAlive.get()) {//没有数据不阻塞？？？
                    final int bytesTransferred = connection.bulkTransfer(endpointIn, buff, buff.length, USB_TIMEOUT_IN_MS);
                    if (bytesTransferred > 0) {
                        printString("device> " + new String(buff, 0, bytesTransferred));
                    }

                    synchronized (sendBuffer) {
                        if (sendBuffer.size() > 0) {
                            final byte[] sendBuff = sendBuffer.get(0).getBytes();
                            connection.bulkTransfer(endpointOut, sendBuff, sendBuff.length, USB_TIMEOUT_IN_MS);
                            sendBuffer.remove(0);
                        }
                    }
                }
            }

            connection.releaseInterface(usbInterface);
            connection.close();
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        keepThreadAlive.set(true);
    }

    private void sendString(final String string) {
        sendBuffer.add(string);
    }

    private void printString(final String string) {
        runOnUiThread(() -> contentTextView.append(string + "\n"));
    }
}
