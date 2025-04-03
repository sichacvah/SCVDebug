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
};

Context GlobalContext = { 0 };

enum AppErrors
{
  SHADER_COMPILE_ERR,
  SHADERS_LINK_ERR,
};

void
InitContext(Context* ctx, SCVRect window, f32 scaleFactor)
{
  SCVError error = { 0 };
  scvAssert(ctx);
  scvClear((void *)ctx, sizeof(Context));
  scvInitTimer(&ctx->Timer);

  scvArenaInit(&ctx->arena, &error);
  scvAssert(error.tag == 0);
  scvGLCtxInit(&ctx->arena, &ctx->GLContext, 1024, 1, window, scaleFactor);
}

void
AppInit(Context* ctx, SCVRect window, f32 scaleFactor)
{
  InitContext(ctx, window, scaleFactor);
}

void
AppUpdate(Context* ctx)
{
  SCVGLCtx *glctx; 
  glctx = &ctx->GLContext;
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  scvGLBegin(glctx);

  scvGLDrawRect(
      glctx,
      (SCVRect){
        .origin = { 0.0f, 0.0f },
        .size = { 256.0f, 256.0f } 
      },
      (SCVColor){ 255, 255, 255, 255 }
  );

  scvGLDrawRect(
      glctx,
      (SCVRect){
        .origin = { 256.0f, 256.0f },
        .size = { 256.0f, 256.0f }
      },
      (SCVColor){ 255, 255, 255, 255 }
  );

  scvGLEnd(glctx);
}

#endif
