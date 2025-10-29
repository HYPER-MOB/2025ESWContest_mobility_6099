# ============================================================
# Lambda Function Debug Script
# ============================================================
# 목적: Lambda 함수의 상태 및 로그를 확인
# ============================================================

param(
    [Parameter(Mandatory=$false)]
    [string]$FunctionName = "hypermob-health"
)

$ErrorActionPreference = "Continue"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Lambda Function Debugger" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# ============================================================
# 1. Lambda 함수 상태 확인
# ============================================================

Write-Host "[1] Checking Lambda function status..." -ForegroundColor Cyan

$functionInfo = aws lambda get-function --function-name $FunctionName 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ✗ Function not found: $FunctionName" -ForegroundColor Red
    Write-Host "  Error: $functionInfo" -ForegroundColor Red
    exit 1
}

$function = $functionInfo | ConvertFrom-Json
$config = $function.Configuration

Write-Host "  Function Name: $($config.FunctionName)" -ForegroundColor Green
Write-Host "  Runtime: $($config.Runtime)" -ForegroundColor Gray
Write-Host "  Handler: $($config.Handler)" -ForegroundColor Gray
Write-Host "  Memory: $($config.MemorySize) MB" -ForegroundColor Gray
Write-Host "  Timeout: $($config.Timeout) seconds" -ForegroundColor Gray
Write-Host "  State: $($config.State)" -ForegroundColor $(if ($config.State -eq 'Active') { 'Green' } else { 'Red' })
Write-Host "  Last Modified: $($config.LastModified)" -ForegroundColor Gray

if ($config.VpcConfig -and $config.VpcConfig.VpcId) {
    Write-Host "  VPC: $($config.VpcConfig.VpcId)" -ForegroundColor Gray
    Write-Host "  Subnets: $($config.VpcConfig.SubnetIds -join ', ')" -ForegroundColor Gray
    Write-Host "  Security Groups: $($config.VpcConfig.SecurityGroupIds -join ', ')" -ForegroundColor Gray
}

# Environment variables 확인
if ($config.Environment -and $config.Environment.Variables) {
    Write-Host "`n  Environment Variables:" -ForegroundColor Gray
    $config.Environment.Variables.PSObject.Properties | ForEach-Object {
        if ($_.Name -eq 'DB_PASSWORD') {
            Write-Host "    $($_.Name): [HIDDEN]" -ForegroundColor DarkGray
        } else {
            Write-Host "    $($_.Name): $($_.Value)" -ForegroundColor DarkGray
        }
    }
}

# ============================================================
# 2. 최근 로그 확인
# ============================================================

Write-Host "`n[2] Checking recent logs..." -ForegroundColor Cyan

$logGroupName = "/aws/lambda/$FunctionName"

Write-Host "  Log Group: $logGroupName" -ForegroundColor Gray

# 최근 로그 스트림 가져오기
$streams = aws logs describe-log-streams `
    --log-group-name $logGroupName `
    --order-by LastEventTime `
    --descending `
    --max-items 1 `
    --query 'logStreams[0].logStreamName' `
    --output text 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ⚠ No logs found or log group doesn't exist" -ForegroundColor Yellow
} else {
    Write-Host "  Latest Log Stream: $streams" -ForegroundColor Gray

    # 최근 로그 이벤트 가져오기
    Write-Host "`n  Recent Log Events:" -ForegroundColor Yellow
    Write-Host "  " + ("=" * 70) -ForegroundColor Gray

    $logs = aws logs get-log-events `
        --log-group-name $logGroupName `
        --log-stream-name $streams `
        --limit 20 `
        --output json 2>&1 | ConvertFrom-Json

    if ($logs.events) {
        foreach ($event in $logs.events) {
            $timestamp = [DateTimeOffset]::FromUnixTimeMilliseconds($event.timestamp).LocalDateTime.ToString("yyyy-MM-dd HH:mm:ss")
            $message = $event.message.TrimEnd()

            # 에러 메시지 색상 구분
            if ($message -match "ERROR|Error|error|Exception|FAILED") {
                Write-Host "  [$timestamp] " -NoNewline -ForegroundColor Gray
                Write-Host $message -ForegroundColor Red
            } elseif ($message -match "WARN|Warning|warning") {
                Write-Host "  [$timestamp] " -NoNewline -ForegroundColor Gray
                Write-Host $message -ForegroundColor Yellow
            } else {
                Write-Host "  [$timestamp] $message" -ForegroundColor Gray
            }
        }
    } else {
        Write-Host "  No log events found" -ForegroundColor Yellow
    }

    Write-Host "  " + ("=" * 70) -ForegroundColor Gray
}

# ============================================================
# 3. Lambda 함수 테스트 호출
# ============================================================

Write-Host "`n[3] Testing Lambda function..." -ForegroundColor Cyan

$testPayload = @{
    httpMethod = "GET"
    path = "/health"
    headers = @{}
    queryStringParameters = @{}
} | ConvertTo-Json

$testPayloadFile = Join-Path $PSScriptRoot "test-payload.json"
$testPayload | Out-File -FilePath $testPayloadFile -Encoding UTF8

Write-Host "  Invoking function with test payload..." -ForegroundColor Gray

$invokeResult = aws lambda invoke `
    --function-name $FunctionName `
    --payload "fileb://$testPayloadFile" `
    --output json `
    response.json 2>&1

Remove-Item $testPayloadFile -Force -ErrorAction SilentlyContinue

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ✗ Invocation failed" -ForegroundColor Red
    Write-Host "  Error: $invokeResult" -ForegroundColor Red
} else {
    $invoke = $invokeResult | ConvertFrom-Json

    Write-Host "  Status Code: $($invoke.StatusCode)" -ForegroundColor $(if ($invoke.StatusCode -eq 200) { 'Green' } else { 'Red' })

    if (Test-Path "response.json") {
        $response = Get-Content "response.json" -Raw

        Write-Host "`n  Response:" -ForegroundColor Yellow
        Write-Host "  " + ("=" * 70) -ForegroundColor Gray

        try {
            $responseJson = $response | ConvertFrom-Json | ConvertTo-Json -Depth 10
            Write-Host $responseJson -ForegroundColor Gray
        } catch {
            Write-Host $response -ForegroundColor Gray
        }

        Write-Host "  " + ("=" * 70) -ForegroundColor Gray

        Remove-Item "response.json" -Force -ErrorAction SilentlyContinue
    }

    if ($invoke.FunctionError) {
        Write-Host "`n  ✗ Function Error: $($invoke.FunctionError)" -ForegroundColor Red
    } else {
        Write-Host "`n  ✓ Function executed successfully" -ForegroundColor Green
    }
}

# ============================================================
# 4. VPC 연결 확인
# ============================================================

if ($config.VpcConfig -and $config.VpcConfig.VpcId) {
    Write-Host "`n[4] Checking VPC connectivity..." -ForegroundColor Cyan

    # ENI 상태 확인
    Write-Host "  Checking network interfaces..." -ForegroundColor Gray

    $enis = aws ec2 describe-network-interfaces `
        --filters "Name=description,Values=AWS Lambda VPC ENI-$FunctionName*" `
        --query 'NetworkInterfaces[*].[NetworkInterfaceId,Status,PrivateIpAddress]' `
        --output json 2>&1

    if ($LASTEXITCODE -eq 0) {
        $eniList = $enis | ConvertFrom-Json

        if ($eniList.Count -gt 0) {
            Write-Host "  Found $($eniList.Count) network interface(s):" -ForegroundColor Green
            foreach ($eni in $eniList) {
                Write-Host "    - $($eni[0]): $($eni[1]) ($($eni[2]))" -ForegroundColor Gray
            }
        } else {
            Write-Host "  ⚠ No network interfaces found (Lambda may be cold)" -ForegroundColor Yellow
        }
    }
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Debug complete!" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan
