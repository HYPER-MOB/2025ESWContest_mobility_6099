# ============================================================
# RDS Network Configuration Helper
# ============================================================
# 목적: RDS 인스턴스의 VPC/Subnet/Security Group 정보를 자동으로 가져옴
# 사용법: .\Get-RDSNetworkConfig.ps1 -DBIdentifier your-db-instance-id
# ============================================================

param(
    [Parameter(Mandatory=$false)]
    [string]$DBIdentifier,

    [Parameter(Mandatory=$false)]
    [string]$DBHost
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "RDS Network Configuration Helper" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# AWS CLI 설정 확인
$awsAccountId = aws sts get-caller-identity --query Account --output text 2>$null
if (-not $awsAccountId) {
    Write-Host "ERROR: AWS CLI not configured. Run 'aws configure' first." -ForegroundColor Red
    exit 1
}

Write-Host "AWS Account ID: $awsAccountId" -ForegroundColor Green

# DB Identifier가 없으면 DB Host에서 추출 시도
if (-not $DBIdentifier -and $DBHost) {
    # DB Host: your-db.abc123.us-east-1.rds.amazonaws.com
    $DBIdentifier = $DBHost.Split('.')[0]
    Write-Host "Extracted DB Identifier from host: $DBIdentifier`n" -ForegroundColor Yellow
}

# DB Identifier가 여전히 없으면 모든 RDS 인스턴스 목록 표시
if (-not $DBIdentifier) {
    Write-Host "Available RDS instances:" -ForegroundColor Yellow
    $instances = aws rds describe-db-instances --query "DBInstances[*].[DBInstanceIdentifier,Endpoint.Address,Engine,DBInstanceStatus]" --output table
    Write-Host $instances
    Write-Host "`nUsage:" -ForegroundColor Cyan
    Write-Host "  .\Get-RDSNetworkConfig.ps1 -DBIdentifier your-db-instance-id"
    Write-Host "  .\Get-RDSNetworkConfig.ps1 -DBHost your-db.abc123.us-east-1.rds.amazonaws.com"
    exit 0
}

Write-Host "Looking up RDS instance: $DBIdentifier`n" -ForegroundColor Yellow

# RDS 정보 가져오기
try {
    $rdsInfo = aws rds describe-db-instances --db-instance-identifier $DBIdentifier --output json | ConvertFrom-Json
    $db = $rdsInfo.DBInstances[0]
} catch {
    Write-Host "ERROR: Unable to find RDS instance '$DBIdentifier'" -ForegroundColor Red
    Write-Host "Run without parameters to see available instances." -ForegroundColor Yellow
    exit 1
}

# RDS 정보 출력
Write-Host "========================================" -ForegroundColor Green
Write-Host "RDS Instance Information" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host "DB Identifier: $($db.DBInstanceIdentifier)"
Write-Host "DB Host: $($db.Endpoint.Address)"
Write-Host "DB Port: $($db.Endpoint.Port)"
Write-Host "Engine: $($db.Engine) $($db.EngineVersion)"
Write-Host "Status: $($db.DBInstanceStatus)"
Write-Host "VPC ID: $($db.DBSubnetGroup.VpcId)`n"

# Subnet 정보 가져오기
$vpcId = $db.DBSubnetGroup.VpcId
$subnetIds = $db.DBSubnetGroup.Subnets | ForEach-Object { $_.SubnetIdentifier }

Write-Host "========================================" -ForegroundColor Green
Write-Host "Subnet Configuration" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host "Subnet Group: $($db.DBSubnetGroup.DBSubnetGroupName)"
Write-Host "Subnets ($(($subnetIds).Count)):"

foreach ($subnetId in $subnetIds) {
    $subnetInfo = aws ec2 describe-subnets --subnet-ids $subnetId --query "Subnets[0].[AvailabilityZone,CidrBlock]" --output text
    Write-Host "  - $subnetId ($subnetInfo)"
}

$subnetIdsString = $subnetIds -join ','
Write-Host "`nLAMBDA_SUBNET_IDS: $subnetIdsString" -ForegroundColor Cyan

# Security Group 정보 가져오기
$securityGroupIds = $db.VpcSecurityGroups | ForEach-Object { $_.VpcSecurityGroupId }

Write-Host "`n========================================" -ForegroundColor Green
Write-Host "Security Group Configuration" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host "Security Groups ($(($securityGroupIds).Count)):"

foreach ($sgId in $securityGroupIds) {
    $sgInfo = aws ec2 describe-security-groups --group-ids $sgId --query "SecurityGroups[0].[GroupName,Description]" --output text
    Write-Host "  - $sgId ($sgInfo)"
}

$securityGroupIdsString = $securityGroupIds -join ','
Write-Host "`nLAMBDA_SECURITY_GROUPS: $securityGroupIdsString" -ForegroundColor Cyan

# Security Group 규칙 확인
Write-Host "`n========================================" -ForegroundColor Yellow
Write-Host "Security Group Rules Check" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Yellow

$hasLambdaAccess = $false

foreach ($sgId in $securityGroupIds) {
    $rules = aws ec2 describe-security-groups --group-ids $sgId --query "SecurityGroups[0].IpPermissions" --output json | ConvertFrom-Json

    foreach ($rule in $rules) {
        if ($rule.FromPort -eq 3306 -or $rule.ToPort -eq 3306) {
            Write-Host "✓ Found MySQL port (3306) rule in $sgId" -ForegroundColor Green

            # 소스 확인
            if ($rule.IpRanges) {
                foreach ($range in $rule.IpRanges) {
                    Write-Host "  Source CIDR: $($range.CidrIp)" -ForegroundColor Gray
                }
            }
            if ($rule.UserIdGroupPairs) {
                foreach ($pair in $rule.UserIdGroupPairs) {
                    Write-Host "  Source SG: $($pair.GroupId)" -ForegroundColor Gray
                }
            }

            $hasLambdaAccess = $true
        }
    }
}

if (-not $hasLambdaAccess) {
    Write-Host "⚠ WARNING: No MySQL (3306) inbound rule found!" -ForegroundColor Red
    Write-Host "  Lambda functions will not be able to connect to RDS." -ForegroundColor Red
    Write-Host "  Add an inbound rule allowing port 3306 from the Lambda security group." -ForegroundColor Yellow
}

# deploy-config.json 생성 제안
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Generated Configuration" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$config = @{
    "LAMBDA_SUBNET_IDS" = $subnetIdsString
    "LAMBDA_SECURITY_GROUPS" = $securityGroupIdsString
    "DB_HOST" = $db.Endpoint.Address
    "DB_PASSWORD" = "YOUR_SECURE_PASSWORD_HERE"
    "DB_USER" = $db.MasterUsername
    "DB_NAME" = $db.DBName
} | ConvertTo-Json -Depth 10

Write-Host $config

$outputFile = Join-Path $PSScriptRoot "deploy-config.json"

if (Test-Path $outputFile) {
    Write-Host "`n⚠ deploy-config.json already exists!" -ForegroundColor Yellow
    $overwrite = Read-Host "Overwrite? (y/N)"

    if ($overwrite -eq 'y' -or $overwrite -eq 'Y') {
        $config | Out-File -FilePath $outputFile -Encoding UTF8
        Write-Host "✓ Saved to: $outputFile" -ForegroundColor Green
    } else {
        Write-Host "Not saved. You can manually copy the config above." -ForegroundColor Yellow
    }
} else {
    $save = Read-Host "`nSave to deploy-config.json? (Y/n)"

    if ($save -ne 'n' -and $save -ne 'N') {
        $config | Out-File -FilePath $outputFile -Encoding UTF8
        Write-Host "✓ Saved to: $outputFile" -ForegroundColor Green
        Write-Host "`nNext steps:" -ForegroundColor Cyan
        Write-Host "  1. Edit deploy-config.json and set DB_PASSWORD"
        Write-Host "  2. Run: .\Deploy-Unified-Platform.ps1"
    } else {
        Write-Host "Not saved. You can manually create deploy-config.json with the config above." -ForegroundColor Yellow
    }
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Configuration lookup complete!" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan
