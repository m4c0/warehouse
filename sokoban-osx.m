@import AppKit;
@import AudioUnit;
@import MetalKit;

#include "mtl.h"

@interface POCWindow : NSWindow
@end
@implementation POCWindow
- (BOOL)acceptsFirstResponder {
  return YES;
}
- (void)keyDown:(NSEvent *)event {
  NSString * chrs = event.charactersIgnoringModifiers;
  if (chrs.length != 1) return;

  unichar c = [chrs characterAtIndex:0];
  switch (c) {
    case NSLeftArrowFunctionKey:  return glu_move(-1,  0);
    case NSRightArrowFunctionKey: return glu_move( 1,  0);
    case NSUpArrowFunctionKey:    return glu_move( 0, -1);
    case NSDownArrowFunctionKey:  return glu_move( 0,  1);
  }
}
- (void) mouseDown:(NSEvent *)event {
  CGPoint liw = [event locationInWindow];
  CGPoint p = [self.contentViewController.view convertPoint:liw fromView:nil];
  glu_mouse_down(p.x, self.contentViewController.view.frame.size.height - p.y);
}
- (void) mouseUp:(NSEvent *)event {
  CGPoint liw = [event locationInWindow];
  CGPoint p = [self.contentViewController.view convertPoint:liw fromView:nil];
  glu_mouse_up(p.x, self.contentViewController.view.frame.size.height - p.y);
}
- (void) mouseMoved:(NSEvent *)event {
  CGPoint liw = [event locationInWindow];
  CGPoint p = [self.contentViewController.view convertPoint:liw fromView:nil];
  glu_mouse_move(p.x, self.contentViewController.view.frame.size.height - p.y);
}
- (void) mouseDragged:(NSEvent *)event {
  CGPoint liw = [event locationInWindow];
  CGPoint p = [self.contentViewController.view convertPoint:liw fromView:nil];
  glu_mouse_move(p.x, self.contentViewController.view.frame.size.height - p.y);
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
  glu_deinit();
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app {
  return YES;
}
@end

static void run() {
  POCViewController * vc = [POCViewController new];
  vc.view = [POCViewDelegate new];

  NSWindow * w = [POCWindow new];
  w.acceptsMouseMovedEvents = YES;
  w.contentViewController = vc;
  w.styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

  NSRect crect = NSMakeRect(0, 0, 800, 600);
  NSRect frect = [w frameRectForContentRect:crect];
  [w setFrame:frect display:YES];
  [w center];
  [w makeKeyAndOrderFront:w];

  // Apple menu
  NSMenu * menu = [NSMenu new];
  [menu       addItem:[[NSMenuItem alloc]
        initWithTitle:@"Quit Sokoban"
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
