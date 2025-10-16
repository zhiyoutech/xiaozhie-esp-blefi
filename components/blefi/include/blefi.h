#ifndef BLUFI_H
#define BLUFI_H


esp_err_t ble_init( );

typedef void (*ble_wifi_config_callback_t)(char *ssid, char *password);
void ble_register_wifi_config_callback(ble_wifi_config_callback_t callback);

typedef void (*ble_mac_callback_t)(char *mac);
void ble_register_mac_callback(ble_mac_callback_t callback);


#endif