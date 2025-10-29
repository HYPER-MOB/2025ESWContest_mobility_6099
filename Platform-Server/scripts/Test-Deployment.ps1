# ============================================================
# Deployment Test Script
# ============================================================
# 목적: 배포 스크립트의 각 단계를 개별적으로 테스트
# ============================================================

$ErrorActionPreference = "Continue"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Deployment Pre-flight Check" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# ============================================================
# 1. AWS CLI 확인
# ============================================================

Write-Host "[1/7] Checking AWS CLI..." -ForegroundColor Cyan

$awsVersion = aws --version 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ AWS CLI installed: $awsVersion" -ForegroundColor Green
} else {
    Write-Host "  ✗ AWS CLI not found" -ForegroundColor Red
    Write-Host "    Install from: https://aws.amazon.com/cli/" -ForegroundColor Yellow
}

$awsRegion = aws configure get region 2>$null
$awsAccountId = aws sts get-caller-identity --query Account --output text 2>$null

if ($awsAccountId) {
    Write-Host "  ✓ AWS Configured:" -ForegroundColor Green
    Write-Host "    Account ID: $awsAccountId" -ForegroundColor Gray
    Write-Host "    Region: $awsRegion" -ForegroundColor Gray
} else {
    Write-Host "  ✗ AWS CLI not configured" -ForegroundColor Red
    Write-Host "    Run: aws configure" -ForegroundColor Yellow
}

# ============================================================
# 2. 설정 파일 확인
# ============================================================

Write-Host "`n[2/7] Checking configuration file..." -ForegroundColor Cyan

$configFile = "deploy-config.json"
if (Test-Path $configFile) {
    Write-Host "  ✓ deploy-config.json found" -ForegroundColor Green

    try {
        $config = Get-Content $configFile -Raw | ConvertFrom-Json
        Write-Host "    LAMBDA_SUBNET_IDS: $($config.LAMBDA_SUBNET_IDS)" -ForegroundColor Gray
        Write-Host "    LAMBDA_SECURITY_GROUPS: $($config.LAMBDA_SECURITY_GROUPS)" -ForegroundColor Gray
        Write-Host "    DB_HOST: $($config.DB_HOST)" -ForegroundColor Gray
        Write-Host "    DB_PASSWORD: $(if ($config.DB_PASSWORD -eq 'YOUR_SECURE_PASSWORD_HERE') { '[NOT SET]' } else { '[SET]' })" -ForegroundColor Gray
    } catch {
        Write-Host "  ✗ Invalid JSON format" -ForegroundColor Red
    }
} else {
    Write-Host "  ✗ deploy-config.json not found" -ForegroundColor Red
    Write-Host "    Run: .\Get-RDSNetworkConfig.ps1" -ForegroundColor Yellow
}

# ============================================================
# 3. Lambda 함수 디렉토리 확인
# ============================================================

Write-Host "`n[3/7] Checking Lambda function directories..." -ForegroundColor Cyan

$lambdaDir = Join-Path $PSScriptRoot "..\lambdas"
if (Test-Path $lambdaDir) {
    $lambdaDirs = Get-ChildItem $lambdaDir -Directory
    $validLambdas = @()

    foreach ($dir in $lambdaDirs) {
        # shared와 lambda-layer는 제외
        if ($dir.Name -in @('shared', 'lambda-layer')) {
            continue
        }

        # index.js 또는 lambda_handler.py 확인
        $hasNodeJs = Test-Path (Join-Path $dir.FullName "index.js")
        $hasPython = Test-Path (Join-Path $dir.FullName "lambda_handler.py")

        if ($hasNodeJs -or $hasPython) {
            $validLambdas += $dir.Name
            $type = if ($hasNodeJs) { "Node.js" } else { "Python" }
            Write-Host "  ✓ $($dir.Name) ($type)" -ForegroundColor Green
        } else {
            Write-Host "  ⚠ $($dir.Name) (no handler found)" -ForegroundColor Yellow
        }
    }

    Write-Host "`n  Total valid Lambda functions: $($validLambdas.Count)" -ForegroundColor Cyan
} else {
    Write-Host "  ✗ Lambda directory not found: $lambdaDir" -ForegroundColor Red
}

# ============================================================
# 4. Node.js 확인
# ============================================================

Write-Host "`n[4/7] Checking Node.js..." -ForegroundColor Cyan

$nodeVersion = node --version 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ Node.js installed: $nodeVersion" -ForegroundColor Green
} else {
    Write-Host "  ✗ Node.js not found" -ForegroundColor Red
    Write-Host "    Required for Node.js Lambda functions" -ForegroundColor Yellow
}

$npmVersion = npm --version 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ npm installed: $npmVersion" -ForegroundColor Green
} else {
    Write-Host "  ✗ npm not found" -ForegroundColor Red
}

# ============================================================
# 5. Python 확인
# ============================================================

Write-Host "`n[5/7] Checking Python..." -ForegroundColor Cyan

$pythonVersion = python --version 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ Python installed: $pythonVersion" -ForegroundColor Green
} else {
    Write-Host "  ✗ Python not found" -ForegroundColor Red
    Write-Host "    Required for Python Lambda functions (facedata, measure, recommend)" -ForegroundColor Yellow
}

$pipVersion = pip --version 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ pip installed: $pipVersion" -ForegroundColor Green
} else {
    Write-Host "  ✗ pip not found" -ForegroundColor Red
}

# ============================================================
# 6. MySQL 클라이언트 확인
# ============================================================

Write-Host "`n[6/7] Checking MySQL client..." -ForegroundColor Cyan

$mysqlVersion = mysql --version 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ MySQL client installed: $mysqlVersion" -ForegroundColor Green
} else {
    Write-Host "  ⚠ MySQL client not found" -ForegroundColor Yellow
    Write-Host "    Database initialization will be skipped" -ForegroundColor Yellow
    Write-Host "    You can use -SkipDatabase flag to suppress this warning" -ForegroundColor Gray
}

# ============================================================
# 7. 데이터베이스 스크립트 확인
# ============================================================

Write-Host "`n[7/7] Checking database scripts..." -ForegroundColor Cyan

$schemaFile = Join-Path $PSScriptRoot "..\database\scripts\unified-schema.sql"
$seedsFile = Join-Path $PSScriptRoot "..\database\scripts\seeds.sql"

if (Test-Path $schemaFile) {
    $schemaLines = (Get-Content $schemaFile).Count
    Write-Host "  ✓ unified-schema.sql found ($schemaLines lines)" -ForegroundColor Green
} else {
    Write-Host "  ✗ unified-schema.sql not found" -ForegroundColor Red
}

if (Test-Path $seedsFile) {
    $seedsLines = (Get-Content $seedsFile).Count
    Write-Host "  ✓ seeds.sql found ($seedsLines lines)" -ForegroundColor Green
} else {
    Write-Host "  ✗ seeds.sql not found" -ForegroundColor Red
}

# ============================================================
# 요약
# ============================================================

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$canDeploy = $true
$warnings = @()

if (-not $awsAccountId) {
    Write-Host "✗ Cannot deploy: AWS CLI not configured" -ForegroundColor Red
    $canDeploy = $false
}

if (-not (Test-Path $configFile)) {
    Write-Host "✗ Cannot deploy: deploy-config.json not found" -ForegroundColor Red
    $canDeploy = $false
}

if (-not (Test-Path $lambdaDir)) {
    Write-Host "✗ Cannot deploy: Lambda functions not found" -ForegroundColor Red
    $canDeploy = $false
}

if (-not $nodeVersion) {
    $warnings += "Node.js not installed - Node.js Lambda functions will fail"
}

if (-not $pythonVersion) {
    $warnings += "Python not installed - Python Lambda functions will fail"
}

if (-not $mysqlVersion) {
    $warnings += "MySQL client not installed - Use -SkipDatabase flag"
}

if ($canDeploy) {
    Write-Host "`n✓ Ready to deploy!" -ForegroundColor Green

    if ($warnings.Count -gt 0) {
        Write-Host "`nWarnings:" -ForegroundColor Yellow
        foreach ($warning in $warnings) {
            Write-Host "  ⚠ $warning" -ForegroundColor Yellow
        }
    }

    Write-Host "`nNext steps:" -ForegroundColor Cyan
    Write-Host "  1. Review your deploy-config.json" -ForegroundColor Gray
    Write-Host "  2. Run: .\Deploy-Unified-Platform.ps1 -DryRun" -ForegroundColor Gray
    Write-Host "  3. Run: .\Deploy-Unified-Platform.ps1" -ForegroundColor Gray
} else {
    Write-Host "`n✗ Not ready to deploy" -ForegroundColor Red
    Write-Host "Please fix the errors above before deploying." -ForegroundColor Yellow
}

Write-Host ""
