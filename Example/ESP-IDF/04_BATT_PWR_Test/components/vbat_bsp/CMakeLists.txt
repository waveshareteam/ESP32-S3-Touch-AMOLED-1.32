idf_component_register(
  SRCS "vbat_bsp.cpp"
  PRIV_REQUIRES driver
  REQUIRES esp_adc
  INCLUDE_DIRS "./")
