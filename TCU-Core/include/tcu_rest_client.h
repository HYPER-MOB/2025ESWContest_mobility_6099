/**
 * @file tcu_rest_client.h
 * @brief TCU REST API Client
 * @description Handles REST API communication with Platform-Application and Platform-AI
 */

#ifndef TCU_REST_CLIENT_H
#define TCU_REST_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// API endpoint configuration
typedef struct {
    char app_api_base_url[256];     // Platform-Application API base URL
    char ai_api_base_url[256];      // Platform-AI API base URL
    char car_id[32];                // Car ID (e.g., "CAR123")
    uint32_t timeout_ms;            // HTTP request timeout in milliseconds
} TcuRestConfig;

// Authentication session data
typedef struct {
    char session_id[64];
    char face_id[32];
    char hashkey[32];
    char nfc_uid[32];
    char user_id[32];
    char status[16];
} TcuAuthSession;

// User profile data structures (matching API schemas)
typedef struct {
    float height;
    float upper_arm;
    float forearm;
    float thigh;
    float calf;
    float torso;
} TcuBodyMeasurements;

typedef struct {
    int seat_position;
    int seat_angle;
    int seat_front_height;
    int seat_rear_height;
    int mirror_left_yaw;
    int mirror_left_pitch;
    int mirror_right_yaw;
    int mirror_right_pitch;
    int mirror_room_yaw;
    int mirror_room_pitch;
    int wheel_position;
    int wheel_angle;
} TcuVehicleSettings;

typedef struct {
    char user_id[32];
    TcuBodyMeasurements body_measurements;
    TcuVehicleSettings vehicle_settings;
    bool has_profile;
} TcuUserProfile;

// Booking information
typedef struct {
    char booking_id[32];
    char user_id[32];
    char car_id[32];
    char status[32];
    char start_time[32];
    char end_time[32];
} TcuBooking;

/**
 * @brief Initialize REST API client
 * @param config Configuration for API endpoints
 * @return 0 on success, -1 on error
 */
int tcu_rest_init(const TcuRestConfig* config);

/**
 * @brief Shutdown REST API client
 */
void tcu_rest_shutdown(void);

// ========== Platform-Application MFA Authentication APIs ==========

/**
 * @brief Request authentication session from server
 * @param car_id Car ID
 * @param user_id User ID
 * @param session Output session data
 * @return 0 on success, -1 on error
 */
int tcu_rest_get_auth_session(const char* car_id, const char* user_id,
                               TcuAuthSession* session);

/**
 * @brief Report authentication result to server
 * @param session_id Session ID
 * @param car_id Car ID
 * @param face_verified Face authentication result
 * @param ble_verified BLE authentication result
 * @param nfc_verified NFC authentication result
 * @return 0 on success (MFA_SUCCESS), 1 on partial failure (MFA_FAILED), -1 on error
 */
int tcu_rest_post_auth_result(const char* session_id, const char* car_id,
                               bool face_verified, bool ble_verified, bool nfc_verified);

/**
 * @brief Get BLE hashkey for authentication
 * @param user_id User ID
 * @param car_id Car ID
 * @param hashkey_out Output buffer for hashkey (16 hex chars + null)
 * @param expires_at_out Output buffer for expiry time (optional, can be NULL)
 * @return 0 on success, -1 on error
 */
int tcu_rest_get_ble_hashkey(const char* user_id, const char* car_id,
                              char* hashkey_out, char* expires_at_out);

// ========== Platform-Application User Profile APIs ==========

/**
 * @brief Get user profile from server
 * @param user_id User ID
 * @param profile Output profile data
 * @return 0 on success, -1 on error
 */
int tcu_rest_get_user_profile(const char* user_id, TcuUserProfile* profile);

/**
 * @brief Update user profile on server
 * @param user_id User ID
 * @param profile Profile data to update
 * @return 0 on success, -1 on error
 */
int tcu_rest_put_user_profile(const char* user_id, const TcuUserProfile* profile);

// ========== Platform-Application Vehicle Settings APIs ==========

/**
 * @brief Apply vehicle settings automatically
 * @param car_id Car ID
 * @param user_id User ID
 * @param settings Vehicle settings to apply
 * @return 0 on success, -1 on error
 */
int tcu_rest_apply_vehicle_settings(const char* car_id, const char* user_id,
                                     const TcuVehicleSettings* settings);

/**
 * @brief Record manual vehicle adjustment
 * @param car_id Car ID
 * @param user_id User ID
 * @param settings Vehicle settings (manual adjustment)
 * @return 0 on success, -1 on error
 */
int tcu_rest_record_manual_settings(const char* car_id, const char* user_id,
                                     const TcuVehicleSettings* settings);

/**
 * @brief Get current vehicle settings
 * @param car_id Car ID
 * @param settings Output current settings
 * @return 0 on success, -1 on error
 */
int tcu_rest_get_current_settings(const char* car_id, TcuVehicleSettings* settings);

// ========== Platform-Application Booking APIs ==========

/**
 * @brief Get active booking for a car
 * @param car_id Car ID
 * @param booking Output booking information
 * @return 0 on success, 1 if no active booking, -1 on error
 */
int tcu_rest_get_active_booking(const char* car_id, TcuBooking* booking);

// ========== Platform-AI Body Measurement APIs ==========

/**
 * @brief Request body measurement from AI service
 * @param image_url S3 image URL
 * @param height_cm User's height in cm
 * @param user_id User ID (optional)
 * @param arm_cm Output arm length
 * @param leg_cm Output leg length
 * @param torso_cm Output torso length
 * @return 0 on success, -1 on error
 */
int tcu_rest_ai_measure_body(const char* image_url, float height_cm,
                              const char* user_id,
                              float* arm_cm, float* leg_cm, float* torso_cm);

/**
 * @brief Request vehicle settings recommendation from AI service
 * @param body_measurements Body measurement data
 * @param user_id User ID (optional)
 * @param settings Output recommended vehicle settings
 * @return 0 on success, -1 on error
 */
int tcu_rest_ai_recommend_settings(const TcuBodyMeasurements* body_measurements,
                                    const char* user_id,
                                    TcuVehicleSettings* settings);

// ========== Health Check APIs ==========

/**
 * @brief Check Platform-Application API health
 * @return 0 if healthy, -1 on error
 */
int tcu_rest_health_check_app(void);

/**
 * @brief Check Platform-AI API health
 * @return 0 if healthy, -1 on error
 */
int tcu_rest_health_check_ai(void);

#ifdef __cplusplus
}
#endif

#endif // TCU_REST_CLIENT_H
