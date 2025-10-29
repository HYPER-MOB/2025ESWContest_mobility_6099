###############################################################################
# Facedata API Gateway 업데이트 및 Lambda 권한 추가
###############################################################################

param(
    [string]$AwsRegion = "ap-northeast-2"
)

$ErrorActionPreference = "Continue"

Write-Host "`n=========================================" -ForegroundColor Cyan
Write-Host " Updating Facedata API Gateway" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# API ID 가져오기
$apiId = aws apigateway get-rest-apis `
    --query "items[?name=='HYPERMOB Platform AI API'].id" `
    --output text `
    --region $AwsRegion

if (-not $apiId) {
    Write-Host "[ERROR] API Gateway not found!" -ForegroundColor Red
    Write-Host "Run deploy_api_gateway.ps1 first" -ForegroundColor Yellow
    exit 1
}

Write-Host "[INFO] API ID: $apiId" -ForegroundColor Blue
Write-Host ""

# 1. API Gateway 업데이트 (openapi.yml 적용)
Write-Host "[1/3] Updating API Gateway with openapi.yml..." -ForegroundColor Yellow

try {
    aws apigateway put-rest-api `
        --rest-api-id $apiId `
        --mode overwrite `
        --body fileb://openapi.yml `
        --region $AwsRegion | Out-Null

    Write-Host "[SUCCESS] API Gateway updated" -ForegroundColor Green
}
catch {
    Write-Host "[ERROR] Failed to update API Gateway: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""

# 2. 배포
Write-Host "[2/3] Deploying to prod stage..." -ForegroundColor Yellow

try {
    $deployment = aws apigateway create-deployment `
        --rest-api-id $apiId `
        --stage-name prod `
        --region $AwsRegion | ConvertFrom-Json

    Write-Host "[SUCCESS] Deployed (deployment ID: $($deployment.id))" -ForegroundColor Green
}
catch {
    Write-Host "[ERROR] Failed to deploy: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""

# 3. Lambda 권한 추가
Write-Host "[3/3] Adding Lambda permissions..." -ForegroundColor Yellow

# Measure 권한
Write-Host "[INFO] Adding permission for hypermob-measure..." -ForegroundColor Blue
aws lambda add-permission `
    --function-name hypermob-measure `
    --statement-id apigateway-invoke-measure `
    --action lambda:InvokeFunction `
    --principal apigateway.amazonaws.com `
    --source-arn "arn:aws:execute-api:$AwsRegion:471464546082:$apiId/*/*/measure" `
    --region $AwsRegion 2>$null

if ($LASTEXITCODE -eq 0) {
    Write-Host "[SUCCESS] Measure permission added" -ForegroundColor Green
} else {
    Write-Host "[INFO] Measure permission already exists (OK)" -ForegroundColor Blue
}

# Recommend 권한
Write-Host "[INFO] Adding permission for hypermob-recommend..." -ForegroundColor Blue
aws lambda add-permission `
    --function-name hypermob-recommend `
    --statement-id apigateway-invoke-recommend `
    --action lambda:InvokeFunction `
    --principal apigateway.amazonaws.com `
    --source-arn "arn:aws:execute-api:$AwsRegion:471464546082:$apiId/*/*/recommend" `
    --region $AwsRegion 2>$null

if ($LASTEXITCODE -eq 0) {
    Write-Host "[SUCCESS] Recommend permission added" -ForegroundColor Green
} else {
    Write-Host "[INFO] Recommend permission already exists (OK)" -ForegroundColor Blue
}

# Facedata 권한 (NEW)
Write-Host "[INFO] Adding permission for hypermob-facedata..." -ForegroundColor Blue
aws lambda add-permission `
    --function-name hypermob-facedata `
    --statement-id apigateway-invoke-facedata `
    --action lambda:InvokeFunction `
    --principal apigateway.amazonaws.com `
    --source-arn "arn:aws:execute-api:$AwsRegion:471464546082:$apiId/*/*/facedata" `
    --region $AwsRegion 2>$null

if ($LASTEXITCODE -eq 0) {
    Write-Host "[SUCCESS] Facedata permission added" -ForegroundColor Green
} else {
    Write-Host "[INFO] Facedata permission already exists (OK)" -ForegroundColor Blue
}

Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Update Complete!" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "API Endpoints:" -ForegroundColor Yellow
Write-Host "  Health:    https://$apiId.execute-api.$AwsRegion.amazonaws.com/prod/health" -ForegroundColor White
Write-Host "  Measure:   https://$apiId.execute-api.$AwsRegion.amazonaws.com/prod/measure" -ForegroundColor White
Write-Host "  Recommend: https://$apiId.execute-api.$AwsRegion.amazonaws.com/prod/recommend" -ForegroundColor White
Write-Host "  Facedata:  https://$apiId.execute-api.$AwsRegion.amazonaws.com/prod/facedata" -ForegroundColor Cyan
Write-Host ""
Write-Host "Test the API:" -ForegroundColor Yellow
Write-Host "  .\test_all_endpoints.ps1" -ForegroundColor Cyan
Write-Host ""
