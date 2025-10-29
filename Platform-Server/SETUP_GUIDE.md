# HYPERMOB Platform - Setup Guide

## 🔒 Security First!

**NEVER commit credentials to GitHub!**

Files that should NEVER be committed:
- `config.json` (contains database passwords)
- `.env` (contains environment variables)
- Any `*.pem` files
- Any `*.config.json` files

These files are already in `.gitignore` for your protection.

## 📋 Prerequisites

1. **AWS Account** with appropriate permissions
2. **AWS CLI** installed and configured
3. **Docker Desktop** (for AI services)
4. **Node.js 18+**
5. **PowerShell 5.1+** (Windows) or **Bash** (Mac/Linux)
6. **MySQL Client** (optional, for database access)

## 🚀 Step-by-Step Setup

### Step 1: Clone Repository
```bash
git clone https://github.com/your-org/hypermob-platform.git
cd hypermob-platform/Platform-Application
```

### Step 2: Configure Credentials

#### Option A: Using JSON Configuration (Recommended)
```bash
# Copy example configuration
cp config.example.json config.json

# Edit config.json with your actual values
# IMPORTANT: Update the password field!
```

#### Option B: Using Environment Variables
```bash
# Copy example environment file
cp .env.example .env

# Edit .env with your actual values
```

### Step 3: Set up AWS RDS Database

1. Create RDS MySQL instance (if not exists):
```bash
aws rds create-db-instance \
  --db-instance-identifier mydrive3dx \
  --db-instance-class db.t3.micro \
  --engine mysql \
  --master-username admin \
  --master-user-password YOUR_PASSWORD \
  --allocated-storage 20 \
  --publicly-accessible
```

2. Wait for database to be available:
```bash
aws rds wait db-instance-available --db-instance-identifier mydrive3dx
```

3. Get the endpoint:
```bash
aws rds describe-db-instances \
  --db-instance-identifier mydrive3dx \
  --query 'DBInstances[0].Endpoint.Address' \
  --output text
```

4. Update `config.json` with the RDS endpoint

### Step 4: Deploy the Platform

```powershell
# Full deployment (first time)
.\scripts\Deploy-Complete-Platform.ps1

# If database is already set up
.\scripts\Deploy-Complete-Platform.ps1 -SkipDatabase

# If Docker is not available
.\scripts\Deploy-Complete-Platform.ps1 -SkipDocker
```

### Step 5: Verify Deployment

```powershell
# Test all endpoints
.\scripts\Test-ApiEndpoints.ps1

# Test AI services with images
.\scripts\Test-AI-Services.ps1
```

## 🔧 Configuration Reference

### config.json Structure
```json
{
  "aws": {
    "region": "ap-northeast-2",      // AWS region
    "profile": "default"              // AWS CLI profile
  },
  "database": {
    "host": "xxx.rds.amazonaws.com", // RDS endpoint
    "database": "hypermob",          // Database name
    "username": "admin",              // Master username
    "password": "YOUR_PASSWORD",      // ⚠️ UPDATE THIS!
    "port": 3306                     // MySQL port
  },
  "s3": {
    "imagesBucket": "hypermob-images",   // For user images
    "resultsBucket": "hypermob-results"  // For AI results
  }
}
```

## 🐳 Docker Setup (for AI Services)

Platform-AI services require Docker:

1. Install Docker Desktop from https://www.docker.com/products/docker-desktop

2. Start Docker Desktop

3. Login to AWS ECR:
```bash
aws ecr get-login-password --region ap-northeast-2 | \
  docker login --username AWS --password-stdin \
  $(aws sts get-caller-identity --query Account --output text).dkr.ecr.ap-northeast-2.amazonaws.com
```

## 📊 Database Schema

The deployment script automatically creates:
- `users` table - User profiles and authentication
- `vehicles` table - Vehicle information
- `bookings` table - Booking records
- `sessions` table - Active sessions
- `ai_analysis_results` table - AI processing results
- `vehicle_settings_history` table - Personalized settings

To manually initialize:
```bash
mysql -h YOUR_RDS_HOST -u admin -p < database/init-database.sql
```

## 🧪 Testing

### Test with provided images:
```powershell
# Images are in assets/ folder
# - face.jpg (face image for registration)
# - body.jpg (full body image for measurements)

.\scripts\Test-AI-Services.ps1
```

### Manual API testing:
```bash
# Register user with face
curl -X POST https://YOUR_API_ID.execute-api.ap-northeast-2.amazonaws.com/v1/auth/face \
  -F "image=@assets/face.jpg"

# Upload body image
curl -X POST https://YOUR_API_ID.execute-api.ap-northeast-2.amazonaws.com/v1/body/upload \
  -F "image=@assets/body.jpg" \
  -F "user_id=USER_ID"
```

## ❗ Common Issues

### Issue: "Configuration file not found"
**Solution**: Copy `config.example.json` to `config.json` and update values

### Issue: "Docker is not running"
**Solution**: Start Docker Desktop application

### Issue: "Access denied" from RDS
**Solution**: Ensure RDS security group allows your IP

### Issue: Lambda timeout
**Solution**: RDS must be publicly accessible (not in VPC)

### Issue: "hypermob-results bucket not found"
**Solution**: The deployment script creates it automatically

## 📝 Environment Variables for Lambda

These are automatically set by the deployment script:
- `DB_HOST` - RDS endpoint
- `DB_NAME` - Database name (hypermob)
- `DB_USER` - Database username
- `DB_PASSWORD` - Database password
- `S3_BUCKET` - Image storage bucket

## 🔐 Security Best Practices

1. **Use IAM roles** instead of access keys when possible
2. **Rotate database passwords** regularly
3. **Use AWS Secrets Manager** for production
4. **Enable RDS encryption** at rest
5. **Use VPC endpoints** for S3 access (production)
6. **Enable CloudTrail** for audit logging

## 📚 Additional Resources

- [AWS Lambda Documentation](https://docs.aws.amazon.com/lambda/)
- [API Gateway Documentation](https://docs.aws.amazon.com/apigateway/)
- [RDS MySQL Documentation](https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/CHAP_MySQL.html)
- [Docker Documentation](https://docs.docker.com/)

## 🤝 Support

For issues or questions:
- Check deployment logs: `aws logs tail /aws/lambda/hypermob-* --follow`
- Review API Gateway logs
- Open an issue on GitHub
- Contact: dev@hypermob.com