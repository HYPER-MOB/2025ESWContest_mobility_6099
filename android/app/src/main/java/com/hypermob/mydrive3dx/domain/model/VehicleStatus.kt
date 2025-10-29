package com.hypermob.mydrive3dx.domain.model

/**
 * Vehicle Status
 *
 * 차량 상태
 */
enum class VehicleStatus {
    AVAILABLE,      // 대여 가능
    RENTED,         // 대여 중
    MAINTENANCE,    // 정비 중
    RESERVED        // 예약됨
}
