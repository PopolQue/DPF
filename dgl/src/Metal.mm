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

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "../Metal.hpp"
#include "../Color.hpp"
#include "../ImageBaseWidgets.hpp"

#include "SubWidgetPrivateData.hpp"
#include "TopLevelWidgetPrivateData.hpp"
#include "WidgetPrivateData.hpp"
#include "WindowPrivateData.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Check for correct build config

#ifndef DGL_METAL
# error Build config error, Metal was NOT requested while building Metal code
#endif
#ifdef DGL_CAIRO
# error Build config error, Cairo requested while building Metal code
#endif
#ifdef DGL_OPENGL
# error Build config error, OpenGL requested while building Metal code
#endif
#ifdef DGL_VULKAN
# error Build config error, Vulkan requested while building Metal code
#endif

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

static void notImplemented(const char* const name)
{
    d_stderr2("Metal function not implemented: %s", name);
}

// --------------------------------------------------------------------------------------------------------------------
// Color

void Color::setFor(const GraphicsContext&, bool)
{
    notImplemented("Color::setFor");
}

// --------------------------------------------------------------------------------------------------------------------
// Line

template<typename T>
void Line<T>::draw(const GraphicsContext&, T)
{
    notImplemented("Line::draw");
}

#ifdef DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Line<T>::draw()
{
    notImplemented("Line::draw");
}
#endif

template class Line<double>;
template class Line<float>;
template class Line<int>;
template class Line<uint>;
template class Line<short>;
template class Line<ushort>;

// --------------------------------------------------------------------------------------------------------------------
// Circle

template<typename T>
void Circle<T>::draw(const GraphicsContext&)
{
    notImplemented("Circle::draw");
}

template<typename T>
void Circle<T>::drawOutline(const GraphicsContext&, T)
{
    notImplemented("Circle::drawOutline");
}

#ifdef DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Circle<T>::draw()
{
    notImplemented("Circle::draw");
}

template<typename T>
void Circle<T>::drawOutline()
{
    notImplemented("Circle::drawOutline");
}
#endif

template class Circle<double>;
template class Circle<float>;
template class Circle<int>;
template class Circle<uint>;
template class Circle<short>;
template class Circle<ushort>;

// --------------------------------------------------------------------------------------------------------------------
// Triangle

template<typename T>
void Triangle<T>::draw(const GraphicsContext&)
{
    notImplemented("Triangle::draw");
}

template<typename T>
void Triangle<T>::drawOutline(const GraphicsContext&, T)
{
    notImplemented("Triangle::drawOutline");
}

#ifdef DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Triangle<T>::draw()
{
    notImplemented("Triangle::draw");
}

template<typename T>
void Triangle<T>::drawOutline()
{
    notImplemented("Triangle::drawOutline");
}
#endif

template class Triangle<double>;
template class Triangle<float>;
template class Triangle<int>;
template class Triangle<uint>;
template class Triangle<short>;
template class Triangle<ushort>;

// --------------------------------------------------------------------------------------------------------------------
// Rectangle

template<typename T>
void Rectangle<T>::draw(const GraphicsContext&)
{
    notImplemented("Rectangle::draw");
}

template<typename T>
void Rectangle<T>::drawOutline(const GraphicsContext&, T)
{
    notImplemented("Rectangle::drawOutline");
}

#ifdef DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Rectangle<T>::draw()
{
    notImplemented("Rectangle::draw");
}

template<typename T>
void Rectangle<T>::drawOutline()
{
    notImplemented("Rectangle::drawOutline");
}
#endif

template class Rectangle<double>;
template class Rectangle<float>;
template class Rectangle<int>;
template class Rectangle<uint>;
template class Rectangle<short>;
template class Rectangle<ushort>;

// --------------------------------------------------------------------------------------------------------------------

void SubWidget::PrivateData::display(const uint width, const uint height, const double autoScaleFactor)
{
    self->onDisplay();
    selfw->pData->displaySubWidgets(width, height, autoScaleFactor);
}

// --------------------------------------------------------------------------------------------------------------------

void TopLevelWidget::PrivateData::display()
{
    if (!selfw->pData->visible)
        return;

    const Size<uint> size(window.getSize());
    const uint width  = size.getWidth();
    const uint height = size.getHeight();
    const double autoScaleFactor = window.pData->autoScaleFactor;

    self->onDisplay();
    selfw->pData->displaySubWidgets(width, height, autoScaleFactor);
}

// --------------------------------------------------------------------------------------------------------------------

void Window::PrivateData::renderToPicture(const char*, const GraphicsContext&, uint, uint)
{
    notImplemented("Window::PrivateData::renderToPicture");
}

// --------------------------------------------------------------------------------------------------------------------

void Window::PrivateData::createContextIfNeeded()
{
    MetalGraphicsContext& ctx = reinterpret_cast<MetalGraphicsContext&>(graphicsContext);
    if (ctx.device != nullptr)
        return;

    void* const layerPtr = puglGetContext(view);
    if (layerPtr == nullptr)
        return;

    CAMetalLayer* const layer = (DGL_OBJC_BRIDGE CAMetalLayer*)layerPtr;
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil)
        return;

    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = YES;

    id<MTLCommandQueue> queue = [device newCommandQueue];

   #if __has_feature(objc_arc)
    ctx.layer = (__bridge void*)layer;
    ctx.device = (__bridge_retained void*)device;
    ctx.commandQueue = (__bridge_retained void*)queue;
   #else
    ctx.layer = (void*)layer;
    ctx.device = (void*)device;
    ctx.commandQueue = (void*)queue;
   #endif
    ctx.currentDrawable = nullptr;
    ctx.currentCommandEncoder = nullptr;
    ctx.currentCommandBuffer = nullptr;
    ctx.width = 0;
    ctx.height = 0;
    ctx.clearColorR = 0.0;
    ctx.clearColorG = 0.0;
    ctx.clearColorB = 0.0;
    ctx.clearColorA = 1.0;
}

void Window::PrivateData::destroyContext()
{
    MetalGraphicsContext& ctx = reinterpret_cast<MetalGraphicsContext&>(graphicsContext);

    if (ctx.commandQueue != nullptr)
    {
       #if __has_feature(objc_arc)
        id<MTLCommandQueue> queue = (__bridge_transfer id<MTLCommandQueue>)ctx.commandQueue;
        queue = nil;
       #else
        [(id<MTLCommandQueue>)ctx.commandQueue release];
       #endif
        ctx.commandQueue = nullptr;
    }

    if (ctx.device != nullptr)
    {
       #if __has_feature(objc_arc)
        id<MTLDevice> device = (__bridge_transfer id<MTLDevice>)ctx.device;
        device = nil;
       #else
        [(id<MTLDevice>)ctx.device release];
       #endif
        ctx.device = nullptr;
    }

    ctx.layer = nullptr;
    ctx.currentDrawable = nullptr;
    ctx.currentCommandEncoder = nullptr;
    ctx.currentCommandBuffer = nullptr;
}

void Window::PrivateData::startContext()
{
    createContextIfNeeded();

    MetalGraphicsContext& ctx = reinterpret_cast<MetalGraphicsContext&>(graphicsContext);
    if (ctx.layer == nullptr || ctx.commandQueue == nullptr)
        return;

    CAMetalLayer* const layer = (DGL_OBJC_BRIDGE CAMetalLayer*)ctx.layer;
    const PuglArea size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);
    ctx.width = size.width;
    ctx.height = size.height;

    const CGSize boundsSize = layer.bounds.size;
    CGFloat scale = layer.contentsScale;
    if (scale < 1.0)
        scale = 1.0;

    const CGSize drawSize = CGSizeMake(boundsSize.width * scale, boundsSize.height * scale);
    if (drawSize.width > 0.0 && drawSize.height > 0.0)
        layer.drawableSize = drawSize;

    id<CAMetalDrawable> drawable = [layer nextDrawable];
    if (drawable == nil)
        return;

   #if __has_feature(objc_arc)
    ctx.currentDrawable = (__bridge void*)drawable;
   #else
    ctx.currentDrawable = (void*)drawable;
   #endif

    id<MTLCommandQueue> queue = (DGL_OBJC_BRIDGE id<MTLCommandQueue>)ctx.commandQueue;
    id<MTLCommandBuffer> cmdBuffer = [queue commandBuffer];
   #if __has_feature(objc_arc)
    ctx.currentCommandBuffer = (__bridge void*)cmdBuffer;
   #else
    ctx.currentCommandBuffer = (void*)cmdBuffer;
   #endif

    MTLRenderPassDescriptor* const passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    passDesc.colorAttachments[0].texture = drawable.texture;
    passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDesc.colorAttachments[0].clearColor = MTLClearColorMake(ctx.clearColorR, ctx.clearColorG, ctx.clearColorB, ctx.clearColorA);
    passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;

    id<MTLRenderCommandEncoder> encoder = [cmdBuffer renderCommandEncoderWithDescriptor:passDesc];
   #if __has_feature(objc_arc)
    ctx.currentCommandEncoder = (__bridge void*)encoder;
   #else
    ctx.currentCommandEncoder = (void*)encoder;
   #endif
}

void Window::PrivateData::endContext()
{
    MetalGraphicsContext& ctx = reinterpret_cast<MetalGraphicsContext&>(graphicsContext);

    if (ctx.currentCommandEncoder != nullptr)
    {
        id<MTLRenderCommandEncoder> encoder = (DGL_OBJC_BRIDGE id<MTLRenderCommandEncoder>)ctx.currentCommandEncoder;
        [encoder endEncoding];
        ctx.currentCommandEncoder = nullptr;
    }

    if (ctx.currentCommandBuffer != nullptr)
    {
        id<MTLCommandBuffer> cmdBuffer = (DGL_OBJC_BRIDGE id<MTLCommandBuffer>)ctx.currentCommandBuffer;

        if (ctx.currentDrawable != nullptr)
        {
            id<CAMetalDrawable> drawable = (DGL_OBJC_BRIDGE id<CAMetalDrawable>)ctx.currentDrawable;
            [cmdBuffer presentDrawable:drawable];
            ctx.currentDrawable = nullptr;
        }

        [cmdBuffer commit];
        ctx.currentCommandBuffer = nullptr;
    }
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
