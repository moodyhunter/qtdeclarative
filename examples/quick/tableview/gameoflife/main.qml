// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml 2.12
import QtQuick 2.12
import QtQuick.Window 2.3
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import GameOfLifeModel 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 760
    height: 810
    minimumWidth: 475
    minimumHeight: 300

    color: "#09102B"
    title: qsTr("Conway’s Game of Life")

    //! [tableview]
    TableView {
        id: tableView
        anchors.fill: parent

        rowSpacing: 1
        columnSpacing: 1

        ScrollBar.horizontal: ScrollBar {}
        ScrollBar.vertical: ScrollBar {}

        delegate: Rectangle {
            id: cell
            implicitWidth: 15
            implicitHeight: 15

            required property var model
            required property bool value

            color: value ? "#f3f3f4" : "#b5b7bf"

            MouseArea {
                anchors.fill: parent
                onClicked: parent.model.value = !parent.value
            }
        }
        //! [tableview]

        //! [model]
        model: GameOfLifeModel {
            id: gameOfLifeModel
        }
        //! [model]

        //! [scroll]
        contentX: (contentWidth - width) / 2;
        contentY: (contentHeight - height) / 2;
        //! [scroll]
    }

    footer: Rectangle {
        signal nextStep

        id: footer
        height: 50
        color: "#F3F3F4"

        RowLayout {
            anchors.centerIn: parent

            //! [next]
            Button {
                text: qsTr("Next")
                onClicked: gameOfLifeModel.nextStep()
            }
            //! [next]

            Item {
                width: 50
            }

            Slider {
                id: slider
                x: 245
                y: 17
                from: 0
                to: 1
                value: 0.9
            }

            Button {
                text: timer.running ? "❙❙" : "▶️"
                onClicked: timer.running = !timer.running
            }
        }

        Timer {
            id: timer
            interval: 1000 - (980 * slider.value)
            running: true
            repeat: true

            onTriggered: gameOfLifeModel.nextStep()
        }
    }

    Component.onCompleted: {
        gameOfLifeModel.loadFile(":/gosperglidergun.cells");
    }
}
