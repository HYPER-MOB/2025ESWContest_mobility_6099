/**
 * Lambda: POST /body/upload
 *
 * Body image upload for measurement
 * - Upload full body image (multipart/form-data)
 * - Store image in S3
 * - Return S3 URL for /measure endpoint
 */

const { S3Client, PutObjectCommand } = require('@aws-sdk/client-s3');
const db = require('/opt/nodejs/shared/db');
const {
  createResponse,
  createErrorResponse,
  parseMultipartFormData,
  log,
} = require('/opt/nodejs/shared/utils');

const s3Client = new S3Client({ region: process.env.AWS_REGION || 'ap-northeast-2' });

exports.handler = async (event) => {
  log('INFO', 'POST /body/upload - Body image upload started');

  try {
    // 1. Parse request body
    const contentType = event.headers['Content-Type'] || event.headers['content-type'];
    log('INFO', 'Content-Type received', { contentType });

    if (!contentType || !contentType.includes('multipart/form-data')) {
      return createErrorResponse(400, 'invalid_content_type', 'Content-Type must be multipart/form-data');
    }

    const body = event.isBase64Encoded
      ? Buffer.from(event.body, 'base64').toString('binary')
      : event.body;

    log('INFO', 'Body parsed', { bodyLength: body.length });

    const { files, fields } = parseMultipartFormData(body, contentType);

    log('INFO', 'Files parsed', {
      hasImage: !!files.image,
      fileCount: Object.keys(files).length
    });

    if (!files.image) {
      return createErrorResponse(400, 'missing_image', 'Image file is required');
    }

    // Get user_id from fields if provided
    const userId = fields.user_id || 'anonymous';

    const imageFile = files.image;
    const imageData = imageFile.data;

    log('INFO', 'Image data extracted', {
      dataLength: imageData.length,
      isBuffer: Buffer.isBuffer(imageData),
      firstBytes: imageData.slice(0, 10).toString('hex')
    });

    // 2. Check image size (max 5MB for body images)
    if (imageData.length > 5 * 1024 * 1024) {
      return createErrorResponse(400, 'file_too_large', 'Image size must not exceed 5MB');
    }

    // 3. Validate image format (JPEG header check)
    const byte0 = imageData[0];
    const byte1 = imageData[1];
    const isJpeg = byte0 === 0xff && byte1 === 0xd8;

    log('INFO', 'JPEG validation', {
      byte0: byte0 ? byte0.toString(16) : 'null',
      byte1: byte1 ? byte1.toString(16) : 'null',
      isJpeg
    });

    if (!isJpeg) {
      return createErrorResponse(400, 'invalid_image', `Image must be in JPEG format (got: 0x${byte0?.toString(16)} 0x${byte1?.toString(16)})`);
    }

    // 4. Upload image to S3
    const bucketName = process.env.S3_BUCKET || 'hypermob-images';
    const timestamp = Date.now();
    const s3Key = `body/${userId}_${timestamp}.jpg`;

    try {
      const command = new PutObjectCommand({
        Bucket: bucketName,
        Key: s3Key,
        Body: imageData,
        ContentType: 'image/jpeg',
      });
      await s3Client.send(command);

      log('INFO', 'Image uploaded to S3', { bucket: bucketName, key: s3Key });
    } catch (s3Error) {
      log('ERROR', 'S3 upload failed', { error: s3Error.message });
      return createErrorResponse(500, 's3_upload_failed', 'Failed to upload image to S3');
    }

    const region = process.env.AWS_REGION || 'ap-northeast-2';
    const bodyImageUrl = `https://${bucketName}.s3.${region}.amazonaws.com/${s3Key}`;

    // 5. Success response
    const response = {
      user_id: userId,
      body_image_url: bodyImageUrl,
      status: 'ok',
      timestamp: new Date().toISOString(),
    };

    log('INFO', 'Body image upload successful', response);
    return createResponse(200, response);
  } catch (error) {
    log('ERROR', 'Body image upload error', { error: error.message, stack: error.stack });
    return createErrorResponse(500, 'internal_error', 'Internal server error', {
      error: error.message,
    });
  }
};
