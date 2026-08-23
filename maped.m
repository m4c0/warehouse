@import MetalKit;

#include "maped.h"

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

static unsigned * mpd_grid_ptr;
void mpd_update_map() {
  for (int i = 0; i < LVL_SZ; i++) mpd_grid_ptr[i] = mpd_ptr[i];
}

static inline char * slurp(const char * file, unsigned * osz) {
  FILE * f = fopen(file, "rb");
  assert(f);

  assert(0 == fseek(f, 0, SEEK_END));
  long sz = ftell(f);
  assert(sz);
  assert(0 == fseek(f, 0, SEEK_SET));

  char * data = malloc(sz + 1);
  assert(1 == fread(data, sz, 1, f));
  data[sz] = 0;

  fclose(f);
  if (osz) *osz = sz;
  return data;
}

@interface POCViewDelegate : MTKView<MTKViewDelegate>
@property (nonatomic,strong) id<MTLCommandQueue> queue;
@property (nonatomic,strong) id<MTLRenderPipelineState> pipeline;
@property (nonatomic,strong) id<MTLBuffer> grid;
@property (nonatomic,strong) id<MTLTexture> atlas;
@property (nonatomic,strong) id<MTLSamplerState> atlas_smp;
+ (id)newWithDevice:(id<MTLDevice>)device;
@end
@implementation POCViewDelegate
+ (id)newWithDevice:(id<MTLDevice>)device {
  POCViewDelegate * d = [POCViewDelegate new];
  d.device = device;
  d.queue = [device newCommandQueue];
  d.grid = [device newBufferWithLength:LVL_SZ * 4 options:MTLResourceStorageModeShared];
  mpd_grid_ptr = d.grid.contents;

  MTLTextureDescriptor * td = [MTLTextureDescriptor new];
  td.pixelFormat = MTLPixelFormatR8Unorm;
  td.width       = 128;
  td.height      = 32;
  d.atlas = [device newTextureWithDescriptor:td];

  char atlas[128 * 32];
  FILE * f = fopen("atlas.img", "rb");
  fread(atlas, 128 * 32, 1, f);
  fclose(f);
  MTLRegion r = { {0,0,0}, {128,32,1} };
  [d.atlas replaceRegion:r mipmapLevel:0 withBytes:atlas bytesPerRow:128];

  MTLSamplerDescriptor * sd = [MTLSamplerDescriptor new];
  sd.minFilter = sd.magFilter = MTLSamplerMinMagFilterNearest;
  d.atlas_smp = [device newSamplerStateWithDescriptor:sd];

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

  unsigned sz;
  const char * data = slurp("levels.txt", &sz);
  lvl_init(data, sz);

  mpd_load_map(0);

  return d;
}
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
}
- (void)drawInMTKView:(MTKView *)view {
  MTLRenderPassDescriptor * rpd = view.currentRenderPassDescriptor;
  if (rpd == nil) return;

  mpd_frame();

  id<MTLCommandBuffer> cb = [self.queue commandBuffer];

  id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rpd];
  [enc setRenderPipelineState:self.pipeline];
  [enc setVertexBytes:&mpd_pc length:sizeof(mpd_upc_t) atIndex:0];
  [enc setFragmentBytes:&mpd_pc length:sizeof(mpd_upc_t) atIndex:1];
  [enc setFragmentBuffer:self.grid offset:0 atIndex:0];
  [enc setFragmentTexture:self.atlas atIndex:0];
  [enc setFragmentSamplerState:self.atlas_smp atIndex:0];
  [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
  [enc endEncoding];

  [cb presentDrawable:view.currentDrawable];
  [cb commit];
}

- (BOOL)acceptsFirstResponder {
  return YES;
}
- (void)keyDown:(NSEvent *)event {
  NSString * chrs = event.charactersIgnoringModifiers;
  if (chrs.length != 1) return;

  unichar c = [chrs characterAtIndex:0];
  switch (c) {
    case NSLeftArrowFunctionKey:  return mpd_cursor(-1,  0);
    case NSRightArrowFunctionKey: return mpd_cursor( 1,  0);
    case NSUpArrowFunctionKey:    return mpd_cursor( 0, -1);
    case NSDownArrowFunctionKey:  return mpd_cursor( 0,  1);

    //case ' ': return mpd_toggle();

    case 'b': return mpd_box();
    case 'p': return mpd_player();
    case 't': return mpd_target();
    case 'w': return mpd_wall();
    case ' ': return mpd_empty();

    case '[': return mpd_load_prev_level();
    case ']': return mpd_load_next_level();
  }
}
@end

@interface POCViewController : NSViewController
@end
@implementation POCViewController
@end

@interface POCAppDelegate : NSObject<NSApplicationDelegate>
@end
@implementation POCAppDelegate
- (void)applicationWillTerminate:(NSApplication *)app {
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app {
  return YES;
}
- (void)save {
  mpd_save();
}
@end

static void run() {
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();

  POCViewDelegate * v = [POCViewDelegate newWithDevice:device];
  v.delegate = v;

  POCViewController * vc = [POCViewController new];
  vc.view = v;

  NSWindow * w = [NSWindow new];
  w.contentViewController = vc;
  w.styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;

  NSRect crect = NSMakeRect(0, 0, 800, 600);
  NSRect frect = [w frameRectForContentRect:crect];
  [w setFrame:frect display:YES];
  [w center];
  [w makeKeyAndOrderFront:w];

  // Apple menu
  NSMenu * menu = [NSMenu new];
  [menu       addItem:[[NSMenuItem alloc]
        initWithTitle:@"Save"
               action:@selector(save)
        keyEquivalent:@"s"]];
  [menu       addItem:[[NSMenuItem alloc]
        initWithTitle:@"Quit Maped"
               action:@selector(terminate:)
        keyEquivalent:@"q"]];

  NSMenuItem * item = [NSMenuItem new];
  item.submenu = menu;

  NSMenu * bar = [NSMenu new];
  [bar addItem:item];

  NSApplication * a = [NSApplication sharedApplication];
  a.delegate = [POCAppDelegate new];
  a.mainMenu = bar;
  [a activateIgnoringOtherApps:YES];
  [a run];
}

int main() {
  @autoreleasepool {
    run();
  }
}
