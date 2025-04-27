#ifndef APP
#define APP

// NOTE(sichirc): For now put all things in one Context
// in future maybe better put things related to renderer
// into different RendererContext, but while there only OpenGL
// Renderer its fine

typedef struct Context Context;
struct Context
{
  u64           TimeStart;
  SCVArena      arena;
  SCVGLCtx      GLContext;
  SCVRect       Window;
  SCVTimer      Timer;
  u32           scvTextureID;
  SCVFont       *font;
};

Context GlobalContext = {0};

enum AppErrors
{
  SHADER_COMPILE_ERR,
  SHADERS_LINK_ERR,
};

void
InitContext(Context* ctx, SCVRect window, f32 scaleFactor)
{
  SCVError error = {0};
  scvAssert(ctx);
  scvClear((void *)ctx, sizeof(Context));
  scvInitTimer(&ctx->Timer);

  scvArenaInit(&ctx->arena, &error);
  scvAssert(error.tag == 0);
  scvGLCtxInit(&ctx->GLContext, &((SCVGLCtxDesc){
    .arena       = &ctx->arena,
    .scaleFactor = scaleFactor,
    .viewport    = window
  }));
}

void
AppInit(Context* ctx, SCVRect window, f32 scaleFactor)
{
  int      comp, width, height;
  SCVSlice scvImageContent = {0};
  SCVError error = {0};
  SCVImage scvLogoImage = {0};
  InitContext(ctx, window, scaleFactor);

  scvImageContent = scvLoadFile(scvUnsafeCString("scv.jpg"), &error);
  scvAssert(error.tag == 0);

  scvLogoImage.data = stbi_load_from_memory(
      scvImageContent.base,
      scvImageContent.len,
      &width,
      &height,
      &comp,
      0
  );

  scvAssert(scvLogoImage.data);
  scvLogoImage.width = (u64)width;
  scvLogoImage.height = (u64)height;
  scvLogoImage.mipmapcount = 1;
  
  if (comp == 1)      scvLogoImage.pixelformat = SCV_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
  else if (comp == 2) scvLogoImage.pixelformat = SCV_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA;
  else if (comp == 3) scvLogoImage.pixelformat = SCV_PIXELFORMAT_UNCOMPRESSED_R8G8B8;
  else if (comp == 4) scvLogoImage.pixelformat = SCV_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

  ctx->scvTextureID = scvGLLoadTexture(scvLogoImage);

  ctx->font = scvFontInit(&ctx->GLContext, &ctx->arena, &((SCVFontDesc){
    .fontsize = 36.0,
    .fontpath = scvUnsafeCString("./assets/3270-Regular.ttf")
  }));

  int maxTexSize;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
  scvPrint("MAXTEXSIZE");
  scvPrintU64((u64)maxTexSize);
  
  stbi_image_free(scvLogoImage.data);
  scvUnloadFile(scvImageContent);
}

void
AppUpdate(Context* ctx)
{
  SCVGLCtx *glctx; 
  glctx = &ctx->GLContext;
  glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  scvGLBegin(glctx);

  scvGLDrawImage(
      glctx,
      (SCVRect){
        .origin = { 256.0f, 256.0f },
        .size = { 256.0f, 256.0f }
      },
      (SCVColor){ 255, 255, 255, 255 },
      ctx->scvTextureID
  );

  SCVString str = scvUnsafeCString("абвгдеёжзиклмнопрст");

  SCVRect rect = {0};
  
  rect.origin.x = 10.0f;
  rect.size = scvMeasureText(ctx->font, str);

  //scvGLDrawRect(glctx, rect, (SCVColor){ 255, 255, 0, 255});

  scvDrawText(
      glctx, 
      (SCVColor){ 255, 0, 255, 255 },
      ctx->font,
      (SCVPoint){ 10.0f, 0.0f },
      str
  );

  scvGLEnd(glctx);
}

#endif
