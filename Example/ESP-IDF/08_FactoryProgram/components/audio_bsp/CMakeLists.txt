idf_component_register(
  SRCS "user_audio.cpp" 
  PRIV_REQUIRES 
  espressif__esp_codec_dev
  codec_board
  INCLUDE_DIRS "./"
  EMBED_FILES "canon.pcm")