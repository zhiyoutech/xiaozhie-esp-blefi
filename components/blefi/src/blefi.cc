#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"

#include "blefi.h"

#define TAG "GATTS_WIFI"

ble_wifi_config_callback_t wifi_config_callback = NULL;
ble_mac_callback_t ble_mac_callback = NULL;

// WiFi credentials buffer and parsing
#define MAX_WIFI_CONFIG_SIZE 512
static char wifi_config_buffer[MAX_WIFI_CONFIG_SIZE];
static int wifi_config_buffer_len = 0;
static bool wifi_config_started = false;

// Function to parse WiFi config and restart
static void process_wifi_config();

///Declare the static function
static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);


// Service UUID
static uint8_t wifi_config_service_uuid[16] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x18, 0x00, 0x00
};

// Characteristic UUIDs
static uint8_t wifi_param_write_char_uuid[16] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x19, 0x2a, 0x00, 0x00
};

static uint8_t wifi_status_notify_char_uuid[16] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x1a, 0x2a, 0x00, 0x00
};


#define GATTS_NUM_HANDLE_TEST_A     7 // Increased for 2 characteristics

#define ESP_BLE_MAC_LEN 13

static char BleMac[ESP_BLE_MAC_LEN] = "\0";
static char test_device_name[ESP_BLE_ADV_DATA_LEN_MAX] = "LinkerAI.cn-";

#define TEST_MANUFACTURER_DATA_LEN  17

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

#define PREPARE_BUF_MAX_SIZE 1024

static uint8_t char1_str[] = {0x11,0x22,0x33};
static esp_gatt_char_prop_t a_property = 0;

static esp_attr_value_t gatts_demo_char1_val =
{
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};

static uint8_t adv_config_done = 0;
#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

#ifdef CONFIG_EXAMPLE_SET_RAW_ADV_DATA
static uint8_t raw_adv_data[] = {
    /* Flags */
    0x02, ESP_BLE_AD_TYPE_FLAG, 0x06,               // Length 2, Data Type ESP_BLE_AD_TYPE_FLAG, Data 1 (LE General Discoverable Mode, BR/EDR Not Supported)
    /* TX Power Level */
    0x02, ESP_BLE_AD_TYPE_TX_PWR, 0xEB,             // Length 2, Data Type ESP_BLE_AD_TYPE_TX_PWR, Data 2 (-21)
    /* Complete 16-bit Service UUIDs */
    0x03, ESP_BLE_AD_TYPE_16SRV_CMPL, 0xAB, 0xCD    // Length 3, Data Type ESP_BLE_AD_TYPE_16SRV_CMPL, Data 3 (UUID)
};

static uint8_t raw_scan_rsp_data[] = {
    /* Complete Local Name */
    0x0F, ESP_BLE_AD_TYPE_NAME_CMPL, 'L', 'i', 'n', 'k', 'e', 'r', 'A', 'I', '.', 'c', 'n', '-', 'x', 'x'   // Length 15, Data Type ESP_BLE_AD_TYPE_NAME_CMPL, Data (ESP_GATTS_DEMO)
};
#else

static uint8_t adv_service_uuid128[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x0f, 0x18, 0x00, 0x00
};

// The length of adv data must be less than 31 bytes
//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
//adv data
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    //.min_interval = 0x0006,
    //.max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

#endif /* CONFIG_SET_RAW_ADV_DATA */

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

// Second characteristic handle
static uint16_t notify_char_handle = 0;
static esp_gatt_char_prop_t notify_property = 0;

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

static prepare_type_env_t a_prepare_write_env;


void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);


static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
#ifdef CONFIG_SET_RAW_ADV_DATA
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done==0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done==0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
#else
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:                                 ///* 当广告数据集完成时，事件就来了 */
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:                            ///*！<扫描响应数据集完成后，事件出现*/
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
#endif
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:                                    ///*！<当开始广告完成时，活动就来了*/
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising start failed, status %d", param->adv_start_cmpl.status);
        }else{
            ESP_LOGI(TAG, "Advertising start successfully");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising stop failed, status %d", param->adv_stop_cmpl.status);
        }else{
            ESP_LOGI(TAG, "Advertising stop successfully");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:                                    ///*！<更新连接参数完成后，事件出现*/
         ESP_LOGI(TAG, "Connection params update, status %d, conn_int %d, latency %d, timeout %d",
                  param->update_conn_params.status,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:                               ///*！<当设置pkt长度完成时，事件出现*/
        ESP_LOGI(TAG, "Packet length update, status %d, rx %d, tx %d",
                  param->pkt_data_length_cmpl.status,
                  param->pkt_data_length_cmpl.params.rx_len,
                  param->pkt_data_length_cmpl.params.tx_len);
        break;
    default:
        break;
    }
}

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp){
        if (param->write.is_prep) {
            if (param->write.offset > PREPARE_BUF_MAX_SIZE) {
                status = ESP_GATT_INVALID_OFFSET;
            } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
                status = ESP_GATT_INVALID_ATTR_LEN;
            }
            if (status == ESP_GATT_OK && prepare_write_env->prepare_buf == NULL) {
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE*sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL) {
                    ESP_LOGE(TAG, "Gatt_server prep no mem");
                    status = ESP_GATT_NO_RESOURCES;
                }
            }

            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            if (gatt_rsp) {
                gatt_rsp->attr_value.len = param->write.len;
                gatt_rsp->attr_value.handle = param->write.handle;
                gatt_rsp->attr_value.offset = param->write.offset;
                gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
                memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
                esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
                if (response_err != ESP_OK){
                    ESP_LOGE(TAG, "Send response error\n");
                }
                free(gatt_rsp);
            } else {
                ESP_LOGE(TAG, "malloc failed, no resource to send response error\n");
                status = ESP_GATT_NO_RESOURCES;
            }
            if (status != ESP_GATT_OK){
                return;
            }
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                   param->write.value,
                   param->write.len);
            prepare_write_env->prepare_len += param->write.len;

        }else{
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC){
        ESP_LOG_BUFFER_HEX(TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(TAG,"Prepare write cancel");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) 
{
    switch(event) 
    {
        case ESP_GATTS_REG_EVT:
        {
            ESP_LOGI(TAG, "GATT server register, status %d, app_id %d, gatts_if %d", param->reg.status, param->reg.app_id, gatts_if);
            gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
            gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
            gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_128;
            memcpy(gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid128, wifi_config_service_uuid, ESP_UUID_LEN_128);

            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(test_device_name);
            if (set_dev_name_ret){
                ESP_LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
#ifdef CONFIG_EXAMPLE_SET_RAW_ADV_DATA
            esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
            if (raw_adv_ret){
                ESP_LOGE(TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
            }
            adv_config_done |= adv_config_flag;
            esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
            if (raw_scan_ret){
                ESP_LOGE(TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
            }
            adv_config_done |= scan_rsp_config_flag;
#else
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= adv_config_flag;
            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
            if (ret){
                ESP_LOGE(TAG, "config scan response data failed, error code = %x", ret);
            }
            adv_config_done |= scan_rsp_config_flag;
#endif
            esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
            break;
        }
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(TAG, "Characteristic read, conn_id %d, trans_id %" PRIu32 ", handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xed;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        //ESP_LOGI(TAG, "Characteristic write, conn_id %d, trans_id %" PRIu32 ", handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep){
            // ESP_LOGI(TAG, "value len %d, value ", param->write.len);
            // ESP_LOG_BUFFER_HEX(TAG, param->write.value, param->write.len);
            
            // Handle WiFi configuration data
            if (gl_profile_tab[PROFILE_A_APP_ID].char_handle == param->write.handle) {
                // Create a null-terminated string from the received data
                char received_data[param->write.len + 1];
                memcpy(received_data, param->write.value, param->write.len);
                received_data[param->write.len] = '\0';
                ESP_LOGI(TAG, "Received data: %s", received_data);
                
                // Send device ID as an indicate message after successful write
                const uint8_t *mac_addr = esp_bt_dev_get_address();
                if (mac_addr && notify_char_handle != 0) {
                    char mac_str[18];
                    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", 
                           mac_addr[0], mac_addr[1], mac_addr[2], 
                           mac_addr[3], mac_addr[4], mac_addr[5]);
                    
                    ESP_LOGI(TAG, "Sending device ID indication after write: %s", mac_str);
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id,
                                               notify_char_handle,
                                               strlen(mac_str), (uint8_t *)mac_str, true); // true for indicate
                }
                
                // Check for <start> tag
                if (strstr(received_data, "<start>") != NULL) {
                    ESP_LOGI(TAG, "WiFi config started");
                    wifi_config_started = true;
                    wifi_config_buffer_len = 0;
                    memset(wifi_config_buffer, 0, MAX_WIFI_CONFIG_SIZE);
                }
                
                // If we're collecting WiFi config data
                if (wifi_config_started) {
                    // Make sure we don't overflow the buffer
                    if (wifi_config_buffer_len + param->write.len < MAX_WIFI_CONFIG_SIZE) {
                        memcpy(wifi_config_buffer + wifi_config_buffer_len, param->write.value, param->write.len);
                        wifi_config_buffer_len += param->write.len;
                        wifi_config_buffer[wifi_config_buffer_len] = '\0';
                        
                        // Check if we received the end marker
                        if (strstr(wifi_config_buffer, "<end>") != NULL) {
                            ESP_LOGI(TAG, "WiFi config completed, processing...");
                            wifi_config_started = false;
                            
                            // Process the WiFi configuration
                            process_wifi_config();
                        }
                    } else {
                        ESP_LOGE(TAG, "WiFi config buffer overflow");
                        wifi_config_started = false;
                    }
                }
            }
            
            if (gl_profile_tab[PROFILE_A_APP_ID].descr_handle == param->write.handle && param->write.len == 2){
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0001){
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
                        ESP_LOGI(TAG, "Notification enable");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++i)
                        {
                            notify_data[i] = i%0xff;
                        }
                        //the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                                sizeof(notify_data), notify_data, false);
                    }
                }else if (descr_value == 0x0002){
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE){
                        ESP_LOGI(TAG, "Indication enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i%0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                                sizeof(indicate_data), indicate_data, true);
                    }
                }
                else if (descr_value == 0x0000){
                    ESP_LOGI(TAG, "Notification/Indication disable");
                }else{
                    ESP_LOGE(TAG, "Unknown descriptor value");
                    ESP_LOG_BUFFER_HEX(TAG, param->write.value, param->write.len);
                }

            }
        }
        example_write_event_env(gatts_if, &a_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(TAG,"Execute write");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
    {
        ESP_LOGI(TAG, "MTU exchange, MTU %d", param->mtu.mtu);
        break;
    }
    case ESP_GATTS_UNREG_EVT:
    {
        break;
    }
    case ESP_GATTS_CREATE_EVT:
    {
        ESP_LOGI(TAG, "Service create, status %d, service_handle %d", param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_A_APP_ID].service_handle = param->create.service_handle;
        
        // Set the UUID for the first characteristic (WRITE)
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_128;
        memcpy(gl_profile_tab[PROFILE_A_APP_ID].char_uuid.uuid.uuid128, wifi_param_write_char_uuid, ESP_UUID_LEN_128);

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_A_APP_ID].service_handle);
        
        // Set properties for first characteristic - writable only
        a_property = ESP_GATT_CHAR_PROP_BIT_WRITE;
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_WRITE,
                                                        a_property,
                                                        &gatts_demo_char1_val, NULL);
        if (add_char_ret){
            ESP_LOGE(TAG, "add char failed, error code =%x",add_char_ret);
        }
        break;
    }
    case ESP_GATTS_ADD_INCL_SRVC_EVT:{
        break;
    }
    case ESP_GATTS_ADD_CHAR_EVT: {
        uint16_t length = 0;
        const uint8_t *prf_char;

        // ESP_LOGI(TAG, "Characteristic add, status %d, attr_handle %d, service_handle %d",
        //         param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
                
        // Check which characteristic this is
        if (param->add_char.char_uuid.len == ESP_UUID_LEN_128 && 
            memcmp(param->add_char.char_uuid.uuid.uuid128, wifi_param_write_char_uuid, ESP_UUID_LEN_128) == 0) {
            // This is the first characteristic (WRITE)
            gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
            
            // Add descriptor to first characteristic
            gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
            
            esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
            if (get_attr_ret == ESP_FAIL){
                ESP_LOGE(TAG, "ILLEGAL HANDLE");
            }

            // ESP_LOGI(TAG, "the gatts demo char length = %x", length);
            // for(int i = 0; i < length; i++){
            //     ESP_LOGI(TAG, "prf_char[%x] =%x",i,prf_char[i]);
            // }
            
            esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(
                gl_profile_tab[PROFILE_A_APP_ID].service_handle,
                &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                NULL, NULL);
                
            if (add_descr_ret){
                ESP_LOGE(TAG, "add char descr failed, error code =%x", add_descr_ret);
            }
            
            // Add second characteristic (NOTIFY)
            esp_bt_uuid_t notify_char_uuid;
            notify_char_uuid.len = ESP_UUID_LEN_128;
            memcpy(notify_char_uuid.uuid.uuid128, wifi_status_notify_char_uuid, ESP_UUID_LEN_128);
            
            notify_property = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
            
            esp_err_t add_notify_char_ret = esp_ble_gatts_add_char(
                gl_profile_tab[PROFILE_A_APP_ID].service_handle,
                &notify_char_uuid,
                ESP_GATT_PERM_READ,
                notify_property,
                NULL, NULL);
                
            if (add_notify_char_ret){
                ESP_LOGE(TAG, "add notify char failed, error code =%x", add_notify_char_ret);
            }

        } else {
            // This is the second characteristic (NOTIFY)
            notify_char_handle = param->add_char.attr_handle;
            
            // Add descriptor to second characteristic
            esp_bt_uuid_t notify_descr_uuid;
            notify_descr_uuid.len = ESP_UUID_LEN_16;
            notify_descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
            
            esp_err_t add_notify_descr_ret = esp_ble_gatts_add_char_descr(
                gl_profile_tab[PROFILE_A_APP_ID].service_handle,
                &notify_descr_uuid,
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                NULL, NULL);
                
            if (add_notify_descr_ret){
                ESP_LOGE(TAG, "add notify char descr failed, error code =%x", add_notify_descr_ret);
            }
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:{
        gl_profile_tab[PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
        // ESP_LOGI(TAG, "Descriptor add, status %d, attr_handle %d, service_handle %d",
        //          param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    }
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "Service start, status %d, service_handle %d",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT: {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
        //ESP_LOGI(TAG, "Connected, conn_id %u, remote "ESP_BD_ADDR_STR"",
        //         param->connect.conn_id, ESP_BD_ADDR_HEX(param->connect.remote_bda));
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
                //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);

        vTaskDelay(50 / portTICK_PERIOD_MS);
        // Get device MAC address and send it as a notification
        const uint8_t *mac_addr = esp_bt_dev_get_address();
        if (mac_addr && notify_char_handle != 0) {
            char mac_str[18];
            sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", 
                   mac_addr[0], mac_addr[1], mac_addr[2], 
                   mac_addr[3], mac_addr[4], mac_addr[5]);
            
            ESP_LOGI(TAG, "Sending device ID notification: %s", mac_str);
            esp_ble_gatts_send_indicate(gatts_if, param->connect.conn_id,
                                       notify_char_handle,
                                       strlen(mac_str), (uint8_t *)mac_str, false);
        }
        
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        //ESP_LOGI(TAG, "Disconnected, remote "ESP_BD_ADDR_STR", reason 0x%02x",
        //         ESP_BD_ADDR_HEX(param->disconnect.remote_bda), param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "Confirm receive, status %d, attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK){
            ESP_LOG_BUFFER_HEX(TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:    {break;}
    case ESP_GATTS_CANCEL_OPEN_EVT:     {break;}
    case ESP_GATTS_CLOSE_EVT:   {break;}
    case ESP_GATTS_LISTEN_EVT:  {break;}
    case ESP_GATTS_CONGEST_EVT: {break;}
    case ESP_GATTS_SEND_SERVICE_CHANGE_EVT: 
        {break;}
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each application profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == gl_profile_tab[idx].gatts_if) {
                if (gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}


static void process_wifi_config() {
    ESP_LOGI(TAG, "Processing WiFi config: %s", wifi_config_buffer);
    
    // Find the JSON content between <start> and <end>
    char *start_ptr = strstr(wifi_config_buffer, "<start>");
    char *end_ptr = strstr(wifi_config_buffer, "<end>");
    
    if (start_ptr == NULL || end_ptr == NULL || start_ptr >= end_ptr) {
        ESP_LOGE(TAG, "Invalid WiFi config format");
        return;
    }
    
    // Extract the JSON content
    start_ptr += 7; // Skip "<start>"
    *end_ptr = '\0'; // Terminate the string at <end>
    
    ESP_LOGI(TAG, "WiFi config JSON: %s", start_ptr);
    
    // Simple parsing to extract SSID and password
    char *ssid_start = strstr(start_ptr, "\"ssid\":\"");
    char *pwd_start = strstr(start_ptr, "\"password\":\"");
    
    if (ssid_start == NULL || pwd_start == NULL) {
        ESP_LOGE(TAG, "Could not find SSID or password in config");
        return;
    }
    
    // Extract SSID
    ssid_start += 8; // Skip "\"ssid\":\""
    char *ssid_end = strchr(ssid_start, '\"');
    if (ssid_end == NULL) {
        ESP_LOGE(TAG, "Invalid SSID format");
        return;
    }
    
    int ssid_len = ssid_end - ssid_start;
    char ssid[33]; // Max SSID length is 32 bytes
    if (ssid_len > 32) ssid_len = 32;
    strncpy(ssid, ssid_start, ssid_len);
    ssid[ssid_len] = '\0';
    
    // Extract password
    pwd_start += 12; // Skip "\"password\":\""
    char *pwd_end = strchr(pwd_start, '\"');
    if (pwd_end == NULL) {
        ESP_LOGE(TAG, "Invalid password format");
        return;
    }
    
    int pwd_len = pwd_end - pwd_start;
    char password[65]; // Max password length is 64 bytes
    if (pwd_len > 64) pwd_len = 64;
    strncpy(password, pwd_start, pwd_len);
    password[pwd_len] = '\0';
    
    ESP_LOGI(TAG, "Extracted SSID: %s, Password: %s", ssid, password);
    
    if(wifi_config_callback != NULL){
        wifi_config_callback(ssid, password); 
    }

    // Send success notification
    uint8_t response[10] = "SUCCESS";
    esp_ble_gatts_send_indicate(gl_profile_tab[PROFILE_A_APP_ID].gatts_if, 
                           gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                           notify_char_handle,
                           sizeof(response), 
                           response, 
                           false);
    
     // Restart after 3 seconds
     xTaskCreate([](void *ctx){
        ESP_LOGI(TAG, "Restarting in 3 second");
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart();
    }, "restart_task", 4096, NULL, 5, NULL);
}


void ble_register_wifi_config_callback(ble_wifi_config_callback_t callback)
{
    wifi_config_callback = callback;
}

void ble_register_mac_callback(ble_mac_callback_t callback)
{
    ble_mac_callback = callback;
}

esp_err_t ble_init( )
{
    esp_err_t ret = ESP_OK;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }

    //Set device name with BT MAC address
    const uint8_t *mac = esp_bt_dev_get_address();
    if (mac) {
        sprintf(test_device_name + strlen(test_device_name), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        ESP_LOGI(TAG, "Device name set to: %s", test_device_name);
        sprintf(BleMac, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        ESP_LOGI(TAG, "BleMac set to: %s", BleMac);
        if(ble_mac_callback != NULL){
            ble_mac_callback(BleMac);
        }
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        return ESP_FAIL;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        return ESP_FAIL;
    }
    ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
        return ESP_FAIL;
    }
    
    ret = esp_ble_gatt_set_local_mtu(500);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "set local MTU failed, error code = %x", ret);
        return ESP_FAIL;
    }

    return ESP_OK;
}