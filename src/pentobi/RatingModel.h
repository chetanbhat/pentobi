//-----------------------------------------------------------------------------
/** @file pentobi_qml/RatingModel.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef PENTOBI_QML_RATING_MODEL_H
#define PENTOBI_QML_PLAYER_MODEL_H

// Needed in the header because moc_*.cpp does not include config.h
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QObject>
#include "libboardgame_base/Rating.h"

class GameModel;

using libboardgame_base::Rating;

//-----------------------------------------------------------------------------

class RatedGameInfo
    : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int number READ number CONSTANT)

    /** Color played by the human.
        In game variants with multiple colors per player, the human played
        all colors played by the player of this color. */
    Q_PROPERTY(int color READ color CONSTANT)

    /** Game result.
        0=Loss, 0.5=tie, 1=win from the viewpoint of the human. */
    Q_PROPERTY(double result READ result CONSTANT)

    /** Date of the game in "YYYY-MM-DD" format. */
    Q_PROPERTY(QString date READ date CONSTANT)

    /** The playing level of the computer opponent. */
    Q_PROPERTY(int level READ level CONSTANT)

    /** The rating of the human after the game. */
    Q_PROPERTY(double rating READ rating CONSTANT)

    /** The SGF game tree. */
    Q_PROPERTY(QByteArray sgf READ sgf CONSTANT)

public:
    RatedGameInfo(QObject* parent, int number, int color, double result,
                  const QString& date, int level, double rating,
                  const QByteArray& sgf);

    int number() const { return m_number; }

    int color() const { return m_color; }

    double result() const { return m_result; }

    const QString& date() const { return m_date; }

    int level() const { return m_level; }

    double rating() const { return m_rating; }

    const QByteArray& sgf() const { return m_sgf; }

private:
    int m_number;

    int m_color;

    int m_level;

    double m_result;

    double m_rating;

    QString m_date;

    QByteArray m_sgf;
};

//-----------------------------------------------------------------------------

class RatingModel
    : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double bestRating READ bestRating NOTIFY bestRatingChanged)
    Q_PROPERTY(QString gameVariant MEMBER m_gameVariant WRITE setGameVariant NOTIFY gameVariantChanged)
    Q_PROPERTY(QList<QObject*> history READ history NOTIFY historyChanged)
    Q_PROPERTY(int numberGames READ numberGames NOTIFY numberGamesChanged)
    Q_PROPERTY(double rating READ rating NOTIFY ratingChanged)

public:
    explicit RatingModel(QObject* parent = nullptr);


    Q_INVOKABLE void addResult(GameModel* gameModel, int level);

    Q_INVOKABLE void clearRating();

    Q_INVOKABLE int getNextHumanPlayer() const;

    Q_INVOKABLE int getNextLevel(int maxLevel) const;

    Q_INVOKABLE void setInitialRating(double rating);


    double bestRating() const { return m_bestRating.get(); }

    const QList<QObject*>& history() const { return m_history; }

    int numberGames() const { return m_numberGames; }

    double rating() const { return m_rating.get(); }

    void setGameVariant(const QString& gameVariant);

signals:
    void bestRatingChanged();

    void gameVariantChanged();

    void historyChanged();

    void numberGamesChanged();

    void ratingChanged();

private:
    int m_numberGames = 0;

    Rating m_bestRating = Rating(1000.);

    Rating m_rating = Rating(1000.);

    QString m_gameVariant;

    QList<QObject*> m_history;

    void saveSettings() const;

    void setBestRating(double rating);

    void setRating(double rating);

    void setNumberGames(int numberGames);
};

//-----------------------------------------------------------------------------

#endif // PENTOBI_QML_PLAYER_MODEL_H
