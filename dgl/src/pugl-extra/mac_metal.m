/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2026 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if __has_include("../pugl-upstream/src/stub.h")
# include "../pugl-upstream/src/internal.h"
# include "../pugl-upstream/src/mac.h"
# include "../pugl-upstream/src/stub.h"
# include "../pugl-upstream/src/types.h"
#else
# include "pugl-upstream/src/internal.h"
# include "pugl-upstream/src/mac.h"
# include "pugl-upstream/src/stub.h"
# include "pugl-upstream/src/types.h"
#endif

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface PuglMetalView : NSView<CALayerDelegate>
@end

@implementation PuglMetalView {
@public
    PuglView* puglview;
}

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        self.wantsLayer = YES;
        self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawOnSetNeedsDisplay;
    }
    return self;
}

- (CALayer*)makeBackingLayer
{
    CAMetalLayer* const layer = [CAMetalLayer layer];
    [layer setDelegate:self];
    return layer;
}

- (void)viewDidChangeBackingProperties
{
    [super viewDidChangeBackingProperties];
    NSWindow* const window = [self window];
    if (window != nil) {
        self.layer.contentsScale = [window backingScaleFactor];
    }
}

- (void)setFrameSize:(NSSize)newSize
{
    PuglWrapperView* const wrapper = (PuglWrapperView*)[self superview];
    [super setFrameSize:newSize];
    if (wrapper != nil) {
        [wrapper setReshaped];
    }
    self.layer.frame = self.bounds;
    NSWindow* const window = [self window];
    if (window != nil) {
        self.layer.contentsScale = [window backingScaleFactor];
    }
}

- (void)displayLayer:(CALayer*)layer
{
    (void)layer;
    PuglWrapperView* const wrapper = (PuglWrapperView*)[self superview];
    if (wrapper != nil) {
        [wrapper dispatchExpose:[self bounds]];
    }
}

@end

static PuglStatus
puglMacMetalCreate(PuglView* view)
{
    PuglInternals* const impl = view->impl;
    PuglMetalView* const drawView = [PuglMetalView alloc];
    const NSRect rect = NSMakeRect(0, 0, view->lastConfigure.width, view->lastConfigure.height);

    drawView->puglview = view;
    [drawView initWithFrame:rect];
    if (view->hints[PUGL_RESIZABLE]) {
        [drawView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    } else {
        [drawView setAutoresizingMask:NSViewNotSizable];
    }

    impl->drawView = drawView;
    return PUGL_SUCCESS;
}

static void
puglMacMetalDestroy(PuglView* view)
{
    PuglMetalView* const drawView = (PuglMetalView*)view->impl->drawView;
    if (drawView != nil) {
        [drawView removeFromSuperview];
        [drawView release];
        view->impl->drawView = nil;
    }
}

static void*
puglMacMetalGetContext(PuglView* view)
{
    PuglInternals* const impl = view->impl;
    if (impl != NULL && impl->drawView != nil) {
        return (void*)[impl->drawView layer];
    }
    return NULL;
}

const PuglBackend*
puglMetalBackend(void)
{
    static const PuglBackend backend = {
        puglStubConfigure,
        puglMacMetalCreate,
        puglMacMetalDestroy,
        puglStubEnter,
        puglStubLeave,
        puglMacMetalGetContext
    };
    return &backend;
}
