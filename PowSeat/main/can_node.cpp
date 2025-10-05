#include "can_node.hpp"
#include "driver/twai.h"
#include "esp_log.h"
#include "config.h"

static const char* TAG = "CAN";

bool CanNode::begin(){
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t  t = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t  f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f)!=ESP_OK) { ESP_LOGE(TAG, "install failed"); return false; }
  if (twai_start()!=ESP_OK) { ESP_LOGE(TAG, "start failed"); return false; }
  ESP_LOGI(TAG, "TWAI up @%d", CAN_BITRATE);
  return true;
}

void CanNode::pollRx(){
  twai_message_t m;
  if (twai_receive(&m, 0)==ESP_OK){
    uint16_t id = m.identifier & 0x7FF;
    uint8_t d[8]={0};
    for(int i=0;i<m.data_length_code && i<8;i++) d[i]=m.data[i];
    onFrame(id, d);
  }
}

void CanNode::onFrame(uint16_t id, const uint8_t d[8]){
  // Seat command ID window example: 0x200 ~ 0x20F
  if (id>=0x200 && id<0x210){
    if (onSeatCmd) onSeatCmd(CanMsg{ .id=id, .dlc=8, .data={d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7]} });
  }
}

bool CanNode::sendState(uint16_t id, const uint8_t d[8]){
  twai_message_t m{};
  m.identifier = id;
  m.data_length_code = 8;
  for(int i=0;i<8;i++) m.data[i]=d[i];
  return twai_transmit(&m, pdMS_TO_TICKS(5))==ESP_OK;
}
