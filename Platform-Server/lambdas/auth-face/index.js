/**
 * Lambda: POST /auth/face
 *
 * Face image registration
 * - Upload image (multipart/form-data)
 * - Generate Face ID (SHA256 hash)
 * - Generate User ID (UUID-based)
 * - Store user info in DB
 */

const { S3Client, PutObjectCommand } = require('@aws-sdk/client-s3');
const db = require('/opt/nodejs/shared/db');
const {
  generateUserId,
  generateFaceId,
  generateNfcUid,
  createResponse,
  createErrorResponse,
  parseMultipartFormData,
  log,
} = require('/opt/nodejs/shared/utils');

const s3Client = new S3Client({ region: process.env.AWS_REGION || 'ap-northeast-2' });

exports.handler = async (event) => {
  log('INFO', 'POST /auth/face - Face registration started');

  try {
    // 1. Parse request body
    const contentType = event.headers['Content-Type'] || event.headers['content-type'];
    log('INFO', 'Content-Type received', { contentType });

    if (!contentType || !contentType.includes('multipart/form-data')) {
      return createErrorResponse(400, 'invalid_content_type', 'Content-Type must be multipart/form-data');
    }

    // API Gateway encodes binary data as base64
    log('INFO', 'Request encoding', {
      isBase64Encoded: event.isBase64Encoded,
      bodyLength: event.body ? event.body.length : 0
    });

    const body = event.isBase64Encoded
      ? Buffer.from(event.body, 'base64').toString('binary')
      : event.body;

    log('INFO', 'Body parsed', { bodyLength: body.length });

    const { files } = parseMultipartFormData(body, contentType);

    log('INFO', 'Files parsed', {
      hasImage: !!files.image,
      fileCount: Object.keys(files).length
    });

    if (!files.image) {
      return createErrorResponse(400, 'missing_image', 'Image file is required');
    }

    const imageFile = files.image;
    const imageData = imageFile.data;

    log('INFO', 'Image data extracted', {
      dataLength: imageData.length,
      isBuffer: Buffer.isBuffer(imageData),
      firstBytes: imageData.slice(0, 10).toString('hex')
    });

    // 2. Check image size (max 1MB)
    if (imageData.length > 1024 * 1024) {
      return createErrorResponse(400, 'file_too_large', 'Image size must not exceed 1MB');
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

    // 4. Generate User ID, Face ID, and NFC UID
    const userId = generateUserId();
    const faceId = generateFaceId(imageData);
    const nfcUid = generateNfcUid(userId);

    log('INFO', 'Generated IDs', { userId, faceId, nfcUid });

    // 5. Upload image to S3
    const bucketName = process.env.S3_BUCKET || 'hypermob-images';
    const s3Key = `faces/${userId}.jpg`;

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
    const faceImagePath = `https://${bucketName}.s3.${region}.amazonaws.com/${s3Key}`;

    // 6. Store user info in database
    try {
      // Generate required fields
      const name = 'User_' + userId.substring(1, 6);
      const email = userId.toLowerCase() + '@hypermob.com';
      const phone = '010-' + Math.floor(1000 + Math.random() * 9000) + '-' + Math.floor(1000 + Math.random() * 9000);

      await db.query(
        'INSERT INTO users (user_id, name, email, phone, face_id, profile_image_url, nfc_uid) VALUES (?, ?, ?, ?, ?, ?, ?)',
        [userId, name, email, phone, faceId, faceImagePath, nfcUid]
      );
    } catch (dbError) {
      // Unique constraint violation (duplicate face_id)
      if (dbError.code === 'ER_DUP_ENTRY') {
        log('WARN', 'Duplicate face_id', { faceId });
        return createErrorResponse(409, 'face_already_exists', 'This face is already registered');
      }
      throw dbError;
    }

    // 7. Success response
    const response = {
      user_id: userId,
      face_id: faceId,
      nfc_uid: nfcUid,
      face_image_url: faceImagePath,
      status: 'ok',
    };

    log('INFO', 'Face registration successful', response);
    return createResponse(200, response);
  } catch (error) {
    log('ERROR', 'Face registration error', { error: error.message, stack: error.stack });
    return createErrorResponse(500, 'internal_error', 'Internal server error', {
      error: error.message,
    });
  }
};
