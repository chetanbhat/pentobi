function about() {
    var url = "http://pentobi.sourceforge.net"
    // Don't use &copy; or &ndash;, this is currently not rendered in the label
    // of the message box (last tested with Qt 5.8-rc)
    showInfo("<h2>" + qsTr("Pentobi") + "</h2><br/>" +
             qsTr("Version %1").arg(Qt.application.version) + "<br/><br/>" +
             qsTr("Computer opponent for the board game Blokus.") + "<br/>" +
             qsTr("Copyright 2011-%1 Markus&nbsp;Enzenberger").arg(2017) +
             "<br><a href=\"" + url + "\">" + url + "</a></p>")
}

function analyzeGame() {
    if (! gameModel.isMainVar) {
        showInfo(qsTr("Game analysis is only possible in main variation."))
        return
    }
    gameDisplay.showAnalyzeGame()
    Logic.cancelRunning()
    analyzeGameModel.start(gameModel, playerModel)
}

function autoSave() {
    wasGenMoveRunning =
            playerModel.isGenMoveRunning && ! isPlaySingleMoveRunning
    gameModel.autoSave()
    analyzeGameModel.autoSave(gameModel)
}

function cancelRunning() {
    if (analyzeGameModel.isRunning) {
        analyzeGameModel.cancel()
        showTemporaryMessage(qsTr("Game analysis aborted."))
    }
    if (playerModel.isGenMoveRunning) {
        playerModel.cancelGenMove()
        showTemporaryMessage(qsTr("Computer move aborted."))
    }
    delayedCheckComputerMove.stop()
}

function changeGameVariant(gameVariant) {
    if (gameModel.gameVariant === gameVariant)
        return
    verify(function() { changeGameVariantNoVerify(gameVariant) })
}

function changeGameVariantNoVerify(gameVariant) {
    cancelRunning()
    lengthyCommand.run(function() {
        gameDisplay.changeGameVariant(gameVariant)
        isRated = false
        analyzeGameModel.clear()
        initComputerColors()
    })
}

function checkComputerMove() {
    if (gameModel.isGameOver) {
        var msg = gameModel.getResultMessage()
        if (isRated) {
            var oldRating = Math.round(ratingModel.rating)
            ratingModel.addResult(gameModel, getLevel())
            var newRating = Math.round(ratingModel.rating)
            msg += "\n"
            if (newRating > oldRating)
                msg += qsTr("Your rating has increased from %1 to %2.").arg(oldRating).arg(newRating)
            else if (newRating < oldRating)
                msg += qsTr("Your rating has decreased from %1 to %2.").arg(oldRating).arg(newRating)
            else
                msg += qsTr("Your rating stays at %1.").arg(newRating)
            isRated = false
        }
        showInfo(msg);
        return
    }
    if (! isComputerToPlay())
        return
    initComputerColorsOnNewGame = true
    switch (gameModel.toPlay) {
    case 0: if (! gameModel.hasMoves0) return; break
    case 1: if (! gameModel.hasMoves1) return; break
    case 2: if (! gameModel.hasMoves2) return; break
    case 3: if (! gameModel.hasMoves3) return; break
    }
    genMove();
}

function clearRating() {
    showQuestion(qsTr("Delete all rating information for the current game variant?"),
                 clearRatingNoVerify)
}

function clearRatingNoVerify() {
    ratingModel.clearRating()
    showTemporaryMessage(qsTr("Rating information deleted."))
}

/** If the computer already plays the current color to play, start generating
    a move; if he doesn't, make him play the current color (and only the
    current color). */
function computerPlay() {
    if (playerModel.isGenMoveRunning)
        return
    if (! isComputerToPlay()) {
        computerPlays0 = false
        computerPlays1 = false
        computerPlays2 = false
        computerPlays3 = false
        var variant = gameModel.gameVariant
        if (variant == "classic_3" && gameModel.toPlay == 3) {
            switch (gameModel.altPlayer) {
            case 0: computerPlays0 = true; break
            case 1: computerPlays1 = true; break
            case 2: computerPlays2 = true; break
            }
        }
        else
        {
            switch (gameModel.toPlay) {
            case 0:
                computerPlays0 = true
                if (isMultiColor()) computerPlays2 = true
                break;
            case 1:
                computerPlays1 = true
                if (isMultiColor()) computerPlays3 = true
                break;
            case 2:
                computerPlays2 = true
                if (isMultiColor()) computerPlays0 = true
                break;
            case 3:
                computerPlays3 = true
                if (isMultiColor()) computerPlays1 = true
                break;
            }
        }
    }
    checkComputerMove()
}

function computerPlays(color) {
    switch (color) {
    case 0: return computerPlays0
    case 1: return computerPlays1
    case 2: return computerPlays2
    case 3: return computerPlays3
    }
}

function computerPlaysAny() {
    if (computerPlays0) return true
    if (computerPlays1) return true
    if (gameModel.nuColors >= 2 && computerPlays2) return true
    if (gameModel.nuColors >= 3 && computerPlays3) return true
    return false
}

function createTheme(themeName) {
    var source = "themes/" + themeName + "/Theme.qml"
    return Qt.createComponent(source).createObject(root)
}

function deleteAllVar() {
    showQuestion(qsTr("Delete all variations?"), deleteAllVarNoVerify)
}

function deleteAllVarNoVerify() {
    gameModel.deleteAllVar()
    showTemporaryMessage(qsTr("Variations deleted."))
}

function exportAsciiArt(fileUrl) {
    if (! gameModel.saveAsciiArt(getFileFromUrl(fileUrl)))
        showInfo(qsTr("Save failed.") + "\n" + gameModel.lastInputOutputError)
    else
        showTemporaryMessage(qsTr("File saved."))
}

function exportImage(fileUrl) {
    if (! gameDisplay.grabBoardToImage(function(result) {
        if (! result.saveToFile(getFileFromUrl(fileUrl)))
            showInfo(qsTr("Saving image failed."))
        else
            showTemporaryMessage(qsTr("Image saved."))
    }, exportImageWidth))
        showInfo(qsTr("Creating image failed."))
}

function findMove() {
    gameDisplay.showMove(gameModel.findMove())
}

function findNextComment() {
    if (gameModel.findNextComment()) {
        gameDisplay.showComment()
        return
    }
    if (gameModel.canGoBackward)
        // Current is not root
        showQuestion(qsTr("End of tree was reached. Continue search from start of the tree?"),
                     findNextCommentContinueFromRoot)
    else
        showInfo(qsTr("No comment found"))

}

function findNextCommentContinueFromRoot() {
    if (gameModel.findNextCommentContinueFromRoot()) {
        gameDisplay.showComment()
        return
    }
    showInfo(qsTr("No comment found"))
}

function genMove() {
    Logic.cancelRunning()
    gameDisplay.pickedPiece = null
    playerModel.startGenMove(gameModel)
}

function getFileFromUrl(fileUrl) {
    var file = fileUrl.toString()
    file = file.replace(/^(file:\/{3})/,"/")
    return decodeURIComponent(file)
}

function getFileLabel(file, isModified) {
    if (file === "")
        return ""
    var pos = Math.max(file.lastIndexOf("/"), file.lastIndexOf("\\"));
    return (isModified ? "*" : "") + file.substring(pos + 1)
}

function getLevel() {
    switch (gameModel.gameVariant) {
    case "classic_2": return playerModel.levelClassic2
    case "classic_3": return playerModel.levelClassic3
    case "duo": return playerModel.levelDuo
    case "trigon": return playerModel.levelTrigon
    case "trigon_2": return playerModel.levelTrigon2
    case "trigon_3": return playerModel.levelTrigon3
    case "junior": return playerModel.levelJunior
    case "nexos": return playerModel.levelNexos
    case "nexos_2": return playerModel.levelNexos2
    case "callisto": return playerModel.levelCallisto
    case "callisto_2": return playerModel.levelCallisto2
    case "callisto_3": return playerModel.levelCallisto3
    default: return playerModel.levelClassic
    }
}

function help() {
    if (helpWindowLoader.status === Loader.Null)
        helpWindowLoader.source = "HelpWindow.qml"
    helpWindowLoader.item.show()
}

function init() {
    // Settings might contain unusable geometry
    var maxWidth = root.Screen.desktopAvailableWidth
    var maxHeight = root.Screen.desktopAvailableHeight
    if (x < 0 || x + width > maxWidth || y < 0 || y + height > maxHeight) {
        if (width > maxWidth || height > maxHeight) {
            width = defaultWidth
            height = defaultHeight
        }
        x = (root.Screen.width - width) / 2
        y = (root.Screen.height - height) / 2
    }
    if (! gameModel.loadAutoSave()) {
        gameDisplay.createPieces()
        initComputerColors()
        return
    }
    gameDisplay.createPieces()
    if (gameModel.checkFileModifiedOutside())
    {
        showQuestion(qsTr("File has been modified by another application. Reload?"), reloadFile)
        return
    }
    analyzeGameModel.loadAutoSave(gameModel)
    if (wasGenMoveRunning || (isRated && isComputerToPlay()))
        checkComputerMove()
}

function initComputerColors() {
    if (! initComputerColorsOnNewGame)
        return
    // Default setting is that the computer plays all colors but the first
    computerPlays0 = false
    computerPlays1 = true
    computerPlays2 = true
    computerPlays3 = true
    if (isMultiColor())
        computerPlays2 = false
}

function isComputerToPlay() {
    if (gameModel.gameVariant == "classic_3" && gameModel.toPlay == 3)
        return computerPlays(gameModel.altPlayer)
    return computerPlays(gameModel.toPlay)
}

function isMultiColor() {
    var gameVariant = gameModel.gameVariant
    return (gameVariant == "classic_2" || gameVariant == "trigon_2"
            || gameVariant == "nexos_2")

}

function keepOnlyPosition() {
    showQuestion(qsTr("Keep only position?"), keepOnlyPositionNoVerify)
}

function keepOnlyPositionNoVerify() {
    gameModel.keepOnlyPosition()
    showTemporaryMessage(qsTr("Kept only position."))
}

function keepOnlySubtree() {
    showQuestion(qsTr("Keep only subtree?"), keepOnlySubtreeNoVerify)
}

function keepOnlySubtreeNoVerify() {
    gameModel.keepOnlySubtree()
    showTemporaryMessage(qsTr("Kept only subtree."))
}

function moveDownVar() {
    gameModel.moveDownVar()
    showVariationInfo()
}

function moveGenerated(move) {
    gameModel.playMove(move)
    if (isPlaySingleMoveRunning)
        isPlaySingleMoveRunning = false
    else
        delayedCheckComputerMove.restart()
}

function moveUpVar() {
    gameModel.moveUpVar()
    showVariationInfo()
}

function newGame()
{
    verify(newGameNoVerify)
}

function newGameNoVerify()
{
    gameDisplay.newGame()
    isRated = false
    analyzeGameModel.clear()
    initComputerColors()
}

function open() {
    verify(openNoVerify)
}

function openNoVerify() {
    openDialog.open()
}

function openRatedGame(byteArray) {
    verify(function() { openRatedGameNoVerify(byteArray) })
}

function openRatedGameNoVerify(byteArray) {
    lengthyCommand.run(function() {
        var oldGameVariant = gameModel.gameVariant
        var oldEnableAnimations = gameDisplay.enableAnimations
        gameDisplay.enableAnimations = false
        if (! gameModel.openByteArray(byteArray))
            showInfo(qsTr("Open failed.") + "\n" + gameModel.lastInputOutputError)
        computerPlays0 = false
        computerPlays1 = false
        computerPlays2 = false
        computerPlays3 = false
        if (gameModel.gameVariant != oldGameVariant) {
            gameDisplay.destroyPieces()
            gameDisplay.createPieces()
        }
        gameDisplay.showToPlay()
        gameDisplay.setupMode = false
        isRated = false
        analyzeGameModel.clear()
        gameDisplay.showToPlay()
        gameDisplay.enableAnimations = oldEnableAnimations
        // See comment in RatingDialog onTriggered of the menu item
        ratingDialog.item.close()
    })
}

function openFile(file) {
    lengthyCommand.run(function() {
        var oldGameVariant = gameModel.gameVariant
        var oldEnableAnimations = gameDisplay.enableAnimations
        gameDisplay.enableAnimations = false
        if (! gameModel.openFile(file))
            showInfo(qsTr("Open failed.") + "\n" + gameModel.lastInputOutputError)
        else {
            computerPlays0 = false
            computerPlays1 = false
            computerPlays2 = false
            computerPlays3 = false
        }
        if (gameModel.gameVariant != oldGameVariant) {
            gameDisplay.destroyPieces()
            gameDisplay.createPieces()
        }
        gameDisplay.showToPlay()
        gameDisplay.enableAnimations = oldEnableAnimations
        gameDisplay.setupMode = false
        isRated = false
        analyzeGameModel.clear()
        if (gameModel.comment.length > 0)
            gameDisplay.showComment()
    })
}

function openFileUrl() {
    openFile(getFileFromUrl(openDialog.item.fileUrl))
}

function openClipboard() {
    lengthyCommand.run(function() {
        var oldGameVariant = gameModel.gameVariant
        var oldEnableAnimations = gameDisplay.enableAnimations
        gameDisplay.enableAnimations = false
        if (! gameModel.openClipboard())
            showInfo(qsTr("Open failed.") + "\n" + gameModel.lastInputOutputError)
        else {
            computerPlays0 = false
            computerPlays1 = false
            computerPlays2 = false
            computerPlays3 = false
        }
        if (gameModel.gameVariant != oldGameVariant) {
            gameDisplay.destroyPieces()
            gameDisplay.createPieces()
        }
        gameDisplay.showToPlay()
        gameDisplay.enableAnimations = oldEnableAnimations
        gameDisplay.setupMode = false
        isRated = false
        analyzeGameModel.clear()
    })
}

function openGameInfoDialog() {
    gameInfoDialog.open()
    var dialog = gameInfoDialog.item
    dialog.playerName0 = gameModel.playerName0
    dialog.playerName1 = gameModel.playerName1
    dialog.playerName2 = gameModel.playerName2
    dialog.playerName3 = gameModel.playerName3
    dialog.date = gameModel.date
    dialog.time = gameModel.time
    dialog.event = gameModel.event
    dialog.round = gameModel.round
}

function openRecentFile(file) {
    verify(function() { openFile(file) })
}

function play(pieceModel, gameCoord) {
    var wasComputerToPlay = isComputerToPlay()
    gameModel.playPiece(pieceModel, gameCoord)
    // We don't continue automatic play if the human played a move for a color
    // played by the computer.
    if (! wasComputerToPlay)
        delayedCheckComputerMove.restart()
}

function ratedGame()
{
    verify(ratedGameCheckFirstGame)
}

function ratedGameCheckFirstGame() {
    if (ratingModel.numberGames === 0)
        initialRatingDialog.open()
    else
        ratedGameNoVerify()
}

function ratedGameNoVerify()
{
    var player = ratingModel.getNextHumanPlayer()
    var level = ratingModel.getNextLevel(maxLevel)
    var gameVariant = gameModel.gameVariant
    var msg
    switch (player) {
    case 0:
        if (isMultiColor())
            msg = qsTr("Start rated game with Blue/Red against Pentobi level %1?").arg(level)
        else
            msg = qsTr("Start rated game with Blue against Pentobi level %1?").arg(level)
        break
    case 1:
        if (isMultiColor())
            msg = qsTr("Start rated game with Yellow/Green against Pentobi level %1?").arg(level)
        else if (gameModel.nuColors === 2)
            msg = qsTr("Start rated game with Green against Pentobi level %1?").arg(level)
        else
            msg = qsTr("Start rated game with Yellow against Pentobi level %1?").arg(level)
        break
    case 2:
        msg = qsTr("Start rated game with Red against Pentobi level %1?").arg(level)
        break
    case 3:
        msg = qsTr("Start rated game with Green against Pentobi level %1?").arg(level)
        break
    }
    showQuestion(msg, ratedGameStart)
}

function ratedGameStart() {
    var player = ratingModel.getNextHumanPlayer()
    computerPlays0 = (player !== 0)
    computerPlays1 = (player !== 1)
    computerPlays2 = (player !== 2)
    computerPlays3 = (player !== 3)
    if (isMultiColor()) {
        computerPlays2 = computerPlays0
        computerPlays3 = computerPlays1
    }
    setLevel(ratingModel.getNextLevel(maxLevel))
    gameDisplay.newGame()
    gameDisplay.showPieces()
    isRated = true
    analyzeGameModel.clear()
    checkComputerMove()
}

function reloadFile() {
    openFile(gameModel.file)
}

function save() {
    if (gameModel.checkFileModifiedOutside())
        showQuestion(qsTr("File has been modified by another application. Save anyway?"),
                     saveCurrentFile)
    else
        saveCurrentFile()
}

function saveAs() {
    saveDialog.open()
}

function saveCurrentFile() {
    saveFile(gameModel.file)
}

function saveFile(file) {
    if (! gameModel.save(file))
        showInfo(qsTr("Save failed.") + "\n" + gameModel.lastInputOutputError)
    else
        showTemporaryMessage(qsTr("File saved."))
}

function saveFileUrl(fileUrl) {
    saveFile(getFileFromUrl(fileUrl))
}

function setLevel(level) {
    switch (gameModel.gameVariant) {
    case "classic_2": playerModel.levelClassic2 = level; break
    case "classic_3": playerModel.levelClassic3 = level; break
    case "duo": playerModel.levelDuo = level; break
    case "trigon": playerModel.levelTrigon = level; break
    case "trigon_2": playerModel.levelTrigon2 = level; break
    case "trigon_3": playerModel.levelTrigon3 = level; break
    case "junior": playerModel.levelJunior = level; break
    case "nexos": playerModel.levelNexos = level; break
    case "nexos_2": playerModel.levelNexos2 = level; break
    case "callisto": playerModel.levelCallisto = level; break
    case "callisto_2": playerModel.levelCallisto2 = level; break
    case "callisto_3": playerModel.levelCallisto3 = level; break
    default: playerModel.levelClassic = level
    }
}

function showComputerColorDialog() {
    if (computerColorDialogLoader.status === Loader.Null)
        computerColorDialogLoader.sourceComponent =
                computerColorDialogComponent
    var dialog = computerColorDialogLoader.item
    dialog.computerPlays0 = computerPlays0
    dialog.computerPlays1 = computerPlays1
    dialog.computerPlays2 = computerPlays2
    dialog.computerPlays3 = computerPlays3
    dialog.open()
}

function showInfo(text) {
    if (infoMessageLoader.status === Loader.Null)
        infoMessageLoader.sourceComponent = infoMessageComponent
    var dialog = infoMessageLoader.item
    dialog.text = text
    dialog.open()
}

function showQuestion(text, acceptedFunc) {
    if (questionMessageLoader.status === Loader.Null)
        questionMessageLoader.sourceComponent = questionMessageComponent
    questionMessageLoader.item.openWithCallback(text, acceptedFunc)
}

function showTemporaryMessage(text) {
    gameDisplay.showTemporaryMessage(text)
}

function showVariationInfo() {
    showTemporaryMessage(qsTr("Variation is now %1.").arg(gameModel.getVariationInfo()))
}

function truncate() {
    showQuestion(qsTr("Truncate this subtree?"), gameModel.truncate)
}

function truncateChildren() {
    showQuestion(qsTr("Truncate children?"), truncateChildrenNoVerify)
}

function truncateChildrenNoVerify() {
    gameModel.truncateChildren()
    showTemporaryMessage(qsTr("Children truncated."))
}

function undo() {
    gameModel.undo()
}

function verify(callback)
{
    if (gameModel.isModified) {
        showQuestion(qsTr("Discard game?"), callback)
        return
    }
    callback()
}
