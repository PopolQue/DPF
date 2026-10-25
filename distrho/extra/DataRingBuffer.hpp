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

#ifndef DISTRHO_DATA_RING_BUFFER_HPP_INCLUDED
#define DISTRHO_DATA_RING_BUFFER_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

#include <atomic>
#include <cstdint>
#include <cstring>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

/**
   Single-Producer Single-Consumer (SPSC) lock-free telemetry ring buffer.

   Provides wait-free, non-blocking telemetry communication between the DSP thread
   (producer) and the UI main thread (consumer).
   Uses C++11 std::atomic memory barriers (acquire/release) to ensure thread-safety
   across weakly-ordered architectures (such as ARM64).

   All data payloads are guaranteed to be contiguous in memory, allowing zero-copy reads.
 */
class DataRingBuffer
{
    static constexpr const uint32_t kTagWrap = 0xFFFFFFFF;
    static constexpr const uint32_t kHeaderSize = sizeof(uint32_t) * 2; // tag + size

public:
    /**
       Construct a DataRingBuffer with the specified byte capacity.
       @note Memory is allocated in constructor (do not construct on audio thread).
     */
    explicit DataRingBuffer(const uint32_t capacity = 65536)
        : fCapacity(capacity > 1024 ? capacity : 1024),
          fBuffer(new uint8_t[fCapacity]),
          fHead(0),
          fTail(0)
    {
    }

    /**
       Destructor.
     */
    ~DataRingBuffer()
    {
        delete[] fBuffer;
    }

    /**
       Post a telemetry message to the buffer.
       This call is wait-free, lock-free, and real-time safe.
       If there is insufficient space in the buffer, the message is dropped and false is returned.

       @param tag An application-defined identifier (e.g. tag for FFT spectrum, meter, waveform).
       @param data Pointer to the payload bytes (can be null if size is 0).
       @param size Size of the payload in bytes.
       @return true if message was posted, false if dropped due to insufficient space.
     */
    bool write(const uint32_t tag, const void* const data, const uint32_t size) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(tag != kTagWrap, false);

        const uint32_t totalSize = kHeaderSize + size;
        if (totalSize + sizeof(uint32_t) >= fCapacity)
            return false;

        const uint32_t head = fHead.load(std::memory_order_relaxed);
        const uint32_t tail = fTail.load(std::memory_order_acquire);

        if (head >= tail)
        {
            // Case 1: head is ahead of or equal to tail
            if (head + totalSize <= fCapacity)
            {
                // Fits at the end without wrapping
                // Check if head wraps to tail
                if (head + totalSize == fCapacity && tail == 0)
                    return false; // Leave at least 1 byte to distinguish full from empty

                writePacket(head, tag, data, size);
                const uint32_t newHead = (head + totalSize == fCapacity) ? 0 : head + totalSize;
                fHead.store(newHead, std::memory_order_release);
                return true;
            }

            // Does not fit at the end, attempt wrap-around to index 0
            if (tail > totalSize)
            {
                // Write wrap marker at end if there's room for at least 4 bytes
                if (head + sizeof(uint32_t) <= fCapacity)
                {
                    const uint32_t wrapMarker = kTagWrap;
                    std::memcpy(fBuffer + head, &wrapMarker, sizeof(uint32_t));
                }

                writePacket(0, tag, data, size);
                fHead.store(totalSize, std::memory_order_release);
                return true;
            }

            // Insufficient space
            return false;
        }

        // Case 2: head < tail
        const uint32_t available = tail - head - 1;
        if (totalSize <= available)
        {
            writePacket(head, tag, data, size);
            fHead.store(head + totalSize, std::memory_order_release);
            return true;
        }

        return false;
    }

    /**
       Read and dispatch all available telemetry messages in the buffer.
       Invokes callback(tag, data, size) for each message.
       Guaranteed zero-copy: data pointer points directly into the buffer.

       @param callback Callable with signature void(uint32_t tag, const void* data, uint32_t size)
       @return Number of messages dispatched.
     */
    template <typename Callback>
    uint32_t drain(Callback&& callback) noexcept
    {
        uint32_t count = 0;
        uint32_t tail = fTail.load(std::memory_order_relaxed);
        const uint32_t head = fHead.load(std::memory_order_acquire);

        while (tail != head)
        {
            // Check for wrap marker
            if (tail + sizeof(uint32_t) <= fCapacity)
            {
                uint32_t tagCheck;
                std::memcpy(&tagCheck, fBuffer + tail, sizeof(uint32_t));
                if (tagCheck == kTagWrap)
                {
                    tail = 0;
                    if (tail == head)
                        break;
                }
            }

            if (tail + kHeaderSize > fCapacity)
            {
                // Wrapped
                tail = 0;
                if (tail == head)
                    break;
            }

            uint32_t tag, size;
            std::memcpy(&tag, fBuffer + tail, sizeof(uint32_t));
            std::memcpy(&size, fBuffer + tail + sizeof(uint32_t), sizeof(uint32_t));

            const void* const payload = (size > 0) ? (fBuffer + tail + kHeaderSize) : nullptr;
            callback(tag, payload, size);

            tail += kHeaderSize + size;
            if (tail >= fCapacity)
                tail = 0;

            ++count;
        }

        fTail.store(tail, std::memory_order_release);
        return count;
    }

    /**
       Check if there are messages pending in the buffer.
     */
    bool hasData() const noexcept
    {
        return fHead.load(std::memory_order_relaxed) != fTail.load(std::memory_order_relaxed);
    }

    /**
       Clear all pending messages.
     */
    void clear() noexcept
    {
        const uint32_t head = fHead.load(std::memory_order_relaxed);
        fTail.store(head, std::memory_order_release);
    }

private:
    void writePacket(const uint32_t offset, const uint32_t tag, const void* const data, const uint32_t size) noexcept
    {
        std::memcpy(fBuffer + offset, &tag, sizeof(uint32_t));
        std::memcpy(fBuffer + offset + sizeof(uint32_t), &size, sizeof(uint32_t));
        if (size > 0 && data != nullptr)
            std::memcpy(fBuffer + offset + kHeaderSize, data, size);
    }

    const uint32_t fCapacity;
    uint8_t* const fBuffer;
    std::atomic<uint32_t> fHead;
    std::atomic<uint32_t> fTail;

    DISTRHO_DECLARE_NON_COPYABLE(DataRingBuffer)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_DATA_RING_BUFFER_HPP_INCLUDED
