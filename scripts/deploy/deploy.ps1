###############################################################################
# HYPERMOB Platform AI - AWS Lambda Deployment Script (PowerShell)
# Windows 네이티브 PowerShell용 배포 스크립트
###############################################################################

param(
    [string]$AwsRegion = "ap-northeast-2",
    [string]$Stage = "prod"
)

$ErrorActionPreference = "Stop"

# 색상 함수
function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Blue
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] $Message" -ForegroundColor Green
}

function Write-Warning-Custom {
    param([string]$Message)
    Write-Host "[WARNING] $Message" -ForegroundColor Yellow
}

function Write-Error-Custom {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

# AWS 계정 ID 가져오기
function Get-AwsAccountId {
    try {
        $accountId = aws sts get-caller-identity --query Account --output text
        if ($LASTEXITCODE -ne 0) {
            throw "AWS CLI error"
        }
        return $accountId
    }
    catch {
        Write-Error-Custom "AWS credentials not configured. Run 'aws configure' first."
        exit 1
    }
}

# ECR 리포지토리 생성
function New-EcrRepository {
    param([string]$RepoName)

    Write-Info "Checking ECR repository: $RepoName"

    $exists = aws ecr describe-repositories --repository-names $RepoName --region $AwsRegion 2>$null

    if ($LASTEXITCODE -eq 0) {
        Write-Success "ECR repository already exists: $RepoName"
    }
    else {
        Write-Info "Creating ECR repository: $RepoName"
        aws ecr create-repository `
            --repository-name $RepoName `
            --region $AwsRegion `
            --image-scanning-configuration scanOnPush=true `
            --image-tag-mutability MUTABLE | Out-Null
        Write-Success "ECR repository created: $RepoName"
    }
}

# IAM 역할 생성
function New-LambdaIamRole {
    $roleName = "HypermobLambdaExecutionRole"

    Write-Info "Checking IAM role: $roleName"

    $exists = aws iam get-role --role-name $roleName 2>$null

    if ($LASTEXITCODE -eq 0) {
        Write-Success "IAM role already exists: $roleName"
        return
    }

    Write-Info "Creating IAM role: $roleName"

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

    $trustPolicyFile = "$env:TEMP\lambda-trust-policy.json"
    $trustPolicy | Out-File -FilePath $trustPolicyFile -Encoding utf8

    aws iam create-role `
        --role-name $roleName `
        --assume-role-policy-document "file://$trustPolicyFile" | Out-Null

    # Lambda policy
    $lambdaPolicy = @"
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "s3:GetObject",
        "s3:PutObject",
        "s3:PutObjectAcl"
      ],
      "Resource": [
        "arn:aws:s3:::hypermob-*/*"
      ]
    },
    {
      "Effect": "Allow",
      "Action": [
        "logs:CreateLogGroup",
        "logs:CreateLogStream",
        "logs:PutLogEvents"
      ],
      "Resource": "arn:aws:logs:*:*:*"
    }
  ]
}
"@

    $policyFile = "$env:TEMP\lambda-policy.json"
    $lambdaPolicy | Out-File -FilePath $policyFile -Encoding utf8

    aws iam put-role-policy `
        --role-name $roleName `
        --policy-name HypermobLambdaPolicy `
        --policy-document "file://$policyFile" | Out-Null

    Write-Success "IAM role created: $roleName"
    Write-Warning-Custom "Waiting 10 seconds for IAM role to propagate..."
    Start-Sleep -Seconds 10
}

# Docker 이미지 빌드 및 푸시
function Build-AndPushImage {
    param(
        [string]$FunctionDir,
        [string]$ImageName,
        [string]$AccountId
    )

    Write-Info "Building Docker image for: $ImageName"

    Push-Location $FunctionDir

    docker build -t $ImageName .
    if ($LASTEXITCODE -ne 0) {
        Write-Error-Custom "Docker build failed for $ImageName"
        Pop-Location
        exit 1
    }

    $ecrUri = "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com/${ImageName}:latest"

    docker tag "${ImageName}:latest" $ecrUri

    Write-Info "Pushing image to ECR: $ecrUri"
    docker push $ecrUri
    if ($LASTEXITCODE -ne 0) {
        Write-Error-Custom "Docker push failed for $ImageName"
        Pop-Location
        exit 1
    }

    Write-Success "Image pushed: $ecrUri"
    Pop-Location

    return $ecrUri
}

# Lambda 함수 배포
function Deploy-LambdaFunction {
    param(
        [string]$FunctionName,
        [string]$ImageUri,
        [int]$MemorySize,
        [int]$Timeout,
        [string]$AccountId
    )

    Write-Info "Deploying Lambda function: $FunctionName"

    $roleArn = "arn:aws:iam::${AccountId}:role/HypermobLambdaExecutionRole"

    $exists = aws lambda get-function --function-name $FunctionName --region $AwsRegion 2>$null

    if ($LASTEXITCODE -eq 0) {
        Write-Info "Updating existing function: $FunctionName"

        aws lambda update-function-code `
            --function-name $FunctionName `
            --image-uri $ImageUri `
            --region $AwsRegion | Out-Null

        aws lambda update-function-configuration `
            --function-name $FunctionName `
            --memory-size $MemorySize `
            --timeout $Timeout `
            --region $AwsRegion | Out-Null

        Write-Success "Function updated: $FunctionName"
    }
    else {
        Write-Info "Creating new function: $FunctionName"

        # 환경 변수 없이 함수 먼저 생성
        aws lambda create-function `
            --function-name $FunctionName `
            --package-type Image `
            --code "ImageUri=$ImageUri" `
            --role $roleArn `
            --timeout $Timeout `
            --memory-size $MemorySize `
            --region $AwsRegion | Out-Null

        # 환경 변수는 별도로 업데이트
        Start-Sleep -Seconds 2
        aws lambda update-function-configuration `
            --function-name $FunctionName `
            --environment "Variables={RESULT_BUCKET=hypermob-results,AWS_REGION=$AwsRegion}" `
            --region $AwsRegion | Out-Null

        Write-Success "Function created: $FunctionName"
    }

    Write-Info "Waiting for function update to complete..."
    aws lambda wait function-updated --function-name $FunctionName --region $AwsRegion
    Write-Success "Function ready: $FunctionName"
}

# S3 버킷 생성
function New-S3Buckets {
    $buckets = @("hypermob-images", "hypermob-models", "hypermob-results")

    foreach ($bucket in $buckets) {
        Write-Info "Checking S3 bucket: $bucket"

        aws s3 ls "s3://$bucket" 2>$null | Out-Null

        if ($LASTEXITCODE -eq 0) {
            Write-Success "S3 bucket already exists: $bucket"
        }
        else {
            Write-Info "Creating S3 bucket: $bucket"
            aws s3 mb "s3://$bucket" --region $AwsRegion | Out-Null
            Write-Success "S3 bucket created: $bucket"
        }
    }
}

# API Gateway 설정
function Set-ApiGateway {
    param([string]$AccountId)

    Write-Info "Setting up API Gateway..."

    if (Test-Path "openapi.yml") {
        Write-Info "Importing API from openapi.yml"

        $existingApiId = aws apigateway get-rest-apis `
            --query "items[?name=='HYPERMOB Platform AI API'].id" `
            --output text `
            --region $AwsRegion

        if ($existingApiId) {
            Write-Warning-Custom "API already exists with ID: $existingApiId"
            Write-Warning-Custom "Skipping API creation. To update, use AWS Console or re-import."
            return $existingApiId
        }
        else {
            # fileb:// 사용으로 인코딩 문제 해결
            $apiId = aws apigateway import-rest-api `
                --body "fileb://openapi.yml" `
                --region $AwsRegion `
                --fail-on-warnings `
                --query 'id' `
                --output text

            Write-Success "API Gateway created: $apiId"
            return $apiId
        }
    }
    else {
        Write-Error-Custom "openapi.yml not found"
        exit 1
    }
}

# Lambda 권한 추가
function Add-LambdaPermissions {
    param(
        [string]$ApiId,
        [string]$AccountId
    )

    $functions = @("hypermob-measure", "hypermob-recommend")

    foreach ($function in $functions) {
        Write-Info "Adding API Gateway permission for: $function"

        # 기존 권한 제거 (에러 무시)
        aws lambda remove-permission `
            --function-name $function `
            --statement-id "apigateway-$function" `
            --region $AwsRegion 2>$null | Out-Null

        # 새 권한 추가
        aws lambda add-permission `
            --function-name $function `
            --statement-id "apigateway-$function" `
            --action lambda:InvokeFunction `
            --principal apigateway.amazonaws.com `
            --source-arn "arn:aws:execute-api:${AwsRegion}:${AccountId}:${ApiId}/*/*" `
            --region $AwsRegion | Out-Null

        Write-Success "Permission added for: $function"
    }
}

# API 배포
function Publish-Api {
    param([string]$ApiId)

    Write-Info "Deploying API to stage: $Stage"

    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

    aws apigateway create-deployment `
        --rest-api-id $ApiId `
        --stage-name $Stage `
        --description "Deployment $timestamp" `
        --region $AwsRegion | Out-Null

    Write-Success "API deployed to: https://$ApiId.execute-api.$AwsRegion.amazonaws.com/$Stage"
}

###############################################################################
# 메인 스크립트
###############################################################################

Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " HYPERMOB Platform AI Deployment" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# AWS 계정 확인
$accountId = Get-AwsAccountId
Write-Info "AWS Account ID: $accountId"
Write-Info "Region: $AwsRegion"
Write-Info "Stage: $Stage"
Write-Host ""

# Docker 확인
$dockerVersion = docker --version 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Docker is not installed. Please install Docker Desktop for Windows."
    exit 1
}

# 1. S3 버킷 생성
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Step 1: Creating S3 Buckets" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
New-S3Buckets
Write-Host ""

# 2. IAM 역할 생성
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Step 2: Creating IAM Role" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
New-LambdaIamRole
Write-Host ""

# 3. ECR 리포지토리 생성
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Step 3: Creating ECR Repositories" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
New-EcrRepository -RepoName "hypermob-measure"
New-EcrRepository -RepoName "hypermob-recommend"
Write-Host ""

# 4. ECR 로그인
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Step 4: Logging in to ECR" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Info "Authenticating Docker with ECR..."
$ecrPassword = aws ecr get-login-password --region $AwsRegion
$ecrPassword | docker login --username AWS --password-stdin "$accountId.dkr.ecr.$AwsRegion.amazonaws.com"
Write-Success "ECR login successful"
Write-Host ""

# 5. measure 함수 빌드 및 배포
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Step 5: Building and Deploying measure" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
$measureImageUri = Build-AndPushImage -FunctionDir "measure" -ImageName "hypermob-measure" -AccountId $accountId
Deploy-LambdaFunction -FunctionName "hypermob-measure" -ImageUri $measureImageUri -MemorySize 3008 -Timeout 300 -AccountId $accountId
Write-Host ""

# 6. recommend 함수 빌드 및 배포
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Step 6: Building and Deploying recommend" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
$recommendImageUri = Build-AndPushImage -FunctionDir "recommend" -ImageName "hypermob-recommend" -AccountId $accountId
Deploy-LambdaFunction -FunctionName "hypermob-recommend" -ImageUri $recommendImageUri -MemorySize 1024 -Timeout 60 -AccountId $accountId
Write-Host ""

# 7. API Gateway 설정
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Step 7: Setting up API Gateway" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
$apiId = Set-ApiGateway -AccountId $accountId
Write-Host ""

# 8. Lambda 권한 추가
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Step 8: Adding Lambda Permissions" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Add-LambdaPermissions -ApiId $apiId -AccountId $accountId
Write-Host ""

# 9. API 배포
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Step 9: Deploying API" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Publish-Api -ApiId $apiId
Write-Host ""

# 완료
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Deployment Complete!" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""
Write-Success "All services deployed successfully!"
Write-Host ""
Write-Host "API Endpoints:" -ForegroundColor Yellow
Write-Host "  - Base URL: https://$apiId.execute-api.$AwsRegion.amazonaws.com/$Stage"
Write-Host "  - Measure:  POST /measure"
Write-Host "  - Recommend: POST /recommend"
Write-Host "  - Health:   GET /health"
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Test the API using the examples in DEPLOYMENT.md"
Write-Host "  2. Set up CloudWatch alarms for monitoring"
Write-Host "  3. Configure API Gateway usage plans and API keys"
Write-Host ""
