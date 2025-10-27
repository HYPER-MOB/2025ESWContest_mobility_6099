###############################################################################
# HYPERMOB Platform AI - Rebuild Measure Function Only
# measure 함수만 빠르게 재배포
###############################################################################

param(
    [string]$AwsRegion = "ap-northeast-2"
)

$ErrorActionPreference = "Stop"

Write-Host "`n=========================================" -ForegroundColor Cyan
Write-Host " Rebuilding Measure Function" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# AWS 계정 ID
$AccountId = aws sts get-caller-identity --query Account --output text
Write-Host "[INFO] AWS Account: $AccountId" -ForegroundColor Blue
Write-Host "[INFO] Region: $AwsRegion" -ForegroundColor Blue
Write-Host ""

# ECR URI
$MeasureEcrUri = "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com/hypermob-measure:latest"

# ECR 로그인
Write-Host "[1/4] Logging in to ECR..." -ForegroundColor Yellow
$ecrPassword = aws ecr get-login-password --region $AwsRegion
$ecrPassword | docker login --username AWS --password-stdin "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com"
Write-Host "[SUCCESS] ECR login successful" -ForegroundColor Green
Write-Host ""

# measure 함수 리빌드
Write-Host "[2/4] Rebuilding measure function..." -ForegroundColor Yellow
Write-Host "[INFO] This may take 5-7 minutes (downloading MediaPipe models)..." -ForegroundColor Blue

# 프로젝트 루트로 이동 후 lambdas/measure로 이동
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent (Split-Path -Parent $scriptPath)
Push-Location "$projectRoot/lambdas/measure"

Write-Host "[INFO] Starting Docker build..." -ForegroundColor Blue
docker build -t hypermob-measure . 2>&1 | ForEach-Object {
    if ($_ -match "Downloading MediaPipe models") {
        Write-Host $_ -ForegroundColor Cyan
    } elseif ($_ -match "downloaded successfully") {
        Write-Host $_ -ForegroundColor Green
    } elseif ($_ -match "Step \d+/\d+") {
        Write-Host $_ -ForegroundColor Yellow
    } else {
        Write-Host $_
    }
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Docker build failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "[SUCCESS] Docker build complete" -ForegroundColor Green

docker tag hypermob-measure:latest $MeasureEcrUri
docker push $MeasureEcrUri

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Docker push failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

Pop-Location
Write-Host "[SUCCESS] Image pushed to ECR" -ForegroundColor Green
Write-Host ""

# Lambda 함수 업데이트
Write-Host "[3/4] Updating Lambda function..." -ForegroundColor Yellow

Write-Host "[INFO] Updating measure function code..." -ForegroundColor Blue
aws lambda update-function-code `
    --function-name hypermob-measure `
    --image-uri $MeasureEcrUri `
    --region $AwsRegion | Out-Null

Write-Host "[INFO] Waiting for function update to complete..." -ForegroundColor Blue
aws lambda wait function-updated --function-name hypermob-measure --region $AwsRegion

Write-Host "[SUCCESS] Lambda function updated" -ForegroundColor Green
Write-Host ""

# 테스트
Write-Host "[4/4] Testing..." -ForegroundColor Yellow
Write-Host "[INFO] Waiting 5 seconds for Lambda to be ready..." -ForegroundColor Blue
Start-Sleep -Seconds 5

# API ID 확인
$apiId = aws apigateway get-rest-apis `
    --query "items[?name=='HYPERMOB Platform AI API'].id" `
    --output text `
    --region $AwsRegion

if ($apiId) {
    $baseUrl = "https://$apiId.execute-api.$AwsRegion.amazonaws.com/prod"

    Write-Host "[INFO] Testing measure endpoint..." -ForegroundColor Blue

    # S3 테스트 이미지 확인
    $s3Check = aws s3 ls s3://hypermob-images/test/full_body.jpg --region $AwsRegion 2>$null

    if ($s3Check) {
        $body = @{
            image_url = "s3://hypermob-images/test/full_body.jpg"
            height_cm = 175.0
            user_id = "rebuild_test"
        } | ConvertTo-Json

        try {
            Write-Host "[INFO] Sending request to $baseUrl/measure" -ForegroundColor Blue
            $response = Invoke-RestMethod -Uri "$baseUrl/measure" `
                -Method Post `
                -Body $body `
                -ContentType "application/json" `
                -TimeoutSec 60 `
                -ErrorAction Stop

            Write-Host "[SUCCESS] Measure API test passed!" -ForegroundColor Green
            Write-Host ($response | ConvertTo-Json -Depth 10)
        }
        catch {
            Write-Host "[WARNING] Test failed: $_" -ForegroundColor Yellow
            if ($_.ErrorDetails.Message) {
                Write-Host "Error details: $($_.ErrorDetails.Message)" -ForegroundColor Red
            }
            Write-Host "[INFO] Check CloudWatch Logs for details:" -ForegroundColor Yellow
            Write-Host "  aws logs tail /aws/lambda/hypermob-measure --follow --region $AwsRegion" -ForegroundColor Cyan
        }
    } else {
        Write-Host "[INFO] No test image found. Skipping API test." -ForegroundColor Yellow
        Write-Host "[INFO] Upload a test image:" -ForegroundColor Yellow
        Write-Host "  aws s3 cp your_image.jpg s3://hypermob-images/test/full_body.jpg" -ForegroundColor Cyan
    }
} else {
    Write-Host "[WARNING] API Gateway not found. Skipping test." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Rebuild Complete!" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Check CloudWatch Logs:" -ForegroundColor White
Write-Host "     aws logs tail /aws/lambda/hypermob-measure --follow --region $AwsRegion" -ForegroundColor Cyan
Write-Host "  2. Run full test:" -ForegroundColor White
Write-Host "     .\test_all_endpoints.ps1" -ForegroundColor Cyan
Write-Host ""
