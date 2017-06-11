//-----------------------------------------------------------------------------
/** @file libboardgame_sgf/MissingProperty.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBBOARDGAME_SGF_MISSING_PROPERTY_H
#define LIBBOARDGAME_SGF_MISSING_PROPERTY_H

#include "InvalidTree.h"

namespace libboardgame_sgf {

using namespace std;

//-----------------------------------------------------------------------------

class MissingProperty
    : public InvalidTree
{
public:
    explicit MissingProperty(const string& message);

    MissingProperty(const string& id, const string& message);
};

//-----------------------------------------------------------------------------

} // namespace libboardgame_sgf

#endif // LIBBOARDGAME_SGF_MISSING_PROPERTY_H