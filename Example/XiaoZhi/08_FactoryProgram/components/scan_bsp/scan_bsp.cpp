#include <stdio.h>
#include <string.h>
#include "scan_bsp.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "esp_log.h"


#define TAG "Scan_bsp"

static uint16_t Scan_WifiValue;
static uint16_t Scan_BleValue;
#define MAX_SCAN_DEVICES 256 
static uint8_t Scanned_devices[MAX_SCAN_DEVICES][6];

static esp_netif_t *net = NULL;

void ScanWirelessBsp::Scan_WifiTask(void *arg) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_start(NULL, true));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_num(&Scan_WifiValue));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_clear_ap_list());
    vTaskDelete(NULL);
}

void ScanWirelessBsp::Scan_EventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_STA_START) {
        xTaskCreatePinnedToCore(Scan_WifiTask, "Scan_WifiTask", 2 * 1024, NULL, 2, NULL, 0);
    }
}

void ScanWirelessBsp::ScanWirelessBsp_Init() {
    nvs_flash_init();                          // Initializes the default nvs area
    esp_event_loop_create_default();           // Create a default event loop
}

ScanWirelessBsp::ScanWirelessBsp() {
}

ScanWirelessBsp::~ScanWirelessBsp() {
}

void ScanWirelessBsp::Scan_ArrayAssignment(uint8_t *Array_x, uint8_t *Array_y, int Array_Len) {
    int i;
    for (i = 0; i < Array_Len; i++) {
        Array_x[i] = Array_y[i];
    }
    Array_x[i] = 0x00;
}

void ScanWirelessBsp::Scan_WifiSetup() {
    net = esp_netif_create_default_wifi_sta(); // Adds the TCP/IP stack to the default event loop
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // Default config
    esp_wifi_init(&cfg);                                 // initialize
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &Scan_EventCallback, NULL, NULL);
    esp_wifi_set_mode(WIFI_MODE_STA);               // Model STA
    esp_wifi_start();                               // Enable WIFI
}

uint16_t ScanWirelessBsp::Scan_GetWifiValue() {
    return Scan_WifiValue;
}

void ScanWirelessBsp::Scan_WifiDelete() {
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(net);
    esp_event_loop_delete_default();
}

#define REMOTE_SERVICE_UUID        0x00FF
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01
#define PROFILE_NUM      1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0

static esp_ble_scan_params_t ble_scan_params = {                //ble scan parameter Settings
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_ENABLE        //Filter duplicate ads
};

struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
};

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) // This function is called by the esp gattc cb callback function and has the opportunity to be executed
{
    switch (event)
    {
        case ESP_GATTC_REG_EVT: //This callback function, executed by the esp gattc cb callback function, passes in the event parameters
            ESP_LOGI(TAG, "REG_EVT");
            break;
        case ESP_GATTC_CFG_MTU_EVT: //This event is triggered when the GATT client successfully configures the mtu by calling esp ble gattc set mtu()
            if (param->cfg_mtu.status != ESP_GATT_OK)
            {
                ESP_LOGE(TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
            }
            ESP_LOGI(TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
            break;
        default:
            break;
    }
}

static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Didn't get the initial value, so it's ESP GATT IF NONE */
    },
};

static bool is_device_scanned(uint8_t *addr)
{
    for (int i = 0; i < Scan_BleValue; i++) {
        if (memcmp(Scanned_devices[i], addr, 6) == 0) {
            return true;
        }
    }
    return false;
}

static void add_scanned_device(uint8_t *addr)
{
    if (Scan_BleValue >= MAX_SCAN_DEVICES) return;
    memcpy(Scanned_devices[Scan_BleValue], addr, 6);
    Scan_BleValue++;
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: //This event is triggered after the esp ble gap set scan params() function is successfully invoked to set scan parameters
        {
            esp_ble_gap_start_scanning(3);
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: //This event is triggered after the esp ble gap start scanning() function is successfully invoked to start scanning
        {
            if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) //To check whether the ESP BT is successfully started, the value must equal to ESP BT STATUS SUCCESS
            {
                ESP_LOGE(TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
                break;
            }
            ESP_LOGI(TAG, "scan start success");
            break;
        }
        case ESP_GAP_BLE_SCAN_RESULT_EVT: //Enter once every scan, not wait for the scan to finish before entering
        {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
            switch (scan_result->scan_rst.search_evt) //    Search event type     
            {
                case ESP_GAP_SEARCH_INQ_RES_EVT:
                    if (!is_device_scanned(scan_result->scan_rst.bda)) {
                        add_scanned_device(scan_result->scan_rst.bda);
                        ESP_LOGI(TAG, "Found new device: %02X:%02X:%02X:%02X:%02X:%02X (total=%d)",
                        scan_result->scan_rst.bda[0],
                        scan_result->scan_rst.bda[1],
                        scan_result->scan_rst.bda[2],
                        scan_result->scan_rst.bda[3],
                        scan_result->scan_rst.bda[4],
                        scan_result->scan_rst.bda[5],
                        Scan_BleValue);
                    }
                    break;
                case ESP_GAP_SEARCH_INQ_CMPL_EVT: //The event is used to notify the scanning time of the BLE device when the scanning operation is completed
                    break;
                default:
                    break;
            }
            break;
        }
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: //Usually triggered after you call esp ble gap stop scanning() function to stop scanning
            if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGE(TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
                break;
            }
            ESP_LOGI(TAG, "stop scan successfully");

            break;
        default:
            break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT)              // Description The event occurred when the GATT client was registered
    {
        if (param->reg.status == ESP_GATT_OK)    // Get running status
        {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if; // param->reg.app id Get the ID, which is the gattc if generated automatically by the system
            printf("gattc_if:%d\n",gattc_if);
        } 
        else 
        {
            ESP_LOGI(TAG, "reg app failed, app_id %04x, status %d",param->reg.app_id,param->reg.status); // Otherwise, the corresponding error structure is displayed
            return;                                                                                            //Returns, does not execute the following
        }
    }
    do
    {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            if (gattc_if == ESP_GATT_IF_NONE || gattc_if == gl_profile_tab[idx].gattc_if) //if the callback reports gattc if/gatts if as an ESP GATT IF NONE macro, this event does not correspond to any application
            {
                if (gl_profile_tab[idx].gattc_cb) 
                {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param); //Call the callback function
                }
            }
        }
    } while (0);
}

void ScanWirelessBsp::Scan_BleInit() {
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret                               = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret) {
        ESP_LOGE(TAG, "%s gap register failed, error code = %x\n", __func__, ret);
        return;
    }
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if (ret) {
        ESP_LOGE(TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
        return;
    }
    ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
    if (ret) {
        ESP_LOGE(TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret) {
        ESP_LOGE(TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
}

void ScanWirelessBsp::Scan_BleStart() {
    esp_ble_gap_set_scan_params(&ble_scan_params);//set scan
}

void ScanWirelessBsp::Scan_BleDelete() {
    //if(is_Scan_BleValue != 0)esp_ble_gap_stop_scanning();
    esp_ble_gattc_app_unregister(gl_profile_tab[0].gattc_if);
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
}

uint16_t ScanWirelessBsp::Scan_GetBleValue() {
    return Scan_BleValue;
}
