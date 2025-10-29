###############################################################################
# HYPERMOB Platform AI - API Gateway Deployment Only
# API Gateway만 배포하는 스크립트
###############################################################################

param(
    [string]$AwsRegion = "ap-northeast-2",
    [string]$Stage = "prod"
)

$ErrorActionPreference = "Stop"

Write-Host "`n=========================================" -ForegroundColor Cyan
Write-Host " API Gateway Deployment" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# AWS 계정 ID
$AccountId = aws sts get-caller-identity --query Account --output text
Write-Host "[INFO] AWS Account: $AccountId" -ForegroundColor Blue
Write-Host "[INFO] Region: $AwsRegion" -ForegroundColor Blue
Write-Host ""

# API Gateway 확인
Write-Host "[1/4] Checking for existing API..." -ForegroundColor Yellow
$ApiId = aws apigateway get-rest-apis `
    --query "items[?name=='HYPERMOB Platform AI API'].id" `
    --output text `
    --region $AwsRegion

if ($ApiId) {
    Write-Host "[INFO] API Gateway already exists: $ApiId" -ForegroundColor Blue
    Write-Host "[WARNING] Skipping API creation" -ForegroundColor Yellow
} else {
    Write-Host "[INFO] Creating API Gateway from openapi.yml..." -ForegroundColor Blue

    if (Test-Path "openapi.yml") {
        # fileb:// 사용으로 인코딩 문제 해결 (바이너리 모드)
        $ApiId = aws apigateway import-rest-api `
            --body "fileb://openapi.yml" `
            --region $AwsRegion `
            --fail-on-warnings `
            --query 'id' `
            --output text

        if ($ApiId) {
            Write-Host "[SUCCESS] API Gateway created: $ApiId" -ForegroundColor Green
        } else {
            Write-Host "[ERROR] Failed to create API Gateway" -ForegroundColor Red
            exit 1
        }
    } else {
        Write-Host "[ERROR] openapi.yml not found" -ForegroundColor Red
        exit 1
    }
}

Write-Host ""

# Lambda 함수 존재 확인
Write-Host "[2/4] Checking Lambda functions..." -ForegroundColor Yellow

$measureExists = $false
$recommendExists = $false

# measure 함수 확인
$result = aws lambda get-function --function-name hypermob-measure --region $AwsRegion 2>$null
if ($LASTEXITCODE -eq 0) {
    $measureExists = $true
    Write-Host "[INFO] hypermob-measure function found" -ForegroundColor Blue
} else {
    Write-Host "[WARNING] hypermob-measure function not found" -ForegroundColor Yellow
}

# recommend 함수 확인
$result = aws lambda get-function --function-name hypermob-recommend --region $AwsRegion 2>$null
if ($LASTEXITCODE -eq 0) {
    $recommendExists = $true
    Write-Host "[INFO] hypermob-recommend function found" -ForegroundColor Blue
} else {
    Write-Host "[WARNING] hypermob-recommend function not found" -ForegroundColor Yellow
}

if (-not $measureExists -and -not $recommendExists) {
    Write-Host "[ERROR] No Lambda functions found. Deploy Lambda functions first." -ForegroundColor Red
    exit 1
}

Write-Host "[SUCCESS] Lambda functions found" -ForegroundColor Green
Write-Host ""

# Lambda 권한 추가
Write-Host "[3/4] Adding API Gateway permissions to Lambda..." -ForegroundColor Yellow

# measure 함수 권한
if ($measureExists) {
    Write-Host "[INFO] Adding permission for hypermob-measure..." -ForegroundColor Blue

    aws lambda remove-permission `
        --function-name hypermob-measure `
        --statement-id apigateway-measure `
        --region $AwsRegion 2>$null | Out-Null

    aws lambda add-permission `
        --function-name hypermob-measure `
        --statement-id apigateway-measure `
        --action lambda:InvokeFunction `
        --principal apigateway.amazonaws.com `
        --source-arn "arn:aws:execute-api:${AwsRegion}:${AccountId}:${ApiId}/*/*" `
        --region $AwsRegion | Out-Null

    Write-Host "[SUCCESS] Permission added for hypermob-measure" -ForegroundColor Green
}

# recommend 함수 권한
if ($recommendExists) {
    Write-Host "[INFO] Adding permission for hypermob-recommend..." -ForegroundColor Blue

    aws lambda remove-permission `
        --function-name hypermob-recommend `
        --statement-id apigateway-recommend `
        --region $AwsRegion 2>$null | Out-Null

    aws lambda add-permission `
        --function-name hypermob-recommend `
        --statement-id apigateway-recommend `
        --action lambda:InvokeFunction `
        --principal apigateway.amazonaws.com `
        --source-arn "arn:aws:execute-api:${AwsRegion}:${AccountId}:${ApiId}/*/*" `
        --region $AwsRegion | Out-Null

    Write-Host "[SUCCESS] Permission added for hypermob-recommend" -ForegroundColor Green
}

Write-Host ""

# API 배포
Write-Host "[4/4] Deploying API to stage: $Stage..." -ForegroundColor Yellow

$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

aws apigateway create-deployment `
    --rest-api-id $ApiId `
    --stage-name $Stage `
    --description "Deployment $timestamp" `
    --region $AwsRegion | Out-Null

Write-Host "[SUCCESS] API deployed" -ForegroundColor Green
Write-Host ""

# 완료
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " API Gateway Deployment Complete!" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "API Endpoint:" -ForegroundColor Green
Write-Host "  https://$ApiId.execute-api.$AwsRegion.amazonaws.com/$Stage" -ForegroundColor White
Write-Host ""
Write-Host "Available Endpoints:" -ForegroundColor Yellow
Write-Host "  GET  /health" -ForegroundColor White
if ($measureExists) {
    Write-Host "  POST /measure" -ForegroundColor White
}
if ($recommendExists) {
    Write-Host "  POST /recommend" -ForegroundColor White
}
Write-Host ""
Write-Host "Test the API:" -ForegroundColor Yellow
Write-Host "  Invoke-RestMethod -Uri 'https://$ApiId.execute-api.$AwsRegion.amazonaws.com/$Stage/health'" -ForegroundColor Cyan
Write-Host ""
