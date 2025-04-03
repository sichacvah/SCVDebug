#ifndef SCV_GL
#define SCV_GL

/**
 * headers needed:
 *
 * scv.h
 * scv_linalg.h
 * scv_geom.h
 *
 * on apple particular:
 *  <OpenGL/gl3.h>
 *
 */

// NOTE: Support depends on OpenGL version
// only 3.3 core for now
typedef enum
{
  SCV_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE = 1, // 8 bit per pixel (no alpha)
  SCV_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,    // 8*2 bpp (2 channels)
  SCV_PIXELFORMAT_UNCOMPRESSED_R8G8B8,        // 24 bpp
  SCV_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,      // 32 bpp
} SCVPixelFormat;

enum SCVVBOs {
  SCV_VBO_POSITIONS = 0,
  SCV_VBO_TEXCOORDS,
  SCV_VBO_COLORS,
  SCV_VBO_INDICIES,

  SCV_VBO_LENGTH
};

typedef struct SCVVertex SCVVertex;
struct SCVVertex {
  SCVVec3 position;
  SCVVec2 texcoord;
  SCVColor color;
};

typedef struct SCVVertexes SCVVertexes;
struct SCVVertexes {
  f32 *positions; //  x, y, z (3) - component  per vertex; shader (location = 0) 
  f32 *texcoords; //  u, v    (2) - components per vertex; shader (location = 1)
  u8  *colors;    //  RGBA    (4) - components per vertex; shader (location = 2) 
  u32 size;
  u32 index;
};

typedef struct SCVDrawCall SCVDrawCall;
struct SCVDrawCall {
  u32 start;
  u32 len;
  u32 texID;
  u32 shaderID;
};

typedef struct SCVGLCtx SCVGLCtx;
struct SCVGLCtx {
  SCVSlice      Drawcalls;
  u32           VAO;
  u32           DefaultTexuteId;
  i32           PositionLocation;
  i32           TexcoordsLocation;
  i32           ColorLocation;
  i32           MVPLocation;
  u32           DefaultShader;
  u32           VBO[SCV_VBO_LENGTH];
  SCVVertexes   Vertexes;
  SCVSlice      Indicies;
  SCVRect       Viewport;
  f32           Scale;
};

#define SCV_ERROR_SHADER_COMPILE 1
#define SCV_ERROR_SHADER_LINK 2

void scvGLFlush(SCVGLCtx *ctx);
u32 scvGLLoadTexture(byte* data, i32 width, i32 height, i32 format, i32 mipmapcount);
void scvGLPushIndex(SCVGLCtx *ctx, u32 indx);

u32
scvGLCompileShader(SCVString src, i32 type, SCVError* err)
{
  u32 shader;
  i32 success;
  i32 logsize;
  i32 len = (i32)src.len;
  shader = glCreateShader(type);
  glShaderSource(shader, 1, (const char* const*)&src.base, &len);
  glCompileShader(shader);

  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, sizeof(err->errbuf), &logsize, (char*)err->errbuf);
    err->tag = SCV_ERROR_SHADER_COMPILE;
    err->message = scvUnsafeString(err->errbuf, (u64)scvMin((u64)logsize, sizeof(err->errbuf)));
  }

  return shader;
}

u32
scvGLLinkShaderProgram(u32 vertexShader, u32 fragmentShader, SCVError* err)
{
  u32 program;
  i32 success;
  i32 logsize;
  program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(program, sizeof(err->errbuf), &logsize, (char*)err->errbuf);
    err->tag = SCV_ERROR_SHADER_LINK;
    err->message = scvUnsafeString(err->errbuf, (u64)scvMin((u64)logsize, sizeof(err->errbuf)));
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return program;
}

char* scvDefaultVertexShader =
  "#version 330 core                                  \n"
  "in vec3 vertexPosition;                            \n"
  "in vec2 vertexTexCoord;                            \n"
  "in vec4 vertexColor;                               \n"
  "out vec2 fragTexCoord;                             \n"
  "out vec4 fragColor;                                \n"
  "uniform mat4 mvp;                                  \n"
  "void main()                                        \n"
  "{                                                  \n"
  "   fragTexCoord  = vertexTexCoord;                 \n"
  "   fragColor     = vertexColor;                    \n"
  "   gl_Position   = mvp*vec4(vertexPosition, 1.0);  \n" 
  "}                                                  \n";


char* scvDefaultFragmentShader =
  "#version 330 core                                      \n"
  "in vec2 fragTexCoord;                                  \n"
  "in vec4 fragColor;                                     \n"
  "out vec4 finalColor;                                   \n"
  "uniform sampler2D texture0;                            \n"
  "void main()                                            \n"
  "{                                                      \n"
  "   vec4 texelColor = texture(texture0, fragTexCoord);  \n"
  "   finalColor      = texelColor*fragColor;             \n"
  "}                                                      \n";


u32
scvGLBuildDefaultShaders(void)
{
  u32 result;
  u32 vertexShader;
  u32 fragmentShader;
  SCVError error = {0};
  
  vertexShader = scvGLCompileShader(scvUnsafeCString(scvDefaultVertexShader), GL_VERTEX_SHADER, &error);
  if (error.tag) {
    scvFatalError("Failed to compile default vertex shader", &error);
  }
  
  fragmentShader = scvGLCompileShader(scvUnsafeCString(scvDefaultFragmentShader), GL_FRAGMENT_SHADER, &error);
  if (error.tag) {
    scvFatalError("Failed to compile default vertex shader", &error);
  }

  result = scvGLLinkShaderProgram(vertexShader, fragmentShader, &error);
  
  if (error.tag) {
    scvFatalError("Failed to link default shader", &error);
  }

  return result;  
}

void
scvGLCtxInit(
    SCVArena *arena,
    SCVGLCtx *ctx, 
    u32 vertexescount, 
    u32 drawcalls,
    SCVRect viewport,
    f32 scaleFactor)
{
  u32 indiceslen;
  u32 texcoordslen;
  u32 positionslen;
  u32 colorslen;
  u8  whitpixels[4] = { 255, 255, 255, 255 };
  scvAssert(ctx);
  scvAssert(arena);

  indiceslen    = sizeof(u32) * vertexescount * 2;
  texcoordslen  = sizeof(f32) * vertexescount * 2;
  positionslen  = sizeof(f32) * vertexescount * 3;
  colorslen     = sizeof(u8)  * vertexescount * 4;
 
  viewport.origin.x *= scaleFactor;
  viewport.origin.y *= scaleFactor;
  viewport.size.width *= scaleFactor;
  viewport.size.height *= scaleFactor;
  ctx->Viewport = viewport;
  
  ctx->Scale = scaleFactor;

  ctx->DefaultTexuteId = scvGLLoadTexture(whitpixels, 1, 1, SCV_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);

  glGenVertexArrays(1, &ctx->VAO); 
  ctx->Drawcalls = scvMakeSlice(arena, SCVDrawCall, 0, drawcalls);
  scvAssert(ctx->Drawcalls.base);

  ctx->Vertexes.size      = vertexescount;
  ctx->Vertexes.texcoords = (f32 *)scvArenaAlloc(arena, (u64)texcoordslen);
  scvAssert(ctx->Vertexes.texcoords);
  ctx->Vertexes.positions = (f32 *)scvArenaAlloc(arena, (u64)positionslen);
  scvAssert(ctx->Vertexes.positions);
  ctx->Vertexes.colors    = (u8  *)scvArenaAlloc(arena, (u64)colorslen);
  scvAssert(ctx->Vertexes.colors);
  ctx->Indicies           = scvMakeSlice(arena, u32, 0, indiceslen);
  scvAssert(ctx->Indicies.base);

  glBindVertexArray(ctx->VAO);
  glGenBuffers(SCV_VBO_LENGTH, ctx->VBO);

  ctx->DefaultShader = scvGLBuildDefaultShaders();

  ctx->PositionLocation = glGetAttribLocation(ctx->DefaultShader, "vertexPosition");
  scvAssert(ctx->PositionLocation >= 0);

  ctx->TexcoordsLocation = glGetAttribLocation(ctx->DefaultShader, "vertexTexCoord");
  scvAssert(ctx->TexcoordsLocation >= 0);

  ctx->ColorLocation = glGetAttribLocation(ctx->DefaultShader, "vertexColor");
  scvAssert(ctx->ColorLocation >= 0);

  ctx->MVPLocation = glGetUniformLocation(ctx->DefaultShader, "mvp");
  scvAssert(ctx->MVPLocation >= 0);

  glBindBuffer(GL_ARRAY_BUFFER, ctx->VBO[SCV_VBO_POSITIONS]);   
  glBufferData(GL_ARRAY_BUFFER, positionslen, ctx->Vertexes.positions, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(ctx->PositionLocation);
  glVertexAttribPointer(ctx->PositionLocation, 3, GL_FLOAT, 0, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, ctx->VBO[SCV_VBO_TEXCOORDS]);
  glBufferData(GL_ARRAY_BUFFER, texcoordslen, ctx->Vertexes.texcoords, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(ctx->TexcoordsLocation);
  glVertexAttribPointer(ctx->TexcoordsLocation, 2, GL_FLOAT, 0, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, ctx->VBO[SCV_VBO_COLORS]);
  glBufferData(GL_ARRAY_BUFFER, colorslen, ctx->Vertexes.colors, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(ctx->ColorLocation);
  glVertexAttribPointer(ctx->ColorLocation, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->VBO[SCV_VBO_INDICIES]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indiceslen, ctx->Indicies.base, GL_DYNAMIC_DRAW);
}

void
scvGLBegin(SCVGLCtx *ctx)
{
  ctx->Vertexes.index = 0;
  ctx->Indicies.len   = 0;
  ctx->Drawcalls.len  = 0;
  scvSliceAppend(ctx->Drawcalls, ((SCVDrawCall){
      .start    = 0,
      .len      = 0,
      .texID    = ctx->DefaultTexuteId,
      .shaderID = ctx->DefaultShader,
  }));
}

void
scvGLPushIndex(SCVGLCtx *ctx, u32 indx)
{
  u32 *indexes;
  u64 len = ctx->Indicies.len;
  SCVDrawCall *drawcall = &((SCVDrawCall *)ctx->Drawcalls.base)[ctx->Drawcalls.len - 1];

  scvAssert(len < ctx->Indicies.cap);
 
  indexes = (u32 *)ctx->Indicies.base;
  indexes[len] = indx;
  ctx->Indicies.len = len + 1;
  drawcall->len++;
}

u32
scvGLPushVertex(SCVGLCtx *ctx, SCVVertex *vertex)
{
  f32 *positions;
  f32 *texcoords;
  u8  *colors;
  u64 index = ctx->Vertexes.index;
  f32 scale = ctx->Scale;
  
  scvAssert(index + 1 < ctx->Vertexes.size); 

  positions = ctx->Vertexes.positions;

  positions[index * 3 + 0] = vertex->position[0] * scale;
  positions[index * 3 + 1] = vertex->position[1] * scale;
  positions[index * 3 + 2] = vertex->position[2];

  texcoords = ctx->Vertexes.texcoords;
  texcoords[index * 2 + 0] = vertex->texcoord[0];
  texcoords[index * 2 + 1] = vertex->texcoord[1];

  colors = ctx->Vertexes.colors;
  colors[index * 4 + 0] = vertex->color.r;
  colors[index * 4 + 1] = vertex->color.g;
  colors[index * 4 + 2] = vertex->color.b;
  colors[index * 4 + 3] = vertex->color.a;

  ctx->Vertexes.index  = index + 1;

  return index;
}


void
scvGLDrawRectInternal(SCVGLCtx *ctx, SCVRect rect, SCVColor color)
{
  u32 i1, i2, i3, i4;
  f32 x, y, width, height;
  SCVVertex vertex = {0};

  if (ctx->Vertexes.index >= ctx->Vertexes.size - 5) {
    scvGLFlush(ctx);
  }

  x = rect.origin.x;
  y = rect.origin.y;
  width = rect.size.width;
  height = rect.size.height;

  vertex.position[2] = 1.0f;
  vertex.position[0] = x;
  vertex.position[1] = y;
  vertex.color       = color;

  // topleft
  i1 = scvGLPushVertex(ctx, &vertex);

  vertex.position[0] = x + width;
  vertex.position[1] = y;
  vertex.color       = color;

  // topright
  i2 = scvGLPushVertex(ctx, &vertex);

  vertex.position[0] = x + width;
  vertex.position[1] = y + height;
  vertex.color       = color;

  // bottomright
  i3 = scvGLPushVertex(ctx, &vertex);

  vertex.position[0] = x;
  vertex.position[1] = y + height;
  vertex.color       = color;

  // bottomleft
  i4 = scvGLPushVertex(ctx, &vertex);

  // clockwise indexes
  scvGLPushIndex(ctx, i1);
  scvGLPushIndex(ctx, i2);
  scvGLPushIndex(ctx, i3);

  scvGLPushIndex(ctx, i1);
  scvGLPushIndex(ctx, i3);
  scvGLPushIndex(ctx, i4);
}

void
scvGLDrawImage(SCVGLCtx *ctx, SCVRect rect, SCVColor color, u32 texID)
{
  SCVDrawCall drawcall = {0};
  SCVDrawCall currentDrawCall = ((SCVDrawCall *)ctx->Drawcalls.base)[ctx->Drawcalls.len - 1];
  if (currentDrawCall.texID != texID) {
    drawcall.start = ctx->Indicies.len;
    drawcall.len   = 0;
    drawcall.shaderID = ctx->DefaultShader;
    drawcall.texID = texID;
    scvSliceAppend(ctx->Drawcalls, drawcall);
  } 

  scvGLDrawRectInternal(ctx, rect, color);
}

void
scvGLDrawRect(SCVGLCtx *ctx, SCVRect rect, SCVColor color)
{ 
  SCVDrawCall drawcall = {0};
  SCVDrawCall currentDrawCall = ((SCVDrawCall *)ctx->Drawcalls.base)[ctx->Drawcalls.len - 1];
  if (currentDrawCall.texID != ctx->DefaultTexuteId) {
    drawcall.start = ctx->Indicies.len;
    drawcall.len   = 0;
    drawcall.shaderID = ctx->DefaultShader;
    drawcall.texID = ctx->DefaultTexuteId;
    scvSliceAppend(ctx->Drawcalls, drawcall);
  }

  scvGLDrawRectInternal(ctx, rect, color);
}

void
scvGLDrawTriangle(SCVGLCtx *ctx, SCVVec2 p1, SCVVec2 p2, SCVVec2 p3, SCVColor color)
{
  u32 indx;
  SCVVertex vertex = {0};

  vertex.position[0] = p1[0];
  vertex.position[1] = p1[1];
  vertex.position[2] = 1.0f;
  vertex.texcoord[0] = 0.0f;
  vertex.texcoord[1] = 0.0f;
  vertex.color       = color;

  indx = scvGLPushVertex(ctx, &vertex);

  scvGLPushIndex(ctx, indx);

  vertex.position[0] = p2[0];
  vertex.position[1] = p2[1];

  indx = scvGLPushVertex(ctx, &vertex);
  scvGLPushIndex(ctx, indx);

  vertex.position[0] = p3[0];
  vertex.position[1] = p3[1];

  indx = scvGLPushVertex(ctx, &vertex); 
  scvGLPushIndex(ctx, indx);
}

void
scvGLFlush(SCVGLCtx *ctx)
{
  u32 i;
  u32* indicies;
  SCVDrawCall *drawcall;
  SCVPoint origin = ctx->Viewport.origin;
  SCVSize  size   = ctx->Viewport.size;
 
  f32 a = 2.0f / size.width;
  f32 b = -(2.0f / size.height);

  f32 Proj[] =
  {
       a, 0.0f, 0.0f, 0.0f,
    0.0f,    b, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
   -1.0f, 1.0f, 0.0f, 1.0f
  };

  glViewport((u32)origin.x, (u32)origin.y, (u32)size.width, (u32)size.height);
  glUseProgram(ctx->DefaultShader);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ctx->DefaultTexuteId);

  glBindBuffer(GL_ARRAY_BUFFER, ctx->VBO[SCV_VBO_POSITIONS]);
  glBufferSubData(GL_ARRAY_BUFFER, 0, ctx->Vertexes.index * 3 * sizeof(f32), ctx->Vertexes.positions);

  glBindBuffer(GL_ARRAY_BUFFER, ctx->VBO[SCV_VBO_TEXCOORDS]);
  glBufferSubData(GL_ARRAY_BUFFER, 0, ctx->Vertexes.index * 2 * sizeof(f32), ctx->Vertexes.texcoords);

  glBindBuffer(GL_ARRAY_BUFFER, ctx->VBO[SCV_VBO_COLORS]);
  glBufferSubData(GL_ARRAY_BUFFER, 0, ctx->Vertexes.index * 4 * sizeof(u8), ctx->Vertexes.colors);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->VBO[SCV_VBO_INDICIES]);
  glBindVertexArray(ctx->VAO);
  glUniformMatrix4fv(ctx->MVPLocation, 1, false, Proj);

  for (i = 0; i < ctx->Drawcalls.len; ++i) {
    drawcall = scvSliceGet(ctx->Drawcalls, SCVDrawCall, i);
    indicies = scvSliceGet(ctx->Indicies, u32, drawcall->start);
    glUseProgram(drawcall->shaderID);
    glBindTexture(GL_TEXTURE_2D, drawcall->texID);

    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, drawcall->len * sizeof(u32), indicies);
    glDrawElements(GL_TRIANGLES, drawcall->len, GL_UNSIGNED_INT, (void *)0); 
  }

  
  glUseProgram(0);

  ctx->Vertexes.index = 0;
  ctx->Indicies.len   = 0;
}

void
scvGLEnd(SCVGLCtx *ctx)
{
  scvGLFlush(ctx);
}


i32
scvGetPixelDataSize(i32 width, i32 height, i32 format)
{
  i32 dataSize;
  i32 bpp;

  dataSize = 0;
  bpp = 0;

  switch (format) {
    case SCV_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE:
      bpp = 8;
      break;
    case SCV_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA:
    case SCV_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8:
      bpp = 32;
      break;
    case SCV_PIXELFORMAT_UNCOMPRESSED_R8G8B8:
      bpp = 24;
      break;
    default:
      break;
  }


  dataSize = scvMax((bpp * width * height / 8), 1);

  return dataSize;
}

void
scvGLGetTextureFormats(i32 format,
                       u32* glInternalFormat,
                       u32* glFormat,
                       u32* glType)
{
  *glInternalFormat = 0;
  *glFormat = 0;
  *glType = 0;

  switch (format) {
    case SCV_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE:
      *glInternalFormat = GL_R8;
      *glFormat = GL_RED;
      *glType = GL_UNSIGNED_BYTE;
      break;
    case SCV_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA:
      *glInternalFormat = GL_RG8;
      *glFormat = GL_RG;
      *glType = GL_UNSIGNED_BYTE;
      break;
    case SCV_PIXELFORMAT_UNCOMPRESSED_R8G8B8:
      *glInternalFormat = GL_RGB;
      *glFormat = GL_RGB;
      *glType = GL_UNSIGNED_BYTE;
      break;
    case SCV_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8:
      *glInternalFormat = GL_RGBA;
      *glFormat = GL_RGBA;
      *glType = GL_UNSIGNED_BYTE;
      break;
    default:
      scvWarn("GL_TEXT", "unknown gl texture format");
  }
}

void
scvGLBindTexture(u32 id)
{
  glBindTexture(GL_TEXTURE_2D, id);
}

u32
scvGLLoadTexture(byte* data, i32 width, i32 height, i32 format, i32 mipmapcount)
{
  i32 i, mipWidth, mipSize, mipHeight;
  u32 id, glInternalFormat, glFormat, glType;
  i32 swizzlemap[4];

  id = 0;
  glBindTexture(GL_TEXTURE_2D, 0);
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  mipWidth = width;
  mipHeight = height;

  scvGLGetTextureFormats(format, &glInternalFormat, &glFormat, &glType);
      
  if (glInternalFormat != 0) {
    for (i = 0; i < mipmapcount; ++i) {
      mipSize = scvGetPixelDataSize(mipWidth, mipHeight, format);
      glTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, data);
      if (format == SCV_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE) {
        swizzlemap[0] = GL_RED;
        swizzlemap[1] = GL_RED;
        swizzlemap[2] = GL_RED;
        swizzlemap[3] = GL_ONE;
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzlemap);
      } else if (format == SCV_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA) {
        swizzlemap[0] = GL_RED;
        swizzlemap[1] = GL_RED;
        swizzlemap[2] = GL_RED;
        swizzlemap[3] = GL_GREEN;
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzlemap);
      }

      mipWidth /= 2;
      mipHeight /= 2;
      // mipOffset += mipSize;
      if (data != nil) {
        data += mipSize;
      }
      if (mipWidth < 1) mipWidth = 1;
      if (mipHeight < 1) mipHeight = 1;
    }
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);       // Set texture to repeat on x-axis
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // Alternative: GL_LINEAR
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  if (mipmapcount > 1) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmapcount); 
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  if (id == 0) {
    scvWarn("TEX_BIND", "texture bind failed");
  }

  return id;
}


#endif
