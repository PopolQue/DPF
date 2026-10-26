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

#ifndef DGL_VECTOR_TAB_GROUP_HPP_INCLUDED
#define DGL_VECTOR_TAB_GROUP_HPP_INCLUDED

#include "../Accessibility.hpp"
#include "../Color.hpp"
#include "../NanoVG.hpp"
#include "../../distrho/extra/String.hpp"

#include <vector>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Resolution-independent, customizable NanoVG vector tab navigation bar.
   Supports multiple tabs, hover highlighting, active indicator pill/line, and keyboard navigation.
 */
class VectorTabGroup : public NanoSubWidget,
                       public AccessibleElement
{
public:
    enum Style {
        kStyleUnderline = 0,
        kStylePill      = 1
    };

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void vectorTabChanged(VectorTabGroup* tabGroup, uint32_t tabIndex) {}
    };

    explicit VectorTabGroup(Widget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fCurrentTab(0),
          fHoveredTab(-1),
          fStyle(kStylePill),
          fBgColor(Color(28, 30, 34)),
          fNormalTextColor(Color(160, 165, 175)),
          fHoverTextColor(Color(220, 225, 235)),
          fActiveTextColor(Color(255, 255, 255)),
          fActiveIndicatorColor(Color(45, 115, 195)),
          fBorderColor(Color(55, 58, 65)),
          fCornerRadius(4.0f),
          fFontSize(12.0f),
          fIndicatorThickness(3.0f),
          fUserCallback(nullptr)
    {
        initAccessibility();
    }

    explicit VectorTabGroup(NanoSubWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fCurrentTab(0),
          fHoveredTab(-1),
          fStyle(kStylePill),
          fBgColor(Color(28, 30, 34)),
          fNormalTextColor(Color(160, 165, 175)),
          fHoverTextColor(Color(220, 225, 235)),
          fActiveTextColor(Color(255, 255, 255)),
          fActiveIndicatorColor(Color(45, 115, 195)),
          fBorderColor(Color(55, 58, 65)),
          fCornerRadius(4.0f),
          fFontSize(12.0f),
          fIndicatorThickness(3.0f),
          fUserCallback(nullptr)
    {
        initAccessibility();
    }

    explicit VectorTabGroup(NanoTopLevelWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fCurrentTab(0),
          fHoveredTab(-1),
          fStyle(kStylePill),
          fBgColor(Color(28, 30, 34)),
          fNormalTextColor(Color(160, 165, 175)),
          fHoverTextColor(Color(220, 225, 235)),
          fActiveTextColor(Color(255, 255, 255)),
          fActiveIndicatorColor(Color(45, 115, 195)),
          fBorderColor(Color(55, 58, 65)),
          fCornerRadius(4.0f),
          fFontSize(12.0f),
          fIndicatorThickness(3.0f),
          fUserCallback(nullptr)
    {
        initAccessibility();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Tab Management

    void addTab(const char* const label)
    {
        if (label == nullptr)
            return;

        fTabs.push_back(String(label));
        repaint();
    }

    void clearTabs()
    {
        fTabs.clear();
        fCurrentTab = 0;
        fHoveredTab = -1;
        repaint();
    }

    uint32_t getTabCount() const noexcept
    {
        return static_cast<uint32_t>(fTabs.size());
    }

    uint32_t getCurrentTab() const noexcept
    {
        return fCurrentTab;
    }

    void setCurrentTab(const uint32_t index, const bool sendCallback = false)
    {
        if (fTabs.empty() || index >= fTabs.size() || fCurrentTab == index)
            return;

        fCurrentTab = index;
        setAccessibleValue(static_cast<float>(fCurrentTab), getTabLabel(fCurrentTab).buffer());
        repaint();

        if (sendCallback && fUserCallback != nullptr)
            fUserCallback->vectorTabChanged(this, fCurrentTab);
    }

    AccessibleElement* getAccessibleElement() noexcept override
    {
        return this;
    }

    bool performAccessibleAction(const AccessibleAction action) override
    {
        if (action == kAccessibleActionIncrement)
        {
            if (fCurrentTab + 1 < static_cast<uint32_t>(fTabs.size()))
            {
                setCurrentTab(fCurrentTab + 1, true);
                return true;
            }
        }
        else if (action == kAccessibleActionDecrement)
        {
            if (fCurrentTab > 0)
            {
                setCurrentTab(fCurrentTab - 1, true);
                return true;
            }
        }
        return false;
    }

    const String& getTabLabel(const uint32_t index) const noexcept
    {
        static const String sFallback;
        if (index < fTabs.size())
            return fTabs[index];
        return sFallback;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Styling

    void setStyle(const Style style) noexcept
    {
        fStyle = style;
        repaint();
    }

    void setBackgroundColor(const Color& color) noexcept
    {
        fBgColor = color;
        repaint();
    }

    void setNormalTextColor(const Color& color) noexcept
    {
        fNormalTextColor = color;
        repaint();
    }

    void setHoverTextColor(const Color& color) noexcept
    {
        fHoverTextColor = color;
        repaint();
    }

    void setActiveTextColor(const Color& color) noexcept
    {
        fActiveTextColor = color;
        repaint();
    }

    void setActiveIndicatorColor(const Color& color) noexcept
    {
        fActiveIndicatorColor = color;
        repaint();
    }

    void setBorderColor(const Color& color) noexcept
    {
        fBorderColor = color;
        repaint();
    }

    void setCornerRadius(const float radius) noexcept
    {
        fCornerRadius = radius;
        repaint();
    }

    void setFontSize(const float size) noexcept
    {
        fFontSize = size;
        repaint();
    }

    void setCallback(Callback* const cb) noexcept
    {
        fUserCallback = cb;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // NanoVG Rendering

    void onNanoDisplay() override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        if (w <= 4.0f || h <= 4.0f)
            return;

        // 1. Bar Background
        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, fCornerRadius);
        fillColor(fBgColor);
        fill();

        // 2. Bar Border
        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, fCornerRadius);
        strokeColor(fBorderColor);
        strokeWidth(1.0f);
        stroke();

        const uint32_t tabCount = static_cast<uint32_t>(fTabs.size());
        if (tabCount == 0)
            return;

        const float tabWidth = (w - 2.0f) / static_cast<float>(tabCount);

        // 3. Render Active Tab Indicator
        if (fCurrentTab < tabCount)
        {
            const float tabLeft = 1.0f + static_cast<float>(fCurrentTab) * tabWidth;

            if (fStyle == kStylePill)
            {
                beginPath();
                roundedRect(tabLeft + 2.0f, 3.0f, tabWidth - 4.0f, h - 6.0f, fCornerRadius * 0.75f);
                fillColor(fActiveIndicatorColor);
                fill();
            }
            else // Underline
            {
                beginPath();
                roundedRect(tabLeft + 4.0f, h - fIndicatorThickness - 1.0f, tabWidth - 8.0f, fIndicatorThickness, fIndicatorThickness * 0.5f);
                fillColor(fActiveIndicatorColor);
                fill();
            }
        }

        // 4. Render Tab Labels
        fontSize(fFontSize);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);

        for (uint32_t i = 0; i < tabCount; ++i)
        {
            const float tabLeft = 1.0f + static_cast<float>(i) * tabWidth;
            const float tabCenter = tabLeft + tabWidth * 0.5f;

            Color textColor;
            if (i == fCurrentTab)
                textColor = fActiveTextColor;
            else if (static_cast<int32_t>(i) == fHoveredTab)
                textColor = fHoverTextColor;
            else
                textColor = fNormalTextColor;

            fillColor(textColor);
            text(tabCenter, h * 0.5f, fTabs[i].buffer(), nullptr);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Event Handling

    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != 1 || !ev.press || fTabs.empty())
            return false;

        const float w = static_cast<float>(getWidth());
        const float tabWidth = (w - 2.0f) / static_cast<float>(fTabs.size());
        const double x = ev.pos.getX();

        if (x >= 0.0 && x <= w && tabWidth > 0.0f)
        {
            const uint32_t clickedTab = static_cast<uint32_t>(x / tabWidth);
            if (clickedTab < fTabs.size())
            {
                setCurrentTab(clickedTab, true);
                return true;
            }
        }

        return false;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (fTabs.empty())
            return false;

        const float w = static_cast<float>(getWidth());
        const float tabWidth = (w - 2.0f) / static_cast<float>(fTabs.size());
        const double x = ev.pos.getX();

        int32_t newHover = -1;
        if (x >= 0.0 && x <= w && tabWidth > 0.0f)
        {
            const uint32_t hovered = static_cast<uint32_t>(x / tabWidth);
            if (hovered < fTabs.size())
                newHover = static_cast<int32_t>(hovered);
        }

        if (newHover != fHoveredTab)
        {
            fHoveredTab = newHover;
            repaint();
            return true;
        }

        return false;
    }

    bool onKeyboard(const KeyboardEvent& ev) override
    {
        if (!ev.press || fTabs.empty())
            return false;

        if (ev.key == kKeyRight)
        {
            if (fCurrentTab + 1 < fTabs.size())
            {
                setCurrentTab(fCurrentTab + 1, true);
                return true;
            }
        }
        else if (ev.key == kKeyLeft)
        {
            if (fCurrentTab > 0)
            {
                setCurrentTab(fCurrentTab - 1, true);
                return true;
            }
        }

        return false;
    }

private:
    std::vector<String> fTabs;
    uint32_t fCurrentTab;
    int32_t fHoveredTab;
    Style fStyle;

    Color fBgColor;
    Color fNormalTextColor;
    Color fHoverTextColor;
    Color fActiveTextColor;
    Color fActiveIndicatorColor;
    Color fBorderColor;
    float fCornerRadius;
    float fFontSize;
    float fIndicatorThickness;
    Callback* fUserCallback;

    void initAccessibility()
    {
        setAccessibleRole(kAccessibleRoleTab);
        setAccessibleLabel("Tab Group");
        setAccessibleRange(0.0f, static_cast<float>(fTabs.size()));
        setAccessibleValue(static_cast<float>(fCurrentTab), getTabLabel(fCurrentTab).buffer());
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorTabGroup)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_TAB_GROUP_HPP_INCLUDED
