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

#import <Cocoa/Cocoa.h>

#include "WindowPrivateData.hpp"
#include "TopLevelWidgetPrivateData.hpp"
#include "../Accessibility.hpp"
#include "../Widget.hpp"
#include "../SubWidget.hpp"

#include <vector>

// --------------------------------------------------------------------------------------------------------------------
// DGLElementAccessibility

@interface DGLElementAccessibility : NSAccessibilityElement

@property (nonatomic, assign) DGL_NAMESPACE::SubWidget* widget;
@property (nonatomic, assign) DGL_NAMESPACE::AccessibleElement* accessible;
@property (nonatomic, assign) NSView* parentView;

- (instancetype)initWithWidget:(DGL_NAMESPACE::SubWidget*)w
                    accessible:(DGL_NAMESPACE::AccessibleElement*)a
                    parentView:(NSView*)pv;

@end

@implementation DGLElementAccessibility

- (instancetype)initWithWidget:(DGL_NAMESPACE::SubWidget*)w
                    accessible:(DGL_NAMESPACE::AccessibleElement*)a
                    parentView:(NSView*)pv
{
    if ((self = [super init]))
    {
        _widget = w;
        _accessible = a;
        _parentView = pv;
    }
    return self;
}

- (BOOL)isAccessibilityElement
{
    return (_accessible != nullptr && _accessible->isAccessible() &&
            _widget != nullptr && _widget->isVisible()) ? YES : NO;
}

- (NSAccessibilityRole)accessibilityRole
{
    if (_accessible == nullptr)
        return NSAccessibilityUnknownRole;

    switch (_accessible->getAccessibleInfo().role)
    {
    case DGL_NAMESPACE::kAccessibleRoleSlider:
    case DGL_NAMESPACE::kAccessibleRoleDial:
        return NSAccessibilitySliderRole;
    case DGL_NAMESPACE::kAccessibleRoleButton:
        return NSAccessibilityButtonRole;
    case DGL_NAMESPACE::kAccessibleRoleCheckBox:
        return NSAccessibilityCheckBoxRole;
    case DGL_NAMESPACE::kAccessibleRoleComboBox:
        return NSAccessibilityPopUpButtonRole;
    case DGL_NAMESPACE::kAccessibleRoleTextEditor:
        return NSAccessibilityTextFieldRole;
    case DGL_NAMESPACE::kAccessibleRoleTab:
        return NSAccessibilityRadioButtonRole;
    case DGL_NAMESPACE::kAccessibleRoleIndicator:
        return NSAccessibilityValueIndicatorRole;
    case DGL_NAMESPACE::kAccessibleRoleGroup:
        return NSAccessibilityGroupRole;
    default:
        return NSAccessibilityUnknownRole;
    }
}

- (NSString *)accessibilityRoleDescription
{
    if (_accessible == nullptr)
        return @"";

    switch (_accessible->getAccessibleInfo().role)
    {
    case DGL_NAMESPACE::kAccessibleRoleDial:
        return @"knob";
    case DGL_NAMESPACE::kAccessibleRoleSlider:
        return @"slider";
    case DGL_NAMESPACE::kAccessibleRoleButton:
        return @"button";
    case DGL_NAMESPACE::kAccessibleRoleCheckBox:
        return @"switch";
    case DGL_NAMESPACE::kAccessibleRoleComboBox:
        return @"pop up button";
    case DGL_NAMESPACE::kAccessibleRoleTextEditor:
        return @"text field";
    case DGL_NAMESPACE::kAccessibleRoleTab:
        return @"tab";
    case DGL_NAMESPACE::kAccessibleRoleIndicator:
        return @"indicator";
    default:
        return @"";
    }
}

- (NSString *)accessibilityLabel
{
    if (_accessible == nullptr)
        return @"";

    const char* const l = _accessible->getAccessibleInfo().label;
    if (l != nullptr && l[0] != '\0')
        return [NSString stringWithUTF8String:l];

    const char* const name = _widget != nullptr ? _widget->getName() : nullptr;
    if (name != nullptr && name[0] != '\0')
        return [NSString stringWithUTF8String:name];

    return @"Control";
}

- (id)accessibilityValue
{
    if (_accessible == nullptr)
        return nil;

    const auto& info = _accessible->getAccessibleInfo();
    if (info.role == DGL_NAMESPACE::kAccessibleRoleButton)
        return nil;

    if (info.role == DGL_NAMESPACE::kAccessibleRoleCheckBox)
        return @(info.currentValue > 0.5f ? 1 : 0);

    if (info.valueText[0] != '\0')
        return [NSString stringWithUTF8String:info.valueText];

    return @(info.currentValue);
}

- (NSString *)accessibilityValueDescription
{
    if (_accessible == nullptr)
        return nil;

    const char* const vt = _accessible->getAccessibleInfo().valueText;
    if (vt != nullptr && vt[0] != '\0')
        return [NSString stringWithUTF8String:vt];

    return nil;
}

- (id)accessibilityMinValue
{
    if (_accessible == nullptr)
        return nil;
    return @(_accessible->getAccessibleInfo().minValue);
}

- (id)accessibilityMaxValue
{
    if (_accessible == nullptr)
        return nil;
    return @(_accessible->getAccessibleInfo().maxValue);
}

- (NSRect)accessibilityFrame
{
    if (_widget == nullptr || _parentView == nullptr)
        return NSZeroRect;

    // View is flipped (y=0 at top)
    const NSRect viewRect = NSMakeRect(_widget->getAbsoluteX(),
                                       _widget->getAbsoluteY(),
                                       _widget->getWidth(),
                                       _widget->getHeight());

    if (_parentView.window != nil)
    {
        const NSRect winRect = [_parentView convertRect:viewRect toView:nil];
        return [_parentView.window convertRectToScreen:winRect];
    }

    return [_parentView convertRect:viewRect toView:nil];
}

- (id)accessibilityParent
{
    return _parentView;
}

- (id)accessibilityFocusedUIElement
{
    return self;
}

- (BOOL)accessibilityPerformPress
{
    if (_accessible != nullptr)
        return _accessible->performAccessibleAction(DGL_NAMESPACE::kAccessibleActionPress) ? YES : NO;
    return NO;
}

- (BOOL)accessibilityPerformIncrement
{
    if (_accessible != nullptr)
        return _accessible->performAccessibleAction(DGL_NAMESPACE::kAccessibleActionIncrement) ? YES : NO;
    return NO;
}

- (BOOL)accessibilityPerformDecrement
{
    if (_accessible != nullptr)
        return _accessible->performAccessibleAction(DGL_NAMESPACE::kAccessibleActionDecrement) ? YES : NO;
    return NO;
}

@end

// --------------------------------------------------------------------------------------------------------------------
// PuglWrapperView Accessibility Category

static void collectSubWidgetsRecursive(DGL_NAMESPACE::Widget* const parent,
                                       std::vector<DGL_NAMESPACE::SubWidget*>& outWidgets)
{
    if (parent == nullptr)
        return;

    const std::list<DGL_NAMESPACE::SubWidget*> children = parent->getChildren();
    for (DGL_NAMESPACE::SubWidget* const sub : children)
    {
        if (sub != nullptr && sub->isVisible())
        {
            if (sub->getAccessibleElement() != nullptr && sub->getAccessibleElement()->isAccessible())
            {
                outWidgets.push_back(sub);
            }
            collectSubWidgetsRecursive(sub, outWidgets);
        }
    }
}

@interface PuglWrapperView (Accessibility)
@end

@implementation PuglWrapperView (Accessibility)

- (BOOL)isAccessibilityElement
{
    return NO;
}

- (NSArray *)accessibilityChildren
{
    if (puglview == nullptr || puglview->handle == nullptr)
        return [NSArray array];

    DGL_NAMESPACE::Window::PrivateData* const winData =
        static_cast<DGL_NAMESPACE::Window::PrivateData*>(puglGetHandle(puglview));

    if (winData == nullptr)
        return [NSArray array];

    std::vector<DGL_NAMESPACE::SubWidget*> accessibleWidgets;
    for (DGL_NAMESPACE::TopLevelWidget* const tlw : winData->topLevelWidgets)
    {
        if (tlw != nullptr && tlw->isVisible())
            collectSubWidgetsRecursive(tlw, accessibleWidgets);
    }

    NSMutableArray* const elements = [NSMutableArray arrayWithCapacity:accessibleWidgets.size()];
    for (DGL_NAMESPACE::SubWidget* const w : accessibleWidgets)
    {
        DGLElementAccessibility* const elem =
            [[DGLElementAccessibility alloc] initWithWidget:w
                                                 accessible:w->getAccessibleElement()
                                                 parentView:self];
        [elements addObject:elem];
    }

    return elements;
}

- (id)accessibilityHitTest:(NSPoint)screenPoint
{
    NSPoint viewPoint = screenPoint;
    if (self.window != nil)
    {
        const NSPoint windowPoint = [self.window convertPointFromScreen:screenPoint];
        viewPoint = [self convertPoint:windowPoint fromView:nil];
    }
    else
    {
        viewPoint = [self convertPoint:screenPoint fromView:nil];
    }

    NSArray* const children = [self accessibilityChildren];
    // Check in reverse order (top-most first)
    for (NSInteger i = (NSInteger)children.count - 1; i >= 0; --i)
    {
        DGLElementAccessibility* const elem = [children objectAtIndex:i];
        if (elem.widget != nullptr && elem.widget->isVisible())
        {
            if (elem.widget->getAbsoluteArea().contains((int)viewPoint.x, (int)viewPoint.y))
                return elem;
        }
    }

    return self;
}

@end
