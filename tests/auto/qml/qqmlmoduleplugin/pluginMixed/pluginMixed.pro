TEMPLATE = lib
CONFIG += plugin
SOURCES = plugin.cpp
QT = core qml
DESTDIR = ../imports/com/nokia/AutoTestQmlMixedPluginType

QT += core-private gui-private qml-private

IMPORT_FILES = \
        Foo.qml \
        qmldir

include (../../../shared/imports.pri)
