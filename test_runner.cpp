/*
 * SportButton - Desktop Validation Suite
 *
 * Runs the CAN frame table in can_test_data.h through the same detection
 * logic the ESP32 firmware uses, then measures detection cost.
 *
 * Build:  make
 * Run:    make test   (or ./test_runner)
 */

#include <chrono>
#include <cstdio>

#include "can_messages.h"
#include "can_test_data.h"

int main() {
    std::printf("\n=================================================\n");
    std::printf("          SPORTBUTTON DESKTOP TEST SUITE\n");
    std::printf("=================================================\n\n");

    unsigned passed = 0;
    unsigned failed = 0;
    unsigned false_positives = 0;
    unsigned false_negatives = 0;

    for (unsigned i = 0; i < NUM_TEST_FRAMES; i++) {
        const can_test_frame_t& frame = test_frames[i];
        const bool detected = isDiamondButtonPressed(frame.can_id, frame.data, frame.dlc);
        const bool ok = (detected == frame.should_trigger);

        std::printf("Test %02u: %s\n", i + 1, frame.description);
        std::printf("         Expected %s, got %s -- %s\n",
                    frame.should_trigger ? "TRIGGER" : "NO TRIGGER",
                    detected ? "TRIGGER" : "NO TRIGGER",
                    ok ? "PASS" : "FAIL");

        if (ok) {
            passed++;
        } else {
            failed++;
            if (frame.should_trigger && !detected) {
                false_negatives++;
            } else {
                false_positives++;
            }
        }
    }

    // Detection-cost check: 1,000,000 frames against the hot path.
    const can_test_frame_t& hot = test_frames[0];
    const unsigned iterations = 1000000;
    const auto start = std::chrono::high_resolution_clock::now();
    unsigned hits = 0;
    for (unsigned i = 0; i < iterations; i++) {
        if (isDiamondButtonPressed(hot.can_id, hot.data, hot.dlc)) {
            hits++;
        }
    }
    const auto stop = std::chrono::high_resolution_clock::now();
    const double total_us =
        std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(stop - start).count();
    const double per_call_us = total_us / iterations;

    std::printf("\n=================================================\n");
    std::printf("                TEST RESULTS\n");
    std::printf("=================================================\n");
    std::printf("Total test cases : %u\n", static_cast<unsigned>(NUM_TEST_FRAMES));
    std::printf("Passed           : %u\n", passed);
    std::printf("Failed           : %u\n", failed);
    std::printf("False positives  : %u\n", false_positives);
    std::printf("False negatives  : %u\n", false_negatives);
    std::printf("Success rate     : %.1f%%\n",
                100.0 * passed / (passed + failed));
    std::printf("Detection cost   : %.4f us/call (%u matches)\n",
                per_call_us, hits);
    std::printf("=================================================\n\n");

    if (failed == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }

    std::printf("SOME TESTS FAILED\n");
    return 1;
}
