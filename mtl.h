@import AudioToolbox;
@import MetalKit;

#include "glu.h"

void sfx_save_prefs() {
  CFPropertyListRef value = sfx_enabled() ? kCFBooleanTrue : kCFBooleanFalse;
  CFPreferencesSetAppValue(CFSTR("sound"), value, kCFPreferencesCurrentApplication);
  CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
}

void sav_get_path(char * buf, unsigned buf_sz) {
  NSArray * arr = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  NSString * dir = [arr firstObject];
  [[NSFileManager defaultManager] createDirectoryAtPath:dir
                            withIntermediateDirectories:YES
                                             attributes:nil
                                                  error:nil];
  strncpy(buf, dir.UTF8String, buf_sz);
}

static id<MTLLibrary> load_library(id<MTLDevice> device, NSString * name) {
  NSString * path = [[NSBundle mainBundle] pathForResource:name ofType:@"metal"];
  NSString * src = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
  MTLCompileOptions * opts = [MTLCompileOptions new];
  NSError * err;
  id<MTLLibrary> lib = [device newLibraryWithSource:src options:opts error:&err];
  if (err) {
    NSLog(@"Error compiling shader: %@", err);
    return nil;
  }
  return lib;
}

@interface POCStuff : NSObject
@property (nonatomic,strong) id<MTLCommandQueue> queue;
@property (nonatomic,strong) id<MTLRenderPipelineState> pipeline;
@property (nonatomic,strong) id<MTLRenderPipelineState> pipeline_mui;
@property (nonatomic,strong) id<MTLTexture> txt;
@property (nonatomic,strong) id<MTLSamplerState> smp;
@property (nonatomic,strong) id<MTLBuffer> grid;
+ (id)newWithDevice:(id<MTLDevice>)device;
- (void)resize:(CGSize)size;
- (void)draw:(CGSize)size rpd:(MTLRenderPassDescriptor *)rpd into:(id<CAMetalDrawable>)drawable;
@end

static void mtl_mui_draw(void * ptr, const mui_upc_t * t) {
  id<MTLRenderCommandEncoder> enc = ptr;
  [enc setVertexBytes:t length:sizeof(mui_upc_t) atIndex:0];
  [enc setFragmentBytes:t length:sizeof(mui_upc_t) atIndex:0];
  [enc drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
}
static void mtl_mui_scissor(void * ptr, unsigned x, unsigned y, unsigned w, unsigned h) {
  MTLScissorRect rect = {
    .x      = x,
    .y      = y,
    .width  = w,
    .height = h,
  };
  id<MTLRenderCommandEncoder> enc = ptr;
  [enc setScissorRect:rect];
}

@implementation POCStuff
+ (id)newWithDevice:(id<MTLDevice>)device {
  POCStuff * d = [POCStuff new];
  d.queue = [device newCommandQueue];
  d.grid = [device newBufferWithLength:GLU_BUF_SIZE options:MTLResourceStorageModeShared];

  id<MTLLibrary> vert = load_library(device, @"sokoban.vert");
  id<MTLLibrary> frag = load_library(device, @"sokoban.frag");
  if (!vert || !frag) return nil;

  MTLRenderPipelineDescriptor * pd = [MTLRenderPipelineDescriptor new];
  pd.vertexFunction   = [vert newFunctionWithName:@"main0"];
  pd.fragmentFunction = [frag newFunctionWithName:@"main0"];
  pd.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
  NSError * err;
  d.pipeline = [device newRenderPipelineStateWithDescriptor:pd error:&err];
  if (err) return (NSLog(@"Error creating pipeline: %@", err), nil);

  vert = load_library(device, @"mui-vlk.vert");
  frag = load_library(device, @"mui-vlk.frag");
  if (!vert || !frag) return nil;

  pd = [MTLRenderPipelineDescriptor new];
  pd.vertexFunction   = [vert newFunctionWithName:@"main0"];
  pd.fragmentFunction = [frag newFunctionWithName:@"main0"];
  pd.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
  pd.colorAttachments[0].blendingEnabled = true;
  pd.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
  pd.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
  pd.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
  pd.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
  d.pipeline_mui = [device newRenderPipelineStateWithDescriptor:pd error:&err];
  if (err) return (NSLog(@"Error creating pipeline: %@", err), nil);

  MTLTextureDescriptor * td = [MTLTextureDescriptor new];
  td.pixelFormat = MTLPixelFormatR8Unorm;
  td.width       = 128;
  td.height      = 32;
  d.txt = [device newTextureWithDescriptor:td];

  NSString * path = [[NSBundle mainBundle] pathForResource:@"atlas" ofType:@"img"];
  NSData * data = [NSData dataWithContentsOfFile:path];
  MTLRegion r = { {0,0,0}, {128,32,1} };
  [d.txt replaceRegion:r mipmapLevel:0 withBytes:[data bytes] bytesPerRow:128];

  MTLSamplerDescriptor * sd = [MTLSamplerDescriptor new];
  sd.minFilter = sd.magFilter = MTLSamplerMinMagFilterNearest;
  d.smp = [device newSamplerStateWithDescriptor:sd];

  return d;
}
- (void)resize:(CGSize)size {
  glu_resize(size.width, size.height);
}
- (void)draw:(CGSize)size rpd:(MTLRenderPassDescriptor *)rpd into:(id<CAMetalDrawable>)drawable {
  if (rpd == nil) return;

  glu_load(self.grid.contents);
  glu_frame();

  id<MTLCommandBuffer> cb = [self.queue commandBuffer];

  id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rpd];
  [enc setRenderPipelineState:self.pipeline];
  [enc setVertexBytes:&glu_pc length:sizeof(glu_upc_t) atIndex:0];
  [enc setFragmentBytes:&glu_pc length:sizeof(glu_upc_t) atIndex:1];
  [enc setFragmentBuffer:self.grid offset:0 atIndex:0];
  [enc setFragmentTexture:self.txt atIndex:0];
  [enc setFragmentSamplerState:self.smp atIndex:0];
  [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];

  [enc setRenderPipelineState:self.pipeline_mui];
  [enc setFragmentTexture:self.txt atIndex:0];
  [enc setFragmentSamplerState:self.smp atIndex:0];
  glu_ui((mui_api_t[]) {{
    .ptr     = enc,
    .draw    = mtl_mui_draw,
    .scissor = mtl_mui_scissor,
  }});
  [enc endEncoding];

  if (drawable) [cb presentDrawable:drawable];
  [cb commit];
  if (!drawable) [cb waitUntilCompleted];
}
@end

@interface POCViewDelegate : MTKView<MTKViewDelegate>
@property (nonatomic,strong) POCStuff * stuff;
@property (nonatomic) BOOL ready;
+ (id)new;
@end
@implementation POCViewDelegate
+ (id)new {
  POCViewDelegate * d = [[POCViewDelegate alloc] init];
  d.device     = MTLCreateSystemDefaultDevice();
  d.stuff      = [POCStuff newWithDevice:d.device];
  d.clearColor = MTLClearColorMake(0.01, 0.02, 0.03, 1.0);
  d.delegate   = d;
  return d;
}
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
  if (self.ready) [self.stuff resize:view.frame.size];
}
- (void)drawInMTKView:(MTKView *)view {
  if (!self.ready) {
    Boolean exists;
    Boolean res = CFPreferencesGetAppBooleanValue(CFSTR("sound"), kCFPreferencesCurrentApplication, &exists);

    NSString * path = [[NSBundle mainBundle] pathForResource:@"levels" ofType:@"txt"];
    NSData * data = [NSData dataWithContentsOfFile:path];

    glu_init_t t = {
      .level    = [data bytes],
      .level_sz = [data length],
      .sound    = exists ? res : 1,
      .scr_w    = view.frame.size.width,
      .scr_h    = view.frame.size.height,
    };
    glu_init(&t);

    self.ready = YES;
  }

  MTLRenderPassDescriptor * rpd = view.currentRenderPassDescriptor;
  [self.stuff draw:view.frame.size rpd:rpd into:view.currentDrawable];
}
@end
