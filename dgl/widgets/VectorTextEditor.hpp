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

#ifndef DGL_VECTOR_TEXT_EDITOR_HPP_INCLUDED
#define DGL_VECTOR_TEXT_EDITOR_HPP_INCLUDED

#include "../Color.hpp"
#include "../NanoVG.hpp"
#include "../Accessibility.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Modern resolution-independent NanoVG text editor and numeric value entry widget.
   Supports cursor placement, character insertion, selection, backspace/delete, numeric parsing,
   and semantic accessibility (A11y).
 */
class VectorTextEditor : public NanoSubWidget, public AccessibleElement
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void textEditorChanged(VectorTextEditor* editor, const char* newText) { (void)editor; (void)newText; }
        virtual void textEditorSubmitted(VectorTextEditor* editor, const char* text) { (void)editor; (void)text; }
        virtual void textEditorCancelled(VectorTextEditor* editor) { (void)editor; }
    };

    explicit VectorTextEditor(Widget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fUserCallback(nullptr),
          fText(),
          fOrigText(),
          fPlaceholder(),
          fCursorPos(0),
          fSelStart(0),
          fSelEnd(0),
          fFocused(false),
          fNumericMode(false),
          fMinValue(0.0f),
          fMaxValue(100.0f),
          fPrecision(2),
          fCornerRadius(4.0f),
          fFontSize(13.0f),
          fPaddingX(8.0f),
          fBgColor(Color(24, 26, 32)),
          fFocusedBgColor(Color(28, 30, 38)),
          fBorderColor(Color(55, 58, 66)),
          fFocusedBorderColor(Color(64, 156, 255)),
          fTextColor(Color(240, 240, 240)),
          fPlaceholderColor(Color(120, 124, 136)),
          fCursorColor(Color(64, 156, 255)),
          fSelectionColor(Color(64, 156, 255, 80))
    {
        initAccessibility();
    }

    explicit VectorTextEditor(NanoSubWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fUserCallback(nullptr),
          fText(),
          fOrigText(),
          fPlaceholder(),
          fCursorPos(0),
          fSelStart(0),
          fSelEnd(0),
          fFocused(false),
          fNumericMode(false),
          fMinValue(0.0f),
          fMaxValue(100.0f),
          fPrecision(2),
          fCornerRadius(4.0f),
          fFontSize(13.0f),
          fPaddingX(8.0f),
          fBgColor(Color(24, 26, 32)),
          fFocusedBgColor(Color(28, 30, 38)),
          fBorderColor(Color(55, 58, 66)),
          fFocusedBorderColor(Color(64, 156, 255)),
          fTextColor(Color(240, 240, 240)),
          fPlaceholderColor(Color(120, 124, 136)),
          fCursorColor(Color(64, 156, 255)),
          fSelectionColor(Color(64, 156, 255, 80))
    {
        initAccessibility();
    }

    explicit VectorTextEditor(NanoTopLevelWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fUserCallback(nullptr),
          fText(),
          fOrigText(),
          fPlaceholder(),
          fCursorPos(0),
          fSelStart(0),
          fSelEnd(0),
          fFocused(false),
          fNumericMode(false),
          fMinValue(0.0f),
          fMaxValue(100.0f),
          fPrecision(2),
          fCornerRadius(4.0f),
          fFontSize(13.0f),
          fPaddingX(8.0f),
          fBgColor(Color(24, 26, 32)),
          fFocusedBgColor(Color(28, 30, 38)),
          fBorderColor(Color(55, 58, 66)),
          fFocusedBorderColor(Color(64, 156, 255)),
          fTextColor(Color(240, 240, 240)),
          fPlaceholderColor(Color(120, 124, 136)),
          fCursorColor(Color(64, 156, 255)),
          fSelectionColor(Color(64, 156, 255, 80))
    {
        initAccessibility();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Text and Value API

    void setText(const char* const text, const bool sendCallback = false)
    {
        fText = text != nullptr ? text : "";
        fCursorPos = fText.length();
        clearSelection();
        updateAccessibility();
        repaint();

        if (sendCallback && fUserCallback != nullptr)
            fUserCallback->textEditorChanged(this, fText.buffer());
    }

    const String& getText() const noexcept
    {
        return fText;
    }

    void setPlaceholder(const char* const placeholder)
    {
        fPlaceholder = placeholder != nullptr ? placeholder : "";
        repaint();
    }

    const String& getPlaceholder() const noexcept
    {
        return fPlaceholder;
    }

    void setNumericMode(const bool numeric,
                        const float minVal = 0.0f,
                        const float maxVal = 100.0f,
                        const int32_t precision = 2)
    {
        fNumericMode = numeric;
        fMinValue = minVal;
        fMaxValue = maxVal;
        fPrecision = precision;
        setAccessibleRange(minVal, maxVal, std::pow(10.0f, -static_cast<float>(precision)));
    }

    bool isNumericMode() const noexcept
    {
        return fNumericMode;
    }

    float getValueAsFloat() const
    {
        if (fText.isEmpty())
            return fMinValue;

        float val = static_cast<float>(std::atof(fText.buffer()));
        if (fNumericMode)
            val = std::max(fMinValue, std::min(fMaxValue, val));

        return val;
    }

    void setValueAsFloat(const float value, const bool sendCallback = false)
    {
        char buf[32];
        if (fPrecision <= 0)
            std::snprintf(buf, sizeof(buf), "%.0f", static_cast<double>(value));
        else if (fPrecision == 1)
            std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(value));
        else if (fPrecision == 2)
            std::snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(value));
        else
            std::snprintf(buf, sizeof(buf), "%.3f", static_cast<double>(value));

        setText(buf, sendCallback);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Selection and Focus

    void setFocused(const bool focused)
    {
        if (fFocused == focused)
            return;

        fFocused = focused;
        fAccessibleInfo.setFocused(focused);

        if (fFocused)
        {
            fOrigText = fText;
            selectAll();
        }
        else
        {
            clearSelection();
        }

        repaint();
    }

    bool isFocused() const noexcept
    {
        return fFocused;
    }

    void selectAll()
    {
        fSelStart = 0;
        fSelEnd = fText.length();
        fCursorPos = fText.length();
        repaint();
    }

    void clearSelection()
    {
        fSelStart = fSelEnd = fCursorPos;
        repaint();
    }

    bool hasSelection() const noexcept
    {
        return fSelStart != fSelEnd;
    }

    void deleteSelection()
    {
        if (!hasSelection())
            return;

        const size_t start = std::min(fSelStart, fSelEnd);
        const size_t end = std::max(fSelStart, fSelEnd);

        std::string s(fText.buffer());
        s.erase(start, end - start);
        fText = s.c_str();

        fCursorPos = start;
        clearSelection();
        updateAccessibility();

        if (fUserCallback != nullptr)
            fUserCallback->textEditorChanged(this, fText.buffer());

        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Style and Colors

    void setBackgroundColor(const Color& color) noexcept { fBgColor = color; repaint(); }
    void setFocusedBackgroundColor(const Color& color) noexcept { fFocusedBgColor = color; repaint(); }
    void setBorderColor(const Color& color) noexcept { fBorderColor = color; repaint(); }
    void setFocusedBorderColor(const Color& color) noexcept { fFocusedBorderColor = color; repaint(); }
    void setTextColor(const Color& color) noexcept { fTextColor = color; repaint(); }
    void setCornerRadius(const float radius) noexcept { fCornerRadius = radius; repaint(); }
    void setFontSize(const float size) noexcept { fFontSize = size; repaint(); }
    void setCallback(Callback* const cb) noexcept { fUserCallback = cb; }

    // ----------------------------------------------------------------------------------------------------------------
    // Event Handling

    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != kMouseButtonLeft || !ev.press)
            return false;

        setFocused(true);
        return true;
    }

    bool onKeyboard(const KeyboardEvent& ev) override
    {
        if (!fFocused || !ev.press)
            return false;

        if (ev.key == kKeyEscape)
        {
            fText = fOrigText;
            fCursorPos = fText.length();
            setFocused(false);

            if (fUserCallback != nullptr)
                fUserCallback->textEditorCancelled(this);

            return true;
        }

        if (ev.key == kKeyEnter || ev.key == kKeyPadEnter)
        {
            if (fNumericMode)
                setValueAsFloat(getValueAsFloat(), false);

            setFocused(false);

            if (fUserCallback != nullptr)
                fUserCallback->textEditorSubmitted(this, fText.buffer());

            return true;
        }

        if (ev.key == kKeyBackspace)
        {
            if (hasSelection())
            {
                deleteSelection();
            }
            else if (fCursorPos > 0)
            {
                std::string s(fText.buffer());
                s.erase(fCursorPos - 1, 1);
                fText = s.c_str();
                --fCursorPos;
                updateAccessibility();

                if (fUserCallback != nullptr)
                    fUserCallback->textEditorChanged(this, fText.buffer());

                repaint();
            }
            return true;
        }

        if (ev.key == kKeyDelete || ev.key == kKeyPadDelete)
        {
            if (hasSelection())
            {
                deleteSelection();
            }
            else if (fCursorPos < fText.length())
            {
                std::string s(fText.buffer());
                s.erase(fCursorPos, 1);
                fText = s.c_str();
                updateAccessibility();

                if (fUserCallback != nullptr)
                    fUserCallback->textEditorChanged(this, fText.buffer());

                repaint();
            }
            return true;
        }

        if (ev.key == kKeyLeft)
        {
            if (fCursorPos > 0)
                --fCursorPos;
            clearSelection();
            return true;
        }

        if (ev.key == kKeyRight)
        {
            if (fCursorPos < fText.length())
                ++fCursorPos;
            clearSelection();
            return true;
        }

        if (ev.key == kKeyHome)
        {
            fCursorPos = 0;
            clearSelection();
            return true;
        }

        if (ev.key == kKeyEnd)
        {
            fCursorPos = fText.length();
            clearSelection();
            return true;
        }

        return false;
    }

    bool onCharacterInput(const CharacterInputEvent& ev) override
    {
        if (!fFocused || ev.character < 32 || ev.character == 127)
            return false;

        const char c = static_cast<char>(ev.character);

        // Numeric filtering
        if (fNumericMode)
        {
            const bool isDigit = (c >= '0' && c <= '9');
            const bool isDot = (c == '.' && std::strchr(fText.buffer(), '.') == nullptr);
            const bool isMinus = (c == '-' && fCursorPos == 0 && std::strchr(fText.buffer(), '-') == nullptr);

            if (!isDigit && !isDot && !isMinus)
                return false;
        }

        if (hasSelection())
            deleteSelection();

        std::string s(fText.buffer());
        if (fCursorPos >= s.length())
            s.push_back(c);
        else
            s.insert(fCursorPos, 1, c);

        fText = s.c_str();
        ++fCursorPos;
        clearSelection();
        updateAccessibility();

        if (fUserCallback != nullptr)
            fUserCallback->textEditorChanged(this, fText.buffer());

        repaint();
        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // NanoVG Rendering

    void onNanoDisplay() override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        if (w <= 2.0f || h <= 2.0f)
            return;

        // 1. Background
        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, fCornerRadius);
        fillColor(fFocused ? fFocusedBgColor : fBgColor);
        fill();

        // 2. Border
        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, fCornerRadius);
        strokeColor(fFocused ? fFocusedBorderColor : fBorderColor);
        strokeWidth(fFocused ? 1.5f : 1.0f);
        stroke();

        fontSize(fFontSize);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);

        // 3. Placeholder or Text
        if (fText.isEmpty() && fPlaceholder.isNotEmpty() && !fFocused)
        {
            fillColor(fPlaceholderColor);
            text(fPaddingX, h * 0.5f, fPlaceholder.buffer(), nullptr);
        }
        else if (fText.isNotEmpty())
        {
            // Optional Selection Highlight
            if (fFocused && hasSelection())
            {
                const size_t start = std::min(fSelStart, fSelEnd);
                const size_t end = std::max(fSelStart, fSelEnd);

                // Approximate character bounds
                std::string s(fText.buffer());
                const float charW = fFontSize * 0.55f;
                const float selX = fPaddingX + static_cast<float>(start) * charW;
                const float selW = static_cast<float>(end - start) * charW;

                beginPath();
                rect(selX, 3.0f, selW, h - 6.0f);
                fillColor(fSelectionColor);
                fill();
            }

            fillColor(fTextColor);
            text(fPaddingX, h * 0.5f, fText.buffer(), nullptr);
        }

        // 4. Cursor Bar
        if (fFocused)
        {
            const float charW = fFontSize * 0.55f;
            const float cursorX = fPaddingX + static_cast<float>(fCursorPos) * charW;

            beginPath();
            moveTo(cursorX, 4.0f);
            lineTo(cursorX, h - 4.0f);
            strokeColor(fCursorColor);
            strokeWidth(1.5f);
            stroke();
        }
    }

private:
    Callback* fUserCallback;
    String    fText;
    String    fOrigText;
    String    fPlaceholder;
    size_t    fCursorPos;
    size_t    fSelStart;
    size_t    fSelEnd;
    bool      fFocused;
    bool      fNumericMode;
    float     fMinValue;
    float     fMaxValue;
    int32_t   fPrecision;

    float     fCornerRadius;
    float     fFontSize;
    float     fPaddingX;

    Color     fBgColor;
    Color     fFocusedBgColor;
    Color     fBorderColor;
    Color     fFocusedBorderColor;
    Color     fTextColor;
    Color     fPlaceholderColor;
    Color     fCursorColor;
    Color     fSelectionColor;

    void initAccessibility()
    {
        setAccessibleRole(kAccessibleRoleTextEditor);
        setAccessibleLabel("Text Editor");
        setAccessibleValue(0.0f, "");
    }

    void updateAccessibility()
    {
        if (fNumericMode)
            setAccessibleValue(getValueAsFloat(), fText.buffer());
        else
            setAccessibleValue(0.0f, fText.buffer());
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorTextEditor)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_TEXT_EDITOR_HPP_INCLUDED
