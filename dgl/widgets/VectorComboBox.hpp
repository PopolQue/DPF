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

#ifndef DGL_VECTOR_COMBO_BOX_HPP_INCLUDED
#define DGL_VECTOR_COMBO_BOX_HPP_INCLUDED

#include "../Accessibility.hpp"
#include "../Color.hpp"
#include "../NanoVG.hpp"
#include "../../distrho/extra/String.hpp"

#include <algorithm>
#include <vector>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Resolution-independent, customizable NanoVG vector combo box / dropdown selector.
   Supports item lists, hover highlights, vector chevron indicator, popup overlay, and keyboard navigation.
 */
class VectorComboBox : public NanoSubWidget,
                       public AccessibleElement
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void vectorComboBoxIndexChanged(VectorComboBox* comboBox, uint32_t index) {}
    };

    explicit VectorComboBox(Widget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fCurrentIndex(-1),
          fHoveredItem(-1),
          fIsOpen(false),
          fNormalColor(Color(36, 38, 42)),
          fHoverColor(Color(48, 51, 56)),
          fMenuBgColor(Color(28, 30, 34)),
          fMenuItemHoverColor(Color(40, 110, 190)),
          fBorderColor(Color(65, 68, 75)),
          fTextColor(Color(230, 232, 235)),
          fChevronColor(Color(180, 185, 195)),
          fCornerRadius(4.0f),
          fBorderWidth(1.2f),
          fFontSize(12.0f),
          fItemHeight(24.0f),
          fUserCallback(nullptr)
    {
        initAccessibility();
    }

    explicit VectorComboBox(NanoSubWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fCurrentIndex(-1),
          fHoveredItem(-1),
          fIsOpen(false),
          fNormalColor(Color(36, 38, 42)),
          fHoverColor(Color(48, 51, 56)),
          fMenuBgColor(Color(28, 30, 34)),
          fMenuItemHoverColor(Color(40, 110, 190)),
          fBorderColor(Color(65, 68, 75)),
          fTextColor(Color(230, 232, 235)),
          fChevronColor(Color(180, 185, 195)),
          fCornerRadius(4.0f),
          fBorderWidth(1.2f),
          fFontSize(12.0f),
          fItemHeight(24.0f),
          fUserCallback(nullptr)
    {
        initAccessibility();
    }

    explicit VectorComboBox(NanoTopLevelWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fCurrentIndex(-1),
          fHoveredItem(-1),
          fIsOpen(false),
          fNormalColor(Color(36, 38, 42)),
          fHoverColor(Color(48, 51, 56)),
          fMenuBgColor(Color(28, 30, 34)),
          fMenuItemHoverColor(Color(40, 110, 190)),
          fBorderColor(Color(65, 68, 75)),
          fTextColor(Color(230, 232, 235)),
          fChevronColor(Color(180, 185, 195)),
          fCornerRadius(4.0f),
          fBorderWidth(1.2f),
          fFontSize(12.0f),
          fItemHeight(24.0f),
          fUserCallback(nullptr)
    {
        initAccessibility();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Item Management

    void addItem(const char* const item)
    {
        if (item == nullptr)
            return;

        fItems.push_back(String(item));
        if (fCurrentIndex == -1)
            fCurrentIndex = 0;
        repaint();
    }

    bool removeItem(const uint32_t index)
    {
        if (index >= fItems.size())
            return false;

        fItems.erase(fItems.begin() + index);

        if (fItems.empty())
            fCurrentIndex = -1;
        else if (fCurrentIndex >= static_cast<int32_t>(fItems.size()))
            fCurrentIndex = static_cast<int32_t>(fItems.size()) - 1;

        repaint();
        return true;
    }

    void clearItems()
    {
        fItems.clear();
        fCurrentIndex = -1;
        fHoveredItem = -1;
        fIsOpen = false;
        repaint();
    }

    uint32_t getItemCount() const noexcept
    {
        return static_cast<uint32_t>(fItems.size());
    }

    static const String& getFallbackString() noexcept
    {
        static const String sFallback;
        return sFallback;
    }

    const String& getItem(const uint32_t index) const noexcept
    {
        if (index < fItems.size())
            return fItems[index];
        return getFallbackString();
    }

    int32_t getCurrentIndex() const noexcept
    {
        return fCurrentIndex;
    }

    void setCurrentIndex(const int32_t index, const bool sendCallback = false)
    {
        if (index < -1 || index >= static_cast<int32_t>(fItems.size()))
            return;

        if (fCurrentIndex == index)
            return;

        fCurrentIndex = index;
        setAccessibleValue(static_cast<float>(fCurrentIndex), getCurrentItem().buffer());
        repaint();

        if (sendCallback && fUserCallback != nullptr && fCurrentIndex >= 0)
            fUserCallback->vectorComboBoxIndexChanged(this, static_cast<uint32_t>(fCurrentIndex));
    }

    const String& getCurrentItem() const noexcept
    {
        if (fCurrentIndex >= 0 && fCurrentIndex < static_cast<int32_t>(fItems.size()))
            return fItems[fCurrentIndex];
        return getFallbackString();
    }

    bool isOpen() const noexcept
    {
        return fIsOpen;
    }

    void setOpen(const bool open) noexcept
    {
        if (fIsOpen == open)
            return;

        fIsOpen = open;
        if (!fIsOpen)
            fHoveredItem = -1;
        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Styling

    void setNormalColor(const Color& color) noexcept
    {
        fNormalColor = color;
        repaint();
    }

    void setHoverColor(const Color& color) noexcept
    {
        fHoverColor = color;
        repaint();
    }

    void setMenuBackgroundColor(const Color& color) noexcept
    {
        fMenuBgColor = color;
        repaint();
    }

    void setMenuItemHoverColor(const Color& color) noexcept
    {
        fMenuItemHoverColor = color;
        repaint();
    }

    void setBorderColor(const Color& color) noexcept
    {
        fBorderColor = color;
        repaint();
    }

    void setTextColor(const Color& color) noexcept
    {
        fTextColor = color;
        repaint();
    }

    void setChevronColor(const Color& color) noexcept
    {
        fChevronColor = color;
        repaint();
    }

    void setCornerRadius(const float radius) noexcept
    {
        fCornerRadius = radius;
        repaint();
    }

    void setBorderWidth(const float width) noexcept
    {
        fBorderWidth = width;
        repaint();
    }

    void setFontSize(const float size) noexcept
    {
        fFontSize = size;
        repaint();
    }

    void setItemHeight(const float height) noexcept
    {
        fItemHeight = height;
        repaint();
    }

    void setCallback(Callback* const cb) noexcept
    {
        fUserCallback = cb;
    }

    AccessibleElement* getAccessibleElement() noexcept override
    {
        return this;
    }

    bool performAccessibleAction(const AccessibleAction action) override
    {
        if (action == kAccessibleActionIncrement)
        {
            if (fCurrentIndex + 1 < static_cast<int32_t>(fItems.size()))
            {
                setCurrentIndex(fCurrentIndex + 1, true);
                return true;
            }
        }
        else if (action == kAccessibleActionDecrement)
        {
            if (fCurrentIndex > 0)
            {
                setCurrentIndex(fCurrentIndex - 1, true);
                return true;
            }
        }
        else if (action == kAccessibleActionPress)
        {
            setOpen(!fIsOpen);
            return true;
        }
        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // NanoVG Rendering

    void onNanoDisplay() override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        if (w <= 1.0f || h <= 1.0f)
            return;

        // 1. Main Closed Box
        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, fCornerRadius);
        fillColor(fIsOpen ? fHoverColor : fNormalColor);
        fill();

        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, fCornerRadius);
        strokeColor(fBorderColor);
        strokeWidth(fBorderWidth);
        stroke();

        // 2. Selected text label
        const String& curText = getCurrentItem();
        if (curText.isNotEmpty())
        {
            fontSize(fFontSize);
            fillColor(fTextColor);
            textAlign(ALIGN_LEFT | ALIGN_MIDDLE);

            // Left padding 8px
            text(8.0f, h * 0.5f, curText.buffer(), nullptr);
        }

        // 3. Chevron Indicator (triangle arrow)
        const float chevRight = w - 12.0f;
        const float chevMidY = h * 0.5f;
        const float chevHalfW = 4.0f;
        const float chevHalfH = 3.0f;

        beginPath();
        if (fIsOpen)
        {
            // Arrow pointing up
            moveTo(chevRight - chevHalfW, chevMidY + chevHalfH);
            lineTo(chevRight, chevMidY - chevHalfH);
            lineTo(chevRight + chevHalfW, chevMidY + chevHalfH);
        }
        else
        {
            // Arrow pointing down
            moveTo(chevRight - chevHalfW, chevMidY - chevHalfH);
            lineTo(chevRight, chevMidY + chevHalfH);
            lineTo(chevRight + chevHalfW, chevMidY - chevHalfH);
        }
        strokeColor(fChevronColor);
        strokeWidth(1.5f);
        stroke();

        // 4. Dropdown Menu (if open)
        if (fIsOpen && !fItems.empty())
        {
            const float menuTop = h + 2.0f;
            const float menuHeight = static_cast<float>(fItems.size()) * fItemHeight;

            // Background & Border
            beginPath();
            roundedRect(1.0f, menuTop, w - 2.0f, menuHeight, fCornerRadius);
            fillColor(fMenuBgColor);
            fill();

            beginPath();
            roundedRect(1.0f, menuTop, w - 2.0f, menuHeight, fCornerRadius);
            strokeColor(fBorderColor);
            strokeWidth(fBorderWidth);
            stroke();

            // Render items
            fontSize(fFontSize);
            textAlign(ALIGN_LEFT | ALIGN_MIDDLE);

            for (size_t i = 0; i < fItems.size(); ++i)
            {
                const float itemY = menuTop + static_cast<float>(i) * fItemHeight;
                const bool isHovered = (static_cast<int32_t>(i) == fHoveredItem);
                const bool isSelected = (static_cast<int32_t>(i) == fCurrentIndex);

                if (isHovered || isSelected)
                {
                    beginPath();
                    roundedRect(2.0f, itemY + 1.0f, w - 4.0f, fItemHeight - 2.0f, fCornerRadius * 0.5f);
                    fillColor(isHovered ? fMenuItemHoverColor : Color(50, 53, 60));
                    fill();
                }

                fillColor(fTextColor);
                text(8.0f, itemY + fItemHeight * 0.5f, fItems[i].buffer(), nullptr);
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Event Handling

    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != 1 || !ev.press)
            return false;

        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        const double x = ev.pos.getX();
        const double y = ev.pos.getY();

        if (!fIsOpen)
        {
            // Click inside closed box opens menu
            if (x >= 0.0 && x <= w && y >= 0.0 && y <= h)
            {
                setOpen(true);
                return true;
            }
            return false;
        }

        // Menu is currently open
        if (x >= 0.0 && x <= w && y >= 0.0 && y <= h)
        {
            // Clicked main box while open closes menu
            setOpen(false);
            return true;
        }

        const float menuTop = h + 2.0f;
        const float menuHeight = static_cast<float>(fItems.size()) * fItemHeight;

        if (x >= 0.0 && x <= w && y >= menuTop && y <= (menuTop + menuHeight))
        {
            const int32_t selectedIdx = static_cast<int32_t>((y - menuTop) / fItemHeight);
            if (selectedIdx >= 0 && selectedIdx < static_cast<int32_t>(fItems.size()))
            {
                setCurrentIndex(selectedIdx, true);
                setOpen(false);
                return true;
            }
        }

        // Clicked outside closes menu
        setOpen(false);
        return true;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (!fIsOpen || fItems.empty())
            return false;

        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        const double x = ev.pos.getX();
        const double y = ev.pos.getY();
        const float menuTop = h + 2.0f;
        const float menuHeight = static_cast<float>(fItems.size()) * fItemHeight;

        if (x >= 0.0 && x <= w && y >= menuTop && y <= (menuTop + menuHeight))
        {
            const int32_t idx = static_cast<int32_t>((y - menuTop) / fItemHeight);
            if (idx >= 0 && idx < static_cast<int32_t>(fItems.size()) && idx != fHoveredItem)
            {
                fHoveredItem = idx;
                repaint();
                return true;
            }
        }
        else if (fHoveredItem != -1)
        {
            fHoveredItem = -1;
            repaint();
            return true;
        }

        return false;
    }

    bool onKeyboard(const KeyboardEvent& ev) override
    {
        if (!ev.press)
            return false;

        if (ev.key == kKeyEscape)
        {
            if (fIsOpen)
            {
                setOpen(false);
                return true;
            }
        }
        else if (ev.key == kKeyDown)
        {
            if (fCurrentIndex + 1 < static_cast<int32_t>(fItems.size()))
            {
                setCurrentIndex(fCurrentIndex + 1, true);
                return true;
            }
        }
        else if (ev.key == kKeyUp)
        {
            if (fCurrentIndex > 0)
            {
                setCurrentIndex(fCurrentIndex - 1, true);
                return true;
            }
        }
        else if (ev.key == kKeyEnter)
        {
            if (fIsOpen)
            {
                if (fHoveredItem >= 0 && fHoveredItem < static_cast<int32_t>(fItems.size()))
                    setCurrentIndex(fHoveredItem, true);
                setOpen(false);
                return true;
            }
        }

        return false;
    }

private:
    std::vector<String> fItems;
    int32_t fCurrentIndex;
    int32_t fHoveredItem;
    bool fIsOpen;

    Color fNormalColor;
    Color fHoverColor;
    Color fMenuBgColor;
    Color fMenuItemHoverColor;
    Color fBorderColor;
    Color fTextColor;
    Color fChevronColor;
    float fCornerRadius;
    float fBorderWidth;
    float fFontSize;
    float fItemHeight;
    Callback* fUserCallback;

    void initAccessibility()
    {
        setAccessibleRole(kAccessibleRoleComboBox);
        setAccessibleRange(0.0f, static_cast<float>(fItems.size()));
        setAccessibleValue(static_cast<float>(fCurrentIndex), getCurrentItem().buffer());
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorComboBox)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_COMBO_BOX_HPP_INCLUDED
