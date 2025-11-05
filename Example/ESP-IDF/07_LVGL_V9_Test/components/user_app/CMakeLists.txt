idf_component_register(
  SRCS "user_app.cpp"
  PRIV_REQUIRES main lvgl_port_bsp i2c_bsp driver
  REQUIRES lcd_touch_bsp
  INCLUDE_DIRS "./")
