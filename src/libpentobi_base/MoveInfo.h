//-----------------------------------------------------------------------------
/** @file libpentobi_base/MoveInfo.h */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_BASE_MOVE_INFO_H
#define LIBPENTOBI_BASE_MOVE_INFO_H

#include "MovePoints.h"
#include "Piece.h"
#include "PieceInfo.h"

namespace libpentobi_base {

using namespace std;

//-----------------------------------------------------------------------------

struct MoveInfo
{
    Piece piece;

    MovePoints points;

    ArrayList<Point,PieceInfo::max_adj> adj_points;

    ArrayList<Point,PieceInfo::max_attach> attach_points;
};

/** Non-frequently accessed move info.
    Stored separately from MoveInfo to improve CPU cache performance. */
struct MoveInfoExt
{
    bool breaks_symmetry;

    Move symmetric_move;

    Point center;
};

//-----------------------------------------------------------------------------

} // namespace libpentobi_base

//-----------------------------------------------------------------------------

#endif // LIBPENTOBI_BASE_MOVE_INFO_H
