package com.example.usbslave;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    private TextView contentTextView;
    private EditText input;
    private Button mButton;
    private AccessoryCommunicator communicator;
    private byte[] sendBuffer = new byte[1024 * 1024];

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        contentTextView = findViewById(R.id.content_text);
        input = findViewById(R.id.input_edittext);
        mButton = findViewById(R.id.send_button);
        communicator = new AccessoryCommunicator(this) {

            @Override
            public void onReceive(byte[] payload) {
                String msg;
                if (payload.length <= 20) {
                    msg = new String(payload);
                } else {
                    msg = "收到 " + payload.length + " 字节";
                }

                printString("host> " + msg);
            }

            @Override
            public void onError(String msg) {
                printString("notify " + msg);
            }

            @Override
            public void onConnected() {
                printString("connected");
            }

            @Override
            public void onDisconnected() {
                printString("disconnected");
            }
        };
        mButton.setOnClickListener(view -> {
            final String inputString = input.getText().toString();
            if (inputString.length() == 0) {
                return;
            }
            sendString(inputString);
            printString(getString(R.string.local_prompt_device) + inputString);
            input.setText("");
        });
    }

    private void sendString(final String string) {
        if (string != null && !string.isEmpty())
            if (string.equals("l")) {
                communicator.send(sendBuffer);
            } else
                communicator.send(string.getBytes());
    }

    private void printString(final String string) {
        runOnUiThread(() -> contentTextView.append(string + "\n"));
    }
}
