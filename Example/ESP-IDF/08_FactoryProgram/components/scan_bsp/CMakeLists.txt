idf_component_register(
    SRCS "scan_bsp.cpp"
    PRIV_REQUIRES
    esp_event
    nvs_flash
    REQUIRES
    esp_wifi
    bt
    INCLUDE_DIRS "./")
