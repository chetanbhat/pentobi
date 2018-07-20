//-----------------------------------------------------------------------------
/** @file pentobi_qml/HelpFileExtractor.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef PENTOBI_QML_HELP_FILE_EXTRACTOR_H
#define PENTOBI_QML_HELP_FILE_EXTRACTOR_H

// Needed in the header because moc_*.cpp does not include config.h
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QObject>
#include <QUrl>

//-----------------------------------------------------------------------------

/** Extracts help files to a temporary directory because WebView on Android
    cannot open qrc:// URLs. */
class HelpFileExtractor
    : public QObject
{
    Q_OBJECT

public:
    explicit HelpFileExtractor(QObject* parent = nullptr);

    Q_INVOKABLE QUrl extract(const QString& language);
};

//-----------------------------------------------------------------------------

#endif // PENTOBI_QML_HELP_FILE_EXTRACTOR_H