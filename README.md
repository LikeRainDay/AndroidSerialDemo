# AndroidSerialDemo
Android使用USB串口与Ardino进行通讯。
# Ardinuo的源码
```
int prvValue;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinModes(13,OUTPUT);
  prvValue=0;
}

void loop() {
  // put your main code here, to run repeatedly:
  //进行查看从Android手机发送的数据
  if(Serial.availabel()){
    byte cmd=Serial.read();
    if(cmd == 0x02){
      digitalWrite(13,LOW);
    }else{
      digitalWrite(13,HIGH);
    }
  }

  //读取传感器的数据 发送给Android设备
   int sensorValue = analogRead(A0)>>4;
   byte dataToSend;
   if(prvValue!=seneorValue){
      prvValue=sensorValue;

      if(prvValue=0x00){
          dataToSent=(byte)0x01;
        }else{
          dataToSent=(byte)prvValue
        }

        Serial.write(dataToSent);
        delay(100);
   }


}
```
# Android 项目源码
```
package com.example.housh.androidserial;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbRequest;
import android.os.Bundle;
import android.os.SystemClock;
import android.support.v7.app.AppCompatActivity;
import android.widget.TextView;
import android.widget.Toast;

import java.nio.ByteBuffer;

public class MainActivity extends AppCompatActivity {

    private UsbInterface usbInterfaceFound = null;
    private UsbEndpoint endpointOut = null;
    private UsbEndpoint endpointIn = null;
    private UsbManager mUsbManager;
    private boolean LoopSerial = true;
    private TextView mTextView;
    private UsbDeviceConnection mUsbDeviceConnection;
    private Thread mSerialInThread;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mTextView = (TextView) findViewById(R.id.text);


        //注册动态广播进行监听usb的插入状态
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        intentFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        registerReceiver(MyUsbBoradCastReceiver, intentFilter);

    }

    private BroadcastReceiver MyUsbBoradCastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
                //当usb插入后进行的操作
                UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                if (null != device) {
                    //当设备不为空  则进行操作设备的管理
                    operateDevice(device);

                }

            } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                //当usb分离后，进行广播
                Toast.makeText(MainActivity.this, "USB已经拔出", Toast.LENGTH_LONG).show();
            }
        }
    };

    /**
     * 开启对设备的管理
     */
    private void operateDevice(UsbDevice device) {
        //获取usb设备
        mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        //获取到设备列表
        //HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
        //找到指定的设备的接口
        int interfaceCount = device.getInterfaceCount();
        for (int i = 0; i < interfaceCount; i++) {
            UsbInterface usbif = device.getInterface(i);
            UsbEndpoint tOut = null;
            UsbEndpoint tIn = null;
            //设置输出端的数据
            int tEndpointCnt = usbif.getEndpointCount();
            if (tEndpointCnt >= 2) {
                for (int j = 0; j < tEndpointCnt; j++) {
                    if (usbif.getEndpoint(j).getType() == UsbConstants.USB_ENDPOINT_XFER_BULK) {
                        //Bulk endpoint type 类型
                        if (usbif.getEndpoint(j).getDirection() == UsbConstants.USB_DIR_OUT)
                            tOut = usbif.getEndpoint(j);
                        else if (usbif.getEndpoint(j).getDirection() == UsbConstants.USB_DIR_IN)
                            tIn = usbif.getEndpoint(j);
                    }
                }
                if (tOut != null && tIn != null) {
                    //进行对接口进行赋值
                    usbInterfaceFound = usbif;
                    endpointIn = tIn;
                    endpointOut = tOut;
                }
            }
            if (null == usbInterfaceFound)
                return;

            if (null != device) {
                //如果设备不为空，则开始检测权限，通过则进行开启设备
                if (mUsbManager.hasPermission(device)) {
                    mUsbDeviceConnection = mUsbManager.openDevice(device);
                    if (null != mUsbDeviceConnection && mUsbDeviceConnection.claimInterface(usbInterfaceFound, true)) {
                        //开启当前的硬件版
                        sendCommand(1);
                        //读取数据最大的包大小
                        final int inMax = tIn.getMaxPacketSize();
                        final ByteBuffer buffer = ByteBuffer.allocate(inMax);
                        final UsbRequest usbRequest = new UsbRequest();
                        usbRequest.initialize(mUsbDeviceConnection, endpointIn);
                        //进行轮训的读取数据
                        //通过控件进行展示当前串口的内容
                        mSerialInThread = new Thread(new Runnable() {
                            @Override
                            public void run() {
                                while (LoopSerial) {
                                    usbRequest.queue(buffer, inMax);
                                    if (mUsbDeviceConnection.requestWait() == usbRequest) {
                                        byte[] array = buffer.array();
                                        if (array.length != 0) {
                                            //通过控件进行展示当前串口的内容
                                            mTextView.setText(new String(array));
                                        }
                                        SystemClock.sleep(100);
                                    } else {
                                        break;
                                    }
                                }
                            }
                        });
                    } else {
                        Toast.makeText(MainActivity.this, "没有找到对应的设备", Toast.LENGTH_LONG).show();
                    }
                } else {
                    Toast.makeText(MainActivity.this, "没有读取USB的权限", Toast.LENGTH_LONG).show();
                }
            }
        }
    }

    /**
     * 用来像Arduino发送数据
     */
    private void sendCommand(int control) {
        synchronized (this) {
            if (null != mUsbDeviceConnection) {
                byte[] message = new byte[1];
                message[0] = (byte) control;
                mUsbDeviceConnection.bulkTransfer(endpointOut, message, message.length, 0);
            }


        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (null != mSerialInThread) {
            LoopSerial = false;
            mSerialInThread.isInterrupted();
            mSerialInThread = null;
        }
    }
}

```
