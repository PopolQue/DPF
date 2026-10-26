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

#ifndef DGL_ACCESSIBILITY_HPP_INCLUDED
#define DGL_ACCESSIBILITY_HPP_INCLUDED

#include "Base.hpp"
#include <cstdio>
#include <cstring>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Semantic accessibility roles for assistive technologies (screen readers, UI test automations).
 */
enum AccessibleRole {
    kAccessibleRoleUnknown = 0,
    kAccessibleRoleSlider,
    kAccessibleRoleDial,
    kAccessibleRoleButton,
    kAccessibleRoleCheckBox,
    kAccessibleRoleComboBox,
    kAccessibleRoleTextEditor,
    kAccessibleRoleTab,
    kAccessibleRoleIndicator,
    kAccessibleRoleGroup,
    kAccessibleRoleKeyboard
};

/**
   Semantic accessibility actions triggered by assistive technologies (e.g. VoiceOver).
 */
enum AccessibleAction {
    kAccessibleActionPress = 0,
    kAccessibleActionIncrement,
    kAccessibleActionDecrement
};

/**
   Semantic accessibility state for widgets and controls.
 */
struct AccessibleInfo {
    AccessibleRole role;
    char  label[64];
    char  description[128];
    char  valueText[64];
    float currentValue;
    float minValue;
    float maxValue;
    float step;
    bool  isReadOnly;
    bool  isFocused;

    AccessibleInfo() noexcept
        : role(kAccessibleRoleUnknown),
          currentValue(0.0f),
          minValue(0.0f),
          maxValue(1.0f),
          step(0.01f),
          isReadOnly(false),
          isFocused(false)
    {
        label[0] = '\0';
        description[0] = '\0';
        valueText[0] = '\0';
    }

    void setRole(const AccessibleRole r) noexcept
    {
        role = r;
    }

    void setLabel(const char* const l) noexcept
    {
        if (l != nullptr)
        {
            std::strncpy(label, l, sizeof(label) - 1);
            label[sizeof(label) - 1] = '\0';
        }
        else
        {
            label[0] = '\0';
        }
    }

    void setDescription(const char* const d) noexcept
    {
        if (d != nullptr)
        {
            std::strncpy(description, d, sizeof(description) - 1);
            description[sizeof(description) - 1] = '\0';
        }
        else
        {
            description[0] = '\0';
        }
    }

    void setValue(const float val, const char* const text = nullptr) noexcept
    {
        currentValue = val;
        if (text != nullptr)
        {
            std::strncpy(valueText, text, sizeof(valueText) - 1);
            valueText[sizeof(valueText) - 1] = '\0';
        }
        else
        {
            std::snprintf(valueText, sizeof(valueText), "%.2f", static_cast<double>(val));
        }
    }

    void setRange(const float min, const float max, const float s = 0.0f) noexcept
    {
        minValue = min;
        maxValue = max;
        step = s;
    }

    void setReadOnly(const bool ro) noexcept
    {
        isReadOnly = ro;
    }

    void setFocused(const bool f) noexcept
    {
        isFocused = f;
    }
};

/**
   Interface / Mixin for accessible widgets.
 */
class AccessibleElement {
public:
    typedef void (*ValueChangeCallback)(AccessibleElement* elem, void* userData);

    AccessibleElement() noexcept
        : fValueChangeCb(nullptr),
          fValueChangeUserData(nullptr)
    {}

    virtual ~AccessibleElement() {}

    virtual bool isAccessible() const noexcept { return true; }

    virtual const AccessibleInfo& getAccessibleInfo() const noexcept { return fAccessibleInfo; }
    virtual AccessibleInfo& getAccessibleInfo() noexcept { return fAccessibleInfo; }

    virtual bool performAccessibleAction(const AccessibleAction action)
    {
        (void)action;
        return false;
    }

    void setAccessibilityValueChangeCallback(ValueChangeCallback cb, void* userData = nullptr) noexcept
    {
        fValueChangeCb = cb;
        fValueChangeUserData = userData;
    }

    void notifyAccessibilityValueChanged() noexcept
    {
        if (fValueChangeCb != nullptr)
            fValueChangeCb(this, fValueChangeUserData);
    }

    void setAccessibleRole(const AccessibleRole role) noexcept
    {
        fAccessibleInfo.setRole(role);
    }

    void setAccessibleLabel(const char* const label) noexcept
    {
        fAccessibleInfo.setLabel(label);
    }

    void setAccessibleDescription(const char* const desc) noexcept
    {
        fAccessibleInfo.setDescription(desc);
    }

    void setAccessibleValue(const float val, const char* const text = nullptr) noexcept
    {
        fAccessibleInfo.setValue(val, text);
        notifyAccessibilityValueChanged();
    }

    void setAccessibleRange(const float min, const float max, const float step = 0.0f) noexcept
    {
        fAccessibleInfo.setRange(min, max, step);
    }

    void setAccessibleReadOnly(const bool ro) noexcept
    {
        fAccessibleInfo.setReadOnly(ro);
    }

protected:
    AccessibleInfo fAccessibleInfo;
    ValueChangeCallback fValueChangeCb;
    void* fValueChangeUserData;
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_ACCESSIBILITY_HPP_INCLUDED
