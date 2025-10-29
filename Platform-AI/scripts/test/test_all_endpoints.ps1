###############################################################################
# HYPERMOB Platform AI - Complete API Test Script (PowerShell)
# 모든 엔드포인트를 자동으로 테스트합니다
###############################################################################

param(
    [string]$AwsRegion = "ap-northeast-2"
)

$ErrorActionPreference = "Continue"

# 색상 함수
function Write-TestHeader {
    param([string]$Message)
    Write-Host "`n=========================================" -ForegroundColor Cyan
    Write-Host " $Message" -ForegroundColor Cyan
    Write-Host "=========================================" -ForegroundColor Cyan
}

function Write-TestStep {
    param([string]$Message)
    Write-Host "`n[TEST] $Message" -ForegroundColor Yellow
}

function Write-Success {
    param([string]$Message)
    Write-Host "[PASS] $Message" -ForegroundColor Green
}

function Write-Failure {
    param([string]$Message)
    Write-Host "[FAIL] $Message" -ForegroundColor Red
}

function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Blue
}

# API ID 가져오기
Write-TestHeader "API Endpoint Discovery"

$apiId = aws apigateway get-rest-apis `
    --query "items[?name=='HYPERMOB Platform AI API'].id" `
    --output text `
    --region $AwsRegion

if (-not $apiId) {
    Write-Failure "API Gateway not found!"
    Write-Host "Run deploy_api_gateway.ps1 first"
    exit 1
}

$baseUrl = "https://$apiId.execute-api.$AwsRegion.amazonaws.com/prod"

Write-Success "API found!"
Write-Info "API ID: $apiId"
Write-Info "Base URL: $baseUrl"

# 테스트 결과 추적
$testResults = @{
    Total = 0
    Passed = 0
    Failed = 0
}

# 헬퍼 함수: 테스트 결과 기록
function Record-TestResult {
    param([bool]$Passed)
    $script:testResults.Total++
    if ($Passed) {
        $script:testResults.Passed++
    } else {
        $script:testResults.Failed++
    }
}

###############################################################################
# Test 1: Health Check
###############################################################################

Write-TestHeader "Test 1: Health Check"

Write-TestStep "Testing GET /health"

try {
    $startTime = Get-Date
    $response = Invoke-RestMethod -Uri "$baseUrl/health" -Method Get -ErrorAction Stop
    $duration = ((Get-Date) - $startTime).TotalMilliseconds

    Write-Info "Response received in $([math]::Round($duration, 0))ms"
    Write-Host ($response | ConvertTo-Json -Depth 10)

    if ($response.status -eq "healthy") {
        Write-Success "Health check passed"
        Record-TestResult -Passed $true
    } else {
        Write-Failure "Unexpected status: $($response.status)"
        Record-TestResult -Passed $false
    }

    if ($duration -lt 1000) {
        Write-Success "Response time < 1s"
    } else {
        Write-Failure "Response time too slow: $([math]::Round($duration/1000, 2))s"
    }
}
catch {
    Write-Failure "Health check failed: $_"
    Record-TestResult -Passed $false
}

###############################################################################
# Test 2: Recommend API - Valid Request
###############################################################################

Write-TestHeader "Test 2: Recommend API - Valid Request"

Write-TestStep "Testing POST /recommend with valid data"

$recommendBody = @{
    height = 175.0
    upper_arm = 31.0
    forearm = 26.0
    thigh = 51.0
    calf = 36.0
    torso = 61.0
    user_id = "test_user_powershell"
} | ConvertTo-Json

try {
    $startTime = Get-Date
    $response = Invoke-RestMethod -Uri "$baseUrl/recommend" `
        -Method Post `
        -Body $recommendBody `
        -ContentType "application/json" `
        -ErrorAction Stop
    $duration = ((Get-Date) - $startTime).TotalMilliseconds

    Write-Info "Response received in $([math]::Round($duration, 0))ms"
    Write-Host ($response | ConvertTo-Json -Depth 10)

    # 검증
    $passed = $true

    if ($response.status -eq "success") {
        Write-Success "Status is 'success'"
    } else {
        Write-Failure "Unexpected status: $($response.status)"
        $passed = $false
    }

    $requiredFields = @(
        "seat_position", "seat_angle", "seat_front_height", "seat_rear_height",
        "mirror_left_yaw", "mirror_left_pitch", "mirror_right_yaw", "mirror_right_pitch",
        "mirror_room_yaw", "mirror_room_pitch", "wheel_position", "wheel_angle"
    )

    foreach ($field in $requiredFields) {
        if ($null -eq $response.data.$field) {
            Write-Failure "Missing field: $field"
            $passed = $false
        }
    }

    if ($passed) {
        Write-Success "All required fields present"
        Write-Success "Recommend API test passed"
    }

    Record-TestResult -Passed $passed

    if ($duration -lt 5000) {
        Write-Success "Response time < 5s"
    } else {
        Write-Failure "Response time too slow: $([math]::Round($duration/1000, 2))s"
    }
}
catch {
    Write-Failure "Recommend API failed: $_"
    if ($_.Exception.Response) {
        # Error details in $_.ErrorDetails.Message
        Write-Host "Error: $($_.ErrorDetails.Message)"
        
    }
    Record-TestResult -Passed $false
}

###############################################################################
# Test 3: Recommend API - Missing Fields
###############################################################################

Write-TestHeader "Test 3: Recommend API - Error Handling (Missing Fields)"

Write-TestStep "Testing POST /recommend with missing fields"

$invalidBody = @{
    height = 175.0
} | ConvertTo-Json

try {
    $response = Invoke-RestMethod -Uri "$baseUrl/recommend" `
        -Method Post `
        -Body $invalidBody `
        -ContentType "application/json" `
        -ErrorAction Stop

    Write-Failure "Should have returned error for missing fields"
    Record-TestResult -Passed $false
}
catch {
    if ($_.Exception.Response.StatusCode -eq 400) {
        Write-Success "Correctly returned 400 Bad Request for missing fields"
        Record-TestResult -Passed $true

        # Error details in $_.ErrorDetails.Message
        Write-Host "Error: $($_.ErrorDetails.Message)"
        Write-Info "Error response: $errorBody"
    } else {
        Write-Failure "Unexpected status code: $($_.Exception.Response.StatusCode)"
        Record-TestResult -Passed $false
    }
}

###############################################################################
# Test 4: Recommend API - Invalid Values
###############################################################################

Write-TestHeader "Test 4: Recommend API - Error Handling (Invalid Values)"

Write-TestStep "Testing POST /recommend with invalid height"

$invalidBody = @{
    height = -10
    upper_arm = 31.0
    forearm = 26.0
    thigh = 51.0
    calf = 36.0
    torso = 61.0
} | ConvertTo-Json

try {
    $response = Invoke-RestMethod -Uri "$baseUrl/recommend" `
        -Method Post `
        -Body $invalidBody `
        -ContentType "application/json" `
        -ErrorAction Stop

    Write-Failure "Should have returned error for invalid height"
    Record-TestResult -Passed $false
}
catch {
    if ($_.Exception.Response.StatusCode -eq 400) {
        Write-Success "Correctly returned 400 Bad Request for invalid values"
        Record-TestResult -Passed $true

        # Error details in $_.ErrorDetails.Message
        Write-Host "Error: $($_.ErrorDetails.Message)"
        Write-Info "Error response: $errorBody"
    } else {
        Write-Failure "Unexpected status code: $($_.Exception.Response.StatusCode)"
        Record-TestResult -Passed $false
    }
}

###############################################################################
# Test 5: Measure API (Optional - requires S3 image)
###############################################################################

Write-TestHeader "Test 5: Measure API (Optional)"

Write-Info "Measure API test requires a test image uploaded to S3"
Write-Info "To test, upload an image first:"
Write-Host "  aws s3 cp your_image.jpg s3://hypermob-images/test/full_body.jpg" -ForegroundColor Cyan

# S3에 테스트 이미지가 있는지 확인
$s3Check = aws s3 ls s3://hypermob-images/test/full_body.jpg --region $AwsRegion 2>$null

# Measure 응답을 저장할 변수 (Test 6에서 사용)
$measureResult = $null

if ($s3Check) {
    Write-Success "Test image found in S3"
    Write-TestStep "Testing POST /measure"

    $measureBody = @{
        image_url = "s3://hypermob-images/test/full_body.jpg"
        height_cm = 177.7
        user_id = "test_user_powershell"
    } | ConvertTo-Json

    try {
        $startTime = Get-Date
        $response = Invoke-RestMethod -Uri "$baseUrl/measure" `
            -Method Post `
            -Body $measureBody `
            -ContentType "application/json" `
            -ErrorAction Stop
        $duration = ((Get-Date) - $startTime).TotalMilliseconds

        Write-Info "Response received in $([math]::Round($duration/1000, 2))s"
        Write-Host ($response | ConvertTo-Json -Depth 10)

        # 검증
        $passed = $true

        if ($response.status -eq "success") {
            Write-Success "Status is 'success'"
        } else {
            Write-Failure "Unexpected status: $($response.status)"
            $passed = $false
        }

        # 필수 필드 검증 (Recommend API 호환성)
        $requiredFields = @("upper_arm", "forearm", "thigh", "calf", "torso", "height")
        foreach ($field in $requiredFields) {
            if ($null -eq $response.data.$field) {
                Write-Failure "Missing field: $field (required for Recommend API)"
                $passed = $false
            } else {
                Write-Info "$field = $($response.data.$field)"
            }
        }

        if ($passed) {
            Write-Success "All required fields present for Recommend API"
            Write-Success "Measure API test passed"

            # Measure 결과 저장 (Test 6에서 사용)
            $measureResult = $response.data
        }

        Record-TestResult -Passed $passed

        if ($response.data.output_image_url) {
            Write-Success "Result image URL: $($response.data.output_image_url)"
        }

        if ($duration -lt 30000) {
            Write-Success "Response time < 30s"
        } else {
            Write-Failure "Response time too slow: $([math]::Round($duration/1000, 2))s"
        }
    }
    catch {
        Write-Failure "Measure API failed: $_"
        if ($_.Exception.Response) {
            Write-Host "Error: $($_.ErrorDetails.Message)"
        }
        Record-TestResult -Passed $false
    }
} else {
    Write-Info "Skipping Measure API test (no test image found)"
    Write-Info "Upload a test image to run this test:"
    Write-Host "  aws s3 cp your_image.jpg s3://hypermob-images/test/full_body.jpg" -ForegroundColor Cyan
}

###############################################################################
# Test 6: Measure → Recommend Integration (연계 테스트)
###############################################################################

Write-TestHeader "Test 6: Measure → Recommend Integration"

if ($measureResult) {
    Write-TestStep "Using Measure API results to call Recommend API"

    Write-Info "Measure results:"
    Write-Host "  height     = $($measureResult.height)" -ForegroundColor Cyan
    Write-Host "  upper_arm  = $($measureResult.upper_arm)" -ForegroundColor Cyan
    Write-Host "  forearm    = $($measureResult.forearm)" -ForegroundColor Cyan
    Write-Host "  thigh      = $($measureResult.thigh)" -ForegroundColor Cyan
    Write-Host "  calf       = $($measureResult.calf)" -ForegroundColor Cyan
    Write-Host "  torso      = $($measureResult.torso)" -ForegroundColor Cyan

    # Measure 결과를 Recommend API에 직접 전달
    $recommendFromMeasure = @{
        height = $measureResult.height
        upper_arm = $measureResult.upper_arm
        forearm = $measureResult.forearm
        thigh = $measureResult.thigh
        calf = $measureResult.calf
        torso = $measureResult.torso
        user_id = $measureResult.user_id
    } | ConvertTo-Json

    Write-Info "Calling Recommend API with Measure results..."

    try {
        $startTime = Get-Date
        $response = Invoke-RestMethod -Uri "$baseUrl/recommend" `
            -Method Post `
            -Body $recommendFromMeasure `
            -ContentType "application/json" `
            -ErrorAction Stop
        $duration = ((Get-Date) - $startTime).TotalMilliseconds

        Write-Info "Response received in $([math]::Round($duration/1000, 2))s"
        Write-Host ($response | ConvertTo-Json -Depth 10)

        # 검증
        $passed = $true

        if ($response.status -eq "success") {
            Write-Success "Status is 'success'"
        } else {
            Write-Failure "Unexpected status: $($response.status)"
            $passed = $false
        }

        # 추천 결과 검증
        $requiredFields = @(
            "seat_position", "seat_angle", "seat_front_height", "seat_rear_height",
            "mirror_left_yaw", "mirror_left_pitch", "mirror_right_yaw", "mirror_right_pitch",
            "mirror_room_yaw", "mirror_room_pitch", "wheel_position", "wheel_angle"
        )

        foreach ($field in $requiredFields) {
            if ($null -eq $response.data.$field) {
                Write-Failure "Missing field: $field"
                $passed = $false
            }
        }

        if ($passed) {
            Write-Success "All required fields present"
            Write-Success "Measure → Recommend integration test passed!"
            Write-Success "Full workflow completed successfully!"
        }

        Record-TestResult -Passed $passed

        if ($duration -lt 10000) {
            Write-Success "Response time < 10s"
        } else {
            Write-Failure "Response time too slow: $([math]::Round($duration/1000, 2))s"
        }
    }
    catch {
        Write-Failure "Recommend API failed with Measure data: $_"
        if ($_.Exception.Response) {
            Write-Host "Error: $($_.ErrorDetails.Message)"
        }
        Record-TestResult -Passed $false
    }
} else {
    Write-Info "Skipping Measure → Recommend integration test"
    Write-Info "Reason: Measure API test was skipped or failed"
    Write-Info "Upload a test image to run the full workflow:"
    Write-Host "  aws s3 cp your_image.jpg s3://hypermob-images/test/full_body.jpg" -ForegroundColor Cyan
}

###############################################################################
# Test 7: Facedata API (Optional - requires face image)
###############################################################################

Write-TestHeader "Test 7: Facedata API (Optional)"

Write-Info "Facedata API test requires a test face image uploaded to S3"
Write-Info "To test, upload a face image first:"
Write-Host "  aws s3 cp your_face.jpg s3://hypermob-images/test/face.jpg" -ForegroundColor Cyan

# S3에 테스트 얼굴 이미지가 있는지 확인
$s3FaceCheck = aws s3 ls s3://hypermob-images/test/face.jpg --region $AwsRegion 2>$null

if ($s3FaceCheck) {
    Write-Success "Test face image found in S3"
    Write-TestStep "Testing POST /facedata"

    # Test 7a: 전체 랜드마크 추출
    Write-Host "`n[TEST 7a] Full landmark extraction (all 468 landmarks)" -ForegroundColor Yellow

    $facedataBody = @{
        image_url = "s3://hypermob-images/test/face.jpg"
        user_id = "test_user_powershell"
        refine = $false
        indices = ""
    } | ConvertTo-Json

    try {
        $startTime = Get-Date
        $response = Invoke-RestMethod -Uri "$baseUrl/facedata" `
            -Method Post `
            -Body $facedataBody `
            -ContentType "application/json" `
            -ErrorAction Stop
        $duration = ((Get-Date) - $startTime).TotalMilliseconds

        Write-Info "Response received in $([math]::Round($duration/1000, 2))s"

        # 검증
        $passed = $true

        if ($response.status -eq "success") {
            Write-Success "Status is 'success'"
        } else {
            Write-Failure "Unexpected status: $($response.status)"
            $passed = $false
        }

        # 랜드마크 개수 확인 (468개 x 3좌표 = 1404개 예상)
        $landmarkCount = $response.data.landmark_count
        Write-Info "Extracted $landmarkCount landmarks"

        if ($landmarkCount -gt 0) {
            Write-Success "Landmarks extracted successfully"
        } else {
            Write-Failure "No landmarks extracted"
            $passed = $false
        }

        # 샘플 랜드마크 출력
        if ($response.data.landmarks) {
            $sampleKeys = $response.data.landmarks.PSObject.Properties.Name | Select-Object -First 5
            Write-Info "Sample landmarks:"
            foreach ($key in $sampleKeys) {
                Write-Host "  $key = $($response.data.landmarks.$key)" -ForegroundColor Cyan
            }
        }

        if ($response.data.output_file_url) {
            Write-Success "Output file URL: $($response.data.output_file_url)"
        }

        if ($passed) {
            Write-Success "Facedata API (full) test passed"
        }

        Record-TestResult -Passed $passed

        if ($duration -lt 30000) {
            Write-Success "Response time < 30s"
        } else {
            Write-Failure "Response time too slow: $([math]::Round($duration/1000, 2))s"
        }
    }
    catch {
        Write-Failure "Facedata API failed: $_"
        if ($_.Exception.Response) {
            Write-Host "Error: $($_.ErrorDetails.Message)"
        }
        Record-TestResult -Passed $false
    }

    # Test 7b: 선택적 랜드마크 추출
    Write-Host "`n[TEST 7b] Selective landmark extraction (specific indices)" -ForegroundColor Yellow

    $selectiveBody = @{
        image_url = "s3://hypermob-images/test/face.jpg"
        user_id = "test_user_powershell"
        refine = $false
        indices = "0010,0011,0012,0330,0331,0332"  # 6개만 추출
    } | ConvertTo-Json

    try {
        $startTime = Get-Date
        $response = Invoke-RestMethod -Uri "$baseUrl/facedata" `
            -Method Post `
            -Body $selectiveBody `
            -ContentType "application/json" `
            -ErrorAction Stop
        $duration = ((Get-Date) - $startTime).TotalMilliseconds

        Write-Info "Response received in $([math]::Round($duration/1000, 2))s"

        # 검증
        $passed = $true
        $landmarkCount = $response.data.landmark_count

        Write-Info "Extracted $landmarkCount landmarks (expected 6)"

        if ($landmarkCount -eq 6) {
            Write-Success "Correct number of landmarks extracted"
        } else {
            Write-Failure "Expected 6 landmarks, got $landmarkCount"
            $passed = $false
        }

        # 추출된 랜드마크 출력
        if ($response.data.landmarks) {
            Write-Info "Extracted landmarks:"
            foreach ($key in $response.data.landmarks.PSObject.Properties.Name) {
                Write-Host "  $key = $($response.data.landmarks.$key)" -ForegroundColor Cyan
            }
        }

        if ($passed) {
            Write-Success "Facedata API (selective) test passed"
        }

        Record-TestResult -Passed $passed

        if ($duration -lt 30000) {
            Write-Success "Response time < 30s"
        }
    }
    catch {
        Write-Failure "Facedata API (selective) failed: $_"
        if ($_.Exception.Response) {
            Write-Host "Error: $($_.ErrorDetails.Message)"
        }
        Record-TestResult -Passed $false
    }

} else {
    Write-Info "Skipping Facedata API test (no test face image found)"
    Write-Info "Upload a test face image to run this test:"
    Write-Host "  aws s3 cp your_face.jpg s3://hypermob-images/test/face.jpg" -ForegroundColor Cyan
}

###############################################################################
# Test Summary
###############################################################################

Write-TestHeader "Test Summary"

Write-Host ""
Write-Host "Total Tests: $($testResults.Total)" -ForegroundColor White
Write-Host "Passed:      $($testResults.Passed)" -ForegroundColor Green
Write-Host "Failed:      $($testResults.Failed)" -ForegroundColor Red
Write-Host ""

if ($testResults.Failed -eq 0) {
    Write-Host "All tests passed! " -ForegroundColor Green -NoNewline
    Write-Host "API is working correctly." -ForegroundColor White
    exit 0
} else {
    Write-Host "Some tests failed. " -ForegroundColor Red -NoNewline
    Write-Host "Please check the errors above." -ForegroundColor White
    exit 1
}
