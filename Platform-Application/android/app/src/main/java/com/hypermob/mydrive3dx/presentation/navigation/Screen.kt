package com.hypermob.mydrive3dx.presentation.navigation

/**
 * Navigation Screen Routes
 *
 * 앱의 모든 화면 경로를 정의
 */
sealed class Screen(val route: String) {
    // 인증 관련
    data object Login : Screen("login")
    data object Register : Screen("register")

    // 메인 화면 (Bottom Navigation)
    data object Home : Screen("home")
    data object Rental : Screen("rental")
    data object Control : Screen("control")
    data object Settings : Screen("settings")

    // 차량 관련 (시나리오2)
    data object VehicleList : Screen("vehicles")
    data object VehicleDetail : Screen("vehicle/{vehicleId}") {
        fun createRoute(vehicleId: String) = "vehicle/$vehicleId"
    }

    // Rental 상세
    data object RentalDetail : Screen("rental/{rentalId}") {
        fun createRoute(rentalId: String) = "rental/$rentalId"
    }

    // 대여 승인 화면 (시나리오2)
    data object RentalApproval : Screen("rental/approval/{rentalId}") {
        fun createRoute(rentalId: String) = "rental/approval/$rentalId"
    }

    // MFA 인증
    data object MfaAuth : Screen("mfa/{rentalId}") {
        fun createRoute(rentalId: String) = "mfa/$rentalId"
    }

    // 설정 상세
    data object ProfileEdit : Screen("settings/profile")
    data object SecuritySettings : Screen("settings/security")
    data object RegisterFace : Screen("settings/register-face")
    data object RegisterNfc : Screen("settings/register-nfc")
    data object RegisterBle : Screen("settings/register-ble")

    // 3D 바디스캔
    data object BodyScan : Screen("bodyscan")

    // 졸음 감지 모니터
    data object DrowsinessMonitor : Screen("drowsiness")
}
