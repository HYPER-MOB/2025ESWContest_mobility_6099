package com.hypermob.mydrive3dx.domain.model

/**
 * Rental Status Enum
 *
 * 대여 상태
 */
enum class RentalStatus {
    REQUESTED,   // 대여 요청
    APPROVED,    // 승인됨
    PICKED,      // 차량 인수
    RETURNED,    // 반납 완료
    CANCELED;    // 취소됨

    companion object {
        fun fromString(status: String): RentalStatus {
            return when (status.uppercase()) {
                "REQUESTED" -> REQUESTED
                "APPROVED" -> APPROVED
                "PICKED" -> PICKED
                "RETURNED" -> RETURNED
                "CANCELED" -> CANCELED
                else -> REQUESTED
            }
        }
    }
}
