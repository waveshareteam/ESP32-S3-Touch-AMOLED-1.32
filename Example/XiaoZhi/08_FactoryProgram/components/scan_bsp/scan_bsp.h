#ifndef SCAN_BSP_H
#define SCAN_BSP_H

#include "esp_event.h" 

class ScanWirelessBsp
{
private:
    static void Scan_WifiTask(void *arg);
    void Scan_ArrayAssignment(uint8_t *Array_x,uint8_t *Array_y,int Array_Len);
    static void Scan_EventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
public:
    ScanWirelessBsp();
    ~ScanWirelessBsp();

    void ScanWirelessBsp_Init();
    void Scan_WifiSetup();
    uint16_t Scan_GetWifiValue();
    void Scan_WifiDelete();

    void Scan_BleInit();
    void Scan_BleStart();    
    void Scan_BleDelete(); 
    uint16_t Scan_GetBleValue(); 
};


#endif
