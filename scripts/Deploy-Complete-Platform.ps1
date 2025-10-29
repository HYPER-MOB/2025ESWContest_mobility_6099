###############################################################################
# HYPERMOB Complete Platform Deployment Script v2.0
# Description: Deploys entire HYPERMOB platform from scratch
# Requirements: AWS CLI, Docker, Node.js, PowerShell 5.1+
###############################################################################

param(
    [string]$ConfigFile = "config.json",
    [switch]$SkipDatabase = $false,
    [switch]$SkipDocker = $false
)

$ErrorActionPreference = "Stop"

# Load configuration
function Load-Configuration {
    $configPath = Join-Path (Split-Path $PSScriptRoot) $ConfigFile

    if (!(Test-Path $configPath)) {
        Write-Host "ERROR: Configuration file not found: $configPath" -ForegroundColor Red
        Write-Host "Please copy config.example.json to config.json and update with your settings." -ForegroundColor Yellow
        exit 1
    }

    try {
        $config = Get-Content $configPath | ConvertFrom-Json
        return $config
    } catch {
        Write-Host "ERROR: Failed to parse configuration file: $_" -ForegroundColor Red
        exit 1
    }
}

# Load config
$configData = Load-Configuration

# Configuration
$script:Config = @{
    Region = $configData.aws.region
    Profile = $configData.aws.profile
    AccountId = $null
    Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"

    # RDS Configuration
    RDS = @{
        Host = $configData.database.host
        Database = $configData.database.database
        Username = $configData.database.username
        Password = $configData.database.password
        Port = $configData.database.port
    }

    # S3 Buckets
    S3 = @{
        ImagesBucket = $configData.s3.imagesBucket
        ResultsBucket = $configData.s3.resultsBucket
    }

    # Lambda Configuration
    Lambda = @{
        ExecutionRoleName = $configData.lambda.executionRoleName
        LayerName = $configData.lambda.layerName
        Timeout = $configData.lambda.timeout
        MemorySize = $configData.lambda.memorySize
    }

    # API Gateway
    APIGateway = @{
        Name = $configData.apiGateway.name
        Stage = $configData.apiGateway.stage
    }
}

function Write-Log {
    param(
        [string]$Message,
        [string]$Level = "INFO"
    )

    $color = switch($Level) {
        "SUCCESS" { "Green" }
        "ERROR" { "Red" }
        "WARNING" { "Yellow" }
        "INFO" { "Cyan" }
        default { "White" }
    }

    Write-Host "[$Level] $Message" -ForegroundColor $color
}

function Test-Prerequisites {
    Write-Log "Checking prerequisites..."

    # Check AWS CLI
    if (!(Get-Command aws -ErrorAction SilentlyContinue)) {
        throw "AWS CLI is not installed"
    }

    # Check Docker (if not skipped)
    if (!$SkipDocker) {
        if (!(Get-Command docker -ErrorAction SilentlyContinue)) {
            throw "Docker is not installed"
        }

        # Test Docker
        docker ps 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "Docker is not running"
        }
    }

    # Check Node.js
    if (!(Get-Command node -ErrorAction SilentlyContinue)) {
        throw "Node.js is not installed"
    }

    # Get AWS Account ID
    $script:Config.AccountId = aws sts get-caller-identity --query Account --output text --region $Config.Region
    Write-Log "AWS Account ID: $($script:Config.AccountId)" "SUCCESS"
}

function Initialize-Database {
    if ($SkipDatabase) {
        Write-Log "Skipping database initialization" "WARNING"
        return
    }

    Write-Log "Initializing database..."

    $sqlFile = Join-Path (Split-Path $PSScriptRoot) "database\init-database.sql"
    if (!(Test-Path $sqlFile)) {
        throw "Database initialization file not found: $sqlFile"
    }

    # Execute SQL file using mysql command
    $mysqlCmd = "mysql -h $($Config.RDS.Host) -u $($Config.RDS.Username) -p$($Config.RDS.Password) < `"$sqlFile`""

    Write-Log "Executing database initialization script..."
    cmd /c $mysqlCmd 2>&1

    if ($LASTEXITCODE -eq 0) {
        Write-Log "Database initialized successfully" "SUCCESS"
    } else {
        Write-Log "Database initialization completed with warnings" "WARNING"
    }
}

function Create-S3Buckets {
    Write-Log "Creating S3 buckets..."

    foreach ($bucket in @($Config.S3.ImagesBucket, $Config.S3.ResultsBucket)) {
        $exists = aws s3api head-bucket --bucket $bucket --region $AwsRegion 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Log "Creating bucket: $bucket"
            aws s3api create-bucket `
                --bucket $bucket `
                --region $AwsRegion `
                --create-bucket-configuration LocationConstraint=$AwsRegion | Out-Null

            # Enable public read for images bucket
            if ($bucket -eq $Config.S3.ImagesBucket) {
                $policy = @"
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "PublicReadGetObject",
            "Effect": "Allow",
            "Principal": "*",
            "Action": "s3:GetObject",
            "Resource": "arn:aws:s3:::$bucket/*"
        }
    ]
}
"@
                $policy | Out-File -FilePath "$env:TEMP\bucket-policy.json" -Encoding utf8
                aws s3api put-bucket-policy --bucket $bucket --policy file://$env:TEMP/bucket-policy.json --region $AwsRegion
                Remove-Item "$env:TEMP\bucket-policy.json" -Force
            }

            Write-Log "Bucket created: $bucket" "SUCCESS"
        } else {
            Write-Log "Bucket already exists: $bucket" "INFO"
        }
    }
}

function Create-IAMRole {
    Write-Log "Creating IAM execution role..."

    $roleExists = aws iam get-role --role-name $Config.Lambda.ExecutionRoleName 2>&1
    if ($LASTEXITCODE -ne 0) {
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
        $trustPolicy | Out-File -FilePath "$env:TEMP\trust-policy.json" -Encoding utf8

        aws iam create-role `
            --role-name $Config.Lambda.ExecutionRoleName `
            --assume-role-policy-document file://$env:TEMP/trust-policy.json | Out-Null

        # Attach policies
        aws iam attach-role-policy `
            --role-name $Config.Lambda.ExecutionRoleName `
            --policy-arn "arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole"

        aws iam attach-role-policy `
            --role-name $Config.Lambda.ExecutionRoleName `
            --policy-arn "arn:aws:iam::aws:policy/AmazonS3FullAccess"

        # Add RDS access policy
        $rdsPolicy = @"
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "rds:Describe*",
                "rds-db:connect"
            ],
            "Resource": "*"
        }
    ]
}
"@
        $rdsPolicy | Out-File -FilePath "$env:TEMP\rds-policy.json" -Encoding utf8

        aws iam put-role-policy `
            --role-name $Config.Lambda.ExecutionRoleName `
            --policy-name "RDSAccessPolicy" `
            --policy-document file://$env:TEMP/rds-policy.json

        Remove-Item "$env:TEMP\*.json" -Force

        Write-Log "IAM role created" "SUCCESS"
        Start-Sleep -Seconds 10  # Wait for role to be available
    } else {
        Write-Log "IAM role already exists" "INFO"
    }
}

function Build-LambdaLayer {
    Write-Log "Building Lambda layer..."

    $layerPath = Join-Path (Split-Path $PSScriptRoot) "lambdas\layer_build"
    Push-Location $layerPath

    try {
        # Install dependencies
        Write-Log "Installing layer dependencies..."
        Push-Location nodejs
        npm install --production
        Pop-Location

        # Package layer
        Write-Log "Packaging layer..."
        Compress-Archive -Path nodejs -DestinationPath layer.zip -Force

        # Publish layer
        $layerArn = aws lambda publish-layer-version `
            --layer-name $Config.Lambda.LayerName `
            --description "Shared dependencies for HYPERMOB Lambda functions" `
            --compatible-runtimes nodejs18.x `
            --zip-file fileb://layer.zip `
            --region $AwsRegion `
            --query LayerVersionArn `
            --output text

        $script:Config.Lambda.LayerArn = $layerArn
        Write-Log "Lambda layer published: $layerArn" "SUCCESS"

        Remove-Item layer.zip -Force
    } finally {
        Pop-Location
    }
}

function Deploy-ApplicationLambdas {
    Write-Log "Deploying Platform-Application Lambda functions..."

    $lambdasPath = Join-Path (Split-Path $PSScriptRoot) "lambdas"
    $roleArn = "arn:aws:iam::$($Config.AccountId):role/$($Config.Lambda.ExecutionRoleName)"

    $functions = @(
        "auth-face",
        "auth-nfc-get",
        "auth-session",
        "auth-result",
        "auth-nfc-verify",
        "body-upload",
        "bookings",
        "vehicle-control"
    )

    foreach ($func in $functions) {
        $funcPath = Join-Path $lambdasPath $func

        if (!(Test-Path $funcPath)) {
            Write-Log "Function path not found: $funcPath" "WARNING"
            continue
        }

        Write-Log "Deploying function: hypermob-$func"

        Push-Location $funcPath
        try {
            # Create deployment package
            Compress-Archive -Path "*.js" -DestinationPath "function.zip" -Force

            # Check if function exists
            $exists = aws lambda get-function --function-name "hypermob-$func" --region $AwsRegion 2>&1

            if ($LASTEXITCODE -eq 0) {
                # Update function
                aws lambda update-function-code `
                    --function-name "hypermob-$func" `
                    --zip-file fileb://function.zip `
                    --region $AwsRegion | Out-Null

                aws lambda update-function-configuration `
                    --function-name "hypermob-$func" `
                    --layers $Config.Lambda.LayerArn `
                    --timeout $Config.Lambda.Timeout `
                    --memory-size $Config.Lambda.MemorySize `
                    --environment "Variables={DB_HOST=$($Config.RDS.Host),DB_NAME=$($Config.RDS.Database),DB_USER=$($Config.RDS.Username),DB_PASSWORD=$($Config.RDS.Password),S3_BUCKET=$($Config.S3.ImagesBucket)}" `
                    --region $AwsRegion | Out-Null
            } else {
                # Create function
                aws lambda create-function `
                    --function-name "hypermob-$func" `
                    --runtime nodejs18.x `
                    --role $roleArn `
                    --handler index.handler `
                    --layers $Config.Lambda.LayerArn `
                    --timeout $Config.Lambda.Timeout `
                    --memory-size $Config.Lambda.MemorySize `
                    --environment "Variables={DB_HOST=$($Config.RDS.Host),DB_NAME=$($Config.RDS.Database),DB_USER=$($Config.RDS.Username),DB_PASSWORD=$($Config.RDS.Password),S3_BUCKET=$($Config.S3.ImagesBucket)}" `
                    --zip-file fileb://function.zip `
                    --region $AwsRegion | Out-Null
            }

            Remove-Item function.zip -Force
            Write-Log "Function deployed: hypermob-$func" "SUCCESS"
        } finally {
            Pop-Location
        }
    }
}

function Deploy-AILambdas {
    if ($SkipDocker) {
        Write-Log "Skipping Platform-AI Lambda deployment (Docker required)" "WARNING"
        return
    }

    Write-Log "Deploying Platform-AI Lambda functions (Docker)..."

    # Login to ECR
    $ecrPassword = aws ecr get-login-password --region $AwsRegion
    $ecrPassword | docker login --username AWS --password-stdin "$($Config.AccountId).dkr.ecr.$AwsRegion.amazonaws.com"

    $aiPath = Join-Path (Split-Path (Split-Path $PSScriptRoot)) "Platform-AI\lambdas"
    $roleArn = "arn:aws:iam::$($Config.AccountId):role/$($Config.Lambda.ExecutionRoleName)"

    $aiFunctions = @("facedata", "measure", "recommend")

    foreach ($func in $aiFunctions) {
        $funcPath = Join-Path $aiPath $func

        if (!(Test-Path $funcPath)) {
            Write-Log "AI function path not found: $funcPath" "WARNING"
            continue
        }

        Write-Log "Building and deploying: hypermob-$func"

        Push-Location $funcPath
        try {
            # Create ECR repository if needed
            $repoExists = aws ecr describe-repositories --repository-names "hypermob-$func" --region $AwsRegion 2>&1
            if ($LASTEXITCODE -ne 0) {
                aws ecr create-repository --repository-name "hypermob-$func" --region $AwsRegion | Out-Null
            }

            # Build Docker image
            $imageUri = "$($Config.AccountId).dkr.ecr.$AwsRegion.amazonaws.com/hypermob-$func:latest"
            docker build -t "hypermob-$func" .
            docker tag "hypermob-$func:latest" $imageUri
            docker push $imageUri

            # Check if function exists
            $exists = aws lambda get-function --function-name "hypermob-$func" --region $AwsRegion 2>&1

            if ($LASTEXITCODE -eq 0) {
                # Update function
                aws lambda update-function-code `
                    --function-name "hypermob-$func" `
                    --image-uri $imageUri `
                    --region $AwsRegion | Out-Null
            } else {
                # Create function
                aws lambda create-function `
                    --function-name "hypermob-$func" `
                    --package-type Image `
                    --code ImageUri=$imageUri `
                    --role $roleArn `
                    --timeout 60 `
                    --memory-size 2048 `
                    --environment "Variables={DB_HOST=$($Config.RDS.Host),DB_NAME=$($Config.RDS.Database),DB_USER=$($Config.RDS.Username),DB_PASSWORD=$($Config.RDS.Password)}" `
                    --region $AwsRegion | Out-Null
            }

            Write-Log "AI function deployed: hypermob-$func" "SUCCESS"
        } finally {
            Pop-Location
        }
    }
}

function Deploy-APIGateway {
    Write-Log "Deploying API Gateway..."

    # Check if API exists
    $apiId = aws apigateway get-rest-apis `
        --query "items[?name=='$($Config.APIGateway.Name)'].id" `
        --output text `
        --region $AwsRegion

    if (!$apiId) {
        # Create new API
        $apiId = aws apigateway create-rest-api `
            --name $Config.APIGateway.Name `
            --description "HYPERMOB Unified Platform API" `
            --endpoint-configuration types=REGIONAL `
            --query id `
            --output text `
            --region $AwsRegion

        Write-Log "API Gateway created: $apiId" "SUCCESS"
    } else {
        Write-Log "API Gateway already exists: $apiId" "INFO"
    }

    $script:Config.APIGateway.ApiId = $apiId

    # Get root resource
    $rootId = aws apigateway get-resources `
        --rest-api-id $apiId `
        --query "items[?path=='/'].id" `
        --output text `
        --region $AwsRegion

    # Define endpoints
    $endpoints = @(
        @{ Path = "auth"; SubPaths = @("face", "nfc", "session", "result") }
        @{ Path = "body"; SubPaths = @("upload") }
        @{ Path = "bookings"; SubPaths = @() }
        @{ Path = "vehicle"; SubPaths = @("control", "status") }
        @{ Path = "facedata"; SubPaths = @() }
        @{ Path = "measure"; SubPaths = @() }
        @{ Path = "recommend"; SubPaths = @() }
    )

    # Create resources and integrations
    foreach ($endpoint in $endpoints) {
        # Create parent resource
        $parentId = aws apigateway get-resources `
            --rest-api-id $apiId `
            --query "items[?pathPart=='$($endpoint.Path)'].id" `
            --output text `
            --region $AwsRegion

        if (!$parentId) {
            $parentId = aws apigateway create-resource `
                --rest-api-id $apiId `
                --parent-id $rootId `
                --path-part $endpoint.Path `
                --query id `
                --output text `
                --region $AwsRegion
        }

        # Handle endpoints without subpaths
        if ($endpoint.SubPaths.Count -eq 0) {
            $funcName = "hypermob-$($endpoint.Path)"
            Create-APIMethod -ApiId $apiId -ResourceId $parentId -FunctionName $funcName -HttpMethod "POST"
        } else {
            # Create sub-resources
            foreach ($subPath in $endpoint.SubPaths) {
                $subId = aws apigateway get-resources `
                    --rest-api-id $apiId `
                    --query "items[?pathPart=='$subPath' && parentId=='$parentId'].id" `
                    --output text `
                    --region $AwsRegion

                if (!$subId) {
                    $subId = aws apigateway create-resource `
                        --rest-api-id $apiId `
                        --parent-id $parentId `
                        --path-part $subPath `
                        --query id `
                        --output text `
                        --region $AwsRegion
                }

                $funcName = if ($endpoint.Path -eq "auth" -and $subPath -eq "nfc") {
                    "hypermob-auth-nfc-get"
                } else {
                    "hypermob-$($endpoint.Path)-$subPath"
                }

                $httpMethod = if ($subPath -eq "nfc") { "GET" } else { "POST" }
                Create-APIMethod -ApiId $apiId -ResourceId $subId -FunctionName $funcName -HttpMethod $httpMethod
            }
        }
    }

    # Deploy API
    Write-Log "Deploying API to stage: $($Config.APIGateway.Stage)"
    aws apigateway create-deployment `
        --rest-api-id $apiId `
        --stage-name $Config.APIGateway.Stage `
        --description "Deployment $($Config.Timestamp)" `
        --region $AwsRegion | Out-Null

    $apiUrl = "https://$apiId.execute-api.$AwsRegion.amazonaws.com/$($Config.APIGateway.Stage)"
    Write-Log "API Gateway deployed: $apiUrl" "SUCCESS"

    # Save configuration
    $configOutput = @{
        ApiId = $apiId
        ApiUrl = $apiUrl
        Region = $AwsRegion
        Timestamp = $Config.Timestamp
    } | ConvertTo-Json -Depth 3

    $configOutput | Out-File -FilePath "$PSScriptRoot\deployment-config.json" -Encoding utf8
}

function Create-APIMethod {
    param(
        [string]$ApiId,
        [string]$ResourceId,
        [string]$FunctionName,
        [string]$HttpMethod
    )

    # Check if method exists
    $methodExists = aws apigateway get-method `
        --rest-api-id $ApiId `
        --resource-id $ResourceId `
        --http-method $HttpMethod `
        --region $AwsRegion 2>&1

    if ($LASTEXITCODE -ne 0) {
        # Create method
        aws apigateway put-method `
            --rest-api-id $ApiId `
            --resource-id $ResourceId `
            --http-method $HttpMethod `
            --authorization-type NONE `
            --region $AwsRegion | Out-Null

        # Create integration
        $functionArn = "arn:aws:lambda:$($AwsRegion):$($Config.AccountId):function:$FunctionName"
        aws apigateway put-integration `
            --rest-api-id $ApiId `
            --resource-id $ResourceId `
            --http-method $HttpMethod `
            --type AWS_PROXY `
            --integration-http-method POST `
            --uri "arn:aws:apigateway:$($AwsRegion):lambda:path/2015-03-31/functions/$functionArn/invocations" `
            --region $AwsRegion | Out-Null

        # Add Lambda permission
        aws lambda add-permission `
            --function-name $FunctionName `
            --statement-id "apigateway-$ApiId-$ResourceId-$HttpMethod" `
            --action lambda:InvokeFunction `
            --principal apigateway.amazonaws.com `
            --source-arn "arn:aws:execute-api:$($AwsRegion):$($Config.AccountId):$ApiId/*/*" `
            --region $AwsRegion 2>&1 | Out-Null
    }
}

# Main execution
try {
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host " HYPERMOB Platform Deployment v2.0" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan

    Test-Prerequisites
    Initialize-Database
    Create-S3Buckets
    Create-IAMRole
    Build-LambdaLayer
    Deploy-ApplicationLambdas
    Deploy-AILambdas
    Deploy-APIGateway

    Write-Host "`n========================================" -ForegroundColor Green
    Write-Host " Deployment Completed Successfully!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green

    Write-Host "`nAPI Endpoint: " -NoNewline
    Write-Host "https://$($Config.APIGateway.ApiId).execute-api.$AwsRegion.amazonaws.com/$($Config.APIGateway.Stage)" -ForegroundColor Yellow

    Write-Host "`nNext Steps:" -ForegroundColor Cyan
    Write-Host "1. Test the deployment: .\Test-ApiEndpoints.ps1" -ForegroundColor White
    Write-Host "2. Test AI services: .\Test-AI-Services.ps1" -ForegroundColor White
    Write-Host "3. View logs: aws logs tail /aws/lambda/hypermob-* --follow" -ForegroundColor White

} catch {
    Write-Log "Deployment failed: $_" "ERROR"
    throw
}