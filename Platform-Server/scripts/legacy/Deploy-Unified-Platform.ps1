# ============================================================
# HYPERMOB Unified Platform Deployment Script
# ============================================================
# 목적: Platform-Application과 Platform-AI의 모든 Lambda 함수를
#      하나의 API Gateway에 통합 배포
# 요구사항: AWS CLI 설정 완료, deploy-config.json 파일
# ============================================================

param(
    [string]$ConfigFile = "deploy-config.json",
    [switch]$SkipDatabase,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

# ============================================================
# 1. 설정 로드
# ============================================================

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "HYPERMOB Unified Platform Deployment" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

if (-not (Test-Path $ConfigFile)) {
    Write-Host "ERROR: Config file not found: $ConfigFile" -ForegroundColor Red
    Write-Host "Please create deploy-config.json from deploy-config.template.json" -ForegroundColor Yellow
    exit 1
}

$config = Get-Content $ConfigFile -Raw | ConvertFrom-Json

# AWS CLI에서 자동 감지
$AWS_CLI_REGION = (aws configure get region 2>$null)
$AWS_CLI_ACCOUNT_ID = (aws sts get-caller-identity --query Account --output text 2>$null)

if (-not $AWS_CLI_ACCOUNT_ID) {
    Write-Host "ERROR: AWS CLI not configured. Run 'aws configure' first." -ForegroundColor Red
    exit 1
}

# 우선순위: 설정 파일 > AWS CLI
$AWS_REGION = if ($config.AWS_REGION) { $config.AWS_REGION } else { $AWS_CLI_REGION }
$AWS_ACCOUNT_ID = $AWS_CLI_ACCOUNT_ID

$LAMBDA_SUBNET_IDS = $config.LAMBDA_SUBNET_IDS
$LAMBDA_SECURITY_GROUPS = $config.LAMBDA_SECURITY_GROUPS
$DB_HOST = $config.DB_HOST
$DB_PASSWORD = $config.DB_PASSWORD
$DB_USER = if ($config.DB_USER) { $config.DB_USER } else { "admin" }
$DB_NAME = if ($config.DB_NAME) { $config.DB_NAME } else { "hypermob" }
$API_NAME = if ($config.API_NAME) { $config.API_NAME } else { "hypermob-unified-api" }

Write-Host "Configuration loaded:" -ForegroundColor Green
Write-Host "  AWS Region: $AWS_REGION"
Write-Host "  AWS Account: $AWS_ACCOUNT_ID"
Write-Host "  DB Host: $DB_HOST"
Write-Host "  DB Name: $DB_NAME"
Write-Host "  API Name: $API_NAME"
Write-Host ""

# ============================================================
# 2. Lambda 함수 목록 정의
# ============================================================

$LAMBDA_FUNCTIONS = @(
    # JavaScript Lambda Functions (Node.js 18.x)
    @{
        Name = "hypermob-health"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/health"
        Timeout = 30
        MemorySize = 256
        Type = "nodejs"
    },
    @{
        Name = "hypermob-users"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/users"
        Timeout = 30
        MemorySize = 256
        Type = "nodejs"
    },
    @{
        Name = "hypermob-auth-face"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/auth-face"
        Timeout = 30
        MemorySize = 512
        Type = "nodejs"
    },
    @{
        Name = "hypermob-auth-nfc"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/auth-nfc"
        Timeout = 30
        MemorySize = 256
        Type = "nodejs"
    },
    @{
        Name = "hypermob-auth-nfc-get"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/auth-nfc-get"
        Timeout = 30
        MemorySize = 256
        Type = "nodejs"
    },
    @{
        Name = "hypermob-auth-ble"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/auth-ble"
        Timeout = 30
        MemorySize = 256
        Type = "nodejs"
    },
    @{
        Name = "hypermob-auth-session"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/auth-session"
        Timeout = 30
        MemorySize = 256
        Type = "nodejs"
    },
    @{
        Name = "hypermob-auth-result"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/auth-result"
        Timeout = 30
        MemorySize = 256
        Type = "nodejs"
    },
    @{
        Name = "hypermob-auth-nfc-verify"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/auth-nfc-verify"
        Timeout = 30
        MemorySize = 256
        Type = "nodejs"
    },
    @{
        Name = "hypermob-vehicles"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/vehicles"
        Timeout = 30
        MemorySize = 256
        Type = "nodejs"
    },
    @{
        Name = "hypermob-bookings"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/bookings"
        Timeout = 30
        MemorySize = 256
        Type = "nodejs"
    },
    @{
        Name = "hypermob-user-profile"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/user-profile"
        Timeout = 30
        MemorySize = 256
        Type = "nodejs"
    },
    @{
        Name = "hypermob-vehicle-control"
        Handler = "index.handler"
        Runtime = "nodejs18.x"
        Path = "lambdas/vehicle-control"
        Timeout = 30
        MemorySize = 256
        Type = "nodejs"
    },

    # Python Lambda Functions (Python 3.11)
    @{
        Name = "hypermob-facedata"
        Handler = "lambda_handler.handler"
        Runtime = "python3.11"
        Path = "lambdas/facedata"
        Timeout = 60
        MemorySize = 1024
        Type = "python"
    },
    @{
        Name = "hypermob-measure"
        Handler = "lambda_handler.handler"
        Runtime = "python3.11"
        Path = "lambdas/measure"
        Timeout = 60
        MemorySize = 1024
        Type = "python"
    },
    @{
        Name = "hypermob-recommend"
        Handler = "lambda_handler.handler"
        Runtime = "python3.11"
        Path = "lambdas/recommend"
        Timeout = 60
        MemorySize = 512
        Type = "python"
    }
)

Write-Host "Total Lambda functions to deploy: $($LAMBDA_FUNCTIONS.Count)" -ForegroundColor Green
Write-Host "  - Node.js functions: $(($LAMBDA_FUNCTIONS | Where-Object {$_.Type -eq 'nodejs'}).Count)"
Write-Host "  - Python functions: $(($LAMBDA_FUNCTIONS | Where-Object {$_.Type -eq 'python'}).Count)"
Write-Host ""

if ($DryRun) {
    Write-Host "DRY RUN mode - no actual deployment will occur`n" -ForegroundColor Yellow
}

# ============================================================
# 3. Lambda IAM Role 생성
# ============================================================

Write-Host "[Step 1/6] Creating Lambda IAM Role..." -ForegroundColor Cyan

$ROLE_NAME = "hypermob-lambda-execution-role"
$TRUST_POLICY = @"
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

$roleExists = aws iam get-role --role-name $ROLE_NAME 2>$null
if (-not $roleExists) {
    if (-not $DryRun) {
        aws iam create-role --role-name $ROLE_NAME --assume-role-policy-document $TRUST_POLICY

        # Attach policies
        aws iam attach-role-policy --role-name $ROLE_NAME --policy-arn "arn:aws:iam::aws:policy/service-role/AWSLambdaVPCAccessExecutionRole"
        aws iam attach-role-policy --role-name $ROLE_NAME --policy-arn "arn:aws:iam::aws:policy/AWSLambdaExecute"
        aws iam attach-role-policy --role-name $ROLE_NAME --policy-arn "arn:aws:iam::aws:policy/AmazonS3FullAccess"

        Write-Host "  Role created: $ROLE_NAME" -ForegroundColor Green
        Write-Host "  Waiting 10 seconds for IAM propagation..." -ForegroundColor Yellow
        Start-Sleep -Seconds 10
    } else {
        Write-Host "  [DRY RUN] Would create role: $ROLE_NAME" -ForegroundColor Yellow
    }
} else {
    Write-Host "  Role already exists: $ROLE_NAME" -ForegroundColor Yellow
}

$LAMBDA_ROLE_ARN = "arn:aws:iam::${AWS_ACCOUNT_ID}:role/${ROLE_NAME}"
Write-Host "  Role ARN: $LAMBDA_ROLE_ARN`n" -ForegroundColor Green

# ============================================================
# 4. Lambda Layer 배포 (shared 코드)
# ============================================================

Write-Host "[Step 2/6] Deploying Lambda Layer (shared code)..." -ForegroundColor Cyan

$layerPath = Join-Path $PSScriptRoot "..\lambdas\shared"
$LAYER_ARN = $null

if (Test-Path $layerPath) {
    Write-Host "  Creating Lambda Layer package..." -ForegroundColor Yellow

    # Layer 구조: nodejs/node_modules/ 와 nodejs/shared/
    $layerBuildPath = Join-Path $PSScriptRoot "layer-build"
    $layerNodePath = Join-Path $layerBuildPath "nodejs"
    $layerNodeModulesPath = Join-Path $layerNodePath "node_modules"

    # 기존 빌드 디렉토리 정리
    if (Test-Path $layerBuildPath) {
        Remove-Item $layerBuildPath -Recurse -Force
    }

    # Layer 구조 생성
    New-Item -ItemType Directory -Path $layerNodePath -Force | Out-Null
    New-Item -ItemType Directory -Path $layerNodeModulesPath -Force | Out-Null

    # shared 코드 복사
    Copy-Item -Path $layerPath -Destination $layerNodePath -Recurse -Force

    # mysql2 설치
    Write-Host "  Installing mysql2 dependency..." -ForegroundColor Gray
    Push-Location $layerNodePath
    npm install mysql2 --production --no-save --quiet 2>&1 | Out-Null
    Pop-Location

    # ZIP 파일 생성
    $layerZipPath = Join-Path $PSScriptRoot "lambda-layer.zip"
    if (Test-Path $layerZipPath) {
        Remove-Item $layerZipPath -Force
    }

    Push-Location $layerBuildPath
    Compress-Archive -Path * -DestinationPath $layerZipPath -Force
    Pop-Location

    $layerSize = (Get-Item $layerZipPath).Length / 1MB
    Write-Host "  Layer package size: $([math]::Round($layerSize, 2)) MB" -ForegroundColor Gray

    if (-not $DryRun) {
        # Lambda Layer 배포
        Write-Host "  Publishing Lambda Layer..." -ForegroundColor Yellow

        $layerResult = aws lambda publish-layer-version `
            --layer-name "hypermob-shared-layer" `
            --description "Shared code for HYPERMOB Lambda functions" `
            --zip-file "fileb://$layerZipPath" `
            --compatible-runtimes nodejs18.x `
            --output json 2>&1

        if ($LASTEXITCODE -ne 0) {
            Write-Host "  ✗ Failed to publish layer: $layerResult" -ForegroundColor Red
        } else {
            $layer = $layerResult | ConvertFrom-Json
            $LAYER_ARN = $layer.LayerVersionArn
            Write-Host "  ✓ Layer published: $LAYER_ARN" -ForegroundColor Green
        }
    } else {
        Write-Host "  [DRY RUN] Would publish Lambda Layer" -ForegroundColor Yellow
    }

    # 정리
    Remove-Item $layerBuildPath -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item $layerZipPath -Force -ErrorAction SilentlyContinue
} else {
    Write-Host "  ⚠ Shared code not found: $layerPath" -ForegroundColor Yellow
}

Write-Host ""

# ============================================================
# 5. Lambda 함수 배포
# ============================================================

Write-Host "[Step 3/6] Deploying Lambda Functions..." -ForegroundColor Cyan

$deployedFunctions = @()
$failedFunctions = @()

foreach ($func in $LAMBDA_FUNCTIONS) {
    Write-Host "  Deploying: $($func.Name)..." -ForegroundColor Yellow

    $funcPath = Join-Path $PSScriptRoot "..\$($func.Path)"

    if (-not (Test-Path $funcPath)) {
        Write-Host "    ✗ ERROR: Function path not found: $funcPath" -ForegroundColor Red
        $failedFunctions += $func.Name
        continue
    }

    try {
        # Create deployment package
        $zipFile = "$($func.Name).zip"
        $zipPath = Join-Path $PSScriptRoot $zipFile

        if (Test-Path $zipPath) {
            Remove-Item $zipPath -Force
        }

        Write-Host "    Creating deployment package..." -ForegroundColor Gray

        if ($func.Type -eq "nodejs") {
            # Node.js 함수
            Push-Location $funcPath
            if (Test-Path "package.json") {
                Write-Host "      Installing npm dependencies..." -ForegroundColor Gray
                npm install --production --quiet 2>&1 | Out-Null
            }
            Compress-Archive -Path * -DestinationPath $zipPath -Force
            Pop-Location
        } else {
            # Python 함수
            Push-Location $funcPath
            if (Test-Path "requirements.txt") {
                Write-Host "      Installing Python dependencies..." -ForegroundColor Gray
                pip install -r requirements.txt -t . --quiet --disable-pip-version-check 2>&1 | Out-Null
            }
            Compress-Archive -Path * -DestinationPath $zipPath -Force
            Pop-Location
        }

        $zipSize = (Get-Item $zipPath).Length / 1MB
        Write-Host "      Package size: $([math]::Round($zipSize, 2)) MB" -ForegroundColor Gray

        if (-not $DryRun) {
            # Check if function exists
            Write-Host "      Checking if function exists..." -ForegroundColor Gray
            $functionExists = aws lambda get-function --function-name $func.Name 2>$null

            if ($functionExists) {
                # Update existing function
                Write-Host "      Updating function code..." -ForegroundColor Gray
                $updateCodeResult = aws lambda update-function-code `
                    --function-name $func.Name `
                    --zip-file "fileb://$zipPath" 2>&1

                if ($LASTEXITCODE -ne 0) {
                    Write-Host "    ✗ Failed to update code: $updateCodeResult" -ForegroundColor Red
                    $failedFunctions += $func.Name
                    continue
                }

                Start-Sleep -Seconds 2

                Write-Host "      Updating function configuration..." -ForegroundColor Gray

                # Layer 연결 여부 확인 (Node.js 함수만)
                if ($func.Type -eq "nodejs" -and $LAYER_ARN) {
                    $updateConfigResult = aws lambda update-function-configuration `
                        --function-name $func.Name `
                        --handler $func.Handler `
                        --runtime $func.Runtime `
                        --timeout $func.Timeout `
                        --memory-size $func.MemorySize `
                        --layers $LAYER_ARN `
                        --environment "Variables={DB_HOST=$DB_HOST,DB_USER=$DB_USER,DB_PASSWORD=$DB_PASSWORD,DB_NAME=$DB_NAME}" 2>&1
                } else {
                    $updateConfigResult = aws lambda update-function-configuration `
                        --function-name $func.Name `
                        --handler $func.Handler `
                        --runtime $func.Runtime `
                        --timeout $func.Timeout `
                        --memory-size $func.MemorySize `
                        --environment "Variables={DB_HOST=$DB_HOST,DB_USER=$DB_USER,DB_PASSWORD=$DB_PASSWORD,DB_NAME=$DB_NAME}" 2>&1
                }

                if ($LASTEXITCODE -ne 0) {
                    Write-Host "    ✗ Failed to update configuration: $updateConfigResult" -ForegroundColor Red
                    $failedFunctions += $func.Name
                    continue
                }

                Write-Host "    ✓ Updated: $($func.Name)" -ForegroundColor Green
            } else {
                # Create new function
                Write-Host "      Creating new function..." -ForegroundColor Gray

                # Layer 옵션 (Node.js 함수만)
                $layerOption = if ($func.Type -eq "nodejs" -and $LAYER_ARN) { "--layers $LAYER_ARN" } else { "" }

                if ($LAMBDA_SUBNET_IDS -and $LAMBDA_SECURITY_GROUPS) {
                    if ($layerOption) {
                        $createResult = aws lambda create-function `
                            --function-name $func.Name `
                            --runtime $func.Runtime `
                            --role $LAMBDA_ROLE_ARN `
                            --handler $func.Handler `
                            --zip-file "fileb://$zipPath" `
                            --timeout $func.Timeout `
                            --memory-size $func.MemorySize `
                            --layers $LAYER_ARN `
                            --environment "Variables={DB_HOST=$DB_HOST,DB_USER=$DB_USER,DB_PASSWORD=$DB_PASSWORD,DB_NAME=$DB_NAME}" `
                            --vpc-config "SubnetIds=$LAMBDA_SUBNET_IDS,SecurityGroupIds=$LAMBDA_SECURITY_GROUPS" 2>&1
                    } else {
                        $createResult = aws lambda create-function `
                            --function-name $func.Name `
                            --runtime $func.Runtime `
                            --role $LAMBDA_ROLE_ARN `
                            --handler $func.Handler `
                            --zip-file "fileb://$zipPath" `
                            --timeout $func.Timeout `
                            --memory-size $func.MemorySize `
                            --environment "Variables={DB_HOST=$DB_HOST,DB_USER=$DB_USER,DB_PASSWORD=$DB_PASSWORD,DB_NAME=$DB_NAME}" `
                            --vpc-config "SubnetIds=$LAMBDA_SUBNET_IDS,SecurityGroupIds=$LAMBDA_SECURITY_GROUPS" 2>&1
                    }
                } else {
                    if ($layerOption) {
                        $createResult = aws lambda create-function `
                            --function-name $func.Name `
                            --runtime $func.Runtime `
                            --role $LAMBDA_ROLE_ARN `
                            --handler $func.Handler `
                            --zip-file "fileb://$zipPath" `
                            --timeout $func.Timeout `
                            --memory-size $func.MemorySize `
                            --layers $LAYER_ARN `
                            --environment "Variables={DB_HOST=$DB_HOST,DB_USER=$DB_USER,DB_PASSWORD=$DB_PASSWORD,DB_NAME=$DB_NAME}" 2>&1
                    } else {
                        $createResult = aws lambda create-function `
                            --function-name $func.Name `
                            --runtime $func.Runtime `
                            --role $LAMBDA_ROLE_ARN `
                            --handler $func.Handler `
                            --zip-file "fileb://$zipPath" `
                            --timeout $func.Timeout `
                            --memory-size $func.MemorySize `
                            --environment "Variables={DB_HOST=$DB_HOST,DB_USER=$DB_USER,DB_PASSWORD=$DB_PASSWORD,DB_NAME=$DB_NAME}" 2>&1
                    }
                }

                if ($LASTEXITCODE -ne 0) {
                    Write-Host "    ✗ Failed to create: $createResult" -ForegroundColor Red
                    $failedFunctions += $func.Name
                    continue
                }

                Write-Host "    ✓ Created: $($func.Name)" -ForegroundColor Green
            }

            # Clean up zip file
            Remove-Item $zipPath -Force

            $deployedFunctions += $func.Name
        } else {
            Write-Host "    [DRY RUN] Would deploy: $($func.Name)" -ForegroundColor Yellow
            Remove-Item $zipPath -Force
        }
    } catch {
        Write-Host "    ✗ Exception: $_" -ForegroundColor Red
        $failedFunctions += $func.Name
        if (Test-Path $zipPath) {
            Remove-Item $zipPath -Force -ErrorAction SilentlyContinue
        }
    }
}

Write-Host "`n  Deployed: $($deployedFunctions.Count)/$($LAMBDA_FUNCTIONS.Count) functions" -ForegroundColor Green
if ($failedFunctions.Count -gt 0) {
    Write-Host "  Failed: $($failedFunctions -join ', ')" -ForegroundColor Red
}
Write-Host ""

# ============================================================
# 5. API Gateway 생성
# ============================================================

Write-Host "[Step 3/6] Creating API Gateway..." -ForegroundColor Cyan

$apiExists = aws apigateway get-rest-apis --query "items[?name=='$API_NAME'].id" --output text

if ($apiExists) {
    $API_ID = $apiExists
    Write-Host "  API Gateway already exists: $API_NAME (ID: $API_ID)" -ForegroundColor Yellow
} else {
    if (-not $DryRun) {
        $apiResult = aws apigateway create-rest-api --name $API_NAME --description "HYPERMOB Unified Platform API" --output json | ConvertFrom-Json
        $API_ID = $apiResult.id
        Write-Host "  Created API Gateway: $API_NAME (ID: $API_ID)" -ForegroundColor Green
    } else {
        Write-Host "  [DRY RUN] Would create API Gateway: $API_NAME" -ForegroundColor Yellow
        $API_ID = "DRYRUN-API-ID"
    }
}

Write-Host "  API Gateway ID: $API_ID`n" -ForegroundColor Green

# ============================================================
# 6. API Gateway 설정 (OpenAPI Import)
# ============================================================

Write-Host "[Step 4/6] Configuring API Gateway with OpenAPI spec..." -ForegroundColor Cyan

$openapiFile = Join-Path $PSScriptRoot "..\docs\openapi-unified.yaml"

if (Test-Path $openapiFile) {
    if (-not $DryRun) {
        Write-Host "  Reading OpenAPI spec: $openapiFile" -ForegroundColor Gray

        # OpenAPI 파일 읽기 및 변수 치환
        $openapiContent = Get-Content $openapiFile -Raw

        # AWS 변수 치환
        $openapiContent = $openapiContent -replace '\$\{AWS::Region\}', $AWS_REGION
        $openapiContent = $openapiContent -replace '\$\{AWS::AccountId\}', $AWS_ACCOUNT_ID

        # 임시 파일로 저장
        $tempOpenapiFile = Join-Path $PSScriptRoot "openapi-temp.yaml"
        $openapiContent | Out-File -FilePath $tempOpenapiFile -Encoding UTF8

        Write-Host "  Importing OpenAPI spec to API Gateway..." -ForegroundColor Yellow

        # API Gateway에 OpenAPI import
        $importResult = aws apigateway put-rest-api `
            --rest-api-id $API_ID `
            --mode overwrite `
            --body "fileb://$tempOpenapiFile" 2>&1

        if ($LASTEXITCODE -ne 0) {
            Write-Host "  ✗ Failed to import OpenAPI spec" -ForegroundColor Red
            Write-Host "  Error: $importResult" -ForegroundColor Red

            # 임시 파일 삭제
            Remove-Item $tempOpenapiFile -Force -ErrorAction SilentlyContinue
        } else {
            Write-Host "  ✓ OpenAPI spec imported successfully" -ForegroundColor Green

            # API 배포
            Write-Host "  Deploying API to 'v1' stage..." -ForegroundColor Yellow

            $deployResult = aws apigateway create-deployment `
                --rest-api-id $API_ID `
                --stage-name v1 `
                --stage-description "Production stage" `
                --description "Deployed by Deploy-Unified-Platform.ps1" 2>&1

            if ($LASTEXITCODE -ne 0) {
                Write-Host "  ✗ Failed to deploy API" -ForegroundColor Red
                Write-Host "  Error: $deployResult" -ForegroundColor Red
            } else {
                Write-Host "  ✓ API deployed to stage 'v1'" -ForegroundColor Green
            }

            # 임시 파일 삭제
            Remove-Item $tempOpenapiFile -Force -ErrorAction SilentlyContinue
        }
    } else {
        Write-Host "  [DRY RUN] Would import OpenAPI spec and deploy API" -ForegroundColor Yellow
    }
} else {
    Write-Host "  ⚠ WARNING: OpenAPI spec not found: $openapiFile" -ForegroundColor Yellow
    Write-Host "  API Gateway created but not configured. You'll need to configure it manually." -ForegroundColor Yellow
}

Write-Host ""

# ============================================================
# 7. Lambda 권한 부여 (API Gateway에서 Lambda 호출 허용)
# ============================================================

Write-Host "`n[Step 5/6] Granting API Gateway permissions to invoke Lambda..." -ForegroundColor Cyan

foreach ($func in $LAMBDA_FUNCTIONS) {
    if ($deployedFunctions -contains $func.Name) {
        if (-not $DryRun) {
            $statementId = "apigateway-invoke-$($func.Name)"

            # Remove existing permission if exists
            aws lambda remove-permission --function-name $func.Name --statement-id $statementId 2>$null

            # Add new permission
            aws lambda add-permission `
                --function-name $func.Name `
                --statement-id $statementId `
                --action lambda:InvokeFunction `
                --principal apigateway.amazonaws.com `
                --source-arn "arn:aws:execute-api:${AWS_REGION}:${AWS_ACCOUNT_ID}:${API_ID}/*" 2>&1 | Out-Null

            Write-Host "  Granted permission: $($func.Name)" -ForegroundColor Green
        } else {
            Write-Host "  [DRY RUN] Would grant permission: $($func.Name)" -ForegroundColor Yellow
        }
    }
}

# ============================================================
# 8. 데이터베이스 초기화
# ============================================================

if (-not $SkipDatabase) {
    Write-Host "`n[Step 6/6] Initializing Database..." -ForegroundColor Cyan

    $schemaFile = Join-Path $PSScriptRoot "..\database\scripts\unified-schema.sql"
    $seedsFile = Join-Path $PSScriptRoot "..\database\scripts\seeds.sql"

    if ((Test-Path $schemaFile) -and (Test-Path $seedsFile)) {
        if (-not $DryRun) {
            Write-Host "  Creating database tables..." -ForegroundColor Yellow

            # MySQL 환경 변수 설정 (비밀번호를 안전하게 전달)
            $env:MYSQL_PWD = $DB_PASSWORD

            try {
                # PowerShell에서는 Get-Content로 파일을 읽어서 파이프로 전달
                Write-Host "    Executing schema file: $schemaFile" -ForegroundColor Gray
                $schemaOutput = Get-Content $schemaFile -Raw | mysql -h $DB_HOST -u $DB_USER $DB_NAME 2>&1

                if ($LASTEXITCODE -ne 0) {
                    Write-Host "  ERROR: Schema creation failed (Exit code: $LASTEXITCODE)" -ForegroundColor Red
                    Write-Host "  MySQL Output:" -ForegroundColor Red
                    Write-Host $schemaOutput -ForegroundColor Red
                    throw "MySQL schema execution failed"
                }

                Write-Host "    Schema created successfully" -ForegroundColor Green
                Write-Host "  Inserting seed data..." -ForegroundColor Yellow
                Write-Host "    Executing seeds file: $seedsFile" -ForegroundColor Gray

                $seedsOutput = Get-Content $seedsFile -Raw | mysql -h $DB_HOST -u $DB_USER $DB_NAME 2>&1

                if ($LASTEXITCODE -ne 0) {
                    Write-Host "  ERROR: Seed data insertion failed (Exit code: $LASTEXITCODE)" -ForegroundColor Red
                    Write-Host "  MySQL Output:" -ForegroundColor Red
                    Write-Host $seedsOutput -ForegroundColor Red
                    throw "MySQL seed execution failed"
                }

                Write-Host "    Seeds inserted successfully" -ForegroundColor Green
                Write-Host "  Database initialized successfully!" -ForegroundColor Green
            } catch {
                Write-Host "`n  Database initialization failed, but continuing with deployment..." -ForegroundColor Yellow
                Write-Host "  You can initialize the database manually later using:" -ForegroundColor Yellow
                Write-Host "    mysql -h $DB_HOST -u $DB_USER -p $DB_NAME < database/scripts/unified-schema.sql" -ForegroundColor Gray
                Write-Host "    mysql -h $DB_HOST -u $DB_USER -p $DB_NAME < database/scripts/seeds.sql" -ForegroundColor Gray
            } finally {
                # 환경 변수 정리
                Remove-Item Env:\MYSQL_PWD -ErrorAction SilentlyContinue
            }
        } else {
            Write-Host "  [DRY RUN] Would initialize database" -ForegroundColor Yellow
        }
    } else {
        Write-Host "  WARNING: Database scripts not found" -ForegroundColor Yellow
        Write-Host "    Schema: $schemaFile"
        Write-Host "    Seeds: $seedsFile"
    }
} else {
    Write-Host "`n[Step 6/6] Skipping database initialization (-SkipDatabase flag)" -ForegroundColor Yellow
}

# ============================================================
# 9. 배포 완료
# ============================================================

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Deployment Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

Write-Host "Lambda Functions Deployed: $($deployedFunctions.Count)/$($LAMBDA_FUNCTIONS.Count)" -ForegroundColor Green
Write-Host "API Gateway ID: $API_ID" -ForegroundColor Green
Write-Host "API Endpoint: https://${API_ID}.execute-api.${AWS_REGION}.amazonaws.com/v1" -ForegroundColor Green

if ($failedFunctions.Count -gt 0) {
    Write-Host "`nFailed Functions: $($failedFunctions -join ', ')" -ForegroundColor Red
}

Write-Host "`nNext Steps:" -ForegroundColor Cyan
Write-Host "  1. Import OpenAPI spec to complete API Gateway setup:"
Write-Host "     aws apigateway put-rest-api --rest-api-id $API_ID --mode overwrite --body file://docs/openapi-unified.yaml"
Write-Host "  2. Deploy API to stage:"
Write-Host "     aws apigateway create-deployment --rest-api-id $API_ID --stage-name v1"
Write-Host "  3. Test health endpoint:"
Write-Host "     curl https://${API_ID}.execute-api.${AWS_REGION}.amazonaws.com/v1/health"

Write-Host "`nDeployment completed successfully!`n" -ForegroundColor Green
