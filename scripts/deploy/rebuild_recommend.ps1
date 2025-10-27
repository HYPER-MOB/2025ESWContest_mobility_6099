###############################################################################
# HYPERMOB Platform AI - Rebuild Recommend Function Only
# recommend 함수만 빠르게 재배포 (S3 모델 로딩 버전)
###############################################################################

param(
    [string]$AwsRegion = "ap-northeast-2"
)

$ErrorActionPreference = "Stop"

Write-Host "`n=========================================" -ForegroundColor Cyan
Write-Host " Rebuilding Recommend Function" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# AWS 계정 ID
$AccountId = aws sts get-caller-identity --query Account --output text
Write-Host "[INFO] AWS Account: $AccountId" -ForegroundColor Blue
Write-Host "[INFO] Region: $AwsRegion" -ForegroundColor Blue
Write-Host ""

# ECR URI
$RecommendEcrUri = "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com/hypermob-recommend:latest"

# ECR 로그인
Write-Host "[1/5] Logging in to ECR..." -ForegroundColor Yellow
$ecrPassword = aws ecr get-login-password --region $AwsRegion
$ecrPassword | docker login --username AWS --password-stdin "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com"
Write-Host "[SUCCESS] ECR login successful" -ForegroundColor Green
Write-Host ""

# recommend 함수 리빌드
Write-Host "[2/5] Rebuilding recommend function..." -ForegroundColor Yellow
Write-Host "[INFO] This should be quick (no model bundling)..." -ForegroundColor Blue

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent (Split-Path -Parent $scriptPath)
Push-Location "$projectRoot/lambdas/recommend"

Write-Host "[INFO] Starting Docker build..." -ForegroundColor Blue
docker build -t hypermob-recommend . 2>&1 | ForEach-Object {
    if ($_ -match "Step \d+/\d+") {
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

docker tag hypermob-recommend:latest $RecommendEcrUri
docker push $RecommendEcrUri

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Docker push failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

Pop-Location
Write-Host "[SUCCESS] Image pushed to ECR" -ForegroundColor Green
Write-Host ""

# Lambda IAM 역할에 S3 읽기 권한 확인 및 추가
Write-Host "[3/5] Checking Lambda IAM role permissions..." -ForegroundColor Yellow

$roleName = "hypermob-recommend-role"
$roleExists = $false
$result = aws iam get-role --role-name $roleName 2>$null
if ($LASTEXITCODE -eq 0) {
    $roleExists = $true
}

if ($roleExists) {
    Write-Host "[INFO] Attaching S3 read policy to Lambda role..." -ForegroundColor Blue

    # S3 읽기 권한 정책 생성
    $s3Policy = @"
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "s3:GetObject",
        "s3:ListBucket"
      ],
      "Resource": [
        "arn:aws:s3:::hypermob-models",
        "arn:aws:s3:::hypermob-models/*"
      ]
    }
  ]
}
"@

    $policyFile = "$env:TEMP\recommend-s3-policy.json"
    $s3Policy | Out-File -FilePath $policyFile -Encoding utf8

    # 인라인 정책 추가
    aws iam put-role-policy `
        --role-name $roleName `
        --policy-name "S3ModelReadAccess" `
        --policy-document "file://$policyFile" `
        --region $AwsRegion

    Remove-Item $policyFile -ErrorAction SilentlyContinue
    Write-Host "[SUCCESS] S3 read permissions added" -ForegroundColor Green
} else {
    Write-Host "[WARNING] IAM role not found. Will be created during first deploy." -ForegroundColor Yellow
}

Write-Host ""

# Lambda 함수 업데이트
Write-Host "[4/5] Updating Lambda function..." -ForegroundColor Yellow

Write-Host "[INFO] Updating recommend function code..." -ForegroundColor Blue
aws lambda update-function-code `
    --function-name hypermob-recommend `
    --image-uri $RecommendEcrUri `
    --region $AwsRegion | Out-Null

Write-Host "[INFO] Waiting for function update to complete..." -ForegroundColor Blue
aws lambda wait function-updated --function-name hypermob-recommend --region $AwsRegion

# 환경 변수 업데이트
Write-Host "[INFO] Updating environment variables..." -ForegroundColor Blue
aws lambda update-function-configuration `
    --function-name hypermob-recommend `
    --environment "Variables={AWS_REGION=$AwsRegion,MODEL_BUCKET=hypermob-models,MODEL_KEY=car_fit_model.joblib}" `
    --region $AwsRegion | Out-Null

Write-Host "[INFO] Waiting for configuration update to complete..." -ForegroundColor Blue
aws lambda wait function-updated --function-name hypermob-recommend --region $AwsRegion

Write-Host "[SUCCESS] Lambda function updated" -ForegroundColor Green
Write-Host ""

# 테스트
Write-Host "[5/5] Testing..." -ForegroundColor Yellow
Write-Host "[INFO] Waiting 5 seconds for Lambda to be ready..." -ForegroundColor Blue
Start-Sleep -Seconds 5

# API ID 확인
$apiId = aws apigateway get-rest-apis `
    --query "items[?name=='HYPERMOB Platform AI API'].id" `
    --output text `
    --region $AwsRegion

if ($apiId) {
    $baseUrl = "https://$apiId.execute-api.$AwsRegion.amazonaws.com/prod"

    Write-Host "[INFO] Testing recommend endpoint..." -ForegroundColor Blue

    $body = @{
        height = 175.0
        upper_arm = 32.5
        forearm = 26.8
        thigh = 45.2
        calf = 38.9
        torso = 61.5
        user_id = "rebuild_test"
    } | ConvertTo-Json

    try {
        Write-Host "[INFO] Sending request to $baseUrl/recommend" -ForegroundColor Blue
        $response = Invoke-RestMethod -Uri "$baseUrl/recommend" `
            -Method Post `
            -Body $body `
            -ContentType "application/json" `
            -TimeoutSec 30 `
            -ErrorAction Stop

        Write-Host "[SUCCESS] Recommend API test passed!" -ForegroundColor Green
        Write-Host ($response | ConvertTo-Json -Depth 10)
    }
    catch {
        Write-Host "[WARNING] Test failed: $_" -ForegroundColor Yellow
        if ($_.ErrorDetails.Message) {
            Write-Host "Error details: $($_.ErrorDetails.Message)" -ForegroundColor Red
        }
        Write-Host "[INFO] Check CloudWatch Logs for details:" -ForegroundColor Yellow
        Write-Host "  aws logs tail /aws/lambda/hypermob-recommend --follow --region $AwsRegion" -ForegroundColor Cyan
    }
} else {
    Write-Host "[WARNING] API Gateway not found. Skipping test." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Rebuild Complete!" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Key improvements:" -ForegroundColor Yellow
Write-Host "  • Model loaded from S3 (s3://hypermob-models/car_fit_model.joblib)" -ForegroundColor White
Write-Host "  • Smaller Docker image (no model bundled)" -ForegroundColor White
Write-Host "  • Model can be updated without redeploying Lambda" -ForegroundColor White
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Check CloudWatch Logs:" -ForegroundColor White
Write-Host "     aws logs tail /aws/lambda/hypermob-recommend --follow --region $AwsRegion" -ForegroundColor Cyan
Write-Host "  2. Run full test:" -ForegroundColor White
Write-Host "     .\test_all_endpoints.ps1" -ForegroundColor Cyan
Write-Host ""
