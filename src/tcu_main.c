/**
 * @file tcu_main.c
 * @brief HYPERMOB Platform TCU - Main Entry Point
 * @description TCU program using Library-CAN and REST API client
 *
 * Purpose:
 * - Handle authentication flow (Face/BLE/NFC) via SCA and server
 * - Manage user profile loading and application
 * - Communicate with Platform-Application via REST APIs
 * - Handle CAN communication with SCA and DCU using Library-CAN
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include "../Library-CAN/can_api.h"
#include "../Library-CAN/canmessage.h"
#include "../include/tcu_rest_client.h"

// Global configuration
static const char* g_can_interface = "can0";
static const char* g_car_id = "CAR123";
static volatile int g_running = 1;
static char g_current_user_id[32] = "";
static pthread_mutex_t g_user_mutex = PTHREAD_MUTEX_INITIALIZER;

// Callback prototypes
static void on_sca_user_info_req(const CanFrame* frame, void* user);
static void on_dcu_profile_req(const CanFrame* frame, void* user);
static void on_dcu_profile_update(const CanFrame* frame, void* user);

/**
 * @brief Signal handler for graceful shutdown
 */
static void signal_handler(int signum) {
    printf("\n[TCU] Received signal %d, shutting down...\n", signum);
    g_running = 0;
}

/**
 * @brief Send NFC info to SCA
 */
static void send_nfc_to_sca(const char* nfc_uid_hex) {
    CanMessage msg = {0};

    // Convert hex string to uint64
    uint64_t nfc_val = strtoull(nfc_uid_hex, NULL, 16);
    msg.tcu_sca_user_info_nfc.sig_user_nfc = nfc_val;

    CanFrame frame = can_encode_pcan(PCAN_ID_TCU_SCA_USER_INFO_NFC, &msg, 8);

    if (can_send(g_can_interface, frame, 1000) == CAN_OK) {
        printf("[TCU-CAN] Sent NFC info: 0x%016llX\n", (unsigned long long)nfc_val);
    } else {
        printf("[TCU-CAN] ERROR: Failed to send NFC info\n");
    }
}

/**
 * @brief Send user profile to DCU
 */
static void send_profile_to_dcu(const TcuUserProfile* profile) {
    if (!profile) return;

    CanMessage msg = {0};

    // Send seat profile
    msg.tcu_dcu_user_profile_seat.sig_seat_position = (uint8_t)profile->vehicle_settings.seat_position;
    msg.tcu_dcu_user_profile_seat.sig_seat_angle = (uint8_t)profile->vehicle_settings.seat_angle;
    msg.tcu_dcu_user_profile_seat.sig_seat_front_height = (uint8_t)profile->vehicle_settings.seat_front_height;
    msg.tcu_dcu_user_profile_seat.sig_seat_rear_height = (uint8_t)profile->vehicle_settings.seat_rear_height;

    CanFrame frame = can_encode_pcan(PCAN_ID_TCU_DCU_USER_PROFILE_SEAT, &msg, 4);
    can_send(g_can_interface, frame, 1000);
    printf("[TCU-CAN] Sent SEAT profile: pos=%d, angle=%d\n",
           msg.tcu_dcu_user_profile_seat.sig_seat_position,
           msg.tcu_dcu_user_profile_seat.sig_seat_angle);

    usleep(100000);  // 100ms delay

    // Send mirror profile
    memset(&msg, 0, sizeof(msg));
    msg.tcu_dcu_user_profile_mirror.sig_mirror_left_yaw = (uint8_t)profile->vehicle_settings.mirror_left_yaw;
    msg.tcu_dcu_user_profile_mirror.sig_mirror_left_pitch = (uint8_t)profile->vehicle_settings.mirror_left_pitch;
    msg.tcu_dcu_user_profile_mirror.sig_mirror_right_yaw = (uint8_t)profile->vehicle_settings.mirror_right_yaw;
    msg.tcu_dcu_user_profile_mirror.sig_mirror_right_pitch = (uint8_t)profile->vehicle_settings.mirror_right_pitch;
    msg.tcu_dcu_user_profile_mirror.sig_mirror_room_yaw = (uint8_t)profile->vehicle_settings.mirror_room_yaw;
    msg.tcu_dcu_user_profile_mirror.sig_mirror_room_pitch = (uint8_t)profile->vehicle_settings.mirror_room_pitch;

    frame = can_encode_pcan(PCAN_ID_TCU_DCU_USER_PROFILE_MIRROR, &msg, 6);
    can_send(g_can_interface, frame, 1000);
    printf("[TCU-CAN] Sent MIRROR profile\n");

    usleep(100000);

    // Send wheel profile
    memset(&msg, 0, sizeof(msg));
    msg.tcu_dcu_user_profile_wheel.sig_wheel_position = (uint8_t)profile->vehicle_settings.wheel_position;
    msg.tcu_dcu_user_profile_wheel.sig_wheel_angle = (uint8_t)profile->vehicle_settings.wheel_angle;

    frame = can_encode_pcan(PCAN_ID_TCU_DCU_USER_PROFILE_WHEEL, &msg, 2);
    can_send(g_can_interface, frame, 1000);
    printf("[TCU-CAN] Sent WHEEL profile: pos=%d, angle=%d\n",
           msg.tcu_dcu_user_profile_wheel.sig_wheel_position,
           msg.tcu_dcu_user_profile_wheel.sig_wheel_angle);
}

/**
 * @brief Callback: SCA requests user info
 */
static void on_sca_user_info_req(const CanFrame* frame, void* user) {
    printf("[TCU-CAN] Received SCA_TCU_USER_INFO_REQ\n");

    pthread_mutex_lock(&g_user_mutex);
    if (strlen(g_current_user_id) == 0) {
        pthread_mutex_unlock(&g_user_mutex);
        printf("[TCU] WARNING: No authenticated user\n");
        return;
    }

    char user_id[32];
    strncpy(user_id, g_current_user_id, sizeof(user_id));
    pthread_mutex_unlock(&g_user_mutex);

    // Get auth session from server
    printf("[TCU] Requesting auth session for user: %s\n", user_id);

    TcuAuthSession session = {0};
    if (tcu_rest_get_auth_session(g_car_id, user_id, &session) != 0) {
        printf("[TCU] ERROR: Failed to get auth session\n");
        return;
    }

    // Send NFC info to SCA
    send_nfc_to_sca(session.nfc_uid);

    // Report auth result to server
    printf("[TCU] Reporting auth result to server\n");
    int auth_result = tcu_rest_post_auth_result(session.session_id, g_car_id,
                                                 true, true, true);

    if (auth_result == 0) {
        printf("[TCU] Authentication successful: MFA_SUCCESS\n");
    } else {
        printf("[TCU] Authentication failed\n");
    }
}

/**
 * @brief Callback: DCU requests user profile
 */
static void on_dcu_profile_req(const CanFrame* frame, void* user) {
    printf("[TCU-CAN] Received DCU_TCU_USER_PROFILE_REQ\n");

    pthread_mutex_lock(&g_user_mutex);
    if (strlen(g_current_user_id) == 0) {
        pthread_mutex_unlock(&g_user_mutex);
        printf("[TCU] WARNING: No authenticated user\n");
        return;
    }

    char user_id[32];
    strncpy(user_id, g_current_user_id, sizeof(user_id));
    pthread_mutex_unlock(&g_user_mutex);

    // Get user profile from server
    printf("[TCU] Loading user profile for: %s\n", user_id);

    TcuUserProfile profile = {0};
    if (tcu_rest_get_user_profile(user_id, &profile) != 0) {
        printf("[TCU] ERROR: Failed to load user profile\n");
        return;
    }

    if (!profile.has_profile) {
        printf("[TCU] WARNING: User has no saved profile\n");
        return;
    }

    // Send profile to DCU via CAN
    send_profile_to_dcu(&profile);
}

/**
 * @brief Callback: DCU sends manual profile update
 */
static void on_dcu_profile_update(const CanFrame* frame, void* user) {
    can_msg_pcan_id_t id;
    CanMessage msg = {0};

    if (can_decode_pcan(frame, &id, &msg) != CAN_OK) {
        return;
    }

    pthread_mutex_lock(&g_user_mutex);
    if (strlen(g_current_user_id) == 0) {
        pthread_mutex_unlock(&g_user_mutex);
        return;
    }

    char user_id[32];
    strncpy(user_id, g_current_user_id, sizeof(user_id));
    pthread_mutex_unlock(&g_user_mutex);

    // Get current profile
    TcuUserProfile profile = {0};
    if (tcu_rest_get_user_profile(user_id, &profile) != 0) {
        return;
    }

    // Update based on message type
    switch (id) {
        case 0x206:  // SEAT_UPDATE
            printf("[TCU-CAN] Received SEAT_UPDATE\n");
            profile.vehicle_settings.seat_position = msg.tcu_dcu_user_profile_seat.sig_seat_position;
            profile.vehicle_settings.seat_angle = msg.tcu_dcu_user_profile_seat.sig_seat_angle;
            profile.vehicle_settings.seat_front_height = msg.tcu_dcu_user_profile_seat.sig_seat_front_height;
            profile.vehicle_settings.seat_rear_height = msg.tcu_dcu_user_profile_seat.sig_seat_rear_height;
            break;

        case 0x207:  // MIRROR_UPDATE
            printf("[TCU-CAN] Received MIRROR_UPDATE\n");
            profile.vehicle_settings.mirror_left_yaw = msg.tcu_dcu_user_profile_mirror.sig_mirror_left_yaw;
            profile.vehicle_settings.mirror_left_pitch = msg.tcu_dcu_user_profile_mirror.sig_mirror_left_pitch;
            profile.vehicle_settings.mirror_right_yaw = msg.tcu_dcu_user_profile_mirror.sig_mirror_right_yaw;
            profile.vehicle_settings.mirror_right_pitch = msg.tcu_dcu_user_profile_mirror.sig_mirror_right_pitch;
            profile.vehicle_settings.mirror_room_yaw = msg.tcu_dcu_user_profile_mirror.sig_mirror_room_yaw;
            profile.vehicle_settings.mirror_room_pitch = msg.tcu_dcu_user_profile_mirror.sig_mirror_room_pitch;
            break;

        case 0x208:  // WHEEL_UPDATE
            printf("[TCU-CAN] Received WHEEL_UPDATE\n");
            profile.vehicle_settings.wheel_position = msg.tcu_dcu_user_profile_wheel.sig_wheel_position;
            profile.vehicle_settings.wheel_angle = msg.tcu_dcu_user_profile_wheel.sig_wheel_angle;
            break;

        default:
            return;
    }

    // Update profile on server
    printf("[TCU] Recording manual adjustment to server\n");
    if (tcu_rest_record_manual_settings(g_car_id, user_id, &profile.vehicle_settings) == 0) {
        printf("[TCU] Manual adjustment recorded\n");

        // Send ACK
        CanMessage ack_msg = {0};
        ack_msg.dcu_tcu_user_profile_ack.sig_ack_index = (id == 0x206) ? 1 : (id == 0x207) ? 2 : 3;
        ack_msg.dcu_tcu_user_profile_ack.sig_ack_state = 0;  // OK

        CanFrame ack_frame = can_encode_pcan(0x209, &ack_msg, 2);
        can_send(g_can_interface, ack_frame, 1000);
    }
}

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf(" HYPERMOB Platform TCU\n");
    printf(" Version 1.0.0 (Library-CAN based)\n");
    printf("========================================\n\n");

    // Parse command-line arguments
    const char* app_api_url = "https://your-api-id.execute-api.us-east-1.amazonaws.com/v1";
    const char* ai_api_url = "https://your-api-id.execute-api.ap-northeast-2.amazonaws.com/prod";

    if (argc > 1) g_can_interface = argv[1];
    if (argc > 2) app_api_url = argv[2];
    if (argc > 3) g_car_id = argv[3];

    printf("[TCU] Configuration:\n");
    printf("  CAN Interface: %s\n", g_can_interface);
    printf("  App API URL:   %s\n", app_api_url);
    printf("  Car ID:        %s\n", g_car_id);
    printf("\n");

    // Register signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize CAN library
    printf("[TCU] Initializing CAN...\n");
    if (can_init(CAN_DEVICE_LINUX) != CAN_OK) {
        printf("[TCU] ERROR: Failed to initialize CAN\n");
        return 1;
    }

    // Open CAN interface
    CanConfig can_cfg = {
        .channel = 0,
        .bitrate = 500000,
        .samplePoint = 0.875f,
        .sjw = 1,
        .mode = CAN_MODE_NORMAL
    };

    if (can_open(g_can_interface, can_cfg) != CAN_OK) {
        printf("[TCU] ERROR: Failed to open CAN interface %s\n", g_can_interface);
        can_dispose();
        return 1;
    }

    printf("[TCU] CAN interface %s opened @ 500kbps\n", g_can_interface);

    // Initialize REST client
    printf("[TCU] Initializing REST client...\n");
    TcuRestConfig rest_cfg = {0};
    strncpy(rest_cfg.app_api_base_url, app_api_url, sizeof(rest_cfg.app_api_base_url) - 1);
    strncpy(rest_cfg.ai_api_base_url, ai_api_url, sizeof(rest_cfg.ai_api_base_url) - 1);
    strncpy(rest_cfg.car_id, g_car_id, sizeof(rest_cfg.car_id) - 1);
    rest_cfg.timeout_ms = 5000;

    if (tcu_rest_init(&rest_cfg) != 0) {
        printf("[TCU] ERROR: Failed to initialize REST client\n");
        can_close(g_can_interface);
        can_dispose();
        return 1;
    }

    // Health check
    tcu_rest_health_check_app();

    // Subscribe to CAN messages
    printf("[TCU] Subscribing to CAN messages...\n");

    int sub_ids[10];
    int sub_count = 0;

    CanFilter filter = {.type = CAN_FILTER_MASK};

    // Subscribe to SCA_TCU_USER_INFO_REQ (0x102)
    filter.data.mask.id = 0x102;
    filter.data.mask.mask = 0x7FF;
    can_subscribe(g_can_interface, &sub_ids[sub_count++], filter, on_sca_user_info_req, NULL);
    printf("[TCU-CAN] Subscribed to SCA_TCU_USER_INFO_REQ (0x102)\n");

    // Subscribe to DCU_TCU_USER_PROFILE_REQ (0x201)
    filter.data.mask.id = 0x201;
    can_subscribe(g_can_interface, &sub_ids[sub_count++], filter, on_dcu_profile_req, NULL);
    printf("[TCU-CAN] Subscribed to DCU_TCU_USER_PROFILE_REQ (0x201)\n");

    // Subscribe to profile updates (0x206, 0x207, 0x208)
    for (uint32_t id = 0x206; id <= 0x208; id++) {
        filter.data.mask.id = id;
        can_subscribe(g_can_interface, &sub_ids[sub_count++], filter, on_dcu_profile_update, NULL);
        printf("[TCU-CAN] Subscribed to USER_PROFILE_UPDATE (0x%03X)\n", id);
    }

    printf("\n[TCU] System ready. Press Ctrl+C to exit.\n");
    printf("[TCU] Set current user with: echo \"USER123\" > /tmp/tcu_user\n\n");

    // Set default user (can be changed via file or signal)
    pthread_mutex_lock(&g_user_mutex);
    strncpy(g_current_user_id, "USER123", sizeof(g_current_user_id));
    pthread_mutex_unlock(&g_user_mutex);

    // Main loop
    while (g_running) {
        sleep(5);

        // Check for user change via file
        FILE* f = fopen("/tmp/tcu_user", "r");
        if (f) {
            char new_user[32] = "";
            if (fgets(new_user, sizeof(new_user), f)) {
                // Remove newline
                new_user[strcspn(new_user, "\r\n")] = '\0';

                pthread_mutex_lock(&g_user_mutex);
                if (strcmp(g_current_user_id, new_user) != 0) {
                    strncpy(g_current_user_id, new_user, sizeof(g_current_user_id));
                    printf("[TCU] User changed to: %s\n", g_current_user_id);
                }
                pthread_mutex_unlock(&g_user_mutex);
            }
            fclose(f);
        }
    }

    // Cleanup
    printf("\n[TCU] Shutting down...\n");

    for (int i = 0; i < sub_count; i++) {
        can_unsubscribe(g_can_interface, sub_ids[i]);
    }

    tcu_rest_shutdown();
    can_close(g_can_interface);
    can_dispose();
    pthread_mutex_destroy(&g_user_mutex);

    printf("[TCU] Shutdown complete\n");

    return 0;
}
