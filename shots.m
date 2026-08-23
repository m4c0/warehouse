#include "mtl.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static int run(int w, int h) {
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();

  MTLTextureDescriptor * td = [MTLTextureDescriptor
    texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                 width:w
                                height:h
                             mipmapped:NO];
  td.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead; 

  id<MTLTexture> txt = [device newTextureWithDescriptor:td];
  if (!txt) return (NSLog(@"Failed to create offscreen texture"), 1);

  MTLRenderPassDescriptor * rpd = [MTLRenderPassDescriptor renderPassDescriptor];
  rpd.colorAttachments[0].texture = txt;
  rpd.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);

  NSString * path = [[NSBundle mainBundle] pathForResource:@"levels" ofType:@"txt"];
  NSData * data = [NSData dataWithContentsOfFile:path];
  glu_init_t t = {
    .level    = [data bytes],
    .level_sz = [data length],
    .sound    = 0,
    .scr_w    = w,
    .scr_h    = h,
  };
  glu_init(&t);

  POCStuff * stuff = [POCStuff newWithDevice:device];
  [stuff draw:NSMakeSize(w, h) rpd:rpd into:nil];

  void * raw = malloc(w * h * 4);
  [txt getBytes:raw
    bytesPerRow:w * 4
     fromRegion:MTLRegionMake2D(0, 0, w, h)
    mipmapLevel:0];

  char fn[1024];
  snprintf(fn, 1024, "shot-%dx%d.png", w, h);
  stbi_write_png(fn, w, h, 4, raw, w * 4);

  glu_deinit();

  return 0;
}

int main() {
  @autoreleasepool {
    // icon
    if (run(1024, 1024)) return 1;

    // apple store
    if (run(2736, 1260)) return 1;
    if (run(2778, 1284)) return 1;

    // windows store
    if (run(1366, 768)) return 1;

    // itch
    if (run(630, 500)) return 1;

    return 0;
  }
}
