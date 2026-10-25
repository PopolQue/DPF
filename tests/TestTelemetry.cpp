/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for DataRingBuffer (Lock-free SPSC Telemetry Queue)
 */

#include "distrho/extra/DataRingBuffer.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace DISTRHO_NAMESPACE;

struct TestPacket {
    uint32_t seq;
    uint32_t checksum;
    uint8_t data[64];
};

static uint32_t calcChecksum(const TestPacket& p) {
    uint32_t c = p.seq * 31;
    for (size_t i = 0; i < sizeof(p.data); ++i)
        c += p.data[i];
    return c;
}

void testSingleThreadBasic() {
    DataRingBuffer ring(4096);
    assert(!ring.hasData());

    const char msg1[] = "Hello DPF Telemetry";
    bool ok = ring.write(101, msg1, sizeof(msg1));
    assert(ok);
    assert(ring.hasData());

    uint32_t receivedCount = 0;
    ring.drain([&](uint32_t tag, const void* data, uint32_t size) {
        assert(tag == 101);
        assert(size == sizeof(msg1));
        assert(std::strcmp(static_cast<const char*>(data), msg1) == 0);
        receivedCount++;
    });
    assert(receivedCount == 1);
    assert(!ring.hasData());
    std::cout << "[PASS] testSingleThreadBasic\n";
}

void testWrapAround() {
    DataRingBuffer ring(1024);

    // Write packets until wrapping occurs
    for (uint32_t i = 0; i < 50; ++i) {
        uint32_t val = i;
        bool ok = ring.write(i, &val, sizeof(val));
        assert(ok);

        uint32_t count = ring.drain([&](uint32_t tag, const void* data, uint32_t size) {
            assert(tag == i);
            assert(size == sizeof(uint32_t));
            uint32_t readVal = 0;
            std::memcpy(&readVal, data, sizeof(uint32_t));
            assert(readVal == i);
        });
        assert(count == 1);
    }
    std::cout << "[PASS] testWrapAround\n";
}

void testConcurrentSPSC() {
    const uint32_t kTotalPackets = 100000;
    DataRingBuffer ring(65536);
    std::atomic<bool> producerDone(false);
    std::atomic<uint32_t> receivedCount(0);
    std::atomic<uint32_t> droppedCount(0);

    // Producer Thread (Simulating DSP run thread)
    std::thread producer([&]() {
        for (uint32_t i = 0; i < kTotalPackets; ++i) {
            TestPacket p;
            p.seq = i;
            for (size_t j = 0; j < sizeof(p.data); ++j)
                p.data[j] = static_cast<uint8_t>((i + j) & 0xFF);
            p.checksum = calcChecksum(p);

            while (!ring.write(1, &p, sizeof(p))) {
                droppedCount++;
                std::this_thread::yield();
            }
        }
        producerDone.store(true, std::memory_order_release);
    });

    // Consumer Thread (Simulating UI idle thread)
    std::thread consumer([&]() {
        uint32_t lastSeq = 0;
        bool first = true;

        while (!producerDone.load(std::memory_order_acquire) || ring.hasData()) {
            ring.drain([&](uint32_t tag, const void* data, uint32_t size) {
                assert(tag == 1);
                assert(size == sizeof(TestPacket));
                const TestPacket* p = static_cast<const TestPacket*>(data);
                assert(p->checksum == calcChecksum(*p));

                if (first) {
                    first = false;
                } else {
                    assert(p->seq > lastSeq); // Guaranteed monotonic order
                }
                lastSeq = p->seq;
                receivedCount++;
            });
            std::this_thread::yield();
        }
    });

    producer.join();
    consumer.join();

    assert(receivedCount.load() == kTotalPackets);
    std::cout << "[PASS] testConcurrentSPSC (" << receivedCount.load() << " packets verified without data corruption)\n";
}

int main() {
    std::cout << "Running DataRingBuffer Telemetry Tests...\n";
    testSingleThreadBasic();
    testWrapAround();
    testConcurrentSPSC();
    std::cout << "All Telemetry Tests Passed Successfully!\n";
    return 0;
}
