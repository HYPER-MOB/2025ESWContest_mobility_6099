# Profile Lambda 배포 스크립트

param(
    [Parameter(Mandatory=$false)]
    [string]$DBPassword
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Profile Lambda 배포" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# 현재 디렉토리 확인
$scriptDir = $PSScriptRoot
Write-Host "Working directory: $scriptDir`n" -ForegroundColor Gray

# DB 비밀번호 확인
if (-not $DBPassword) {
    Write-Host "Retrieving DB password from Secrets Manager..." -ForegroundColor Yellow
    try {
        $secretValue = aws secretsmanager get-secret-value --secret-id hypermob-db-password --query SecretString --output text 2>$null
        if ($secretValue) {
            $DBPassword = $secretValue
            Write-Host "  ✓ DB password retrieved`n" -ForegroundColor Green
        } else {
            Write-Host "  Warning: Could not retrieve password from Secrets Manager" -ForegroundColor Yellow
            Write-Host "  Please enter DB password manually:" -ForegroundColor Yellow
            $DBPassword = Read-Host "DB Password" -AsSecureString
            $DBPassword = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto([System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($DBPassword))
        }
    } catch {
        Write-Host "  Error retrieving from Secrets Manager: $_" -ForegroundColor Red
        Write-Host "  Please enter DB password manually:" -ForegroundColor Yellow
        $DBPassword = Read-Host "DB Password" -AsSecureString
        $DBPassword = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto([System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($DBPassword))
    }
}

# 1. 의존성 패키지 설치
Write-Host "[1/5] Installing dependencies..." -ForegroundColor Cyan
$packageDir = Join-Path $scriptDir "package"
if (Test-Path $packageDir) {
    Remove-Item -Path $packageDir -Recurse -Force
}
New-Item -ItemType Directory -Path $packageDir -Force | Out-Null

pip install -r "$scriptDir\requirements.txt" -t $packageDir --quiet
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ✗ Failed to install dependencies" -ForegroundColor Red
    exit 1
}
Write-Host "  ✓ Dependencies installed`n" -ForegroundColor Green

# 2. Lambda 함수 코드를 패키지에 복사
Write-Host "[2/5] Packaging Lambda function..." -ForegroundColor Cyan
Copy-Item "$scriptDir\lambda_handler.py" -Destination $packageDir
Write-Host "  ✓ Lambda code packaged`n" -ForegroundColor Green

# 3. ZIP 파일 생성
Write-Host "[3/5] Creating deployment package..." -ForegroundColor Cyan
$zipFile = Join-Path $scriptDir "lambda_function.zip"
if (Test-Path $zipFile) {
    Remove-Item $zipFile -Force
}

Push-Location $packageDir
Compress-Archive -Path * -DestinationPath $zipFile -Force
Pop-Location
Write-Host "  ✓ Deployment package created: lambda_function.zip`n" -ForegroundColor Green

# 4. Lambda 함수 배포
Write-Host "[4/5] Deploying Lambda function..." -ForegroundColor Cyan
$functionName = "hypermob-profiles"

# Lambda 함수 존재 확인
$functionExists = aws lambda get-function --function-name $functionName 2>$null
if ($functionExists) {
    Write-Host "  Updating existing function..." -ForegroundColor Yellow
    aws lambda update-function-code `
        --function-name $functionName `
        --zip-file "fileb://$zipFile" `
        --region ap-northeast-2 | Out-Null

    if ($LASTEXITCODE -eq 0) {
        # 환경 변수 업데이트
        aws lambda update-function-configuration `
            --function-name $functionName `
            --environment "Variables={DB_HOST=mydrive3dx.climsg8a689n.ap-northeast-2.rds.amazonaws.com,DB_USER=admin,DB_PASSWORD=$DBPassword,DB_NAME=mydrive3dx}" `
            --region ap-northeast-2 | Out-Null

        Write-Host "  ✓ Function updated successfully`n" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Failed to update function" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "  Creating new function..." -ForegroundColor Yellow

    # IAM Role 확인
    $roleName = "lambda-execution-role"
    $roleArn = aws iam get-role --role-name $roleName --query 'Role.Arn' --output text 2>$null

    if (-not $roleArn) {
        Write-Host "  Creating IAM role..." -ForegroundColor Yellow
        # Trust policy 생성
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
        $trustPolicyFile = [System.IO.Path]::GetTempFileName()
        $trustPolicy | Out-File -FilePath $trustPolicyFile -Encoding utf8

        aws iam create-role --role-name $roleName --assume-role-policy-document "file://$trustPolicyFile" | Out-Null
        Remove-Item $trustPolicyFile

        # 기본 Lambda 실행 정책 연결
        aws iam attach-role-policy --role-name $roleName --policy-arn "arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole" | Out-Null
        aws iam attach-role-policy --role-name $roleName --policy-arn "arn:aws:iam::aws:policy/service-role/AWSLambdaVPCAccessExecutionRole" | Out-Null

        $roleArn = aws iam get-role --role-name $roleName --query 'Role.Arn' --output text

        Write-Host "  Waiting for IAM role to propagate..." -ForegroundColor Yellow
        Start-Sleep -Seconds 10
    }

    # Lambda 함수 생성
    aws lambda create-function `
        --function-name $functionName `
        --runtime python3.11 `
        --role $roleArn `
        --handler lambda_handler.handler `
        --zip-file "fileb://$zipFile" `
        --environment "Variables={DB_HOST=mydrive3dx.climsg8a689n.ap-northeast-2.rds.amazonaws.com,DB_USER=admin,DB_PASSWORD=$DBPassword,DB_NAME=mydrive3dx}" `
        --timeout 30 `
        --memory-size 256 `
        --region ap-northeast-2 | Out-Null

    if ($LASTEXITCODE -eq 0) {
        Write-Host "  ✓ Function created successfully`n" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Failed to create function" -ForegroundColor Red
        exit 1
    }
}

# 5. API Gateway 통합
Write-Host "[5/5] Setting up API Gateway integration..." -ForegroundColor Cyan

# API Gateway ID 가져오기
$apiId = aws apigateway get-rest-apis --query "items[?name=='hypermob-unified-api'].id" --output text

if ($apiId) {
    Write-Host "  API Gateway ID: $apiId" -ForegroundColor Gray

    # Lambda 권한 추가
    $accountId = aws sts get-caller-identity --query Account --output text
    $sourceArn = "arn:aws:execute-api:ap-northeast-2:${accountId}:${apiId}/*/*"

    aws lambda add-permission `
        --function-name $functionName `
        --statement-id "apigateway-invoke-profiles" `
        --action "lambda:InvokeFunction" `
        --principal apigateway.amazonaws.com `
        --source-arn $sourceArn `
        --region ap-northeast-2 2>$null

    Write-Host "  ✓ API Gateway integration complete`n" -ForegroundColor Green
    Write-Host "  Note: You need to manually add the route in API Gateway:" -ForegroundColor Yellow
    Write-Host "    POST /v1/profiles/body-scan -> Lambda: $functionName" -ForegroundColor Gray
} else {
    Write-Host "  Warning: API Gateway 'hypermob-unified-api' not found" -ForegroundColor Yellow
    Write-Host "  Please create API Gateway integration manually`n" -ForegroundColor Yellow
}

# 정리
Write-Host "Cleaning up..." -ForegroundColor Cyan
Remove-Item -Path $packageDir -Recurse -Force
Remove-Item -Path $zipFile -Force
Write-Host "  ✓ Cleanup complete`n" -ForegroundColor Green

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "✓ Deployment Complete!" -ForegroundColor Green
Write-Host "========================================`n" -ForegroundColor Cyan
Write-Host "Lambda Function: $functionName" -ForegroundColor White
Write-Host "Region: ap-northeast-2" -ForegroundColor White
Write-Host "`nNext Steps:" -ForegroundColor Cyan
Write-Host "1. Add route in API Gateway: POST /v1/profiles/body-scan" -ForegroundColor White
Write-Host "2. Deploy API Gateway changes" -ForegroundColor White
Write-Host "3. Test the endpoint" -ForegroundColor White
