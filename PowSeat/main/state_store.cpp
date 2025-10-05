#include "state_store.hpp"
#include "esp_log.h"

static const char* TAG="NVS";
static constexpr const char* NS = "seat";

bool StateStore::begin(){
  esp_err_t r = nvs_flash_init();
  if (r==ESP_ERR_NVS_NO_FREE_PAGES || r==ESP_ERR_NVS_NEW_VERSION_FOUND){
    nvs_flash_erase();
    r = nvs_flash_init();
  }
  if (r!=ESP_OK) { ESP_LOGE(TAG, "nvs init fail %s", esp_err_to_name(r)); return false; }
  r = nvs_open(NS, NVS_READWRITE, &h_);
  if (r!=ESP_OK){ ESP_LOGE(TAG, "nvs open fail %s", esp_err_to_name(r)); return false; }
  return true;
}

bool StateStore::load(PersistData& out){
  if (!h_) return false;
  size_t len = sizeof(PersistData);
  esp_err_t r = nvs_get_blob(h_, "persist", &out, &len);
  return (r==ESP_OK);
}

bool StateStore::save(const PersistData& in){
  if (!h_) return false;
  esp_err_t r = nvs_set_blob(h_, "persist", &in, sizeof(PersistData));
  if (r!=ESP_OK) return false;
  return nvs_commit(h_)==ESP_OK;
}
