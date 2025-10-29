# HYPERMOB Platform Server

[![AWS Lambda](https://img.shields.io/badge/AWS-Lambda-orange.svg)](https://aws.amazon.com/lambda/)
[![Node.js](https://img.shields.io/badge/Node.js-18+-green.svg)](https://nodejs.org/)
[![Python](https://img.shields.io/badge/Python-3.9+-blue.svg)](https://www.python.org/)
[![MySQL](https://img.shields.io/badge/MySQL-8.0-blue.svg)](https://www.mysql.com/)

Complete mobility platform with AI-powered personalization, face/NFC authentication, and intelligent vehicle control.

## Features

- **Face Recognition**: Register and authenticate users with face images
- **Body Measurements**: AI-powered body dimension analysis
- **Intelligent Recommendations**: Personalized vehicle settings based on user measurements
- **NFC Authentication**: Secure vehicle access with NFC tags
- **Real-time Vehicle Control**: Control vehicle settings via REST API
- **Booking Management**: Handle vehicle reservations

## Architecture

```
┌─────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│   API Gateway   │────▶│  Lambda Functions │────▶│   RDS MySQL     │
│   (REST API)    │     │  (Node.js/Python) │     │   (Database)    │
└─────────────────┘     └──────────────────┘     └─────────────────┘
                               │
                               ▼
                        ┌──────────────────┐
                        │   S3 Buckets     │
                        │  (Images/Results)│
                        └──────────────────┘
```

### Components

- **Platform-Application**: Node.js Lambda functions for core business logic
- **Platform-AI**: Python Docker-based Lambda functions for AI services
  - Face landmark detection (468 points)
  - Body measurement analysis
  - Vehicle setting recommendations
- **Database**: AWS RDS MySQL for persistent data storage
- **Storage**: S3 for images and AI processing results
- **API**: AWS API Gateway REST API with regional endpoint

## Prerequisites

- **AWS CLI** 2.0+ configured with appropriate credentials
- **Docker Desktop** (required for AI Lambda functions)
- **Node.js** 18.x or higher
- **PowerShell** 5.1+ (Windows) or PowerShell Core 7+ (cross-platform)
- **MySQL Client** (optional, for direct database access)
- **Git** for version control

## Configuration Setup

### 1. Create Configuration File
```bash
# Copy the example configuration
cp config.example.json config.json

# Edit config.json with your settings:
# - Database credentials
# - AWS region
# - S3 bucket names (optional, defaults provided)
```

**Important**: Never commit `config.json` to GitHub! It's in `.gitignore` for security.

## Quick Start

### 1. Complete Deployment (From Scratch)
```powershell
# First, setup your config.json file!

# Deploy everything from clean state
.\scripts\Deploy-Complete-Platform.ps1

# Use custom config file
.\scripts\Deploy-Complete-Platform.ps1 -ConfigFile "my-config.json"

# Skip database initialization if already setup
.\scripts\Deploy-Complete-Platform.ps1 -SkipDatabase

# Skip Docker-based AI functions if Docker not available
.\scripts\Deploy-Complete-Platform.ps1 -SkipDocker
```

### 2. Database Setup Only
```bash
# Initialize database schema
mysql -h <RDS_HOST> -u admin -p < database\init-database.sql
```

### 3. Test Deployment
```powershell
# Test all API endpoints
.\scripts\Test-ApiEndpoints.ps1

# Test AI services with image upload
.\scripts\Test-AI-Services.ps1
```

## API Endpoints

Base URL: `https://dfhab2ellk.execute-api.ap-northeast-2.amazonaws.com/v1`

### Authentication
- `POST /auth/face` - Register user with face image
- `GET /auth/nfc` - Get NFC UID for user
- `POST /auth/nfc/verify` - Verify NFC authentication
- `POST /auth/session` - Create session
- `POST /auth/result` - Record auth result

### Body Upload
- `POST /body/upload` - Upload body image for measurements

### AI Services (Platform-AI)
- `POST /facedata` - Extract face landmarks (468 points)
- `POST /measure` - Measure body dimensions
- `POST /recommend` - Get vehicle setting recommendations

### Vehicle & Bookings
- `POST /bookings` - Manage bookings
- `POST /vehicle/control` - Control vehicle
- `POST /vehicle/status` - Get vehicle status

## Complete Flow Example

1. **Register User**
```bash
curl -X POST https://.../v1/auth/face \
  -F "image=@face.jpg"
```

2. **Upload Body Image**
```bash
curl -X POST https://.../v1/body/upload \
  -F "image=@body.jpg" \
  -F "user_id=USER_ID"
```

3. **Get Measurements**
```bash
curl -X POST https://.../v1/measure \
  -H "Content-Type: application/json" \
  -d '{
    "image_url": "S3_URL",
    "height_cm": 177.7,
    "user_id": "USER_ID"
  }'
```

4. **Get Vehicle Settings**
```bash
curl -X POST https://.../v1/recommend \
  -H "Content-Type: application/json" \
  -d '{
    "user_id": "USER_ID",
    "height": 177.7,
    "upper_arm": 32.6,
    "forearm": 27.0,
    "thigh": 39.4,
    "calf": 28.2,
    "torso": 61.0
  }'
```

## Project Structure

```
Platform-Server/
├── .gitignore                          # Git ignore rules
├── config.example.json                 # Configuration template
├── .env.example                        # Environment variables template
│
├── database/
│   ├── init-database.sql               # Complete database schema
│   └── legacy/                         # Previous schema versions
│
├── docs/
│   ├── openapi.yaml                    # OpenAPI 3.0 specification
│   ├── DEPLOYMENT_GUIDE.md             # Detailed deployment guide
│   ├── SETUP_GUIDE.md                  # Setup instructions
│   └── legacy/                         # Previous documentation
│
├── lambdas/
│   ├── auth-face/                      # Face registration endpoint
│   ├── auth-nfc/                       # NFC authentication
│   ├── auth-session/                   # Session management
│   ├── body-upload/                    # Body image upload
│   ├── bookings/                       # Booking management
│   ├── vehicle-control/                # Vehicle control endpoint
│   ├── vehicle-status/                 # Vehicle status endpoint
│   ├── layer_build/                    # Shared Lambda layer
│   └── ...                             # Other Lambda functions
│
├── scripts/
│   ├── Deploy-Complete-Platform.ps1    # Main deployment script
│   ├── Test-ApiEndpoints.ps1           # API integration tests
│   ├── Test-AI-Services.ps1            # AI service tests
│   ├── Debug-Lambda.ps1                # Lambda debugging utilities
│   ├── layer-build/                    # Layer building utilities
│   └── legacy/                         # Previous scripts
│
└── assets/
    ├── face.jpg                        # Sample face image for testing
    └── body.jpg                        # Sample body image for testing
```

## Configuration

### Environment Variables

Lambda functions use the following environment variables:

| Variable | Description | Example |
|----------|-------------|---------|
| `DB_HOST` | RDS endpoint | `hypermob.xxxxx.ap-northeast-2.rds.amazonaws.com` |
| `DB_NAME` | Database name | `hypermob` |
| `DB_USER` | Database username | `admin` |
| `DB_PASSWORD` | Database password | `your-secure-password` |
| `S3_BUCKET` | Image storage bucket | `hypermob-images` |

### AWS Resources

The deployment creates the following AWS resources:

| Resource Type | Name | Purpose |
|--------------|------|---------|
| **S3 Buckets** | `hypermob-images` | Store uploaded face/body images |
| | `hypermob-results` | Store AI processing results |
| **Lambda Functions** | 11 functions total | 8 Node.js + 3 Docker-based AI functions |
| **Lambda Layer** | `hypermob-shared-layer` | Shared Node.js dependencies (mysql2, AWS SDK) |
| **API Gateway** | `HYPERMOB-Unified-API` | REST API regional endpoint |
| **IAM Role** | `hypermob-lambda-execution-role` | Lambda execution role with S3/RDS/Logs permissions |
| **ECR Repositories** | 3 repositories | Docker images for AI Lambda functions |
| **RDS MySQL** | External | Database (configured separately) |

## Troubleshooting

### Lambda Timeout Issues

**Problem**: Lambda functions timing out
**Solution**:
- Functions are configured without VPC for faster cold starts
- RDS must be publicly accessible
- Default timeout is 60 seconds
- Check security group rules allow Lambda IP ranges

### Docker Issues

**Problem**: Docker build fails
**Solution**:
```bash
# Ensure Docker Desktop is running
docker ps

# Login to ECR
aws ecr get-login-password --region ap-northeast-2 | docker login --username AWS --password-stdin <account-id>.dkr.ecr.ap-northeast-2.amazonaws.com

# Test Docker build locally
cd lambdas/facedata
docker build -t test-build .
```

### Database Connection Errors

**Problem**: Cannot connect to RDS
**Solution**:
- Verify RDS security group allows inbound traffic on port 3306
- Ensure RDS is publicly accessible (or use VPC configuration)
- Check Lambda environment variables match RDS credentials
- Test connection: `mysql -h <RDS_HOST> -u admin -p`

### AI Services Errors

**Problem**: AI processing fails
**Solution**:
- Verify `hypermob-results` S3 bucket exists
- Check Lambda execution role has S3 permissions
- Ensure Docker images are properly built and pushed to ECR
- Check Lambda CloudWatch logs for detailed error messages
- Verify input image format (JPEG/PNG, max 6MB)

### Common Error Messages

| Error | Cause | Solution |
|-------|-------|----------|
| `ER_ACCESS_DENIED_ERROR` | Invalid DB credentials | Update Lambda environment variables |
| `ECONNREFUSED` | RDS not accessible | Check security groups and public access |
| `NoSuchBucket` | S3 bucket missing | Run deployment script to create buckets |
| `Container image not found` | ECR image missing | Build and push Docker images |

## Development

### Running Tests Locally

```powershell
# Test all API endpoints
.\scripts\Test-ApiEndpoints.ps1

# Test AI services with sample images
.\scripts\Test-AI-Services.ps1

# Debug specific Lambda function
.\scripts\Debug-Lambda.ps1 -FunctionName hypermob-auth-face
```

### Building Lambda Layer

```powershell
cd scripts/layer-build
npm install
# Layer is automatically packaged as layer.zip
```

### Updating Lambda Functions

```powershell
# Update specific function
cd lambdas/auth-face
zip -r function.zip .
aws lambda update-function-code --function-name hypermob-auth-face --zip-file fileb://function.zip

# Or use deployment script
.\scripts\Deploy-Complete-Platform.ps1
```

## Security Considerations

- Never commit `config.json` or `.env` files (excluded in `.gitignore`)
- Use AWS Secrets Manager for production credentials
- Enable CloudTrail for API audit logging
- Implement API Gateway throttling and quotas
- Use HTTPS only (enforced by API Gateway)
- Regularly rotate database credentials
- Review IAM role permissions (principle of least privilege)

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is proprietary and confidential. All rights reserved.

## Support

For issues, questions, or feature requests:
- Check CloudWatch logs: `aws logs tail /aws/lambda/hypermob-* --follow`
- Review API Gateway execution logs
- Consult `docs/DEPLOYMENT_GUIDE.md` for detailed setup
- Contact: dev@hypermob.com