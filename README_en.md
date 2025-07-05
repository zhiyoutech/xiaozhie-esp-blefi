# Xiaozhi ESP32 with Bluetooth pairing function
For more information about Ash ESP, please refer to the github project: https://github.com/78/xiaozhi-esp32

Xiaozhi-ESP32 version of this project: 1.7.6

## Adaptation to Bluetooth networking, the content modified by the author
1. In the main\idf_component.yml file, add a reference to the external library blefi;

2. In the main\assets folder, add CONNECT_TO_BLE string;

3. In the main\boards\common\wifi_board.cc file, call the Bluetooth network configuration library function, please fill in the user's mobile phone number as the unique ID in the ble_init function;

4. For ESP32 C3 microcontroller, take the xmini-c3 development board as an example, and modify the corresponding content with reference to the content of the xmini-c3\config.json file and the xmini-c3\xmini_c3_board.cc file;

5. For ESP32 S3 type microcontroller, take xingzhi-cube-0.96oled-wifi development board as an example, refer to xingzhi-cube-0.96oled-wifi\config.json file and xingzhi-cube-0.96oled-wifi\xingzhi-cube-0.96oled-wifi.cc file to modify the corresponding content;

Note: The purpose of modifying the xmini_c3_board.cc file and xingzhi-cube-0.96oled-wifi.cc file is to clear the saved wifi configuration. If you don't need this feature, you can leave these two CC files unmodified.

## Requirements for mobile phones using the Bluetooth pairing function
1. Before using the Bluetooth network distribution function, you must turn on the switch of Wifi, location, and Bluetooth on the mobile phone;

2. Use WeChat to scan the QR code of "Zhiyou AI-Bluetooth Distribution" below;

3. For Android phones, the WeChat applet provides a WiFi hotspot selection button; For Apple phones, due to system limitations, the WeChat Mini Program does not provide a WiFi hotspot selection button.

## Precautions for using the Bluetooth pairing function
1. If the Mini Program prompts an error of 10003 or 10004, please exit the Mini Program and then reopen the Mini Program.

2. If the applet prompts that the mobile phone number is not in the system, please add the following author's WeChat.

## WeChat applet "Zhiyou AI-Bluetooth Distribution" QR code
![智友AI-蓝牙配网](https://github.com/user-attachments/assets/5db37ee9-cc18-4c02-b6a4-c47260e67944)


## Author's WeChat QR code
![image](https://github.com/user-attachments/assets/600c0efa-0fd6-45d9-95b9-0e4500ce89ab)


## ad
The author's team has registered the trademark "Zhiyou AI", providing complete commercial services for chatbots, such as Bluetooth networking, OTA updates, timbre cloning, role customization, business billing, background management, small programs and apps, large models and knowledge bases, etc.

## Acknowledgments and anticipation
Thanks to Brother Shrimp for opening up Xiaozhi ESP32, I look forward to Brother Shrimp being able to use the vehicle-level code specification for Xiaozhi ESP32. If you need technical support, you are welcome to contact the author.
