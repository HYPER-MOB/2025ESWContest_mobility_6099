idf_component_register(
  SRCS
    "app_main.cpp"
    "can_node.cpp"
    "relay_module.cpp"
    "relay_hbridge.cpp"
    "seat_node.cpp"
    "state_store.cpp"
  INCLUDE_DIRS "."
  REQUIRES driver esp_timer nvs_flash
)
