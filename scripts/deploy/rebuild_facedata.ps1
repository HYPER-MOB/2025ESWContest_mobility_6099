###############################################################################
# HYPERMOB Platform AI - Rebuild Facedata Function Only
# facedata 함수만 빠르게 재배포
###############################################################################

param(
    [string]$AwsRegion = "ap-northeast-2"
)

$ErrorActionPreference = "Stop"

Write-Host "`n=========================================" -ForegroundColor Cyan
Write-Host " Rebuilding Facedata Function" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# AWS 계정 ID
$AccountId = aws sts get-caller-identity --query Account --output text
Write-Host "[INFO] AWS Account: $AccountId" -ForegroundColor Blue
Write-Host "[INFO] Region: $AwsRegion" -ForegroundColor Blue
Write-Host ""

# ECR URI
$FacedataEcrUri = "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com/hypermob-facedata:latest"

# ECR 저장소 생성 (없으면)
Write-Host "[INFO] Checking ECR repository..." -ForegroundColor Blue
$repoExists = aws ecr describe-repositories --repository-names hypermob-facedata --region $AwsRegion 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "[INFO] Creating ECR repository: hypermob-facedata" -ForegroundColor Yellow
    aws ecr create-repository --repository-name hypermob-facedata --region $AwsRegion | Out-Null
    Write-Host "[SUCCESS] ECR repository created" -ForegroundColor Green
} else {
    Write-Host "[INFO] ECR repository already exists" -ForegroundColor Blue
}
Write-Host ""

# ECR 로그인
Write-Host "[1/5] Logging in to ECR..." -ForegroundColor Yellow
$ecrPassword = aws ecr get-login-password --region $AwsRegion
$ecrPassword | docker login --username AWS --password-stdin "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com"
Write-Host "[SUCCESS] ECR login successful" -ForegroundColor Green
Write-Host ""

# facedata 함수 리빌드
Write-Host "[2/5] Rebuilding facedata function..." -ForegroundColor Yellow
Write-Host "[INFO] This may take 5-7 minutes (downloading MediaPipe FaceMesh models)..." -ForegroundColor Blue

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent (Split-Path -Parent $scriptPath)
Push-Location "$projectRoot/lambdas/facedata"

Write-Host "[INFO] Starting Docker build..." -ForegroundColor Blue
docker build -t hypermob-facedata . 2>&1 | ForEach-Object {
    if ($_ -match "Downloading MediaPipe") {
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

docker tag hypermob-facedata:latest $FacedataEcrUri
docker push $FacedataEcrUri

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Docker push failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

Pop-Location
Write-Host "[SUCCESS] Image pushed to ECR" -ForegroundColor Green
Write-Host ""

# Lambda IAM 역할 확인 및 생성
Write-Host "[3/5] Checking Lambda IAM role..." -ForegroundColor Yellow

$roleName = "hypermob-facedata-role"
$roleExists = $false
$result = aws iam get-role --role-name $roleName 2>$null
if ($LASTEXITCODE -eq 0) {
    $roleExists = $true
}

if (-not $roleExists) {
    Write-Host "[INFO] Creating IAM role for facedata function..." -ForegroundColor Blue

    # Trust policy
    $trustPolicy = @"
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {
        "Service": "lambda.amazonaws.com"
      },
      "Action": "sts:AssumeRole"
    }
  ]
}
"@

    $policyFile = "$env:TEMP\facedata-trust-policy.json"
    $trustPolicy | Out-File -FilePath $policyFile -Encoding utf8

    aws iam create-role `
        --role-name $roleName `
        --assume-role-policy-document "file://$policyFile" `
        --region $AwsRegion

    Remove-Item $policyFile -ErrorAction SilentlyContinue

    # 기본 Lambda 실행 권한
    aws iam attach-role-policy `
        --role-name $roleName `
        --policy-arn "arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole" `
        --region $AwsRegion

    Write-Host "[SUCCESS] IAM role created" -ForegroundColor Green
} else {
    Write-Host "[INFO] IAM role already exists" -ForegroundColor Blue
}

# S3 읽기/쓰기 권한 추가
Write-Host "[INFO] Attaching S3 permissions to Lambda role..." -ForegroundColor Blue

$s3Policy = @"
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "s3:GetObject",
        "s3:PutObject",
        "s3:ListBucket"
      ],
      "Resource": [
        "arn:aws:s3:::hypermob-images",
        "arn:aws:s3:::hypermob-images/*",
        "arn:aws:s3:::hypermob-results",
        "arn:aws:s3:::hypermob-results/*"
      ]
    }
  ]
}
"@

$policyFile = "$env:TEMP\facedata-s3-policy.json"
$s3Policy | Out-File -FilePath $policyFile -Encoding utf8

aws iam put-role-policy `
    --role-name $roleName `
    --policy-name "S3AccessPolicy" `
    --policy-document "file://$policyFile" `
    --region $AwsRegion

Remove-Item $policyFile -ErrorAction SilentlyContinue
Write-Host "[SUCCESS] S3 permissions added" -ForegroundColor Green
Write-Host ""

# Lambda 함수 생성 또는 업데이트
Write-Host "[4/5] Deploying Lambda function..." -ForegroundColor Yellow

$functionExists = $false
$result = aws lambda get-function --function-name hypermob-facedata --region $AwsRegion 2>$null
if ($LASTEXITCODE -eq 0) {
    $functionExists = $true
}

if ($functionExists) {
    Write-Host "[INFO] Updating existing facedata function..." -ForegroundColor Blue
    aws lambda update-function-code `
        --function-name hypermob-facedata `
        --image-uri $FacedataEcrUri `
        --region $AwsRegion | Out-Null

    Write-Host "[INFO] Waiting for function update to complete..." -ForegroundColor Blue
    aws lambda wait function-updated --function-name hypermob-facedata --region $AwsRegion

    Write-Host "[SUCCESS] Lambda function updated" -ForegroundColor Green
} else {
    Write-Host "[INFO] Creating new facedata function..." -ForegroundColor Blue

    # IAM 역할 ARN 가져오기
    $roleArn = aws iam get-role --role-name $roleName --query 'Role.Arn' --output text

    # 역할 생성 직후에는 사용할 수 없으므로 잠시 대기
    Write-Host "[INFO] Waiting for IAM role to be ready..." -ForegroundColor Blue
    Start-Sleep -Seconds 10

    aws lambda create-function `
        --function-name hypermob-facedata `
        --package-type Image `
        --code ImageUri=$FacedataEcrUri `
        --role $roleArn `
        --timeout 300 `
        --memory-size 2048 `
        --region $AwsRegion | Out-Null

    Write-Host "[INFO] Waiting for function creation to complete..." -ForegroundColor Blue
    aws lambda wait function-active --function-name hypermob-facedata --region $AwsRegion

    Write-Host "[SUCCESS] Lambda function created" -ForegroundColor Green
}

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

    Write-Host "[INFO] Note: Facedata endpoint requires face image in S3" -ForegroundColor Blue
    Write-Host "[INFO] To test, first upload a face image:" -ForegroundColor Yellow
    Write-Host "  aws s3 cp your_face.jpg s3://hypermob-images/test/face.jpg" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "[INFO] API Endpoint: $baseUrl/facedata" -ForegroundColor Blue
} else {
    Write-Host "[WARNING] API Gateway not found. Deploy API Gateway first:" -ForegroundColor Yellow
    Write-Host "  .\deploy_api_gateway.ps1" -ForegroundColor Cyan
}

Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Rebuild Complete!" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Facedata Lambda Function Details:" -ForegroundColor Yellow
Write-Host "  • Function: hypermob-facedata" -ForegroundColor White
Write-Host "  • MediaPipe FaceMesh: 468 landmarks" -ForegroundColor White
Write-Host "  • Extracts x,y,z coordinates (normalized 0-1)" -ForegroundColor White
Write-Host "  • Saves to S3: s3://hypermob-results/facedata/{user_id}/" -ForegroundColor White
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Update API Gateway:" -ForegroundColor White
Write-Host "     .\deploy_api_gateway.ps1" -ForegroundColor Cyan
Write-Host "  2. Upload test face image:" -ForegroundColor White
Write-Host "     aws s3 cp face.jpg s3://hypermob-images/test/face.jpg" -ForegroundColor Cyan
Write-Host "  3. Run tests:" -ForegroundColor White
Write-Host "     .\test_all_endpoints.ps1" -ForegroundColor Cyan
Write-Host ""
