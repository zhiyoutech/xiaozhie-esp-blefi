
## 适配蓝牙配网，作者修改的内容
1，在components文件中，增加blefi库；
 
2，在main\assets文件夹中，增加CONNECT_TO_BLE字符串；

3，在main\boards\common\wifi_board.cc文件中，调用蓝牙配网库函数，请在ble_init函数里填入用户的手机号码作为唯一ID；

4、针对ESP32 C3类型单片机，以xmini-c3开发板为例，参照xmini-c3\config.json文件和xmini-c3\xmini_c3_board.cc文件的内容修改对应内容；

5、针对ESP32 S3类型单片机，以xingzhi-cube-0.96oled-wifi开发板为例，参照xingzhi-cube-0.96oled-wifi\config.json文件和xingzhi-cube-0.96oled-wifi\xingzhi-cube-0.96oled-wifi.cc文件修改对应内容；

## 使用蓝牙配网功能对手机的要求
1，使用蓝牙配网功能前，必须在手机上打开Wifi、位置、蓝牙的开关；

2、使用微信扫一扫下方的“智友AI-蓝牙配网”二维码；

3、针对安卓手机，微信小程序提供wifi热点选择按钮；针对苹果手机，由于系统限制，微信小程序不提供wifi热点选择按钮。

## 微信小程序“智友AI-蓝牙配网”二维码

![image](https://github.com/user-attachments/assets/7316fdda-95d7-4268-b3f2-8287df66d562)

## 广告
作者所在团队注册有商标“智友AI”，提供聊天机器人的完整商用服务，如蓝牙配网，OTA更新、音色克隆，角色定制、业务计费、后台管理、小程序和App、大模型和知识库等一站式解决方案，我们服务专业、价格实惠，欢迎与作者联系！

作者微信：BasicSoftware
