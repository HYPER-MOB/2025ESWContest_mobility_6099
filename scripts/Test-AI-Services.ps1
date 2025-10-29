# Test AI Services with Image Upload

$ApiUrl = "https://dfhab2ellk.execute-api.ap-northeast-2.amazonaws.com/v1"

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "AI Services Test - Complete Flow" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# 1. Upload face image and create new user
Write-Host "1. Uploading face image..." -ForegroundColor Yellow
$faceResult = & cmd /c "curl -s -X POST `"$ApiUrl/auth/face`" -F `"image=@P:\HYPERMOB\Platform-Application\assets\face.jpg`""
$faceResponse = $faceResult | ConvertFrom-Json

if ($faceResponse.face_image_url) {
    Write-Host "   ✓ Face uploaded successfully!" -ForegroundColor Green
    Write-Host "     - User ID: $($faceResponse.user_id)" -ForegroundColor Gray
    Write-Host "     - Face URL: $($faceResponse.face_image_url)" -ForegroundColor Gray
    $userId = $faceResponse.user_id
    $faceImageUrl = $faceResponse.face_image_url
} else {
    Write-Host "   ✗ Face upload failed" -ForegroundColor Red
    exit 1
}

# 2. Upload body image
Write-Host "`n2. Uploading body image..." -ForegroundColor Yellow
$bodyResult = & cmd /c "curl -s -X POST `"$ApiUrl/body/upload`" -F `"image=@P:\HYPERMOB\Platform-Application\assets\body.jpg`" -F `"user_id=$userId`""
$bodyResponse = $bodyResult | ConvertFrom-Json

if ($bodyResponse.body_image_url) {
    Write-Host "   ✓ Body image uploaded successfully!" -ForegroundColor Green
    Write-Host "     - Body URL: $($bodyResponse.body_image_url)" -ForegroundColor Gray
    $bodyImageUrl = $bodyResponse.body_image_url
} else {
    Write-Host "   ✗ Body upload failed" -ForegroundColor Red
    exit 1
}

# 3. Test /facedata endpoint
Write-Host "`n3. Testing face landmark extraction..." -ForegroundColor Yellow
$facedataBody = @{
    image_url = $faceImageUrl
    user_id = $userId
} | ConvertTo-Json

# Write JSON to temp file to avoid escaping issues
$tempFile = "$env:TEMP\facedata.json"
[System.IO.File]::WriteAllText($tempFile, $facedataBody)
$facedataResult = & cmd /c "curl -s -X POST `"$ApiUrl/facedata`" -H `"Content-Type: application/json`" -d @`"$tempFile`""
Remove-Item $tempFile -ErrorAction SilentlyContinue

try {
    $facedataResponse = $facedataResult | ConvertFrom-Json
    if ($facedataResponse.status -eq "success") {
        Write-Host "   ✓ Face landmarks extracted!" -ForegroundColor Green
        # Count the landmarks
        $landmarkCount = ($facedataResponse.data.landmarks | Get-Member -MemberType NoteProperty).Count
        Write-Host "     - Landmarks count: $landmarkCount" -ForegroundColor Gray
    } else {
        Write-Host "   ✗ Face landmark extraction failed" -ForegroundColor Red
        Write-Host "     - Error: $($facedataResponse.error)" -ForegroundColor Red
    }
} catch {
    Write-Host "   ✗ Face landmark extraction failed" -ForegroundColor Red
    Write-Host "     - Error parsing response: $_" -ForegroundColor Red
}

# 4. Test /measure endpoint
Write-Host "`n4. Testing body measurement..." -ForegroundColor Yellow
$measureBody = @{
    image_url = $bodyImageUrl
    height_cm = 177.7
    user_id = $userId
} | ConvertTo-Json

# Write JSON to temp file to avoid escaping issues
$tempFile = "$env:TEMP\measure.json"
[System.IO.File]::WriteAllText($tempFile, $measureBody)
$measureResult = & cmd /c "curl -s -X POST `"$ApiUrl/measure`" -H `"Content-Type: application/json`" -d @`"$tempFile`""
Remove-Item $tempFile -ErrorAction SilentlyContinue

try {
    $measureResponse = $measureResult | ConvertFrom-Json
    if ($measureResponse.status -eq "success") {
        Write-Host "   ✓ Body measured successfully!" -ForegroundColor Green
        Write-Host "     - Upper arm: $($measureResponse.data.upper_arm) cm" -ForegroundColor Gray
        Write-Host "     - Forearm: $($measureResponse.data.forearm) cm" -ForegroundColor Gray
        Write-Host "     - Thigh: $($measureResponse.data.thigh) cm" -ForegroundColor Gray
        Write-Host "     - Calf: $($measureResponse.data.calf) cm" -ForegroundColor Gray
        Write-Host "     - Torso: $($measureResponse.data.torso) cm" -ForegroundColor Gray
        $measurements = $measureResponse.data
    } else {
        Write-Host "   ✗ Body measurement failed" -ForegroundColor Red
        Write-Host "     - Error: $($measureResponse.error)" -ForegroundColor Red
    }
} catch {
    Write-Host "   ✗ Body measurement failed" -ForegroundColor Red
    Write-Host "     - Error parsing response: $_" -ForegroundColor Red
}

# 5. Test /recommend endpoint
if ($measurements) {
    Write-Host "`n5. Testing vehicle settings recommendation..." -ForegroundColor Yellow
    $recommendBody = @{
        user_id = $userId
        height = 177.7
        upper_arm = $measurements.upper_arm
        forearm = $measurements.forearm
        thigh = $measurements.thigh
        calf = $measurements.calf
        torso = $measurements.torso
    } | ConvertTo-Json

    # Write JSON to temp file to avoid escaping issues
    $tempFile = "$env:TEMP\recommend.json"
    [System.IO.File]::WriteAllText($tempFile, $recommendBody)
    $recommendResult = & cmd /c "curl -s -X POST `"$ApiUrl/recommend`" -H `"Content-Type: application/json`" -d @`"$tempFile`""
    Remove-Item $tempFile -ErrorAction SilentlyContinue

    try {
        $recommendResponse = $recommendResult | ConvertFrom-Json
        if ($recommendResponse.status -eq "success") {
            Write-Host "   ✓ Vehicle settings recommended!" -ForegroundColor Green
            Write-Host "     - Seat position: $($recommendResponse.data.seat_position)mm" -ForegroundColor Gray
            Write-Host "     - Seat angle: $($recommendResponse.data.seat_angle)°" -ForegroundColor Gray
            Write-Host "     - Wheel position: $($recommendResponse.data.wheel_position)mm" -ForegroundColor Gray
            Write-Host "     - Wheel angle: $($recommendResponse.data.wheel_angle)°" -ForegroundColor Gray
            Write-Host "     - Mirror settings configured" -ForegroundColor Gray
        } else {
            Write-Host "   ✗ Vehicle settings recommendation failed" -ForegroundColor Red
            Write-Host "     - Error: $($recommendResponse.error)" -ForegroundColor Red
        }
    } catch {
        Write-Host "   ✗ Vehicle settings recommendation failed" -ForegroundColor Red
        Write-Host "     - Error parsing response: $_" -ForegroundColor Red
    }
}

# Summary
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Test Complete!" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Created User ID: $userId" -ForegroundColor Green