###############################################################################
# S3 버킷 권한 수정 - ACL 없이 공개 읽기 가능하도록 설정
###############################################################################

param(
    [string]$AwsRegion = "ap-northeast-2"
)

Write-Host "`n=========================================" -ForegroundColor Cyan
Write-Host " Fixing S3 Bucket Permissions" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# hypermob-results 버킷 정책 설정
Write-Host "[1/3] Setting bucket policy for hypermob-results..." -ForegroundColor Yellow

$bucketPolicy = @"
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "AllowPublicRead",
      "Effect": "Allow",
      "Principal": "*",
      "Action": "s3:GetObject",
      "Resource": "arn:aws:s3:::hypermob-results/*"
    }
  ]
}
"@

$policyFile = "$env:TEMP\results-bucket-policy.json"
$bucketPolicy | Out-File -FilePath $policyFile -Encoding utf8

aws s3api put-bucket-policy `
    --bucket hypermob-results `
    --policy "file://$policyFile" `
    --region $AwsRegion

Remove-Item $policyFile -ErrorAction SilentlyContinue

Write-Host "[SUCCESS] Bucket policy updated for hypermob-results" -ForegroundColor Green
Write-Host ""

# 공개 액세스 차단 해제
Write-Host "[2/3] Removing public access block..." -ForegroundColor Yellow

try {
    aws s3api delete-public-access-block `
        --bucket hypermob-results `
        --region $AwsRegion 2>$null
    Write-Host "[SUCCESS] Public access block removed" -ForegroundColor Green
}
catch {
    Write-Host "[INFO] No public access block to remove" -ForegroundColor Blue
}

Write-Host ""

# 버킷 정책 확인
Write-Host "[3/3] Verifying bucket policy..." -ForegroundColor Yellow

$currentPolicy = aws s3api get-bucket-policy `
    --bucket hypermob-results `
    --region $AwsRegion `
    --query Policy `
    --output text 2>$null

if ($currentPolicy) {
    Write-Host "[SUCCESS] Bucket policy verified" -ForegroundColor Green
    $currentPolicy | ConvertFrom-Json | ConvertTo-Json -Depth 10
}
else {
    Write-Host "[WARNING] Could not verify bucket policy" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Done!" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Result images will now be publicly accessible via URL" -ForegroundColor Green
Write-Host ""
