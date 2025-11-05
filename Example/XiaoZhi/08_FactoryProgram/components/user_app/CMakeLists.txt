idf_component_register(
  SRCS "user_app.cpp"
  PRIV_REQUIRES main lvgl_port_bsp i2c_bsp driver vbat_bsp ui_bsp button_bsp scan_bsp audio_bsp
  REQUIRES lcd_touch_bsp
  INCLUDE_DIRS "./")
