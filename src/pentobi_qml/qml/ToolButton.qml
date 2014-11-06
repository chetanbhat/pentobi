import QtQuick 2.0
import QtQuick.Controls 1.1

/** ToolButton with a dynamically sized image.
    Works around missing support for multi-size icons in QtQuick.Controls 1.1.
    The image source should be a SVG file with size 22x22. */
ToolButton {
    id: root

    property string imageSource

    Image {
        // Use scale factor 1 or 2 unless image is very small or very large
        height: root.height < 26 || root.height > 100 ?
                    0.7 * root.height : root.height < 50 ? 22 : 44
        width: height
        sourceSize.height: height
        sourceSize.width: width
        anchors.centerIn: parent
        source: imageSource
    }
}
