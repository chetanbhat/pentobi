import QtQuick 2.0
import QtQuick.Controls 1.1
import "Main.js" as Logic

Menu {
    title: qsTr("G&o")
    visible: ! isAndroid || backToMainVar.enabled

    MenuItem {
        id: backToMainVar

        text: qsTr("Back to &Main Variation")
        enabled: ! gameModel.isMainVar
        visible: ! isAndroid || enabled
        onTriggered: gameModel.backToMainVar()
    }
}