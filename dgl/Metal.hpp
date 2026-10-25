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

#ifndef DGL_METAL_HPP_INCLUDED
#define DGL_METAL_HPP_INCLUDED

#include "Base.hpp"

#ifdef __OBJC__
@protocol MTLDevice;
@protocol MTLCommandQueue;
@protocol CAMetalDrawable;
@protocol MTLRenderCommandEncoder;
@protocol MTLCommandBuffer;
@class CAMetalLayer;

# if __has_feature(objc_arc)
#  define DGL_OBJC_BRIDGE __bridge
# else
#  define DGL_OBJC_BRIDGE
# endif
#endif

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Apple Metal Graphics context.

   Provides access to the CAMetalLayer, MTLDevice, MTLCommandQueue, current drawable and render encoders.
   Available during Window onDisplay() and Widget onDisplay() passes.
 */
struct MetalGraphicsContext : GraphicsContext
{
    /** CAMetalLayer pointer (bridged to CAMetalLayer* in Obj-C). */
    void* layer;

    /** id<MTLDevice> pointer. */
    void* device;

    /** id<MTLCommandQueue> pointer. */
    void* commandQueue;

    /** Current frame's id<CAMetalDrawable> pointer. */
    void* currentDrawable;

    /** Current frame's id<MTLRenderCommandEncoder> pointer. */
    void* currentCommandEncoder;

    /** Current frame's id<MTLCommandBuffer> pointer. */
    void* currentCommandBuffer;

    /** Current drawable width in pixels. */
    uint width;

    /** Current drawable height in pixels. */
    uint height;

    /** Clear color (RGBA 0.0 - 1.0). */
    double clearColorR;
    double clearColorG;
    double clearColorB;
    double clearColorA;

   #ifdef __OBJC__
    CAMetalLayer* getLayer() const noexcept {
        return (DGL_OBJC_BRIDGE CAMetalLayer*)layer;
    }
    id<MTLDevice> getDevice() const noexcept {
        return (DGL_OBJC_BRIDGE id<MTLDevice>)device;
    }
    id<MTLCommandQueue> getCommandQueue() const noexcept {
        return (DGL_OBJC_BRIDGE id<MTLCommandQueue>)commandQueue;
    }
    id<CAMetalDrawable> getCurrentDrawable() const noexcept {
        return (DGL_OBJC_BRIDGE id<CAMetalDrawable>)currentDrawable;
    }
    id<MTLRenderCommandEncoder> getCurrentCommandEncoder() const noexcept {
        return (DGL_OBJC_BRIDGE id<MTLRenderCommandEncoder>)currentCommandEncoder;
    }
    id<MTLCommandBuffer> getCurrentCommandBuffer() const noexcept {
        return (DGL_OBJC_BRIDGE id<MTLCommandBuffer>)currentCommandBuffer;
    }
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_METAL_HPP_INCLUDED
