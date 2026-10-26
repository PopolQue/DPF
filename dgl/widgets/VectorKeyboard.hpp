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

#ifndef DGL_VECTOR_KEYBOARD_HPP_INCLUDED
#define DGL_VECTOR_KEYBOARD_HPP_INCLUDED

#include "../Accessibility.hpp"
#include "../Color.hpp"
#include "../NanoVG.hpp"
#include "../../distrho/extra/String.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Resolution-independent, customizable NanoVG vector piano keyboard widget.
   Functions both as an interactive performance controller (touch/mouse gliding, velocity mapping,
   MPE per-key pitch bend) and as a real-time MIDI monitor/visualizer.
 */
class VectorKeyboard : public NanoSubWidget,
                       public AccessibleElement
{
public:
    struct KeyRect {
        uint8_t note;
        bool    isBlack;
        float   x;
        float   y;
        float   w;
        float   h;

        KeyRect() noexcept
            : note(0), isBlack(false), x(0.0f), y(0.0f), w(0.0f), h(0.0f) {}
    };

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void vectorKeyboardNoteOn(VectorKeyboard* keyboard, uint8_t note, float velocity) {}
        virtual void vectorKeyboardNoteOff(VectorKeyboard* keyboard, uint8_t note) {}
        virtual void vectorKeyboardPitchBend(VectorKeyboard* keyboard, uint8_t note, float bend) {}
    };

    typedef std::function<void(VectorKeyboard*, uint8_t, float)> NoteOnCallback;
    typedef std::function<void(VectorKeyboard*, uint8_t)>        NoteOffCallback;
    typedef std::function<void(VectorKeyboard*, uint8_t, float)> PitchBendCallback;

    explicit VectorKeyboard(Widget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fStartKey(48), // C3
          fNumKeys(25),  // 2 octaves + 1 note
          fCurrentHeldNote(-1),
          fFocusedKey(48),
          fVelocityEnabled(true),
          fDefaultVelocity(0.8f),
          fMpeEnabled(false),
          fMpePitchBendRange(2.0f),
          fInitialClickX(0.0f),
          fInitialClickY(0.0f),
          fCurrentPitchBend(0.0f),
          fBlackKeyHeightRatio(0.62f),
          fBlackKeyWidthRatio(0.60f),
          fCornerRadius(3.0f),
          fShowLabels(true),
          fWhiteKeyColor(Color(245, 246, 248)),
          fWhiteKeyPressedColor(Color(0, 180, 255)),
          fBlackKeyColor(Color(26, 28, 32)),
          fBlackKeyPressedColor(Color(0, 210, 255)),
          fBorderColor(Color(45, 48, 56)),
          fLabelColor(Color(130, 135, 145)),
          fUserCallback(nullptr)
    {
        std::memset(fKeysDown, 0, sizeof(fKeysDown));
        for (int i = 0; i < 128; ++i)
            fKeyVelocities[i] = 0.0f;

        initAccessibility();
    }

    explicit VectorKeyboard(NanoSubWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fStartKey(48),
          fNumKeys(25),
          fCurrentHeldNote(-1),
          fFocusedKey(48),
          fVelocityEnabled(true),
          fDefaultVelocity(0.8f),
          fMpeEnabled(false),
          fMpePitchBendRange(2.0f),
          fInitialClickX(0.0f),
          fInitialClickY(0.0f),
          fCurrentPitchBend(0.0f),
          fBlackKeyHeightRatio(0.62f),
          fBlackKeyWidthRatio(0.60f),
          fCornerRadius(3.0f),
          fShowLabels(true),
          fWhiteKeyColor(Color(245, 246, 248)),
          fWhiteKeyPressedColor(Color(0, 180, 255)),
          fBlackKeyColor(Color(26, 28, 32)),
          fBlackKeyPressedColor(Color(0, 210, 255)),
          fBorderColor(Color(45, 48, 56)),
          fLabelColor(Color(130, 135, 145)),
          fUserCallback(nullptr)
    {
        std::memset(fKeysDown, 0, sizeof(fKeysDown));
        for (int i = 0; i < 128; ++i)
            fKeyVelocities[i] = 0.0f;

        initAccessibility();
    }

    explicit VectorKeyboard(NanoTopLevelWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fStartKey(48),
          fNumKeys(25),
          fCurrentHeldNote(-1),
          fFocusedKey(48),
          fVelocityEnabled(true),
          fDefaultVelocity(0.8f),
          fMpeEnabled(false),
          fMpePitchBendRange(2.0f),
          fInitialClickX(0.0f),
          fInitialClickY(0.0f),
          fCurrentPitchBend(0.0f),
          fBlackKeyHeightRatio(0.62f),
          fBlackKeyWidthRatio(0.60f),
          fCornerRadius(3.0f),
          fShowLabels(true),
          fWhiteKeyColor(Color(245, 246, 248)),
          fWhiteKeyPressedColor(Color(0, 180, 255)),
          fBlackKeyColor(Color(26, 28, 32)),
          fBlackKeyPressedColor(Color(0, 210, 255)),
          fBorderColor(Color(45, 48, 56)),
          fLabelColor(Color(130, 135, 145)),
          fUserCallback(nullptr)
    {
        std::memset(fKeysDown, 0, sizeof(fKeysDown));
        for (int i = 0; i < 128; ++i)
            fKeyVelocities[i] = 0.0f;

        initAccessibility();
    }

    ~VectorKeyboard() override
    {
        clearAllKeys();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Key Geometry & Range Setup

    static bool isBlackKey(const uint8_t note) noexcept
    {
        const uint8_t semitone = note % 12;
        return (semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10);
    }

    static const char* getNoteName(const uint8_t note) noexcept
    {
        static const char* const kNames[12] = {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        };
        return kNames[note % 12];
    }

    static int getNoteOctave(const uint8_t note) noexcept
    {
        return (static_cast<int>(note) / 12) - 1;
    }

    uint8_t getStartKey() const noexcept
    {
        return fStartKey;
    }

    uint8_t getNumKeys() const noexcept
    {
        return fNumKeys;
    }

    void setKeyRange(const uint8_t startKey, const uint8_t numKeys)
    {
        uint8_t clampedStart = (startKey > 127) ? 127 : startKey;
        uint8_t clampedCount = (numKeys < 1) ? 1 : numKeys;

        if (static_cast<int>(clampedStart) + clampedCount > 128)
            clampedCount = static_cast<uint8_t>(128 - clampedStart);

        if (fStartKey == clampedStart && fNumKeys == clampedCount)
            return;

        fStartKey = clampedStart;
        fNumKeys = clampedCount;

        if (fFocusedKey < fStartKey)
            fFocusedKey = fStartKey;
        else if (fFocusedKey >= fStartKey + fNumKeys)
            fFocusedKey = static_cast<uint8_t>(fStartKey + fNumKeys - 1);

        updateAccessibilityInfo();
        repaint();
    }

    void shiftOctave(const int delta)
    {
        int newStart = static_cast<int>(fStartKey) + (delta * 12);
        if (newStart < 0)
            newStart = 0;
        if (newStart + fNumKeys > 128)
            newStart = 128 - fNumKeys;

        setKeyRange(static_cast<uint8_t>(newStart), fNumKeys);
    }

    uint32_t getWhiteKeyCount() const noexcept
    {
        uint32_t count = 0;
        const uint8_t endKey = static_cast<uint8_t>(fStartKey + fNumKeys);
        for (uint8_t n = fStartKey; n < endKey; ++n)
        {
            if (!isBlackKey(n))
                ++count;
        }
        return count;
    }

    void getKeyRects(std::vector<KeyRect>& outRects) const
    {
        outRects.clear();
        const uint32_t whiteCount = getWhiteKeyCount();
        if (whiteCount == 0)
            return;

        const float totalWidth = static_cast<float>(getWidth());
        const float totalHeight = static_cast<float>(getHeight());
        const float whiteW = totalWidth / static_cast<float>(whiteCount);
        const float blackW = whiteW * fBlackKeyWidthRatio;
        const float blackH = totalHeight * fBlackKeyHeightRatio;

        const uint8_t endKey = static_cast<uint8_t>(fStartKey + fNumKeys);
        outRects.reserve(fNumKeys);

        // 1. Generate White Key rects
        uint32_t currentWhiteIdx = 0;
        for (uint8_t n = fStartKey; n < endKey; ++n)
        {
            if (!isBlackKey(n))
            {
                KeyRect kr;
                kr.note = n;
                kr.isBlack = false;
                kr.x = static_cast<float>(currentWhiteIdx) * whiteW;
                kr.y = 0.0f;
                kr.w = whiteW;
                kr.h = totalHeight;
                outRects.push_back(kr);
                ++currentWhiteIdx;
            }
        }

        // 2. Generate Black Key rects
        for (uint8_t n = fStartKey; n < endKey; ++n)
        {
            if (isBlackKey(n))
            {
                KeyRect kr;
                kr.note = n;
                kr.isBlack = true;
                kr.y = 0.0f;
                kr.w = blackW;
                kr.h = blackH;

                // Find preceding white key position
                float boundaryX = 0.0f;
                bool foundPreceding = false;
                for (size_t i = 0; i < outRects.size(); ++i)
                {
                    if (!outRects[i].isBlack && outRects[i].note == (n - 1))
                    {
                        boundaryX = outRects[i].x + outRects[i].w;
                        foundPreceding = true;
                        break;
                    }
                }

                if (!foundPreceding)
                {
                    // Start key is black key itself
                    boundaryX = 0.0f;
                }

                // Subtle acoustic grand piano horizontal offsets
                float acousticOffset = 0.0f;
                const uint8_t semitone = n % 12;
                if (semitone == 1)       // C#
                    acousticOffset = -0.02f * whiteW;
                else if (semitone == 3)  // D#
                    acousticOffset =  0.02f * whiteW;
                else if (semitone == 6)  // F#
                    acousticOffset = -0.03f * whiteW;
                else if (semitone == 8)  // G#
                    acousticOffset =  0.0f;
                else if (semitone == 10) // A#
                    acousticOffset =  0.03f * whiteW;

                kr.x = boundaryX - (blackW * 0.5f) + acousticOffset;
                outRects.push_back(kr);
            }
        }
    }

    int16_t getNoteAtPos(const int x, const int y) const
    {
        if (x < 0 || y < 0 || x >= static_cast<int>(getWidth()) || y >= static_cast<int>(getHeight()))
            return -1;

        std::vector<KeyRect> rects;
        getKeyRects(rects);

        const float fx = static_cast<float>(x);
        const float fy = static_cast<float>(y);

        // Phase 1: Test black keys first (top priority)
        for (size_t i = 0; i < rects.size(); ++i)
        {
            if (rects[i].isBlack)
            {
                if (fx >= rects[i].x && fx < (rects[i].x + rects[i].w) &&
                    fy >= rects[i].y && fy < (rects[i].y + rects[i].h))
                {
                    return static_cast<int16_t>(rects[i].note);
                }
            }
        }

        // Phase 2: Test white keys
        for (size_t i = 0; i < rects.size(); ++i)
        {
            if (!rects[i].isBlack)
            {
                if (fx >= rects[i].x && fx < (rects[i].x + rects[i].w) &&
                    fy >= rects[i].y && fy < (rects[i].y + rects[i].h))
                {
                    return static_cast<int16_t>(rects[i].note);
                }
            }
        }

        return -1;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // State Tracking (Live Monitoring & Playing)

    template <typename T>
    static inline T clampVal(const T val, const T min, const T max) noexcept
    {
        return (val < min) ? min : (val > max) ? max : val;
    }

    void setKeyDown(const uint8_t note, const bool down, const float velocity = 1.0f)
    {
        if (note > 127)
            return;

        const float clampedVel = clampVal(velocity, 0.0f, 1.0f);
        if (fKeysDown[note] == down && (!down || std::abs(fKeyVelocities[note] - clampedVel) < 0.001f))
            return;

        fKeysDown[note] = down;
        fKeyVelocities[note] = down ? clampedVel : 0.0f;
        repaint();
    }

    bool isKeyDown(const uint8_t note) const noexcept
    {
        return (note <= 127) ? fKeysDown[note] : false;
    }

    float getVelocity(const uint8_t note) const noexcept
    {
        return (note <= 127) ? fKeyVelocities[note] : 0.0f;
    }

    void clearAllKeys()
    {
        bool changed = false;
        for (int i = 0; i < 128; ++i)
        {
            if (fKeysDown[i])
            {
                fKeysDown[i] = false;
                fKeyVelocities[i] = 0.0f;
                changed = true;
            }
        }
        fCurrentHeldNote = -1;
        fCurrentPitchBend = 0.0f;
        if (changed)
            repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Performance & Expressivity Settings

    void setVelocityEnabled(const bool enabled) noexcept
    {
        fVelocityEnabled = enabled;
    }

    bool isVelocityEnabled() const noexcept
    {
        return fVelocityEnabled;
    }

    void setDefaultVelocity(const float vel) noexcept
    {
        fDefaultVelocity = clampVal(vel, 0.0f, 1.0f);
    }

    float getDefaultVelocity() const noexcept
    {
        return fDefaultVelocity;
    }

    void setMpeEnabled(const bool enabled) noexcept
    {
        fMpeEnabled = enabled;
    }

    bool isMpeEnabled() const noexcept
    {
        return fMpeEnabled;
    }

    void setMpePitchBendRange(const float semitones) noexcept
    {
        fMpePitchBendRange = (semitones < 0.1f) ? 0.1f : semitones;
    }

    float getMpePitchBendRange() const noexcept
    {
        return fMpePitchBendRange;
    }

    void setShowLabels(const bool show) noexcept
    {
        if (fShowLabels != show)
        {
            fShowLabels = show;
            repaint();
        }
    }

    bool getShowLabels() const noexcept
    {
        return fShowLabels;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Styling & Colors

    void setWhiteKeyColor(const Color& c) noexcept       { fWhiteKeyColor = c; repaint(); }
    void setWhiteKeyPressedColor(const Color& c) noexcept { fWhiteKeyPressedColor = c; repaint(); }
    void setBlackKeyColor(const Color& c) noexcept       { fBlackKeyColor = c; repaint(); }
    void setBlackKeyPressedColor(const Color& c) noexcept { fBlackKeyPressedColor = c; repaint(); }
    void setBorderColor(const Color& c) noexcept         { fBorderColor = c; repaint(); }
    void setLabelColor(const Color& c) noexcept          { fLabelColor = c; repaint(); }
    void setCornerRadius(const float r) noexcept         { fCornerRadius = r; repaint(); }

    const Color& getWhiteKeyColor() const noexcept       { return fWhiteKeyColor; }
    const Color& getWhiteKeyPressedColor() const noexcept { return fWhiteKeyPressedColor; }
    const Color& getBlackKeyColor() const noexcept       { return fBlackKeyColor; }
    const Color& getBlackKeyPressedColor() const noexcept { return fBlackKeyPressedColor; }
    const Color& getBorderColor() const noexcept         { return fBorderColor; }
    const Color& getLabelColor() const noexcept          { return fLabelColor; }
    float getCornerRadius() const noexcept               { return fCornerRadius; }

    // ----------------------------------------------------------------------------------------------------------------
    // Callbacks

    void setCallback(Callback* const cb) noexcept
    {
        fUserCallback = cb;
    }

    void setNoteOnCallback(NoteOnCallback cb)
    {
        fNoteOnLambda = cb;
    }

    void setNoteOffCallback(NoteOffCallback cb)
    {
        fNoteOffLambda = cb;
    }

    void setPitchBendCallback(PitchBendCallback cb)
    {
        fPitchBendLambda = cb;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Accessibility Conformance

    AccessibleElement* getAccessibleElement() noexcept override
    {
        return this;
    }

    bool performAccessibleAction(const AccessibleAction action) override
    {
        switch (action)
        {
        case kAccessibleActionPress:
            if (!isKeyDown(fFocusedKey))
            {
                setKeyDown(fFocusedKey, true, fDefaultVelocity);
                notifyNoteOn(fFocusedKey, fDefaultVelocity);
            }
            else
            {
                setKeyDown(fFocusedKey, false);
                notifyNoteOff(fFocusedKey);
            }
            notifyAccessibilityValueChanged();
            repaint();
            return true;

        case kAccessibleActionIncrement:
            incrementFocusKey();
            notifyAccessibilityValueChanged();
            repaint();
            return true;

        case kAccessibleActionDecrement:
            decrementFocusKey();
            notifyAccessibilityValueChanged();
            repaint();
            return true;

        default:
            return false;
        }
    }

    void incrementFocusKey()
    {
        const uint8_t maxKey = static_cast<uint8_t>(fStartKey + fNumKeys - 1);
        if (fFocusedKey < maxKey)
            ++fFocusedKey;
        else
            fFocusedKey = fStartKey;
        updateAccessibilityInfo();
    }

    void decrementFocusKey()
    {
        const uint8_t maxKey = static_cast<uint8_t>(fStartKey + fNumKeys - 1);
        if (fFocusedKey > fStartKey)
            --fFocusedKey;
        else
            fFocusedKey = maxKey;
        updateAccessibilityInfo();
    }

    uint8_t getFocusedKey() const noexcept
    {
        return fFocusedKey;
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Widget Event Handlers

    void onNanoDisplay() override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        if (w <= 0.0f || h <= 0.0f)
            return;

        std::vector<KeyRect> rects;
        getKeyRects(rects);
        if (rects.empty())
            return;

        // 1. Draw White Keys
        for (size_t i = 0; i < rects.size(); ++i)
        {
            if (rects[i].isBlack)
                continue;

            const KeyRect& kr = rects[i];
            const bool pressed = isKeyDown(kr.note);
            const float vel = pressed ? getVelocity(kr.note) : 0.0f;

            beginPath();
            roundedRect(kr.x + 0.5f, kr.y + 0.5f, kr.w - 1.0f, kr.h - 1.0f, fCornerRadius);

            if (pressed)
            {
                // Dynamic illuminated active accent
                const float blend = 0.5f + 0.5f * vel;
                Paint activeGrad = linearGradient(
                    kr.x, kr.y, kr.x, kr.y + kr.h,
                    Color(fWhiteKeyPressedColor.red, fWhiteKeyPressedColor.green, fWhiteKeyPressedColor.blue, static_cast<uint8_t>(255 * blend)),
                    Color(static_cast<uint8_t>(fWhiteKeyPressedColor.red * 0.7f),
                          static_cast<uint8_t>(fWhiteKeyPressedColor.green * 0.7f),
                          static_cast<uint8_t>(fWhiteKeyPressedColor.blue * 0.7f),
                          static_cast<uint8_t>(255 * blend)));
                fillPaint(activeGrad);
            }
            else
            {
                // Satin ivory vertical gradient
                Paint keyGrad = linearGradient(
                    kr.x, kr.y, kr.x, kr.y + kr.h,
                    fWhiteKeyColor,
                    Color(static_cast<uint8_t>(fWhiteKeyColor.red * 0.90f),
                          static_cast<uint8_t>(fWhiteKeyColor.green * 0.90f),
                          static_cast<uint8_t>(fWhiteKeyColor.blue * 0.90f)));
                fillPaint(keyGrad);
            }
            fill();

            // Border
            strokeColor(fBorderColor);
            strokeWidth(1.0f);
            stroke();

            // Note Labels on "C" keys
            if (fShowLabels && (kr.note % 12 == 0))
            {
                char labelBuf[16];
                std::snprintf(labelBuf, sizeof(labelBuf), "C%d", getNoteOctave(kr.note));

                fontSize(10.0f);
                textAlign(ALIGN_CENTER | ALIGN_BOTTOM);
                if (pressed)
                    fillColor(Color(255, 255, 255, 230));
                else
                    fillColor(fLabelColor);

                text(kr.x + (kr.w * 0.5f), kr.h - 6.0f, labelBuf, nullptr);
            }
        }

        // 2. Draw Black Keys (Rendered on top with drop shadows)
        for (size_t i = 0; i < rects.size(); ++i)
        {
            if (!rects[i].isBlack)
                continue;

            const KeyRect& kr = rects[i];
            const bool pressed = isKeyDown(kr.note);
            const float vel = pressed ? getVelocity(kr.note) : 0.0f;

            // Soft drop shadow beneath black keys onto white keys
            Paint shadow = boxGradient(
                kr.x, kr.y + 2.0f, kr.w, kr.h,
                fCornerRadius, 3.5f,
                Color(0, 0, 0, 110), Color(0, 0, 0, 0));
            beginPath();
            rect(kr.x - 2.0f, kr.y, kr.w + 4.0f, kr.h + 4.0f);
            fillPaint(shadow);
            fill();

            // Black key body
            beginPath();
            roundedRect(kr.x, kr.y, kr.w, kr.h, fCornerRadius);

            if (pressed)
            {
                const float blend = 0.6f + 0.4f * vel;
                Paint activeGrad = linearGradient(
                    kr.x, kr.y, kr.x, kr.y + kr.h,
                    Color(fBlackKeyPressedColor.red, fBlackKeyPressedColor.green, fBlackKeyPressedColor.blue, static_cast<uint8_t>(255 * blend)),
                    Color(static_cast<uint8_t>(fBlackKeyPressedColor.red * 0.6f),
                          static_cast<uint8_t>(fBlackKeyPressedColor.green * 0.6f),
                          static_cast<uint8_t>(fBlackKeyPressedColor.blue * 0.6f),
                          static_cast<uint8_t>(255 * blend)));
                fillPaint(activeGrad);
            }
            else
            {
                Paint keyGrad = linearGradient(
                    kr.x, kr.y, kr.x, kr.y + kr.h,
                    Color(static_cast<uint8_t>(std::min(255, int(fBlackKeyColor.red * 1.5f))),
                          static_cast<uint8_t>(std::min(255, int(fBlackKeyColor.green * 1.5f))),
                          static_cast<uint8_t>(std::min(255, int(fBlackKeyColor.blue * 1.5f)))),
                    fBlackKeyColor);
                fillPaint(keyGrad);
            }
            fill();

            // Subtle raised 3D cap ridge
            if (!pressed && kr.w > 6.0f && kr.h > 10.0f)
            {
                beginPath();
                roundedRect(kr.x + 1.5f, kr.y + 1.0f, kr.w - 3.0f, kr.h - 4.0f, fCornerRadius * 0.6f);
                Paint capGrad = linearGradient(
                    kr.x, kr.y, kr.x, kr.y + kr.h,
                    Color(75, 80, 90, 140), Color(35, 38, 44, 140));
                fillPaint(capGrad);
                fill();
            }

            // Border
            strokeColor(Color(20, 22, 26, 220));
            strokeWidth(1.0f);
            stroke();
        }
    }

public:
    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != kMouseButtonLeft)
            return false;

        if (ev.press)
        {
            const int16_t note = getNoteAtPos(ev.pos.getX(), ev.pos.getY());
            if (note >= 0)
            {
                const uint8_t uNote = static_cast<uint8_t>(note);
                const float keyH = isBlackKey(uNote) ? (static_cast<float>(getHeight()) * fBlackKeyHeightRatio)
                                                     : static_cast<float>(getHeight());
                const float relY = clampVal(static_cast<float>(ev.pos.getY()) / keyH, 0.0f, 1.0f);
                const float velocity = fVelocityEnabled ? (0.25f + 0.75f * relY) : fDefaultVelocity;

                setKeyDown(uNote, true, velocity);
                fCurrentHeldNote = note;
                fFocusedKey = uNote;
                fInitialClickX = static_cast<float>(ev.pos.getX());
                fInitialClickY = static_cast<float>(ev.pos.getY());
                fCurrentPitchBend = 0.0f;

                updateAccessibilityInfo();
                notifyNoteOn(uNote, velocity);
                repaint();
                return true;
            }
        }
        else
        {
            if (fCurrentHeldNote >= 0)
            {
                const uint8_t uNote = static_cast<uint8_t>(fCurrentHeldNote);
                setKeyDown(uNote, false);
                notifyNoteOff(uNote);

                if (fCurrentPitchBend != 0.0f)
                {
                    fCurrentPitchBend = 0.0f;
                    notifyPitchBend(uNote, 0.0f);
                }

                fCurrentHeldNote = -1;
                repaint();
                return true;
            }
        }

        return false;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (fCurrentHeldNote < 0)
            return false;

        const int16_t newNote = getNoteAtPos(ev.pos.getX(), ev.pos.getY());

        // Mouse Gliding: Dragging across keys
        if (newNote != fCurrentHeldNote)
        {
            const uint8_t oldNote = static_cast<uint8_t>(fCurrentHeldNote);
            setKeyDown(oldNote, false);
            notifyNoteOff(oldNote);

            if (fCurrentPitchBend != 0.0f)
            {
                fCurrentPitchBend = 0.0f;
                notifyPitchBend(oldNote, 0.0f);
            }

            if (newNote >= 0)
            {
                const uint8_t uNew = static_cast<uint8_t>(newNote);
                const float keyH = isBlackKey(uNew) ? (static_cast<float>(getHeight()) * fBlackKeyHeightRatio)
                                                    : static_cast<float>(getHeight());
                const float relY = clampVal(static_cast<float>(ev.pos.getY()) / keyH, 0.0f, 1.0f);
                const float velocity = fVelocityEnabled ? (0.25f + 0.75f * relY) : fDefaultVelocity;

                setKeyDown(uNew, true, velocity);
                fCurrentHeldNote = newNote;
                fFocusedKey = uNew;
                fInitialClickX = static_cast<float>(ev.pos.getX());
                fInitialClickY = static_cast<float>(ev.pos.getY());

                updateAccessibilityInfo();
                notifyNoteOn(uNew, velocity);
            }
            else
            {
                fCurrentHeldNote = -1;
            }

            repaint();
            return true;
        }
        else if (fMpeEnabled)
        {
            // MPE Horizontal Micro-Movement -> Pitch Bend
            const uint32_t whiteCount = getWhiteKeyCount();
            if (whiteCount > 0)
            {
                const float whiteW = static_cast<float>(getWidth()) / static_cast<float>(whiteCount);
                const float deltaX = static_cast<float>(ev.pos.getX()) - fInitialClickX;
                const float normBend = clampVal(deltaX / (whiteW * 1.5f), -1.0f, 1.0f);

                if (std::abs(normBend - fCurrentPitchBend) > 0.015f)
                {
                    fCurrentPitchBend = normBend;
                    notifyPitchBend(static_cast<uint8_t>(fCurrentHeldNote), normBend * fMpePitchBendRange);
                }
            }
            return true;
        }

        return false;
    }

    bool onKeyboard(const KeyboardEvent& ev) override
    {
        if (!isVisible())
            return false;

        if (ev.press)
        {
            switch (ev.key)
            {
            case kKeyLeft:
                decrementFocusKey();
                notifyAccessibilityValueChanged();
                repaint();
                return true;

            case kKeyRight:
                incrementFocusKey();
                notifyAccessibilityValueChanged();
                repaint();
                return true;

            case kKeyUp:
                shiftOctave(1);
                notifyAccessibilityValueChanged();
                return true;

            case kKeyDown:
                shiftOctave(-1);
                notifyAccessibilityValueChanged();
                return true;

            case kKeySpace:
            case kKeyEnter:
                if (!isKeyDown(fFocusedKey))
                {
                    setKeyDown(fFocusedKey, true, fDefaultVelocity);
                    notifyNoteOn(fFocusedKey, fDefaultVelocity);
                    notifyAccessibilityValueChanged();
                    repaint();
                }
                return true;

            default:
                break;
            }
        }
        else
        {
            if (ev.key == kKeySpace || ev.key == kKeyEnter)
            {
                if (isKeyDown(fFocusedKey))
                {
                    setKeyDown(fFocusedKey, false);
                    notifyNoteOff(fFocusedKey);
                    notifyAccessibilityValueChanged();
                    repaint();
                }
                return true;
            }
        }

        return NanoSubWidget::onKeyboard(ev);
    }

private:
    uint8_t fStartKey;
    uint8_t fNumKeys;
    int16_t fCurrentHeldNote;
    uint8_t fFocusedKey;

    bool  fVelocityEnabled;
    float fDefaultVelocity;
    bool  fMpeEnabled;
    float fMpePitchBendRange;
    float fInitialClickX;
    float fInitialClickY;
    float fCurrentPitchBend;

    float fBlackKeyHeightRatio;
    float fBlackKeyWidthRatio;
    float fCornerRadius;
    bool  fShowLabels;

    Color fWhiteKeyColor;
    Color fWhiteKeyPressedColor;
    Color fBlackKeyColor;
    Color fBlackKeyPressedColor;
    Color fBorderColor;
    Color fLabelColor;

    bool  fKeysDown[128];
    float fKeyVelocities[128];

    Callback*        fUserCallback;
    NoteOnCallback    fNoteOnLambda;
    NoteOffCallback   fNoteOffLambda;
    PitchBendCallback fPitchBendLambda;

    void notifyNoteOn(const uint8_t note, const float velocity)
    {
        if (fUserCallback != nullptr)
            fUserCallback->vectorKeyboardNoteOn(this, note, velocity);
        if (fNoteOnLambda != nullptr)
            fNoteOnLambda(this, note, velocity);
    }

    void notifyNoteOff(const uint8_t note)
    {
        if (fUserCallback != nullptr)
            fUserCallback->vectorKeyboardNoteOff(this, note);
        if (fNoteOffLambda != nullptr)
            fNoteOffLambda(this, note);
    }

    void notifyPitchBend(const uint8_t note, const float bend)
    {
        if (fUserCallback != nullptr)
            fUserCallback->vectorKeyboardPitchBend(this, note, bend);
        if (fPitchBendLambda != nullptr)
            fPitchBendLambda(this, note, bend);
    }

    void initAccessibility()
    {
        fAccessibleInfo.setRole(kAccessibleRoleKeyboard);
        fAccessibleInfo.setLabel("Virtual Piano Keyboard");
        fAccessibleInfo.setDescription("Interactive piano keyboard controller and MIDI monitor");
        fAccessibleInfo.minValue = 0.0f;
        fAccessibleInfo.maxValue = 127.0f;
        fAccessibleInfo.step = 1.0f;
        updateAccessibilityInfo();
    }

    void updateAccessibilityInfo()
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s%d (%u)",
                      getNoteName(fFocusedKey),
                      getNoteOctave(fFocusedKey),
                      fFocusedKey);
        fAccessibleInfo.setValue(static_cast<float>(fFocusedKey), buf);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorKeyboard)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_KEYBOARD_HPP_INCLUDED
