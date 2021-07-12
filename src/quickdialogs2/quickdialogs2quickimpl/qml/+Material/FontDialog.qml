/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl
import QtQuick.Layouts
import QtQuick.Templates as T

FontDialogImpl {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    leftPadding: 24
    rightPadding: 24

    standardButtons: T.Dialog.Ok | T.Dialog.Cancel

    Material.elevation: 24

    FontDialogImpl.buttonBox: buttonBox
    FontDialogImpl.familyListView: content.familyListView
    FontDialogImpl.styleListView: content.styleListView
    FontDialogImpl.sizeListView: content.sizeListView
    FontDialogImpl.sampleEdit: content.sampleEdit
    FontDialogImpl.writingSystemComboBox: writingSystemComboBox
    FontDialogImpl.underlineCheckBox: content.underline
    FontDialogImpl.strikeoutCheckBox: content.strikeout
    FontDialogImpl.familyEdit: content.familyEdit
    FontDialogImpl.styleEdit: content.styleEdit
    FontDialogImpl.sizeEdit: content.sizeEdit

    background: Rectangle {
        implicitWidth: 600
        implicitHeight: 400
        radius: 2
        color: control.Material.dialogColor

        layer.enabled: control.Material.elevation > 0
        layer.effect: ElevationEffect {
            elevation: control.Material.elevation
        }
    }

    Overlay.modal: Rectangle {
        color: Color.transparent(control.palette.shadow, 0.5)
    }

    Overlay.modeless: Rectangle {
        color: Color.transparent(control.palette.shadow, 0.12)
    }

    header: Label {
        text: control.title
        visible: control.title.length > 0
        elide: Label.ElideRight
        font.bold: true
        font.pixelSize: 16

        leftPadding: 24
        rightPadding: 24
        topPadding: 24
        bottomPadding: 24
    }

    contentItem: FontDialogContent {
        id: content
    }

    footer: RowLayout {
        id: rowLayout
        spacing: 20

        Label {
            text: qsTr("Writing System")

            Layout.leftMargin: 20
        }
        ComboBox{
            id: writingSystemComboBox

            Layout.fillWidth: true
        }

        DialogButtonBox {
            id: buttonBox
            standardButtons: control.standardButtons
            spacing: 12
            horizontalPadding: 0
            verticalPadding: 20

            Layout.rightMargin: 20
        }
    }
}
