TEMPLATE = subdirs

contains(QT_CONFIG, opengl(es1|es2)?) {
    SUBDIRS =   quick-accessibility \
                animation \
                draganddrop \
                externaldraganddrop \
                canvas \
                imageelements \
                keyinteraction \
                localstorage \
                models \
                views \
                mousearea \
                positioners \
                righttoleft \
                scenegraph \
                shadereffects \
                text \
                threading \
                touchinteraction \
                tutorials \
                customitems \
                imageprovider \
                imageresponseprovider \
                window \
                particles \
                demos \
                rendercontrol

    # Widget dependent examples
    qtHaveModule(widgets) {
        SUBDIRS += embeddedinwidgets
        qtHaveModule(quickwidgets): SUBDIRS += quickwidgets
    }

    EXAMPLE_FILES = \
        shared
}
