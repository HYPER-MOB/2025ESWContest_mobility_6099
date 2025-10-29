/**
 * @file tcu_rest_client.c
 * @brief TCU REST API Client Implementation
 */

#include "../include/tcu_rest_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

// Global configuration
static TcuRestConfig g_config = {0};
static bool g_initialized = false;

// Buffer for HTTP response data
typedef struct {
    char* data;
    size_t size;
} HttpResponse;

/**
 * @brief Callback for libcurl to write response data
 */
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    HttpResponse* mem = (HttpResponse*)userp;

    char* ptr = realloc(mem->data, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("[TCU-REST] ERROR: Out of memory\n");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

/**
 * @brief Perform HTTP GET request
 */
static int http_get(const char* url, HttpResponse* response) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        printf("[TCU-REST] ERROR: Failed to initialize CURL\n");
        return -1;
    }

    response->data = malloc(1);
    response->size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, g_config.timeout_ms);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "HYPERMOB-TCU/1.0");

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        printf("[TCU-REST] ERROR: GET %s failed: %s\n", url, curl_easy_strerror(res));
        free(response->data);
        return -1;
    }

    if (http_code >= 400) {
        printf("[TCU-REST] ERROR: GET %s returned HTTP %ld\n", url, http_code);
        printf("[TCU-REST] Response: %s\n", response->data);
        free(response->data);
        return -1;
    }

    return 0;
}

/**
 * @brief Perform HTTP POST request
 */
static int http_post(const char* url, const char* json_body, HttpResponse* response) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        printf("[TCU-REST] ERROR: Failed to initialize CURL\n");
        return -1;
    }

    response->data = malloc(1);
    response->size = 0;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, g_config.timeout_ms);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "HYPERMOB-TCU/1.0");

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        printf("[TCU-REST] ERROR: POST %s failed: %s\n", url, curl_easy_strerror(res));
        free(response->data);
        return -1;
    }

    if (http_code >= 400) {
        printf("[TCU-REST] ERROR: POST %s returned HTTP %ld\n", url, http_code);
        printf("[TCU-REST] Response: %s\n", response->data);
        free(response->data);
        return -1;
    }

    return 0;
}

/**
 * @brief Perform HTTP PUT request
 */
static int http_put(const char* url, const char* json_body, HttpResponse* response) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        printf("[TCU-REST] ERROR: Failed to initialize CURL\n");
        return -1;
    }

    response->data = malloc(1);
    response->size = 0;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, g_config.timeout_ms);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "HYPERMOB-TCU/1.0");

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        printf("[TCU-REST] ERROR: PUT %s failed: %s\n", url, curl_easy_strerror(res));
        free(response->data);
        return -1;
    }

    if (http_code >= 400) {
        printf("[TCU-REST] ERROR: PUT %s returned HTTP %ld\n", url, http_code);
        printf("[TCU-REST] Response: %s\n", response->data);
        free(response->data);
        return -1;
    }

    return 0;
}

/**
 * @brief Initialize REST API client
 */
int tcu_rest_init(const TcuRestConfig* config) {
    if (!config) {
        printf("[TCU-REST] ERROR: Invalid configuration\n");
        return -1;
    }

    if (g_initialized) {
        printf("[TCU-REST] WARNING: Already initialized\n");
        return 0;
    }

    // Copy configuration
    memcpy(&g_config, config, sizeof(TcuRestConfig));

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);

    g_initialized = true;

    printf("[TCU-REST] Initialized\n");
    printf("[TCU-REST] App API: %s\n", g_config.app_api_base_url);
    printf("[TCU-REST] AI API: %s\n", g_config.ai_api_base_url);
    printf("[TCU-REST] Car ID: %s\n", g_config.car_id);

    return 0;
}

/**
 * @brief Shutdown REST API client
 */
void tcu_rest_shutdown(void) {
    if (!g_initialized) {
        return;
    }

    curl_global_cleanup();
    g_initialized = false;

    printf("[TCU-REST] Shutdown complete\n");
}

/**
 * @brief Request authentication session from server
 */
int tcu_rest_get_auth_session(const char* car_id, const char* user_id,
                               TcuAuthSession* session) {
    if (!g_initialized || !car_id || !user_id || !session) {
        return -1;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s/auth/session?car_id=%s&user_id=%s",
             g_config.app_api_base_url, car_id, user_id);

    HttpResponse response = {0};
    if (http_get(url, &response) != 0) {
        return -1;
    }

    // Parse JSON response
    cJSON* json = cJSON_Parse(response.data);
    if (!json) {
        printf("[TCU-REST] ERROR: Failed to parse JSON response\n");
        free(response.data);
        return -1;
    }

    cJSON* session_id = cJSON_GetObjectItem(json, "session_id");
    cJSON* face_id = cJSON_GetObjectItem(json, "face_id");
    cJSON* hashkey = cJSON_GetObjectItem(json, "hashkey");
    cJSON* nfc_uid = cJSON_GetObjectItem(json, "nfc_uid");
    cJSON* status = cJSON_GetObjectItem(json, "status");

    if (session_id) strncpy(session->session_id, session_id->valuestring, sizeof(session->session_id) - 1);
    if (face_id) strncpy(session->face_id, face_id->valuestring, sizeof(session->face_id) - 1);
    if (hashkey) strncpy(session->hashkey, hashkey->valuestring, sizeof(session->hashkey) - 1);
    if (nfc_uid) strncpy(session->nfc_uid, nfc_uid->valuestring, sizeof(session->nfc_uid) - 1);
    if (status) strncpy(session->status, status->valuestring, sizeof(session->status) - 1);
    strncpy(session->user_id, user_id, sizeof(session->user_id) - 1);

    printf("[TCU-REST] Auth session retrieved: session_id=%s\n", session->session_id);

    cJSON_Delete(json);
    free(response.data);

    return 0;
}

/**
 * @brief Report authentication result to server
 */
int tcu_rest_post_auth_result(const char* session_id, const char* car_id,
                               bool face_verified, bool ble_verified, bool nfc_verified) {
    if (!g_initialized || !session_id || !car_id) {
        return -1;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s/auth/result", g_config.app_api_base_url);

    // Build JSON body
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "session_id", session_id);
    cJSON_AddStringToObject(json, "car_id", car_id);
    cJSON_AddBoolToObject(json, "face_verified", face_verified);
    cJSON_AddBoolToObject(json, "ble_verified", ble_verified);
    cJSON_AddBoolToObject(json, "nfc_verified", nfc_verified);

    char* json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    HttpResponse response = {0};
    if (http_post(url, json_str, &response) != 0) {
        free(json_str);
        return -1;
    }

    // Parse response
    cJSON* resp_json = cJSON_Parse(response.data);
    if (!resp_json) {
        free(json_str);
        free(response.data);
        return -1;
    }

    cJSON* status = cJSON_GetObjectItem(resp_json, "status");
    int result = -1;

    if (status && status->valuestring) {
        if (strcmp(status->valuestring, "MFA_SUCCESS") == 0) {
            result = 0;
            printf("[TCU-REST] Auth result: MFA_SUCCESS\n");
        } else if (strcmp(status->valuestring, "MFA_FAILED") == 0) {
            result = 1;
            printf("[TCU-REST] Auth result: MFA_FAILED\n");
        }
    }

    cJSON_Delete(resp_json);
    free(json_str);
    free(response.data);

    return result;
}

/**
 * @brief Get BLE hashkey for authentication
 */
int tcu_rest_get_ble_hashkey(const char* user_id, const char* car_id,
                              char* hashkey_out, char* expires_at_out) {
    if (!g_initialized || !user_id || !hashkey_out) {
        return -1;
    }

    const char* cid = car_id ? car_id : g_config.car_id;

    char url[512];
    snprintf(url, sizeof(url), "%s/auth/ble?user_id=%s&car_id=%s",
             g_config.app_api_base_url, user_id, cid);

    HttpResponse response = {0};
    if (http_get(url, &response) != 0) {
        return -1;
    }

    cJSON* json = cJSON_Parse(response.data);
    if (!json) {
        free(response.data);
        return -1;
    }

    cJSON* hashkey = cJSON_GetObjectItem(json, "hashkey");
    cJSON* expires_at = cJSON_GetObjectItem(json, "expires_at");

    if (hashkey && hashkey->valuestring) {
        strncpy(hashkey_out, hashkey->valuestring, 16);
        hashkey_out[16] = '\0';
    }

    if (expires_at_out && expires_at && expires_at->valuestring) {
        strncpy(expires_at_out, expires_at->valuestring, 31);
        expires_at_out[31] = '\0';
    }

    printf("[TCU-REST] BLE hashkey retrieved: %s\n", hashkey_out);

    cJSON_Delete(json);
    free(response.data);

    return 0;
}

/**
 * @brief Get user profile from server
 */
int tcu_rest_get_user_profile(const char* user_id, TcuUserProfile* profile) {
    if (!g_initialized || !user_id || !profile) {
        return -1;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s/users/%s/profile", g_config.app_api_base_url, user_id);

    HttpResponse response = {0};
    if (http_get(url, &response) != 0) {
        return -1;
    }

    cJSON* json = cJSON_Parse(response.data);
    if (!json) {
        free(response.data);
        return -1;
    }

    cJSON* data = cJSON_GetObjectItem(json, "data");
    if (!data) {
        cJSON_Delete(json);
        free(response.data);
        return -1;
    }

    // Parse user_id
    cJSON* uid = cJSON_GetObjectItem(data, "user_id");
    if (uid) strncpy(profile->user_id, uid->valuestring, sizeof(profile->user_id) - 1);

    // Parse has_profile
    cJSON* has_prof = cJSON_GetObjectItem(data, "has_profile");
    if (has_prof) profile->has_profile = cJSON_IsTrue(has_prof);

    // Parse body_measurements
    cJSON* body = cJSON_GetObjectItem(data, "body_measurements");
    if (body) {
        cJSON* height = cJSON_GetObjectItem(body, "height");
        cJSON* upper_arm = cJSON_GetObjectItem(body, "upper_arm");
        cJSON* forearm = cJSON_GetObjectItem(body, "forearm");
        cJSON* thigh = cJSON_GetObjectItem(body, "thigh");
        cJSON* calf = cJSON_GetObjectItem(body, "calf");
        cJSON* torso = cJSON_GetObjectItem(body, "torso");

        if (height) profile->body_measurements.height = (float)height->valuedouble;
        if (upper_arm) profile->body_measurements.upper_arm = (float)upper_arm->valuedouble;
        if (forearm) profile->body_measurements.forearm = (float)forearm->valuedouble;
        if (thigh) profile->body_measurements.thigh = (float)thigh->valuedouble;
        if (calf) profile->body_measurements.calf = (float)calf->valuedouble;
        if (torso) profile->body_measurements.torso = (float)torso->valuedouble;
    }

    // Parse vehicle_settings
    cJSON* settings = cJSON_GetObjectItem(data, "vehicle_settings");
    if (settings) {
        cJSON* seat_pos = cJSON_GetObjectItem(settings, "seat_position");
        cJSON* seat_ang = cJSON_GetObjectItem(settings, "seat_angle");
        cJSON* seat_fh = cJSON_GetObjectItem(settings, "seat_front_height");
        cJSON* seat_rh = cJSON_GetObjectItem(settings, "seat_rear_height");
        cJSON* mir_ly = cJSON_GetObjectItem(settings, "mirror_left_yaw");
        cJSON* mir_lp = cJSON_GetObjectItem(settings, "mirror_left_pitch");
        cJSON* mir_ry = cJSON_GetObjectItem(settings, "mirror_right_yaw");
        cJSON* mir_rp = cJSON_GetObjectItem(settings, "mirror_right_pitch");
        cJSON* mir_rmy = cJSON_GetObjectItem(settings, "mirror_room_yaw");
        cJSON* mir_rmp = cJSON_GetObjectItem(settings, "mirror_room_pitch");
        cJSON* whl_pos = cJSON_GetObjectItem(settings, "wheel_position");
        cJSON* whl_ang = cJSON_GetObjectItem(settings, "wheel_angle");

        if (seat_pos) profile->vehicle_settings.seat_position = seat_pos->valueint;
        if (seat_ang) profile->vehicle_settings.seat_angle = seat_ang->valueint;
        if (seat_fh) profile->vehicle_settings.seat_front_height = seat_fh->valueint;
        if (seat_rh) profile->vehicle_settings.seat_rear_height = seat_rh->valueint;
        if (mir_ly) profile->vehicle_settings.mirror_left_yaw = mir_ly->valueint;
        if (mir_lp) profile->vehicle_settings.mirror_left_pitch = mir_lp->valueint;
        if (mir_ry) profile->vehicle_settings.mirror_right_yaw = mir_ry->valueint;
        if (mir_rp) profile->vehicle_settings.mirror_right_pitch = mir_rp->valueint;
        if (mir_rmy) profile->vehicle_settings.mirror_room_yaw = mir_rmy->valueint;
        if (mir_rmp) profile->vehicle_settings.mirror_room_pitch = mir_rmp->valueint;
        if (whl_pos) profile->vehicle_settings.wheel_position = whl_pos->valueint;
        if (whl_ang) profile->vehicle_settings.wheel_angle = whl_ang->valueint;
    }

    printf("[TCU-REST] User profile retrieved: %s\n", profile->user_id);

    cJSON_Delete(json);
    free(response.data);

    return 0;
}

/**
 * @brief Update user profile on server
 */
int tcu_rest_put_user_profile(const char* user_id, const TcuUserProfile* profile) {
    if (!g_initialized || !user_id || !profile) {
        return -1;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s/users/%s/profile", g_config.app_api_base_url, user_id);

    // Build JSON body
    cJSON* json = cJSON_CreateObject();
    cJSON* body = cJSON_CreateObject();
    cJSON* settings = cJSON_CreateObject();

    cJSON_AddNumberToObject(body, "height", profile->body_measurements.height);
    cJSON_AddNumberToObject(body, "upper_arm", profile->body_measurements.upper_arm);
    cJSON_AddNumberToObject(body, "forearm", profile->body_measurements.forearm);
    cJSON_AddNumberToObject(body, "thigh", profile->body_measurements.thigh);
    cJSON_AddNumberToObject(body, "calf", profile->body_measurements.calf);
    cJSON_AddNumberToObject(body, "torso", profile->body_measurements.torso);

    cJSON_AddNumberToObject(settings, "seat_position", profile->vehicle_settings.seat_position);
    cJSON_AddNumberToObject(settings, "seat_angle", profile->vehicle_settings.seat_angle);
    cJSON_AddNumberToObject(settings, "seat_front_height", profile->vehicle_settings.seat_front_height);
    cJSON_AddNumberToObject(settings, "seat_rear_height", profile->vehicle_settings.seat_rear_height);
    cJSON_AddNumberToObject(settings, "mirror_left_yaw", profile->vehicle_settings.mirror_left_yaw);
    cJSON_AddNumberToObject(settings, "mirror_left_pitch", profile->vehicle_settings.mirror_left_pitch);
    cJSON_AddNumberToObject(settings, "mirror_right_yaw", profile->vehicle_settings.mirror_right_yaw);
    cJSON_AddNumberToObject(settings, "mirror_right_pitch", profile->vehicle_settings.mirror_right_pitch);
    cJSON_AddNumberToObject(settings, "mirror_room_yaw", profile->vehicle_settings.mirror_room_yaw);
    cJSON_AddNumberToObject(settings, "mirror_room_pitch", profile->vehicle_settings.mirror_room_pitch);
    cJSON_AddNumberToObject(settings, "wheel_position", profile->vehicle_settings.wheel_position);
    cJSON_AddNumberToObject(settings, "wheel_angle", profile->vehicle_settings.wheel_angle);

    cJSON_AddItemToObject(json, "body_measurements", body);
    cJSON_AddItemToObject(json, "vehicle_settings", settings);

    char* json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    HttpResponse response = {0};
    int result = http_put(url, json_str, &response);

    if (result == 0) {
        printf("[TCU-REST] User profile updated: %s\n", user_id);
    }

    free(json_str);
    free(response.data);

    return result;
}

/**
 * @brief Apply vehicle settings automatically
 */
int tcu_rest_apply_vehicle_settings(const char* car_id, const char* user_id,
                                     const TcuVehicleSettings* settings) {
    if (!g_initialized || !car_id || !user_id || !settings) {
        return -1;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s/vehicles/%s/settings/apply",
             g_config.app_api_base_url, car_id);

    // Build JSON body
    cJSON* json = cJSON_CreateObject();
    cJSON* set = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "user_id", user_id);

    cJSON_AddNumberToObject(set, "seat_position", settings->seat_position);
    cJSON_AddNumberToObject(set, "seat_angle", settings->seat_angle);
    cJSON_AddNumberToObject(set, "seat_front_height", settings->seat_front_height);
    cJSON_AddNumberToObject(set, "seat_rear_height", settings->seat_rear_height);
    cJSON_AddNumberToObject(set, "mirror_left_yaw", settings->mirror_left_yaw);
    cJSON_AddNumberToObject(set, "mirror_left_pitch", settings->mirror_left_pitch);
    cJSON_AddNumberToObject(set, "mirror_right_yaw", settings->mirror_right_yaw);
    cJSON_AddNumberToObject(set, "mirror_right_pitch", settings->mirror_right_pitch);
    cJSON_AddNumberToObject(set, "mirror_room_yaw", settings->mirror_room_yaw);
    cJSON_AddNumberToObject(set, "mirror_room_pitch", settings->mirror_room_pitch);
    cJSON_AddNumberToObject(set, "wheel_position", settings->wheel_position);
    cJSON_AddNumberToObject(set, "wheel_angle", settings->wheel_angle);

    cJSON_AddItemToObject(json, "settings", set);

    char* json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    HttpResponse response = {0};
    int result = http_post(url, json_str, &response);

    if (result == 0) {
        printf("[TCU-REST] Vehicle settings applied for car: %s\n", car_id);
    }

    free(json_str);
    free(response.data);

    return result;
}

/**
 * @brief Record manual vehicle adjustment
 */
int tcu_rest_record_manual_settings(const char* car_id, const char* user_id,
                                     const TcuVehicleSettings* settings) {
    if (!g_initialized || !car_id || !user_id || !settings) {
        return -1;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s/vehicles/%s/settings/manual",
             g_config.app_api_base_url, car_id);

    // Build JSON body (similar to apply_vehicle_settings)
    cJSON* json = cJSON_CreateObject();
    cJSON* set = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "user_id", user_id);

    cJSON_AddNumberToObject(set, "seat_position", settings->seat_position);
    cJSON_AddNumberToObject(set, "seat_angle", settings->seat_angle);
    cJSON_AddNumberToObject(set, "seat_front_height", settings->seat_front_height);
    cJSON_AddNumberToObject(set, "seat_rear_height", settings->seat_rear_height);
    cJSON_AddNumberToObject(set, "mirror_left_yaw", settings->mirror_left_yaw);
    cJSON_AddNumberToObject(set, "mirror_left_pitch", settings->mirror_left_pitch);
    cJSON_AddNumberToObject(set, "mirror_right_yaw", settings->mirror_right_yaw);
    cJSON_AddNumberToObject(set, "mirror_right_pitch", settings->mirror_right_pitch);
    cJSON_AddNumberToObject(set, "mirror_room_yaw", settings->mirror_room_yaw);
    cJSON_AddNumberToObject(set, "mirror_room_pitch", settings->mirror_room_pitch);
    cJSON_AddNumberToObject(set, "wheel_position", settings->wheel_position);
    cJSON_AddNumberToObject(set, "wheel_angle", settings->wheel_angle);

    cJSON_AddItemToObject(json, "settings", set);

    char* json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    HttpResponse response = {0};
    int result = http_post(url, json_str, &response);

    if (result == 0) {
        printf("[TCU-REST] Manual settings recorded for car: %s\n", car_id);
    }

    free(json_str);
    free(response.data);

    return result;
}

/**
 * @brief Get current vehicle settings
 */
int tcu_rest_get_current_settings(const char* car_id, TcuVehicleSettings* settings) {
    if (!g_initialized || !car_id || !settings) {
        return -1;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s/vehicles/%s/settings/current",
             g_config.app_api_base_url, car_id);

    HttpResponse response = {0};
    if (http_get(url, &response) != 0) {
        return -1;
    }

    cJSON* json = cJSON_Parse(response.data);
    if (!json) {
        free(response.data);
        return -1;
    }

    cJSON* data = cJSON_GetObjectItem(json, "data");
    cJSON* set = cJSON_GetObjectItem(data, "settings");

    if (set) {
        cJSON* sp = cJSON_GetObjectItem(set, "seat_position");
        cJSON* sa = cJSON_GetObjectItem(set, "seat_angle");
        cJSON* sfh = cJSON_GetObjectItem(set, "seat_front_height");
        cJSON* srh = cJSON_GetObjectItem(set, "seat_rear_height");
        cJSON* mly = cJSON_GetObjectItem(set, "mirror_left_yaw");
        cJSON* mlp = cJSON_GetObjectItem(set, "mirror_left_pitch");
        cJSON* mry = cJSON_GetObjectItem(set, "mirror_right_yaw");
        cJSON* mrp = cJSON_GetObjectItem(set, "mirror_right_pitch");
        cJSON* mrmy = cJSON_GetObjectItem(set, "mirror_room_yaw");
        cJSON* mrmp = cJSON_GetObjectItem(set, "mirror_room_pitch");
        cJSON* wp = cJSON_GetObjectItem(set, "wheel_position");
        cJSON* wa = cJSON_GetObjectItem(set, "wheel_angle");

        if (sp) settings->seat_position = sp->valueint;
        if (sa) settings->seat_angle = sa->valueint;
        if (sfh) settings->seat_front_height = sfh->valueint;
        if (srh) settings->seat_rear_height = srh->valueint;
        if (mly) settings->mirror_left_yaw = mly->valueint;
        if (mlp) settings->mirror_left_pitch = mlp->valueint;
        if (mry) settings->mirror_right_yaw = mry->valueint;
        if (mrp) settings->mirror_right_pitch = mrp->valueint;
        if (mrmy) settings->mirror_room_yaw = mrmy->valueint;
        if (mrmp) settings->mirror_room_pitch = mrmp->valueint;
        if (wp) settings->wheel_position = wp->valueint;
        if (wa) settings->wheel_angle = wa->valueint;
    }

    printf("[TCU-REST] Current vehicle settings retrieved for car: %s\n", car_id);

    cJSON_Delete(json);
    free(response.data);

    return 0;
}

/**
 * @brief Check Platform-Application API health
 */
int tcu_rest_health_check_app(void) {
    if (!g_initialized) {
        return -1;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s/health", g_config.app_api_base_url);

    HttpResponse response = {0};
    int result = http_get(url, &response);

    if (result == 0) {
        printf("[TCU-REST] Platform-Application health: OK\n");
    } else {
        printf("[TCU-REST] Platform-Application health: FAILED\n");
    }

    free(response.data);
    return result;
}

/**
 * @brief Check Platform-AI API health
 */
int tcu_rest_health_check_ai(void) {
    if (!g_initialized) {
        return -1;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s/health", g_config.ai_api_base_url);

    HttpResponse response = {0};
    int result = http_get(url, &response);

    if (result == 0) {
        printf("[TCU-REST] Platform-AI health: OK\n");
    } else {
        printf("[TCU-REST] Platform-AI health: FAILED\n");
    }

    free(response.data);
    return result;
}

// Placeholder stubs for AI functions (can be implemented later if needed)
int tcu_rest_ai_measure_body(const char* image_url, float height_cm,
                              const char* user_id,
                              float* arm_cm, float* leg_cm, float* torso_cm) {
    printf("[TCU-REST] AI measure_body: Not implemented\n");
    return -1;
}

int tcu_rest_ai_recommend_settings(const TcuBodyMeasurements* body_measurements,
                                    const char* user_id,
                                    TcuVehicleSettings* settings) {
    printf("[TCU-REST] AI recommend_settings: Not implemented\n");
    return -1;
}

int tcu_rest_get_active_booking(const char* car_id, TcuBooking* booking) {
    printf("[TCU-REST] Get active booking: Not implemented\n");
    return -1;
}
