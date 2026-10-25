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

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "dgl/Application.hpp"
#include "dgl/Window.hpp"
#include "dgl/TopLevelWidget.hpp"
#include "dgl/Metal.hpp"
#include <cassert>
#include <cstdio>

USE_NAMESPACE_DGL

class MetalTestWidget : public TopLevelWidget
{
public:
    int renderPassCount;
    bool verifiedContext;

    MetalTestWidget(Window& parent)
        : TopLevelWidget(parent),
          renderPassCount(0),
          verifiedContext(false)
    {
    }

protected:
    void onDisplay() override
    {
        const MetalGraphicsContext& ctx = static_cast<const MetalGraphicsContext&>(getGraphicsContext());

        assert(ctx.layer != nullptr);
        assert(ctx.device != nullptr);
        assert(ctx.commandQueue != nullptr);
        assert(ctx.currentDrawable != nullptr);
        assert(ctx.currentCommandEncoder != nullptr);
        assert(ctx.currentCommandBuffer != nullptr);

        id<MTLDevice> device = ctx.getDevice();
        assert(device != nil);

        CAMetalLayer* layer = ctx.getLayer();
        assert(layer != nil);

        id<CAMetalDrawable> drawable = ctx.getCurrentDrawable();
        assert(drawable != nil);

        id<MTLRenderCommandEncoder> encoder = ctx.getCurrentCommandEncoder();
        assert(encoder != nil);

        if (!verifiedContext)
        {
            printf("[TestMetal] GPU Device Name: %s\n", [[device name] UTF8String]);
            printf("[TestMetal] Low-power device: %s\n", [device isLowPower] ? "YES" : "NO");
            printf("[TestMetal] Headless device: %s\n", [device isHeadless] ? "YES" : "NO");
            printf("[TestMetal] Backing CAMetalLayer bounds: %.1f x %.1f (contentsScale: %.1f)\n",
                   layer.bounds.size.width, layer.bounds.size.height, layer.contentsScale);
            printf("[TestMetal] Drawable texture size: %lu x %lu\n",
                   (unsigned long)drawable.texture.width, (unsigned long)drawable.texture.height);
            verifiedContext = true;
        }

        renderPassCount++;
    }
};

int main()
{
    printf("=== DGL Apple Metal Backend Verification Test ===\n");

    Application app(true);
    Window win(app);
    win.setTitle("DGL Metal Backend Test");
    win.setSize(400, 300);

    MetalTestWidget widget(win);

    win.show();

    // Pump events and render several frames
    for (int i = 0; i < 5; ++i)
    {
        win.repaint();
        app.idle();
    }

    assert(widget.renderPassCount > 0);
    assert(widget.verifiedContext);

    win.close();
    app.quit();

    printf("[TestMetal] SUCCESS: Verified %d Metal render passes with active MTLDevice and CAMetalDrawable presentation!\n",
           widget.renderPassCount);

    return 0;
}
