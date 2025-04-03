#ifndef SCV_LINALG
#define SCV_LINALG

// <math.h> for sinf, cosf

typedef f32 SCVVec2[2];
typedef f32 SCVVec3[3];
typedef f32 SCVVec4[4];
typedef f32 SCVMatrix4x4[4][4];

void
SCVMat4x4Multiply(SCVMatrix4x4 mat1, SCVMatrix4x4 mat2, SCVMatrix4x4 dest)
{
  dest[0][0] = mat1[0][0] * mat2[0][0] + mat1[0][1] * mat2[1][0] + mat1[0][2] * mat2[2][0] + mat1[0][3] * mat2[3][0];
  dest[0][1] = mat1[0][0] * mat2[0][1] + mat1[0][1] * mat2[1][1] + mat1[0][2] * mat2[2][1] + mat1[0][3] * mat2[3][1];
  dest[0][2] = mat1[0][0] * mat2[0][2] + mat1[0][1] * mat2[1][2] + mat1[0][2] * mat2[2][2] + mat1[0][3] * mat2[3][2];
  dest[0][3] = mat1[0][0] * mat2[0][3] + mat1[0][1] * mat2[1][3] + mat1[0][2] * mat2[2][3] + mat1[0][3] * mat2[3][3];

  dest[1][0] = mat1[1][0] * mat2[0][0] + mat1[1][1] * mat2[1][0] + mat1[1][2] * mat2[2][0] + mat1[1][3] * mat2[3][0];
  dest[1][1] = mat1[1][0] * mat2[0][1] + mat1[1][1] * mat2[1][1] + mat1[1][2] * mat2[2][1] + mat1[1][3] * mat2[3][1];
  dest[1][2] = mat1[1][0] * mat2[0][2] + mat1[1][1] * mat2[1][2] + mat1[1][2] * mat2[2][2] + mat1[1][3] * mat2[3][2];
  dest[1][3] = mat1[1][0] * mat2[0][3] + mat1[1][1] * mat2[1][3] + mat1[1][2] * mat2[2][3] + mat1[1][3] * mat2[3][3];

  dest[2][0] = mat1[2][0] * mat2[0][0] + mat1[2][1] * mat2[1][0] + mat1[2][2] * mat2[2][0] + mat1[2][3] * mat2[3][0];
  dest[2][1] = mat1[2][0] * mat2[0][1] + mat1[2][1] * mat2[1][1] + mat1[2][2] * mat2[2][1] + mat1[2][3] * mat2[3][1];
  dest[2][2] = mat1[2][0] * mat2[0][2] + mat1[2][1] * mat2[1][2] + mat1[2][2] * mat2[2][2] + mat1[2][3] * mat2[3][2];
  dest[2][3] = mat1[2][0] * mat2[0][3] + mat1[2][1] * mat2[1][3] + mat1[2][2] * mat2[2][3] + mat1[2][3] * mat2[3][3];

  dest[3][0] = mat1[3][0] * mat2[0][0] + mat1[3][1] * mat2[1][0] + mat1[3][2] * mat2[2][0] + mat1[3][3] * mat2[3][0];
  dest[3][1] = mat1[3][0] * mat2[0][1] + mat1[3][1] * mat2[1][1] + mat1[3][2] * mat2[2][1] + mat1[3][3] * mat2[3][1];
  dest[3][2] = mat1[3][0] * mat2[0][2] + mat1[3][1] * mat2[1][2] + mat1[3][2] * mat2[2][2] + mat1[3][3] * mat2[3][2];
  dest[3][3] = mat1[3][0] * mat2[0][3] + mat1[3][1] * mat2[1][3] + mat1[3][2] * mat2[2][3] + mat1[3][3] * mat2[3][3];
}

void
SCVMat4x4MultiplySCVVec(SCVMatrix4x4 mat, SCVVec4 vec, SCVVec4 dest)
{
  SCVVec4 row1;
  SCVVec4 row2;
  SCVVec4 row3;
  SCVVec4 row4;
  memcpy(row1, mat[0], 4 * sizeof(f32));
  memcpy(row2, mat[1], 4 * sizeof(f32));
  memcpy(row3, mat[2], 4 * sizeof(f32));
  memcpy(row4, mat[3], 4 * sizeof(f32));

  dest[0] = vec[0] * row1[0] + vec[1] * row1[1] + vec[2] * row1[2] + vec[3] * row1[3];
  dest[1] = vec[0] * row2[0] + vec[1] * row2[1] + vec[2] * row2[2] + vec[3] * row2[3];
  dest[2] = vec[0] * row3[0] + vec[1] * row3[1] + vec[2] * row3[2] + vec[3] * row3[3];
  dest[3] = vec[0] * row4[0] + vec[1] * row4[1] + vec[2] * row4[2] + vec[3] * row4[3];
}

void
SCVMat4x4Identity(SCVMatrix4x4 dest)
{
  dest[0][0] = 1.0f;
  dest[0][1] = 0.0f;
  dest[0][2] = 0.0f;
  dest[0][3] = 0.0f;

  dest[1][0] = 0.0f;
  dest[1][1] = 1.0f;
  dest[1][2] = 0.0f;
  dest[1][3] = 0.0f;

  dest[2][0] = 0.0f;
  dest[2][1] = 0.0f;
  dest[2][2] = 1.0f;
  dest[2][3] = 0.0f;

  dest[3][0] = 0.0f;
  dest[3][1] = 0.0f;
  dest[3][2] = 0.0f;
  dest[3][3] = 1.0f;
}

void
SCVMat4x4ZRotation(f32 radians, SCVMatrix4x4 dest)
{
  SCVMat4x4Identity(dest);
  dest[0][0] = cosf(radians);
  dest[0][1] = -sinf(radians);
  dest[1][0] = sinf(radians);
  dest[1][1] = cosf(radians);
}

void
SCVMat4x4Scale(f32 scale, SCVMatrix4x4 dest)
{
  SCVMat4x4Identity(dest);
  dest[0][0] = scale;
  dest[1][1] = scale;
  dest[2][2] = scale;
}

void
SCVMat4x4Translation(f32 tx, f32 ty, f32 tz, SCVMatrix4x4 dest)
{
  SCVMat4x4Identity(dest);
  dest[0][3] = tx;
  dest[1][3] = ty;
  dest[2][3] = tz;
}
#endif 
