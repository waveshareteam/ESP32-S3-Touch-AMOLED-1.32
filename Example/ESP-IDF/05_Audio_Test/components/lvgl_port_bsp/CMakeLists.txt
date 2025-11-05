idf_component_register(
    SRCS "lvgl_port_bsp.cpp"
    PRIV_REQUIRES 
    espressif__esp_lcd_sh8601 
    main
    user_app
    INCLUDE_DIRS "./")
