package com.hypermob.mydrive3dx.presentation.navigation

import androidx.compose.runtime.Composable
import androidx.navigation.NavHostController
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.navArgument
import com.hypermob.mydrive3dx.presentation.control.ControlScreen
import com.hypermob.mydrive3dx.presentation.home.HomeScreen
import com.hypermob.mydrive3dx.presentation.login.LoginScreen
import com.hypermob.mydrive3dx.presentation.mfa.MfaScreen
import com.hypermob.mydrive3dx.presentation.register.RegisterScreen
import com.hypermob.mydrive3dx.presentation.rental.RentalDetailScreen
import com.hypermob.mydrive3dx.presentation.rental.RentalListScreen
import com.hypermob.mydrive3dx.presentation.rental.RentalApprovalScreen
import com.hypermob.mydrive3dx.presentation.vehicle.VehicleListScreen
import com.hypermob.mydrive3dx.presentation.vehicle.VehicleDetailScreen
import com.hypermob.mydrive3dx.presentation.settings.ProfileEditScreen
import com.hypermob.mydrive3dx.presentation.settings.SettingsScreen
import com.hypermob.mydrive3dx.presentation.bodyscan.BodyScanScreen
import com.hypermob.mydrive3dx.presentation.drowsiness.DrowsinessMonitorScreen
import com.hypermob.mydrive3dx.presentation.face.FaceRegisterScreen
import com.hypermob.mydrive3dx.data.hardware.BodyScanManager
import com.hypermob.mydrive3dx.data.hardware.DrowsinessDetectionManager
import com.hypermob.mydrive3dx.data.hardware.FaceDetectionManager
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext

/**
 * Navigation Graph
 *
 * 앱의 전체 네비게이션 그래프 정의
 */
@Composable
fun NavGraph(
    navController: NavHostController,
    startDestination: String = Screen.Login.route,
    bodyScanManager: BodyScanManager,
    drowsinessDetectionManager: DrowsinessDetectionManager,
    faceDetectionManager: FaceDetectionManager,
    modifier: Modifier = Modifier
) {
    NavHost(
        navController = navController,
        startDestination = startDestination,
        modifier = modifier
    ) {
        // 로그인
        composable(route = Screen.Login.route) {
            LoginScreen(
                onLoginSuccess = {
                    // 로그인 성공 시 홈 화면으로 이동
                    navController.navigate(Screen.Home.route) {
                        // 로그인 화면을 스택에서 제거
                        popUpTo(Screen.Login.route) { inclusive = true }
                    }
                },
                onNavigateToRegister = {
                    navController.navigate(Screen.Register.route)
                }
            )
        }

        // 회원가입
        composable(route = Screen.Register.route) {
            RegisterScreen(
                onRegisterSuccess = {
                    // 시나리오1: 회원가입 성공 시 얼굴 등록 화면으로 이동
                    navController.navigate(Screen.RegisterFace.route)
                },
                onNavigateBack = {
                    navController.popBackStack()
                }
            )
        }

        // 홈
        composable(route = Screen.Home.route) {
            HomeScreen(
                onNavigateToRentalDetail = { rentalId ->
                    navController.navigate(Screen.RentalDetail.createRoute(rentalId))
                },
                onNavigateToControl = {
                    navController.navigate(Screen.Control.route)
                }
            )
        }

        // 차량 대여 목록 (시나리오2: 대여 가능 차량)
        // 하단 네비게이션에서 접근하므로 뒤로가기 버튼 없음
        composable(route = Screen.Rental.route) {
            VehicleListScreen(
                onNavigateToDetail = { vehicleId ->
                    navController.navigate(Screen.VehicleDetail.createRoute(vehicleId))
                }
                // onNavigateBack 없음 - 하단 네비게이션에서 접근
            )
        }

        // 차량 목록 (시나리오2: 대여 가능 차량)
        composable(route = Screen.VehicleList.route) {
            VehicleListScreen(
                onNavigateToDetail = { vehicleId ->
                    navController.navigate(Screen.VehicleDetail.createRoute(vehicleId))
                },
                onNavigateBack = {
                    navController.popBackStack()
                }
            )
        }

        // 차량 상세 (시나리오2)
        composable(
            route = Screen.VehicleDetail.route,
            arguments = listOf(
                navArgument("vehicleId") { type = NavType.StringType }
            )
        ) { backStackEntry ->
            val vehicleId = backStackEntry.arguments?.getString("vehicleId") ?: ""
            VehicleDetailScreen(
                vehicleId = vehicleId,
                onNavigateBack = {
                    navController.popBackStack()
                },
                onNavigateToApproval = { rentalId ->
                    // 대여 성공 시 승인 화면으로
                    navController.navigate(Screen.RentalApproval.createRoute(rentalId))
                }
            )
        }

        // 차량 제어
        composable(route = Screen.Control.route) {
            ControlScreen()
        }

        // 설정
        composable(route = Screen.Settings.route) {
            SettingsScreen(
                onNavigateToProfileEdit = {
                    navController.navigate(Screen.ProfileEdit.route)
                },
                onNavigateToSecurity = {
                    navController.navigate(Screen.SecuritySettings.route)
                },
                onLogout = {
                    navController.navigate(Screen.Login.route) {
                        popUpTo(Screen.Login.route) { inclusive = true }
                    }
                }
            )
        }

        // 대여 상세 (파라미터 포함)
        composable(
            route = Screen.RentalDetail.route,
            arguments = listOf(
                navArgument("rentalId") { type = NavType.StringType }
            )
        ) {
            RentalDetailScreen(
                onNavigateBack = {
                    navController.popBackStack()
                },
                onNavigateToMfa = { rentalId ->
                    navController.navigate(Screen.MfaAuth.createRoute(rentalId))
                }
            )
        }

        // 대여 승인 화면 (시나리오2)
        composable(
            route = Screen.RentalApproval.route,
            arguments = listOf(
                navArgument("rentalId") { type = NavType.StringType }
            )
        ) { backStackEntry ->
            val rentalId = backStackEntry.arguments?.getString("rentalId") ?: ""

            // 더미 데이터로 rental 생성 (실제로는 ViewModel에서 가져와야 함)
            val rental = com.hypermob.mydrive3dx.domain.model.Rental(
                rentalId = rentalId,
                vehicleId = "vehicle1",
                userId = "user123",
                vehicle = com.hypermob.mydrive3dx.domain.model.Vehicle(
                    vehicleId = "vehicle1",
                    model = "아이오닉 5",
                    manufacturer = "현대",
                    year = 2024,
                    color = "화이트",
                    licensePlate = "서울 12가 3456",
                    vehicleType = com.hypermob.mydrive3dx.domain.model.VehicleType.ELECTRIC,
                    fuelType = com.hypermob.mydrive3dx.domain.model.FuelType.ELECTRIC,
                    transmission = com.hypermob.mydrive3dx.domain.model.TransmissionType.AUTOMATIC,
                    seatingCapacity = 5,
                    pricePerHour = 10000.0,
                    pricePerDay = 50000.0,
                    imageUrl = null,
                    location = "강남역 주차장",
                    latitude = 37.497942,
                    longitude = 127.027621,
                    status = com.hypermob.mydrive3dx.domain.model.VehicleStatus.AVAILABLE,
                    features = listOf("자율주행", "무선충전", "빌트인캠")
                ),
                startTime = "2024-03-15 10:00",
                endTime = "2024-03-15 18:00",
                status = com.hypermob.mydrive3dx.domain.model.RentalStatus.APPROVED,
                totalPrice = 50000.0,
                pickupLocation = "강남역 주차장",
                returnLocation = "강남역 주차장",
                createdAt = "2024-03-15 09:00:00",
                updatedAt = "2024-03-15 09:00:00",
                mfaStatus = null,
                pickupLatitude = 37.497942,
                pickupLongitude = 127.027621
            )

            RentalApprovalScreen(
                rental = rental,
                onNavigateToHome = {
                    navController.navigate(Screen.Home.route) {
                        popUpTo(Screen.Home.route) { inclusive = true }
                    }
                }
            )
        }

        // MFA 인증 (파라미터 포함)
        // 차량 인수 시 BLE + NFC 인증 (얼굴 인식은 차량 내부에서 수행)
        composable(
            route = Screen.MfaAuth.route,
            arguments = listOf(
                navArgument("rentalId") { type = NavType.StringType }
            )
        ) { backStackEntry ->
            val rentalId = backStackEntry.arguments?.getString("rentalId") ?: ""
            com.hypermob.mydrive3dx.presentation.mfa.SimpleMfaScreen(
                carId = rentalId,  // rentalId를 carId로 사용
                onAuthSuccess = {
                    // MFA 인증 완료 후 제어 화면으로 이동
                    navController.navigate(Screen.Control.route) {
                        popUpTo(Screen.Home.route) { inclusive = false }
                    }
                },
                modifier = Modifier.fillMaxSize()
            )
        }

        // 프로필 편집
        composable(route = Screen.ProfileEdit.route) {
            ProfileEditScreen(
                onNavigateBack = {
                    navController.popBackStack()
                }
            )
        }

        // 보안 설정
        composable(route = Screen.SecuritySettings.route) {
            com.hypermob.mydrive3dx.presentation.settings.SecuritySettingsScreen(
                onNavigateBack = {
                    navController.popBackStack()
                },
                onNavigateToRegisterFace = {
                    navController.navigate(Screen.RegisterFace.route)
                },
                onNavigateToRegisterNfc = {
                    navController.navigate(Screen.RegisterNfc.route)
                },
                onNavigateToRegisterBle = {
                    navController.navigate(Screen.RegisterBle.route)
                }
            )
        }

        // 얼굴 등록
        composable(route = Screen.RegisterFace.route) {
            com.hypermob.mydrive3dx.presentation.face.FaceRegisterScreen(
                onRegistrationSuccess = { userId, faceId, nfcUid ->
                    // 시나리오1: 얼굴 등록 성공 시 전신 촬영 화면으로 이동
                    navController.navigate(Screen.BodyScan.route)
                },
                onNavigateBack = {
                    navController.popBackStack()
                }
            )
        }

        // NFC 등록
        composable(route = Screen.RegisterNfc.route) {
            val context = LocalContext.current
            val nfcManager = remember {
                com.hypermob.mydrive3dx.data.hardware.NfcManager(context)
            }
            com.hypermob.mydrive3dx.presentation.settings.RegisterNfcScreen(
                nfcManager = nfcManager,
                onNavigateBack = {
                    navController.popBackStack()
                },
                onNfcRegistered = { tagId ->
                    // TODO: 서버에 NFC 태그 등록
                }
            )
        }

        // BLE 등록
        composable(route = Screen.RegisterBle.route) {
            val context = LocalContext.current
            val bleManager = remember {
                com.hypermob.mydrive3dx.data.hardware.BleManager(context)
            }
            com.hypermob.mydrive3dx.presentation.settings.RegisterBleScreen(
                bleManager = bleManager,
                onNavigateBack = {
                    navController.popBackStack()
                },
                onBleRegistered = { deviceAddress ->
                    // TODO: 서버에 BLE 장치 등록
                }
            )
        }

        // 3D 바디스캔
        composable(route = Screen.BodyScan.route) {
            BodyScanScreen(
                bodyScanManager = bodyScanManager,
                onNavigateBack = {
                    navController.popBackStack()
                },
                onScanComplete = {
                    // 스캔 완료 후 홈으로 이동
                    navController.navigate(Screen.Home.route) {
                        popUpTo(Screen.BodyScan.route) { inclusive = true }
                    }
                }
            )
        }

        // 졸음 감지 모니터
        composable(route = Screen.DrowsinessMonitor.route) {
            DrowsinessMonitorScreen(
                drowsinessDetectionManager = drowsinessDetectionManager,
                onNavigateBack = {
                    navController.popBackStack()
                }
            )
        }
    }
}

/**
 * Placeholder Screen (임시 화면)
 */
@Composable
private fun PlaceholderScreen(title: String) {
    Box(
        modifier = Modifier.fillMaxSize(),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = title,
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary
        )
    }
}
