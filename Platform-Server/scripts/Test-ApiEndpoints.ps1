# ============================================================
# API Endpoints Test Script
# ============================================================
# 목적: 배포된 모든 API 엔드포인트를 검증
# 사용법: .\Test-ApiEndpoints.ps1 -ApiUrl "https://xxx.execute-api.us-east-1.amazonaws.com/v1"
# ============================================================

param(
    [Parameter(Mandatory=$false)]
    [string]$ApiUrl,

    [Parameter(Mandatory=$false)]
    [switch]$DetailedOutput
)

$ErrorActionPreference = "Continue"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "API Endpoints Test Suite" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# API URL 확인
if (-not $ApiUrl) {
    Write-Host "API URL not provided. Attempting to detect from AWS..." -ForegroundColor Yellow

    # API Gateway 목록에서 hypermob API 찾기
    $apis = aws apigateway get-rest-apis --query "items[?name=='hypermob-unified-api'].[id,name]" --output json 2>$null | ConvertFrom-Json

    if ($apis -and $apis.Count -gt 0) {
        $apiId = $apis[0][0]
        $region = aws configure get region
        $ApiUrl = "https://${apiId}.execute-api.${region}.amazonaws.com/v1"
        Write-Host "  Detected API URL: $ApiUrl`n" -ForegroundColor Green
    } else {
        Write-Host "ERROR: Could not detect API URL. Please provide it manually:" -ForegroundColor Red
        Write-Host "  .\Test-ApiEndpoints.ps1 -ApiUrl 'https://xxx.execute-api.us-east-1.amazonaws.com/v1'" -ForegroundColor Yellow
        exit 1
    }
}

Write-Host "Testing API: $ApiUrl`n" -ForegroundColor Cyan

# 테스트 결과 추적
$testResults = @{
    Passed = 0
    Failed = 0
    Skipped = 0
    Total = 0
}

# 테스트 데이터 저장
$testData = @{
    UserId = $null
    CarId = $null
    BookingId = $null
    SessionId = $null
}

# ============================================================
# 헬퍼 함수
# ============================================================

function Test-Endpoint {
    param(
        [string]$Name,
        [string]$Method,
        [string]$Path,
        [hashtable]$Body = $null,
        [int]$ExpectedStatus = 200,
        [scriptblock]$Validator = $null
    )

    $testResults.Total++
    $fullUrl = "$ApiUrl$Path"

    Write-Host "[$($testResults.Total)] Testing: $Name" -ForegroundColor Cyan
    Write-Host "    Method: $Method $Path" -ForegroundColor Gray

    try {
        $headers = @{
            "Content-Type" = "application/json"
        }

        if ($Body) {
            $bodyJson = $Body | ConvertTo-Json -Depth 10
            if ($DetailedOutput) {
                Write-Host "    Request Body: $bodyJson" -ForegroundColor DarkGray
            }

            $response = Invoke-WebRequest -Uri $fullUrl -Method $Method -Headers $headers -Body $bodyJson -UseBasicParsing 2>&1
        } else {
            $response = Invoke-WebRequest -Uri $fullUrl -Method $Method -Headers $headers -UseBasicParsing 2>&1
        }

        $statusCode = $response.StatusCode
        $content = $response.Content

        if ($DetailedOutput) {
            Write-Host "    Response Status: $statusCode" -ForegroundColor DarkGray
            Write-Host "    Response Body: $content" -ForegroundColor DarkGray
        }

        if ($statusCode -eq $ExpectedStatus) {
            # JSON 파싱 시도
            try {
                $jsonResponse = $content | ConvertFrom-Json

                # Validator 실행
                if ($Validator) {
                    $validationResult = & $Validator $jsonResponse
                    if ($validationResult) {
                        Write-Host "    ✓ PASSED (Status: $statusCode)" -ForegroundColor Green
                        $testResults.Passed++
                        return $jsonResponse
                    } else {
                        Write-Host "    ✗ FAILED (Validation failed)" -ForegroundColor Red
                        $testResults.Failed++
                        return $null
                    }
                } else {
                    Write-Host "    ✓ PASSED (Status: $statusCode)" -ForegroundColor Green
                    $testResults.Passed++
                    return $jsonResponse
                }
            } catch {
                Write-Host "    ✓ PASSED (Status: $statusCode, Non-JSON response)" -ForegroundColor Green
                $testResults.Passed++
                return $content
            }
        } else {
            Write-Host "    ✗ FAILED (Expected: $ExpectedStatus, Got: $statusCode)" -ForegroundColor Red
            $testResults.Failed++
            return $null
        }
    } catch {
        $errorMessage = $_.Exception.Message
        Write-Host "    ✗ FAILED (Error: $errorMessage)" -ForegroundColor Red
        $testResults.Failed++
        return $null
    }
}

function Skip-Test {
    param([string]$Name, [string]$Reason)

    $testResults.Total++
    $testResults.Skipped++

    Write-Host "[$($testResults.Total)] Skipping: $Name" -ForegroundColor Yellow
    Write-Host "    Reason: $Reason" -ForegroundColor Gray
}

# ============================================================
# 1. Health Check
# ============================================================

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "System Tests" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

Test-Endpoint `
    -Name "Health Check" `
    -Method "GET" `
    -Path "/health" `
    -Validator {
        param($response)
        return $response.status -eq "ok"
    }

# ============================================================
# 2. User Management
# ============================================================

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "User Management Tests" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

$userResponse = Test-Endpoint `
    -Name "Create User" `
    -Method "POST" `
    -Path "/users" `
    -Body @{
        name = "Test User"
        email = "test-$(Get-Random)@example.com"
        phone = "010-1234-5678"
        address = "Seoul Gangnam-gu"
    } `
    -ExpectedStatus 201 `
    -Validator {
        param($response)
        return $response.user_id -and $response.status -eq "created"
    }

if ($userResponse) {
    $testData.UserId = $userResponse.user_id
    Write-Host "      → Created User ID: $($testData.UserId)" -ForegroundColor Green
}

# ============================================================
# 3. Vehicle Management
# ============================================================

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Vehicle Management Tests" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

$vehiclesResponse = Test-Endpoint `
    -Name "List Available Vehicles" `
    -Method "GET" `
    -Path "/vehicles?status=available" `
    -Validator {
        param($response)
        return $response.status -eq "success" -and $response.data.vehicles
    }

if ($vehiclesResponse -and $vehiclesResponse.data.vehicles.Count -gt 0) {
    $testData.CarId = $vehiclesResponse.data.vehicles[0].car_id
    Write-Host "      → Found Car ID: $($testData.CarId)" -ForegroundColor Green
}

# ============================================================
# 4. Booking Management
# ============================================================

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Booking Management Tests" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

if ($testData.UserId -and $testData.CarId) {
    $startTime = (Get-Date).AddHours(1).ToString("yyyy-MM-ddTHH:mm:ss")
    $endTime = (Get-Date).AddHours(5).ToString("yyyy-MM-ddTHH:mm:ss")

    $bookingResponse = Test-Endpoint `
        -Name "Create Booking" `
        -Method "POST" `
        -Path "/bookings" `
        -Body @{
            user_id = $testData.UserId
            car_id = $testData.CarId
            start_time = $startTime
            end_time = $endTime
        } `
        -ExpectedStatus 201 `
        -Validator {
            param($response)
            return $response.status -eq "success" -and $response.data.booking_id
        }

    if ($bookingResponse) {
        $testData.BookingId = $bookingResponse.data.booking_id
        Write-Host "      → Created Booking ID: $($testData.BookingId)" -ForegroundColor Green
    }

    Test-Endpoint `
        -Name "Get User Bookings" `
        -Method "GET" `
        -Path "/bookings?user_id=$($testData.UserId)" `
        -Validator {
            param($response)
            return $response.status -eq "success" -and $response.data.bookings
        }
} else {
    Skip-Test -Name "Create Booking" -Reason "User or Car not created"
    Skip-Test -Name "Get User Bookings" -Reason "User not created"
}

# ============================================================
# 5. Authentication (MFA)
# ============================================================

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Authentication Tests (MFA)" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# Face Registration (Multipart 테스트는 복잡하므로 스킵)
Skip-Test -Name "Face Registration (POST /auth/face)" -Reason "Requires multipart/form-data (manual test required)"

if ($testData.UserId) {
    # NFC UID Registration
    Test-Endpoint `
        -Name "NFC UID Registration" `
        -Method "POST" `
        -Path "/auth/nfc" `
        -Body @{
            user_id = $testData.UserId
            nfc_uid = "04$(Get-Random -Minimum 100000000000 -Maximum 999999999999)"
        } `
        -Validator {
            param($response)
            return $response.status -eq "ok"
        }

    # NFC UID Retrieval
    $nfcResponse = Test-Endpoint `
        -Name "NFC UID Retrieval" `
        -Method "GET" `
        -Path "/auth/nfc?user_id=$($testData.UserId)" `
        -Validator {
            param($response)
            return $response.nfc_uid -and $response.status -eq "ok"
        }

    if ($nfcResponse -and $nfcResponse.nfc_uid) {
        $testData.NfcUid = $nfcResponse.nfc_uid
    }

    # BLE Hash Key Request
    if ($testData.CarId) {
        Test-Endpoint `
            -Name "BLE Hash Key Request" `
            -Method "GET" `
            -Path "/auth/ble?user_id=$($testData.UserId)&car_id=$($testData.CarId)" `
            -Validator {
                param($response)
                return $response.hashkey -and $response.expires_at
            }
    } else {
        Skip-Test -Name "BLE Hash Key Request" -Reason "Car not available"
    }

    # Auth Session
    if ($testData.CarId) {
        $sessionResponse = Test-Endpoint `
            -Name "Create Auth Session" `
            -Method "GET" `
            -Path "/auth/session?user_id=$($testData.UserId)&car_id=$($testData.CarId)" `
            -Validator {
                param($response)
                return $response.session_id -and $response.status -eq "active"
            }

        if ($sessionResponse) {
            $testData.SessionId = $sessionResponse.session_id
            Write-Host "      → Created Session ID: $($testData.SessionId)" -ForegroundColor Green

            # Report Auth Result
            Test-Endpoint `
                -Name "Report Auth Result" `
                -Method "POST" `
                -Path "/auth/result" `
                -Body @{
                    session_id = $testData.SessionId
                    car_id = $testData.CarId
                    face_verified = $true
                    ble_verified = $true
                    nfc_verified = $true
                } `
                -Validator {
                    param($response)
                    return $response.status -eq "MFA_SUCCESS"
                }

            # NFC Verification
            if ($testData.NfcUid) {
                Test-Endpoint `
                    -Name "NFC Final Verification" `
                    -Method "POST" `
                    -Path "/auth/nfc/verify" `
                    -Body @{
                        user_id = $testData.UserId
                        nfc_uid = $testData.NfcUid
                        car_id = $testData.CarId
                    } `
                    -Validator {
                        param($response)
                        return $response.status -eq "completed"
                    }
            } else {
                Skip-Test -Name "NFC Final Verification" -Reason "NFC UID not retrieved"
            }
        }
    } else {
        Skip-Test -Name "Create Auth Session" -Reason "Car not available"
        Skip-Test -Name "Report Auth Result" -Reason "Session not created"
        Skip-Test -Name "NFC Final Verification" -Reason "Session not created"
    }
} else {
    Skip-Test -Name "NFC UID Registration" -Reason "User not created"
    Skip-Test -Name "NFC UID Retrieval" -Reason "User not created"
    Skip-Test -Name "BLE Hash Key Request" -Reason "User not created"
    Skip-Test -Name "Create Auth Session" -Reason "User not created"
}

# ============================================================
# 6. AI Services
# ============================================================

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "AI Services Tests" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# Upload images and get S3 URLs
$faceImageUrl = $null
$bodyImageUrl = $null

if ($testData.UserId) {
    # Upload face image using curl
    $facePath = "P:\HYPERMOB\Platform-Application\assets\face.jpg"
    if (Test-Path $facePath) {
        try {
            $curlCmd = "curl -s -X POST '$ApiUrl/auth/face' -F 'image=@$facePath'"
            $response = Invoke-Expression $curlCmd | ConvertFrom-Json

            if ($response.face_image_url) {
                $faceImageUrl = $response.face_image_url
                $newUserId = $response.user_id
                Write-Host "[Face Upload] ✓ Image uploaded to: $faceImageUrl" -ForegroundColor Green
                Write-Host "      → New user created: $newUserId" -ForegroundColor Green
                $testResults.Passed++
            } else {
                Write-Host "[Face Upload] ✗ Failed - Response: $($response | ConvertTo-Json -Compress)" -ForegroundColor Red
                $testResults.Failed++
            }
        } catch {
            Write-Host "[Face Upload] ✗ Failed to upload face image: $_" -ForegroundColor Red
            $testResults.Failed++
        }
        $testResults.Total++
    }

    # Upload body image using curl
    $bodyPath = "P:\HYPERMOB\Platform-Application\assets\body.jpg"
    if (Test-Path $bodyPath) {
        try {
            $curlCmd = "curl -s -X POST '$ApiUrl/body/upload' -F 'image=@$bodyPath' -F 'user_id=$($testData.UserId)'"
            $response = Invoke-Expression $curlCmd | ConvertFrom-Json

            if ($response.body_image_url) {
                $bodyImageUrl = $response.body_image_url
                Write-Host "[Body Upload] ✓ Image uploaded to: $bodyImageUrl" -ForegroundColor Green
                $testResults.Passed++
            } else {
                Write-Host "[Body Upload] ✗ Failed - Response: $($response | ConvertTo-Json -Compress)" -ForegroundColor Red
                $testResults.Failed++
            }
        } catch {
            Write-Host "[Body Upload] ✗ Failed to upload body image: $_" -ForegroundColor Red
            $testResults.Failed++
        }
        $testResults.Total++
    }
}

# Test AI endpoints with uploaded images
if ($faceImageUrl) {
    Test-Endpoint `
        -Name "Face Landmark Extraction (POST /facedata)" `
        -Method "POST" `
        -Path "/facedata" `
        -Body @{
            image_url = $faceImageUrl
            user_id = $testData.UserId
        } `
        -Validator {
            param($response)
            return $response.status -eq "success" -and $response.data.landmarks
        }
} else {
    Skip-Test -Name "Face Landmark Extraction (POST /facedata)" -Reason "Face image not uploaded"
}

if ($bodyImageUrl) {
    $measureResponse = Test-Endpoint `
        -Name "Body Measurement (POST /measure)" `
        -Method "POST" `
        -Path "/measure" `
        -Body @{
            image_url = $bodyImageUrl
            height_cm = 177.7
            user_id = $testData.UserId
        } `
        -Validator {
            param($response)
            return $response.status -eq "success" -and $response.data
        }

    if ($measureResponse -and $measureResponse.data) {
        $testData.Measurements = $measureResponse.data
        Write-Host "      → Measurements captured" -ForegroundColor Green
    }
} else {
    Skip-Test -Name "Body Measurement (POST /measure)" -Reason "Body image not uploaded"
}

if ($testData.Measurements) {
    Test-Endpoint `
        -Name "Vehicle Settings Recommendation (POST /recommend)" `
        -Method "POST" `
        -Path "/recommend" `
        -Body @{
            user_id = $testData.UserId
            height_cm = 177.7
            upper_arm_cm = $testData.Measurements.upper_arm_cm
            forearm_cm = $testData.Measurements.forearm_cm
            thigh_cm = $testData.Measurements.thigh_cm
            calf_cm = $testData.Measurements.calf_cm
            torso_cm = $testData.Measurements.torso_cm
        } `
        -Validator {
            param($response)
            return $response.status -eq "success" -and $response.data.recommended_settings
        }
} else {
    Skip-Test -Name "Vehicle Settings Recommendation (POST /recommend)" -Reason "Body measurements not available"
}

# ============================================================
# 7. User Profile
# ============================================================

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "User Profile Tests" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

if ($testData.UserId) {
    Test-Endpoint `
        -Name "Get User Profile" `
        -Method "GET" `
        -Path "/users/$($testData.UserId)/profile" `
        -ExpectedStatus 200
} else {
    Skip-Test -Name "Get User Profile" -Reason "User not created"
}

# ============================================================
# 8. Vehicle Control
# ============================================================

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Vehicle Control Tests" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

if ($testData.UserId -and $testData.CarId) {
    Test-Endpoint `
        -Name "Apply Vehicle Settings" `
        -Method "POST" `
        -Path "/vehicles/$($testData.CarId)/settings/apply" `
        -Body @{
            user_id = $testData.UserId
            settings = @{
                seat_position = 45
                seat_angle = 15
                seat_front_height = 40
                seat_rear_height = 42
                mirror_left_yaw = 150
                mirror_left_pitch = 10
                mirror_right_yaw = 210
                mirror_right_pitch = 12
                mirror_room_yaw = 180
                mirror_room_pitch = 8
                wheel_position = 90
                wheel_angle = 20
            }
        } `
        -Validator {
            param($response)
            return $response.status -eq "success"
        }
} else {
    Skip-Test -Name "Apply Vehicle Settings" -Reason "User or Car not created"
}

# ============================================================
# 테스트 결과 요약
# ============================================================

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Test Results Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$passRate = if ($testResults.Total -gt 0) {
    [math]::Round(($testResults.Passed / $testResults.Total) * 100, 1)
} else {
    0
}

Write-Host "Total Tests: $($testResults.Total)" -ForegroundColor White
Write-Host "  ✓ Passed: $($testResults.Passed)" -ForegroundColor Green
Write-Host "  ✗ Failed: $($testResults.Failed)" -ForegroundColor Red
Write-Host "  ⊘ Skipped: $($testResults.Skipped)" -ForegroundColor Yellow
Write-Host "Pass Rate: $passRate%" -ForegroundColor $(if ($passRate -ge 80) { "Green" } elseif ($passRate -ge 50) { "Yellow" } else { "Red" })

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Test Data Created" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

if ($testData.UserId) {
    Write-Host "User ID: $($testData.UserId)" -ForegroundColor Gray
}
if ($testData.CarId) {
    Write-Host "Car ID: $($testData.CarId)" -ForegroundColor Gray
}
if ($testData.BookingId) {
    Write-Host "Booking ID: $($testData.BookingId)" -ForegroundColor Gray
}
if ($testData.SessionId) {
    Write-Host "Session ID: $($testData.SessionId)" -ForegroundColor Gray
}

Write-Host "`n========================================" -ForegroundColor Cyan

if ($testResults.Failed -eq 0) {
    Write-Host "✓ All tests passed!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "✗ Some tests failed. Please review the errors above." -ForegroundColor Red
    exit 1
}
