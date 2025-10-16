#ifndef BLUFI_H
#define BLUFI_H


esp_err_t ble_init(const char *phone);

typedef void (*ble_wifi_config_callback_t)(char *ssid, char *password);
void ble_register_wifi_config_callback(ble_wifi_config_callback_t callback);


#endif