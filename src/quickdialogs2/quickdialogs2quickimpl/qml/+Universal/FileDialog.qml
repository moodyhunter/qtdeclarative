// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.Universal
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl
import QtQuick.Layouts
import QtQuick.Templates as T

import "." as DialogsImpl

FileDialogImpl {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitHeaderWidth,
                            implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    padding: 24
    verticalPadding: 18

    standardButtons: T.Dialog.Open | T.Dialog.Cancel

    FileDialogImpl.buttonBox: buttonBox
    FileDialogImpl.nameFiltersComboBox: nameFiltersComboBox
    FileDialogImpl.fileDialogListView: fileDialogListView
    FileDialogImpl.breadcrumbBar: breadcrumbBar

    background: Rectangle {
        implicitWidth: 600
        implicitHeight: 400
        color: control.Universal.chromeMediumLowColor
        border.color: control.Universal.chromeHighColor
        border.width: 1 // FlyoutBorderThemeThickness
    }

    header: ColumnLayout {
        spacing: 12

        Label {
            text: control.title
            elide: Label.ElideRight
            // TODO: QPlatformTheme::TitleBarFont
            font.pixelSize: 20

            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.topMargin: 18
            Layout.fillWidth: true
            Layout.preferredHeight: control.title.length > 0 ? implicitHeight : 0

            background: Rectangle {
                x: 1; y: 1 // // FlyoutBorderThemeThickness
                color: control.Universal.chromeMediumLowColor
                width: parent.width - 2
                height: parent.height - 1
            }
        }

        DialogsImpl.FolderBreadcrumbBar {
            id: breadcrumbBar
            dialog: control

            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.fillWidth: true
            Layout.maximumWidth: parent.width - 48
        }
    }

    contentItem: ListView {
        id: fileDialogListView
        objectName: "fileDialogListView"
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {}

        model: FolderListModel {
            folder: control.currentFolder
            nameFilters: control.selectedNameFilter.globs
            showDirsFirst: PlatformTheme.themeHint(PlatformTheme.ShowDirectoriesFirst)
            sortCaseSensitive: false
        }
        delegate: DialogsImpl.FileDialogDelegate {
            objectName: "fileDialogDelegate" + index
            width: ListView.view.width
            highlighted: ListView.isCurrentItem
            dialog: control
            fileDetailRowWidth: nameFiltersComboBox.width
        }
    }

    footer: RowLayout {
        id: rowLayout
        spacing: 24

        ComboBox {
            id: nameFiltersComboBox
            model: control.nameFilters

            Layout.leftMargin: 24
            Layout.fillWidth: true
            Layout.topMargin: 6
            Layout.bottomMargin: 24
        }

        DialogButtonBox {
            id: buttonBox
            standardButtons: control.standardButtons
            spacing: 12
            horizontalPadding: 0

            Layout.rightMargin: 24
        }
    }

    T.Overlay.modal: Rectangle {
        color: control.Universal.baseLowColor
    }

    T.Overlay.modeless: Rectangle {
        color: control.Universal.baseLowColor
    }
}
