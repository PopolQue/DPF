/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for VectorKeyboard Widget (Interactive Piano Roll & MIDI Monitor)
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "dgl/VectorWidgets.hpp"
#include "dgl/Application.hpp"
#include "dgl/Window.hpp"
#include "dgl/TopLevelWidget.hpp"

USE_NAMESPACE_DGL

class DummyKeyboardCallback : public VectorKeyboard::Callback
{
public:
    int noteOnCount = 0;
    int noteOffCount = 0;
    int pitchBendCount = 0;
    uint8_t lastNote = 0;
    float lastVelocity = 0.0f;
    float lastPitchBend = 0.0f;

    void vectorKeyboardNoteOn(VectorKeyboard*, uint8_t note, float velocity) override
    {
        noteOnCount++;
        lastNote = note;
        lastVelocity = velocity;
    }

    void vectorKeyboardNoteOff(VectorKeyboard*, uint8_t note) override
    {
        noteOffCount++;
        lastNote = note;
    }

    void vectorKeyboardPitchBend(VectorKeyboard*, uint8_t note, float bend) override
    {
        pitchBendCount++;
        lastNote = note;
        lastPitchBend = bend;
    }
};

int main()
{
    Application app(true);
    Window win(app);
    TopLevelWidget container(win);

    std::cout << "=== Testing VectorKeyboard Piano Roll Widget Architecture ===" << std::endl;

    std::cout << "--- Testing Musical Note Math & Semitone Classification ---" << std::endl;
    {
        // Test white keys (C, D, E, F, G, A, B)
        assert(!VectorKeyboard::isBlackKey(48)); // C3
        assert(!VectorKeyboard::isBlackKey(50)); // D3
        assert(!VectorKeyboard::isBlackKey(52)); // E3
        assert(!VectorKeyboard::isBlackKey(53)); // F3
        assert(!VectorKeyboard::isBlackKey(55)); // G3
        assert(!VectorKeyboard::isBlackKey(57)); // A3
        assert(!VectorKeyboard::isBlackKey(59)); // B3
        assert(!VectorKeyboard::isBlackKey(60)); // C4

        // Test black keys (C#, D#, F#, G#, A#)
        assert(VectorKeyboard::isBlackKey(49));  // C#3
        assert(VectorKeyboard::isBlackKey(51));  // D#3
        assert(VectorKeyboard::isBlackKey(54));  // F#3
        assert(VectorKeyboard::isBlackKey(56));  // G#3
        assert(VectorKeyboard::isBlackKey(58));  // A#3

        // Note names & octaves
        assert(std::string(VectorKeyboard::getNoteName(48)) == "C");
        assert(std::string(VectorKeyboard::getNoteName(49)) == "C#");
        assert(std::string(VectorKeyboard::getNoteName(54)) == "F#");
        assert(std::string(VectorKeyboard::getNoteName(60)) == "C");
        assert(VectorKeyboard::getNoteOctave(48) == 3);
        assert(VectorKeyboard::getNoteOctave(60) == 4);

        std::cout << "  ✓ Black/White semitone classification, note names, and octaves verified" << std::endl;
    }

    std::cout << "--- Testing Key Geometry & Range Setup ---" << std::endl;
    {
        VectorKeyboard kbd(&container);
        kbd.setSize(600, 100);

        // Default: 25 keys starting at C3 (48) -> C3 to C5 inclusive
        assert(kbd.getStartKey() == 48);
        assert(kbd.getNumKeys() == 25);
        // 2 octaves + 1 note = 7 + 7 + 1 = 15 white keys
        assert(kbd.getWhiteKeyCount() == 15);

        // Custom 1 octave (C4 to C5 inclusive = 13 keys: 8 white, 5 black)
        kbd.setKeyRange(60, 13);
        assert(kbd.getStartKey() == 60);
        assert(kbd.getNumKeys() == 13);
        assert(kbd.getWhiteKeyCount() == 8);

        std::vector<VectorKeyboard::KeyRect> rects;
        kbd.getKeyRects(rects);
        assert(rects.size() == 13);

        // Verify white key geometry
        float expectedWhiteW = 600.0f / 8.0f; // 75.0f
        int whiteCount = 0;
        int blackCount = 0;
        for (size_t i = 0; i < rects.size(); ++i)
        {
            if (!rects[i].isBlack)
            {
                assert(std::abs(rects[i].w - expectedWhiteW) < 0.01f);
                assert(rects[i].h == 100.0f);
                assert(std::abs(rects[i].x - (whiteCount * expectedWhiteW)) < 0.01f);
                whiteCount++;
            }
            else
            {
                assert(rects[i].h < 100.0f);
                assert(rects[i].w < expectedWhiteW);
                blackCount++;
            }
        }
        assert(whiteCount == 8);
        assert(blackCount == 5);
        (void)expectedWhiteW;
        (void)whiteCount;
        (void)blackCount;

        std::cout << "  ✓ Key range, white/black key counts, and geometric rect calculation verified" << std::endl;
    }

    std::cout << "--- Testing 2-Phase Hit Testing ---" << std::endl;
    {
        VectorKeyboard kbd(&container);
        kbd.setSize(600, 100);
        kbd.setKeyRange(60, 13); // C4 to C5, 8 white keys (w = 75px)

        // C4 white key at bottom (y = 80, x = 30)
        assert(kbd.getNoteAtPos(30, 80) == 60);

        // D4 white key at bottom (y = 80, x = 110)
        assert(kbd.getNoteAtPos(110, 80) == 62);

        // C#4 black key at top (x ~ 75, y = 30)
        // Seam between C4 and D4 is at x = 75
        int16_t noteAtSeamTop = kbd.getNoteAtPos(75, 30);
        assert(noteAtSeamTop == 61); // C#4!
        (void)noteAtSeamTop;

        // Same X coordinate at bottom (y = 80) falls onto white key!
        int16_t noteAtSeamBottom = kbd.getNoteAtPos(75, 80);
        assert(noteAtSeamBottom == 60 || noteAtSeamBottom == 62);
        (void)noteAtSeamBottom;

        // Out of bounds
        assert(kbd.getNoteAtPos(-5, 50) == -1);
        assert(kbd.getNoteAtPos(605, 50) == -1);
        assert(kbd.getNoteAtPos(50, -5) == -1);
        assert(kbd.getNoteAtPos(50, 105) == -1);

        std::cout << "  ✓ 2-Phase hit testing (black key top priority, white key fallback) verified" << std::endl;
    }

    std::cout << "--- Testing Mouse Interaction & Velocity Mapping ---" << std::endl;
    {
        VectorKeyboard kbd(&container);
        kbd.setSize(600, 100);
        kbd.setKeyRange(60, 13);

        DummyKeyboardCallback cb;
        kbd.setCallback(&cb);

        // Press C4 near bottom tip (x = 30, y = 90) -> High velocity
        Widget::MouseEvent pressEv1;
        pressEv1.button = kMouseButtonLeft;
        pressEv1.press = true;
        pressEv1.pos = Point<double>(30.0, 90.0);

        bool handled = kbd.onMouse(pressEv1);
        assert(handled);
        assert(kbd.isKeyDown(60));
        assert(cb.noteOnCount == 1);
        assert(cb.lastNote == 60);
        float highVelocity = cb.lastVelocity;
        assert(highVelocity > 0.85f); // Near bottom = loud velocity
        (void)handled;
        (void)highVelocity;

        // Release C4
        Widget::MouseEvent releaseEv1;
        releaseEv1.button = kMouseButtonLeft;
        releaseEv1.press = false;
        releaseEv1.pos = Point<double>(30.0, 90.0);

        handled = kbd.onMouse(releaseEv1);
        assert(handled);
        assert(!kbd.isKeyDown(60));
        assert(cb.noteOffCount == 1);
        assert(cb.lastNote == 60);

        // Press C4 near top (x = 30, y = 10) -> Soft velocity
        Widget::MouseEvent pressEv2;
        pressEv2.button = kMouseButtonLeft;
        pressEv2.press = true;
        pressEv2.pos = Point<double>(30.0, 10.0);

        handled = kbd.onMouse(pressEv2);
        assert(handled);
        assert(cb.noteOnCount == 2);
        float softVelocity = cb.lastVelocity;
        assert(softVelocity < 0.40f);
        assert(softVelocity < highVelocity);
        (void)softVelocity;

        // Clean up
        releaseEv1.pos = Point<double>(30.0, 10.0);
        kbd.onMouse(releaseEv1);
        assert(cb.noteOffCount == 2);

        std::cout << "  ✓ Note-on/off dispatch and Y-axis velocity sensitivity verified" << std::endl;
    }

    std::cout << "--- Testing Mouse Gliding (Touch Dragging) ---" << std::endl;
    {
        VectorKeyboard kbd(&container);
        kbd.setSize(600, 100);
        kbd.setKeyRange(60, 13);

        DummyKeyboardCallback cb;
        kbd.setCallback(&cb);

        // 1. Mouse down on C4 (x = 30, y = 80)
        Widget::MouseEvent pressEv;
        pressEv.button = kMouseButtonLeft;
        pressEv.press = true;
        pressEv.pos = Point<double>(30.0, 80.0);
        kbd.onMouse(pressEv);

        assert(cb.noteOnCount == 1);
        assert(cb.lastNote == 60);
        assert(kbd.isKeyDown(60));

        // 2. Drag motion into D4 (x = 110, y = 80)
        Widget::MotionEvent motionEv;
        motionEv.pos = Point<double>(110.0, 80.0);
        bool motionHandled = kbd.onMotion(motionEv);

        assert(motionHandled);
        (void)motionHandled;
        // Note-off for 60, Note-on for 62
        assert(cb.noteOffCount == 1);
        assert(!kbd.isKeyDown(60));
        assert(cb.noteOnCount == 2);
        assert(cb.lastNote == 62);
        assert(kbd.isKeyDown(62));

        // 3. Mouse release
        Widget::MouseEvent releaseEv;
        releaseEv.button = kMouseButtonLeft;
        releaseEv.press = false;
        releaseEv.pos = Point<double>(110.0, 80.0);
        kbd.onMouse(releaseEv);

        assert(cb.noteOffCount == 2);
        assert(cb.lastNote == 62);
        assert(!kbd.isKeyDown(62));

        std::cout << "  ✓ Mouse gliding transition (prior note released, new note triggered) verified" << std::endl;
    }

    std::cout << "--- Testing MPE Horizontal Pitch Bend ---" << std::endl;
    {
        VectorKeyboard kbd(&container);
        kbd.setSize(600, 100);
        kbd.setKeyRange(60, 13);
        kbd.setMpeEnabled(true);
        kbd.setMpePitchBendRange(2.0f); // +/- 2 semitones

        DummyKeyboardCallback cb;
        kbd.setCallback(&cb);

        // Click at C4 center (x = 35, y = 80)
        Widget::MouseEvent pressEv;
        pressEv.button = kMouseButtonLeft;
        pressEv.press = true;
        pressEv.pos = Point<double>(35.0, 80.0);
        kbd.onMouse(pressEv);

        // Drag right horizontally inside C4 key area (x = 65, y = 80)
        Widget::MotionEvent motionEv;
        motionEv.pos = Point<double>(65.0, 80.0);
        kbd.onMotion(motionEv);

        assert(cb.pitchBendCount > 0);
        assert(cb.lastNote == 60);
        assert(cb.lastPitchBend > 0.0f); // Positive pitch deflection

        // Release
        Widget::MouseEvent releaseEv;
        releaseEv.button = kMouseButtonLeft;
        releaseEv.press = false;
        releaseEv.pos = Point<double>(65.0, 80.0);
        kbd.onMouse(releaseEv);

        // Reset bend to 0
        assert(cb.lastPitchBend == 0.0f);

        std::cout << "  ✓ MPE horizontal pitch bend tracking and reset verified" << std::endl;
    }

    std::cout << "--- Testing Live MIDI State Tracking & Octave Shifting ---" << std::endl;
    {
        VectorKeyboard kbd(&container);
        kbd.setSize(600, 100);
        kbd.setKeyRange(48, 25);

        // External MIDI down event
        kbd.setKeyDown(64, true, 0.75f);
        assert(kbd.isKeyDown(64));
        assert(std::abs(kbd.getVelocity(64) - 0.75f) < 0.001f);

        // External MIDI up event
        kbd.setKeyDown(64, false);
        assert(!kbd.isKeyDown(64));
        assert(kbd.getVelocity(64) == 0.0f);

        // Multi-note polyphony
        kbd.setKeyDown(48, true, 0.8f);
        kbd.setKeyDown(52, true, 0.9f);
        kbd.setKeyDown(55, true, 0.7f);
        assert(kbd.isKeyDown(48) && kbd.isKeyDown(52) && kbd.isKeyDown(55));

        kbd.clearAllKeys();
        assert(!kbd.isKeyDown(48) && !kbd.isKeyDown(52) && !kbd.isKeyDown(55));

        // Octave shift
        kbd.shiftOctave(1);
        assert(kbd.getStartKey() == 60); // Shifted up 12 semitones
        kbd.shiftOctave(-1);
        assert(kbd.getStartKey() == 48); // Shifted back

        std::cout << "  ✓ External MIDI state tracking, polyphonic chords, and octave shifting verified" << std::endl;
    }

    std::cout << "--- Testing Accessibility & Keyboard Navigation ---" << std::endl;
    {
        VectorKeyboard kbd(&container);
        kbd.setSize(600, 100);
        kbd.setKeyRange(48, 25);

        AccessibleElement* a11y = kbd.getAccessibleElement();
        assert(a11y != nullptr);
        assert(a11y->getAccessibleInfo().role == kAccessibleRoleKeyboard);
        assert(std::string(a11y->getAccessibleInfo().label) == "Virtual Piano Keyboard");

        DummyKeyboardCallback cb;
        kbd.setCallback(&cb);

        // Increment / Decrement actions
        uint8_t initKey = kbd.getFocusedKey();
        a11y->performAccessibleAction(kAccessibleActionIncrement);
        assert(kbd.getFocusedKey() == initKey + 1);

        a11y->performAccessibleAction(kAccessibleActionDecrement);
        assert(kbd.getFocusedKey() == initKey);

        // Action press (plays note)
        a11y->performAccessibleAction(kAccessibleActionPress);
        assert(cb.noteOnCount == 1);
        assert(cb.lastNote == initKey);
        assert(kbd.isKeyDown(initKey));

        // Action press again (releases note)
        a11y->performAccessibleAction(kAccessibleActionPress);
        assert(cb.noteOffCount == 1);
        assert(!kbd.isKeyDown(initKey));
        (void)initKey;

        // Keyboard event: Right arrow
        Widget::KeyboardEvent keyEvRight;
        keyEvRight.press = true;
        keyEvRight.key = kKeyRight;
        kbd.onKeyboard(keyEvRight);
        assert(kbd.getFocusedKey() == initKey + 1);

        // Keyboard event: Spacebar
        Widget::KeyboardEvent keyEvSpacePress;
        keyEvSpacePress.press = true;
        keyEvSpacePress.key = kKeySpace;
        kbd.onKeyboard(keyEvSpacePress);
        assert(kbd.isKeyDown(kbd.getFocusedKey()));

        Widget::KeyboardEvent keyEvSpaceRelease;
        keyEvSpaceRelease.press = false;
        keyEvSpaceRelease.key = kKeySpace;
        kbd.onKeyboard(keyEvSpaceRelease);
        assert(!kbd.isKeyDown(kbd.getFocusedKey()));

        std::cout << "  ✓ Accessibility conformance (Role: Keyboard), VoiceOver actions, and Arrow/Space key navigation verified" << std::endl;
    }

    std::cout << "=== All VectorKeyboard Tests Passed Successfully! ===" << std::endl;
    return 0;
}
