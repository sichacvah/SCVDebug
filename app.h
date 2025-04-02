#ifndef APP
#define APP

#include <OpenGL/gl3.h>
#include "scv.h"
#include "scv_ui.h"
#include "scv_gl.h"
#include "scv_geom.h"
#include <stdbool.h>


// NOTE(sichirc): For now put all things in one Context
// in future maybe better put things related to renderer
// into different RendererContext, but while there only OpenGL
// Renderer its fine

typedef struct Context Context;

struct Context
{
  u64           TimeStart;
  u64           PrevTime;
  SCVArena      arena;
  SCVStorages   storages;
  SCVGLCtx      GLContext;
  SCVRect       Window;
};

Context GlobalContext = { 0 };

enum AppErrors
{
  SHADER_COMPILE_ERR,
  SHADERS_LINK_ERR,
};

SCVImageID imgid;

void
InitContext(Context* ctx, SCVRect window, f32 scaleFactor)
{
  SCVError error = { 0 };
  assert(ctx);
  scvClear((void *)ctx, sizeof(Context));
  scvInitTimer();
  ctx->TimeStart = scvCntVct();
  ctx->PrevTime = ctx->TimeStart;

  scvArenaInit(&ctx->arena, nil, 0);
  scvStoragesInit(&ctx->arena, &ctx->storages, &(SCVStoragesDesc){
    .imagesSize   = 128 * 4,
    .texturesSize = 128 * 4,
  });
  scvGLCtxInit(&ctx->arena, &ctx->GLContext, 1024, 1, window, scaleFactor);

  imgid = scvImageLoad(&ctx->storages, scvUnsafeCString("./wall.png"), &(SCVImageDesc){1}, &error);
  scvAssert(error.tag == 0);
}


void
AppInit(Context* ctx, SCVRect window, f32 scaleFactor)
{
  InitContext(ctx, window, scaleFactor);
}

u64
ElapsedTime(Context* ctx)
{
  u64 result;
  u64 time = scvCntVct();
  result = scvCntVctMs(time - ctx->PrevTime);
  ctx->PrevTime = time;
  return result;
}

f32 prev = 0.0f;
f32 velocity = 0.0001f;

void
AppUpdate(Context* ctx)
{
  //SCVTextureID texid;
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
