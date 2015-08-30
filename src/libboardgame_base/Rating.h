//-----------------------------------------------------------------------------
/** @file libboardgame_base/Rating.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBBOARDGAME_BASE_RATING_H
#define LIBBOARDGAME_BASE_RATING_H

#include <climits>
#include <cmath>
#include <iosfwd>

namespace libboardgame_base {

using namespace std;

//-----------------------------------------------------------------------------

/** Elo-rating of a player. */
class Rating
{
public:
    friend ostream& operator<<(ostream& out, const Rating& rating);
    friend istream& operator>>(istream& in, Rating& rating);

    explicit Rating(float elo = 0);

    /** Get the expected outcome of a game.
        @param elo_opponent Elo-rating of the opponent.
        @param nu_opponents The number of opponents (all with the same rating
        elo_opponent) */
    float get_expected_result(Rating elo_opponent,
                              unsigned nu_opponents = 1) const;

    /** Update a rating after a game.
        @param game_result The outcome of the game (0=loss, 0.5=tie, 1=win)
        @param elo_opponent Elo-rating of the opponent.
        @param k_value The K-value
        @param nu_opponents The number of opponents (all with the same rating
        elo_opponent) */
    void update(float game_result, Rating elo_opponent, float k_value = 32,
                unsigned nu_opponents = 1);

    float get() const;

    /** Get rating rounded to an integer. */
    int to_int() const;

private:
    float m_elo;
};

inline Rating::Rating(float elo)
  : m_elo(elo)
{
}

inline float Rating::get() const
{
    return m_elo;
}

inline int Rating::to_int() const
{
    return static_cast<int>(round(m_elo));
}

//-----------------------------------------------------------------------------

} // namespace libboardgame_base

#endif // LIBBOARDGAME_BASE_RATING_H
