# Deploy body-upload Lambda function

$ErrorActionPreference = "Stop"

Write-Host "Creating hypermob-body-upload Lambda function..." -ForegroundColor Cyan

aws lambda create-function `
  --function-name hypermob-body-upload `
  --runtime nodejs18.x `
  --role arn:aws:iam::471464546082:role/hypermob-lambda-execution-role `
  --handler index.handler `
  --timeout 30 `
  --memory-size 256 `
  --environment "Variables={DB_HOST=mydrive3dx.climsg8a689n.ap-northeast-2.rds.amazonaws.com,DB_USER=admin,DB_PASSWORD=admin123,DB_NAME=hypermob,S3_BUCKET=hypermob-images}" `
  --layers arn:aws:lambda:ap-northeast-2:471464546082:layer:hypermob-shared-layer:4 `
  --zip-file fileb://P:/HYPERMOB/Platform-Application/lambdas/body-upload/function.zip `
  --region ap-northeast-2 `
  --vpc-config SubnetIds=subnet-0a8242768daf4f1cd,subnet-052214c4ce88f39ad,subnet-00d05ae5ebfc8016f,subnet-0fcd77e57b9b2c8e9,SecurityGroupIds=sg-0e8a3b10d44f6f001

if ($LASTEXITCODE -eq 0) {
    Write-Host "Lambda function created successfully!" -ForegroundColor Green
} else {
    Write-Host "Failed to create Lambda function" -ForegroundColor Red
    exit 1
}
