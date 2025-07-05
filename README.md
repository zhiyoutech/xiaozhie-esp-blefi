# 具备蓝牙配网功能的小智ESP32

关于小智ESP的具体内容，请参考github项目： https://github.com/78/xiaozhi-esp32

## 适配蓝牙配网，作者修改的内容
1，在main\idf_component.yml文件中，增加对外部库blefi的引用；
 
2，在main\assets文件夹中，增加CONNECT_TO_BLE字符串；

3，在main\boards\common\wifi_board.cc文件中，调用蓝牙配网库函数，请在ble_init函数里填入用户的手机号码作为唯一ID；；

4、针对ESP32 C3类型单片机，以xmini-c3开发板为例，参照xmini-c3\config.json文件和xmini-c3\xmini_c3_board.cc文件的内容修改对应内容；

5、针对ESP32 S3类型单片机，以xingzhi-cube-0.96oled-wifi开发板为例，参照xingzhi-cube-0.96oled-wifi\config.json文件和xingzhi-cube-0.96oled-wifi\xingzhi-cube-0.96oled-wifi.cc文件修改对应内容；

说明：修改xmini_c3_board.cc文件和xingzhi-cube-0.96oled-wifi.cc文件的目的是清除保存的wifi配置。如果不需要此功能，可以不修改这两个cc类型文件。

## 使用蓝牙配网功能对手机的要求
1，使用蓝牙配网功能前，必须在手机上打开Wifi、位置、蓝牙的开关；

2、使用微信扫一扫下方的“智友AI-蓝牙配网”二维码；

3、针对安卓手机，微信小程序提供wifi热点选择按钮；针对苹果手机，由于系统限制，微信小程序不提供wifi热点选择按钮。

## 使用蓝牙配网功能的注意事项
1、如果小程序提示10003或者10004等类型错误，请退出小程序后再重新打开小程序；

2、如果小程序提示手机号码不在系统中，请添加下方作者微信。

## 微信小程序“智友AI-蓝牙配网”二维码
![智友AI-蓝牙配网](https://github.com/user-attachments/assets/2b75eb2a-f6f7-4c23-8ecb-2f1bb6180638)

## 作者微信二维码
![image](https://github.com/user-attachments/assets/7316fdda-95d7-4268-b3f2-8287df66d562)

## 广告
作者所在团队注册有商标“智友AI”，提供聊天机器人的完整商用服务，如蓝牙配网，OTA更新、音色克隆，角色定制、业务计费、后台管理、小程序和App、大模型和知识库等一站式解决方案，我们服务专业、价格实惠，欢迎与作者联系！

## 致谢和期待
感谢虾哥开源小智ESP32，期待虾哥能将车规级代码规范用于小智ESP32。如果需要技术支持，欢迎虾哥联系作者。
