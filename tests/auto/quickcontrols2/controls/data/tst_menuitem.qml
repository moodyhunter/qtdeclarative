// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "MenuItem"

    Component {
        id: menuItem
        MenuItem { }
    }

    Component {
        id: menu
        Menu { }
    }

    function test_baseline() {
        var control = createTemporaryObject(menuItem, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }

    function test_checkable() {
        var control = createTemporaryObject(menuItem, testCase)
        verify(control)
        verify(control.hasOwnProperty("checkable"))
        verify(!control.checkable)

        mouseClick(control)
        verify(!control.checked)

        control.checkable = true
        mouseClick(control)
        verify(control.checked)

        mouseClick(control)
        verify(!control.checked)
    }

    function test_highlighted() {
        var control = createTemporaryObject(menuItem, testCase)
        verify(control)
        verify(!control.highlighted)

        control.highlighted = true
        verify(control.highlighted)
    }

    function test_display_data() {
        return [
            { "tag": "IconOnly", display: MenuItem.IconOnly },
            { "tag": "TextOnly", display: MenuItem.TextOnly },
            { "tag": "TextUnderIcon", display: MenuItem.TextUnderIcon },
            { "tag": "TextBesideIcon", display: MenuItem.TextBesideIcon },
            { "tag": "IconOnly, mirrored", display: MenuItem.IconOnly, mirrored: true },
            { "tag": "TextOnly, mirrored", display: MenuItem.TextOnly, mirrored: true },
            { "tag": "TextUnderIcon, mirrored", display: MenuItem.TextUnderIcon, mirrored: true },
            { "tag": "TextBesideIcon, mirrored", display: MenuItem.TextBesideIcon, mirrored: true }
        ]
    }

    function test_display(data) {
        var control = createTemporaryObject(menuItem, testCase, {
            text: "MenuItem",
            display: data.display,
            "icon.source": "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png",
            "LayoutMirroring.enabled": !!data.mirrored
        })
        verify(control)
        compare(control.icon.source, "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png")

        var padding = data.mirrored ? control.contentItem.rightPadding : control.contentItem.leftPadding
        var iconImage = findChild(control.contentItem, "image")
        var textLabel = findChild(control.contentItem, "label")

        switch (control.display) {
        case MenuItem.IconOnly:
            verify(iconImage)
            verify(!textLabel)
            compare(iconImage.x, control.mirrored ? control.availableWidth - iconImage.width - padding : padding)
            compare(iconImage.y, (control.availableHeight - iconImage.height) / 2)
            break;
        case MenuItem.TextOnly:
            verify(!iconImage)
            verify(textLabel)
            compare(textLabel.x, control.mirrored ? control.availableWidth - textLabel.width - padding : padding)
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            break;
        case MenuItem.TextUnderIcon:
            verify(iconImage)
            verify(textLabel)
            compare(iconImage.x, control.mirrored ? control.availableWidth - iconImage.width - (textLabel.width - iconImage.width) / 2 - padding : (textLabel.width - iconImage.width) / 2 + padding)
            compare(textLabel.x, control.mirrored ? control.availableWidth - textLabel.width - padding : padding)
            verify(iconImage.y < textLabel.y)
            break;
        case MenuItem.TextBesideIcon:
            verify(iconImage)
            verify(textLabel)
            if (control.mirrored)
                verify(textLabel.x < iconImage.x)
            else
                verify(iconImage.x < textLabel.x)
            compare(iconImage.y, (control.availableHeight - iconImage.height) / 2)
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            break;
        }
    }

    function test_menu() {
        var control = createTemporaryObject(menu, testCase)
        verify(control)

        var item1 = createTemporaryObject(menuItem, testCase)
        verify(item1)
        compare(item1.menu, null)

        var item2 = createTemporaryObject(menuItem, testCase)
        verify(item2)
        compare(item2.menu, null)

        control.addItem(item1)
        compare(item1.menu, control)
        compare(item2.menu, null)

        control.insertItem(1, item2)
        compare(item1.menu, control)
        compare(item2.menu, control)

        control.removeItem(control.itemAt(1))
        compare(item1.menu, control)
        compare(item2.menu, null)

        control.removeItem(control.itemAt(0))
        compare(item1.menu, null)
        compare(item2.menu, null)
    }
}
