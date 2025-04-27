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


typedef struct SCVImage SCVImage;
struct SCVImage {
  void *data;
  u32 width;
  u32 height;
  u32 pitch;
  SCVPixelFormat pixelformat;
  i32 mipmapcount;
};

typedef struct SCVTexture SCVTexture;
struct SCVTexture {
  u32 glTexID;
  f32 width;
  u32 height;
  u32 pitch;
};

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

typedef struct SCVGlyph SCVGlyph;
struct SCVGlyph {
  rune  codepoint;
  i32   index;
  i32   width;
  i32   xoffset;
  i32   yoffset;
  i32   height;
  i32   xadvance;
  i32   lsb; // left side bearing
  i32   tx; // offset inside texture x
  i32   ty; // offset inside texture y
};

typedef struct SCVFont SCVFont;
struct SCVFont {
  SCVTexture *texture;  
  SCVSlice   glyphs;
  f32        size;
  f32        scale;
  i32        ascent;
  i32        descent;
  i32        linegap;
};

typedef struct SCVGLCtx SCVGLCtx;
struct SCVGLCtx {
  SCVSlice      Drawcalls;
  u32           VAO;
  u32           DefaultTextureId;
  i32           PositionLocation;
  i32           TexcoordsLocation;
  i32           ColorLocation;
  i32           MVPLocation;
  u32           DefaultShader;
  u32           VBO[SCV_VBO_LENGTH];
  SCVVertexes   Vertexes;
  SCVSlice      Indicies;
  SCVRect       Viewport;
  SCVPool       Textures;
  SCVPool       Fonts;
  f32           Scale;
};

typedef struct SCVText SCVText;
struct SCVText {
  SCVPoint  point;
  SCVString str;
  SCVString font;
  f32       fontSize;
};



#define SCV_ERROR_SHADER_COMPILE 1
#define SCV_ERROR_SHADER_LINK 2

void scvGLFlush(SCVGLCtx *ctx);
u32 scvGLLoadTexture(SCVImage image);
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

typedef struct SCVGLCtxDesc SCVGLCtxDesc;
struct SCVGLCtxDesc {
  SCVRect viewport;
  SCVArena *arena; 
  u32 vertexescount;
  u32 drawcalls;
  u32 texturescount;
  u32 fontscount;
  f32 scaleFactor;
};

void
scvGLCtxDescDefault(SCVGLCtxDesc *desc) {
  scvAssert(desc);
  scvAssert(desc->arena);
  scvAssert(desc->scaleFactor > 0);

  desc->vertexescount = desc->vertexescount == 0 ? 1024 : desc->vertexescount;
  desc->drawcalls     = desc->drawcalls     == 0 ? 16   : desc->drawcalls;
  desc->texturescount = desc->texturescount == 0 ? 256  : desc->texturescount;
  desc->fontscount    = desc->fontscount    == 0 ? 128  : desc->fontscount;
}

void
scvGLCtxInit(SCVGLCtx *ctx, SCVGLCtxDesc *desc)
{
  SCVArena *arena;
  u32 indiceslen;
  u32 texcoordslen;
  u32 positionslen;
  u32 colorslen;
  SCVSlice texturesMem;
  SCVSlice fontsMem;
  SCVImage defaultTextureImg = {0};
  u8 whitepixels[4] = { 255, 255, 255, 255 };
  scvGLCtxDescDefault(desc);

  arena = desc->arena;
  indiceslen    = sizeof(u32) * desc->vertexescount * 2;
  texcoordslen  = sizeof(f32) * desc->vertexescount * 2;
  positionslen  = sizeof(f32) * desc->vertexescount * 3;
  colorslen     = sizeof(u8)  * desc->vertexescount * 4;
 
  desc->viewport.origin.x *= desc->scaleFactor;
  desc->viewport.origin.y *= desc->scaleFactor;
  desc->viewport.size.width *= desc->scaleFactor;
  desc->viewport.size.height *= desc->scaleFactor;
  ctx->Viewport = desc->viewport;
  
  ctx->Scale = desc->scaleFactor;

  defaultTextureImg.data = whitepixels;
  defaultTextureImg.width = 1;
  defaultTextureImg.height = 1;
  defaultTextureImg.pixelformat = SCV_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
  defaultTextureImg.mipmapcount = 1;
  ctx->DefaultTextureId = scvGLLoadTexture(defaultTextureImg);

  glGenVertexArrays(1, &ctx->VAO); 
  ctx->Drawcalls = scvMakeSlice(arena, SCVDrawCall, 0, desc->drawcalls);
  scvAssert(ctx->Drawcalls.base);

  ctx->Vertexes.size      = desc->vertexescount;
  ctx->Vertexes.texcoords = (f32 *)scvArenaAlloc(arena, (u64)texcoordslen);
  scvAssert(ctx->Vertexes.texcoords);
  ctx->Vertexes.positions = (f32 *)scvArenaAlloc(arena, (u64)positionslen);
  scvAssert(ctx->Vertexes.positions);
  ctx->Vertexes.colors    = (u8  *)scvArenaAlloc(arena, (u64)colorslen);
  scvAssert(ctx->Vertexes.colors);
  ctx->Indicies           = scvMakeSlice(arena, u32, 0, indiceslen);
  scvAssert(ctx->Indicies.base);
  texturesMem             = scvMakeSlice(arena, SCVTexture, 0, desc->texturescount);
  scvPoolInitDefault(&ctx->Textures, texturesMem, sizeof(SCVTexture));
  scvAssert(ctx->Textures.buf);

  fontsMem = scvMakeSlice(arena, SCVFont, 0, desc->fontscount);
  scvPoolInitDefault(&ctx->Fonts, fontsMem, sizeof(SCVFont));
  scvAssert(ctx->Fonts.buf);

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
      .texID    = ctx->DefaultTextureId,
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

typedef struct SCVUVRect SCVUVRect;
struct SCVUVRect {
  SCVVec2 topleft;
  SCVVec2 topright;
  SCVVec2 bottomleft;
  SCVVec2 bottomright;
};


void
scvGLDrawRectInternal(SCVGLCtx *ctx, SCVRect rect, SCVColor color, SCVUVRect *uvs)
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
  vertex.texcoord[0] = uvs ? uvs->topleft[0] : 0.0f;
  vertex.texcoord[1] = uvs ? uvs->topleft[1] : 0.0f;
  vertex.color       = color;

  // topleft
  i1 = scvGLPushVertex(ctx, &vertex);

  vertex.position[0] = x + width;
  vertex.position[1] = y;
  vertex.color       = color;
  vertex.texcoord[0] = uvs ? uvs->topright[0] : 1.0f;
  vertex.texcoord[1] = uvs ? uvs->topright[1] : 0.0f;

  // topright
  i2 = scvGLPushVertex(ctx, &vertex);

  vertex.position[0] = x + width;
  vertex.position[1] = y + height;
  vertex.color       = color;
  vertex.texcoord[0] = uvs ? uvs->bottomright[0] : 1.0f;
  vertex.texcoord[1] = uvs ? uvs->bottomright[1] : 1.0f;

  // bottomright
  i3 = scvGLPushVertex(ctx, &vertex);

  vertex.position[0] = x;
  vertex.position[1] = y + height;
  vertex.color       = color;
  vertex.texcoord[0] = uvs ? uvs->bottomleft[0] : 0.0f;
  vertex.texcoord[1] = uvs ? uvs->bottomleft[1] : 1.0f;

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

  scvGLDrawRectInternal(ctx, rect, color, nil);
}

void
scvGLDrawRect(SCVGLCtx *ctx, SCVRect rect, SCVColor color)
{ 
  SCVDrawCall drawcall = {0};
  SCVDrawCall currentDrawCall = ((SCVDrawCall *)ctx->Drawcalls.base)[ctx->Drawcalls.len - 1];
  if (currentDrawCall.texID != ctx->DefaultTextureId) {
    drawcall.start = ctx->Indicies.len;
    drawcall.len   = 0;
    drawcall.shaderID = ctx->DefaultShader;
    drawcall.texID = ctx->DefaultTextureId;
    scvSliceAppend(ctx->Drawcalls, drawcall);
  }

  scvGLDrawRectInternal(ctx, rect, color, nil);
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
  glEnable(GL_BLEND); 
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);
  glUseProgram(ctx->DefaultShader);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ctx->DefaultTextureId);

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
    glActiveTexture(GL_TEXTURE0);
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
scvGLLoadTexture(SCVImage image)
{
  byte* data;
  i32 width, height, format, mipmapcount;
  i32 i, mipWidth, mipSize, mipHeight;
  u32 id, glInternalFormat, glFormat, glType;
  i32 swizzlemap[4];

  id = 0;

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  data = image.data;
  width = image.width;
  height = image.height;
  format = image.pixelformat;
  mipmapcount = image.mipmapcount;

  mipWidth = width;
  mipHeight = height;

  scvGLGetTextureFormats(format, &glInternalFormat, &glFormat, &glType);
      
  if (glInternalFormat != 0) {
    for (i = 0; i < mipmapcount; ++i) {
      mipSize = scvGetPixelDataSize(mipWidth, mipHeight, format);
      glGenerateMipmap(GL_TEXTURE_2D);
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

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

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

SCVImage
scvImage(SCVArena *arena, u32 width, u32 height)
{
  SCVError error = {0};
  SCVImage result = {0};

  result.pitch = 4 * width;
  result.mipmapcount = 1;
  result.pixelformat = SCV_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
  result.data = scvArenaAllocErr(arena, result.pitch * height, &error);
  if (error.tag) {
    scvFatalError("can't alloc memory", &error);
  }
  memset(result.data, 0, result.pitch * height);
  result.width = width;
  result.height = height;

  return result;
}

SCVTexture*
scvLoadTexture(SCVGLCtx *ctx, SCVImage image)
{
  SCVTexture* tex = (SCVTexture *)scvPoolAlloc(&ctx->Textures);
 
  tex->glTexID = scvGLLoadTexture(image);
  tex->width = image.width;
  tex->height = image.height;
  tex->pitch = image.pitch;

  return tex;
}

#define JA 1103 // я
#define CA 1040 // А - cyrillic 
#define CPLEN (95 + JA - CA)


typedef struct SCVFontDesc SCVFontDesc;
struct SCVFontDesc {
  f32       fontsize;
  SCVString fontpath; 
};

i32 codepointsbuffer[CPLEN];

SCVFont*
scvFontInit(SCVGLCtx *ctx, SCVArena *arena, SCVFontDesc *desc)
{
  SCVFont *font;
  f32 fontSize = desc->fontsize;
  stbtt_fontinfo fontInfo = {0};
  SCVError error = {0};
  SCVSlice fontData = scvLoadFile(desc->fontpath, &error);
  scvAssert(error.tag == 0);

  font = scvPoolAlloc(&ctx->Fonts);

  SCVSlice codepoints = scvUnsafeSlice(codepointsbuffer, CPLEN);

  // TODO(sichirc): figure out is it need to delete font, and if we need to do it
  // need to know how to allocate glyphs and release them.
  font->glyphs = scvMakeSlice(arena, SCVGlyph, codepoints.len, codepoints.len);
  SCVGlyph *glyphs = (SCVGlyph *)font->glyphs.base;

  for (u64 i = 0; i < codepoints.len; ++i) {
    ((rune *)codepoints.base)[i] = (i < 95) ? (i32)i + 32 : (i32)i + CA - 95;
  }

  stbtt_InitFont(&fontInfo, (unsigned char *)fontData.base, 0);
  f32 scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);

  stbtt_GetFontVMetrics(&fontInfo, &font->ascent, &font->descent, &font->linegap);
  font->scale = scale;
  font->size = desc->fontsize;

  i32 width = 0;
  i32 height = 0;
  rune *cp = (rune *)codepoints.base;
  i32 x1, x2, y1, y2;
  i32 padding = 2;

  for (u64 i = 0; i < codepoints.len; ++i) {
    stbtt_GetCodepointBitmapBox(&fontInfo, cp[i], scale, scale, &x1, &y1, &x2, &y2);
    glyphs[i].width = (x2 - x1);
    glyphs[i].height = (y2 - y1);
    width += (glyphs[i].width + padding);
    height = scvMax(height, glyphs[i].height);
    stbtt_GetCodepointHMetrics(&fontInfo, cp[i], &glyphs[i].xadvance, &glyphs[i].lsb);
    glyphs[i].xadvance = (i32)(scale * (f32)glyphs[i].xadvance);
  }

  height += padding;


  // packing 
  // NOTE(sichirc): for know simplest packing just in one very wide texture
  // where width is sum of glyphs width and height is height of tallest glyphs
  SCVImage bitmapImage = scvImage(arena, (u32)width, (u32)height);
  SCVGlyph *g; 
  u32 offset = 0;
  i32 tx = 0;
  i32 ty = 0;

  for (u64 i = 0; i < codepoints.len; ++i) {
    rune cp = ((rune *)codepoints.base)[i];
    g = glyphs + i;
    g->tx = tx;
    g->ty = ty;
    g->codepoint = cp;
    u8 *bitmap = (u8 *)stbtt_GetCodepointBitmap(
        &fontInfo,
        scale,
        scale,
        cp,
        nil,
        nil,
        &g->xoffset,
        &g->yoffset
    );
    u8 *source = bitmap; 
    byte *destRow = (byte *)bitmapImage.data + offset;
    for (i32 y = 0; y < g->height; ++y) {
      u32 *dest = (u32 *)destRow;
      for (i32 x = 0; x < g->width; ++x) {
        u8 alpha = *source++; 
        *dest++ = ((alpha << 24) | (alpha << 16) | (alpha << 8) | (alpha << 0));
      }
      destRow += bitmapImage.pitch;
    } 
    offset += (g->width * 4 + padding * 4);
    stbtt_FreeBitmap(bitmap, nil);
    tx += (g->width + padding);
  }

  font->texture = scvLoadTexture(ctx, bitmapImage);

  scvUnloadFile(fontData);

  return font;
}

u64
scvFindGlyph(SCVFont *font, rune codepoint)
{
  u64 result = 0;
  SCVGlyph* glyphs = (SCVGlyph *)font->glyphs.base;
  for (u64 i = 0; i < font->glyphs.len; ++i) {
    if (glyphs[i].codepoint == codepoint) {
      result = i;
      break;
    }
  }

  return result;
}

void
scvBindTexture(SCVGLCtx *ctx, SCVTexture *texture)
{
  SCVDrawCall drawcall = {0};
  scvAssert(ctx);
  scvAssert(texture);
  SCVDrawCall currentDrawCall = ((SCVDrawCall *)ctx->Drawcalls.base)[ctx->Drawcalls.len - 1];
  if (currentDrawCall.texID != texture->glTexID) {
    drawcall.start = ctx->Indicies.len;
    drawcall.len   = 0;
    drawcall.shaderID = ctx->DefaultShader;
    drawcall.texID = texture->glTexID;
    scvSliceAppend(ctx->Drawcalls, drawcall);
  }
}

SCVSize
scvMeasureText(SCVFont *font, SCVString text)
{
  SCVSize size = {0};
  SCVGlyph *glyph = nil;
  i32 glyphIndex = 0;
  rune r = 0;
  SCVError error = {0};
  SCVUTF8Iterator iterator = scvUTF8Iterator(text);

  size.height = font->size;
  while (scvUTF8HasNext(&iterator)) {
    r = scvUTF8GetNext(&iterator, &error);
    if (error.tag) {
      scvFatalError("not valid utf8", &error);
    }
    glyphIndex = scvFindGlyph(font, r);
    glyph = (SCVGlyph *)font->glyphs.base + glyphIndex;
    size.width += ((f32)glyph->xoffset + (f32)glyph->width);
  }

  return size;
}

#define roundf(n) (f32)((i32)(n))


void
scvDrawText(SCVGLCtx *ctx, SCVColor color, SCVFont *font, SCVPoint origin, SCVString text)
{

  SCVGlyph *glyph;
  SCVTexture *texture;
  u64 glyphIndex = 0;
  f32 baseline = 0.0f;
  rune r = 0;
  SCVError error = {0};
  SCVUTF8Iterator iterator = scvUTF8Iterator(text);
  SCVRect rect = {0};
  SCVUVRect uvrect = {0};

  texture = font->texture;
  rect.origin = origin;

  baseline = origin.y + (f32)font->ascent * (f32)font->scale;

  scvBindTexture(ctx, texture);

  while (scvUTF8HasNext(&iterator)) {
    r = scvUTF8GetNext(&iterator, &error);

    if (error.tag) {
      scvFatalError("not valid utf8", &error);
    }

    glyphIndex = scvFindGlyph(font, r);
    glyph = (SCVGlyph *)font->glyphs.base + glyphIndex;
    rect.origin.y = roundf(baseline + glyph->yoffset);

    rect.size.width = (f32)glyph->width;
    rect.size.height = (f32)glyph->height;

    uvrect.topleft[0] = ((f32)glyph->tx / (f32)texture->width); 
    uvrect.topleft[1] = ((f32)glyph->ty / (f32)texture->height);

    uvrect.topright[0] = ((f32)(glyph->tx + glyph->width) / (f32)texture->width);
    uvrect.topright[1] = ((f32)glyph->ty / (f32)texture->height);

    uvrect.bottomleft[0] = ((f32)glyph->tx / (f32)texture->width);
    uvrect.bottomleft[1] = ((f32)(glyph->ty + glyph->height) / (f32)texture->height);

    uvrect.bottomright[0] = ((f32)(glyph->tx + glyph->width) / (f32)texture->width);
    uvrect.bottomright[1] = ((f32)glyph->height / (f32)texture->height); 

    scvGLDrawRectInternal(ctx, rect, color, &uvrect);
    rect.origin.x += (f32)glyph->xoffset + (f32)glyph->xadvance;
  }

}


/*
void
scvFontInit(SCVArena *arena, SCVFont *font, SCVSlice codepoints, SCVError  *error)
{
  i32 defaultCpsLen;
  SCVImage image = {0};
  if (codepoints.len == 0) {
    // all latins + cyrillic symbols (russian subset)
    defaultCpsLen = 95 + JA - CA;
    codepoints = scvMakeSlice(arena, rune, defaultCpsLen, defaultCpsLen);
    for (i32 i = 0; i < 95; ++i) {
      (rune *)codepoints.base[i] = i + 32;
    }
    for (i32 i = A; i <= JA; ++i) {
      (rune *)codepoints.base[i] = i;
    }
  }
  rune *cp = (rune *)codepoints.base;
  SCVSlice glyphsSlice = scvMakeSlice(arena, SCVGlyph, codepoints.len, codepoints.len);
  SCVGlyph *glyphs = (SCVGlyph *)glyphsSlice.base;
  u32 texturewidth = 0;
  u32 textureheight = 0;

  byte* textureBuffer = scvArenaAlloc(arena, PAGE_SIZE);
  u64   textureBufferSize = PAGE_SIZE;
  i32   byteoffset = 0;

  if (stbtt_InitFont(&fontInfo, (unsigned char *)fileData, 0)) {
     
    // FONT_INFO
    font->scale = stbtt_ScaleForPixelHeight(&fontInfo, (f32)fontSize);
    stbtt_GetFontVMetrics(&)
    
    for (i32 i = 0; i < codepoints.len; ++i) {
      rune ch = cp[i];
      glyphs[i].codepoint = ch;
      i32 index = stbtt_FindGlyphIndex(&fontInfo, ch);
      if (index <= 0) {
        index = stbtt_ScaleForPixelHeight(&fontInfo, 0x0000FFFD);
        scvAssert(indx > 0);
      }
      glyphs[i].index = index;


      glyphBuffer = stbtt_GetGlyphBitmap(
          &fontInfo,
          font->scale,
          font->scale,
          glyphs[i].index,
          &glyphs[i].width,
          &glyphs[i].height,
          &glyphs[i].xoff,
          &glyphs[i].yoff
      );

      byteoffset += width 


      stbtt_FreeBitmap(glyphBuffer, nil);

      stbtt_GetCodepointHMetrics(&fontInfo, index, &glyphs[i].advanceWith, &glyphs[i].leftSideBearing);

      u64 datasize = glyphs[i].width * glyphs[i].height * 4;
      texturewidth += glyphs[i].width;
      textureheight = scvMax(textureheight, glyphs[i].height);
    } 


  } else {
    scvErrorSet(error, "failed to initialize font", 1);
  }

}

void
scvRenderText(SCVGLCtx *ctx, SCVText *text)
{
  
}

*/
#endif
