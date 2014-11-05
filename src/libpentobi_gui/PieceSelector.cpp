//-----------------------------------------------------------------------------
/** @file libpentobi_gui/PieceSelector.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "PieceSelector.h"

#include <algorithm>
#include <QMouseEvent>
#include <QPainter>
#include "libboardgame_base/GeometryUtil.h"
#include "libboardgame_util/StringUtil.h"
#include "libpentobi_gui/Util.h"

using namespace std;
using libboardgame_base::CoordPoint;
using libboardgame_base::geometry_util::type_match_shift;
using libboardgame_util::log;
using libboardgame_util::trim;
using libpentobi_base::BoardConst;
using libpentobi_base::BoardType;
using libpentobi_base::Variant;
using libpentobi_base::Geometry;
using libpentobi_base::PieceMap;

//-----------------------------------------------------------------------------

namespace {

const string pieceLayoutClassic =
    " 1 .Z4Z4 . .L4L4L4 . O O . P P .L5L5L5L5 .V5V5V5 . U U U . N . . ."
    " . . .Z4Z4 . . .L4 . O O . P P .L5 . . . .V5 . . . U . U . N N .I5"
    " 2 2 . . . .T4 . . . . . . P . . . . X . .V5 .Z5 . . . . . . N .I5"
    " . . .I3 .T4T4T4 . . W W . . . F . X X X . . .Z5Z5Z5 . .T5 . N .I5"
    "V3 . .I3 . . . . . . . W W . F F . . X . . Y . . .Z5 . .T5 . . .I5"
    "V3V3 .I3 . .I4I4I4I4 . . W . . F F . . . Y Y Y Y . . .T5T5T5 . .I5";

const string pieceLayoutJunior =
    "1 . 1 . V3V3. . L4L4L4. T4T4T4. . O O . O O . P P . . I5. I5. . L5L5"
    ". . . . V3. . . L4. . . . T4. . . O O . O O . P P . . I5. I5. . . L5"
    "2 . 2 . . . V3. . . . L4. . . T4. . . . . . . P . . . I5. I5. L5. L5"
    "2 . 2 . . V3V3. . L4L4L4. . T4T4T4. . Z4. Z4. . . P . I5. I5. L5. L5"
    ". . . . . . . . . . . . . . . . . . Z4Z4. Z4Z4. P P . I5. I5. L5. . "
    "I3I3I3. I3I3I3. I4I4I4I4. I4I4I4I4. Z4. . . Z4. P P . . . . . L5L5. ";

const string pieceLayoutTrigon =
    "L5L5 . . F F F F . .L6L6 . . O O O . . X X X . . .A6A6 . . G G . G . .C4C4 . . Y Y Y Y"
    "L5L5 . . F . F . . .L6L6 . . O O O . . X X X . .A6A6A6A6 . . G G G . .C4C4 . . Y Y . ."
    " .L5 . . . . . . S . .L6L6 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 2"
    " . . . . . S S S S . . . . . . . .P5P5P5P5 . . .I6I6 . .I5I5I5I5I5 . . W W W W W . . 2"
    "C5C5 . . . S . . . . V V . .P6 . . . .P5 . .A4 . .I6I6 . . . . . . . . . . W . . . . ."
    "C5C5C5 . . . . V V V V . .P6P6P6P6P6 . . .A4A4A4 . .I6I6 . .I3I3I3 . . 1 . . .I4I4I4I4";

} // namespace

//-----------------------------------------------------------------------------

PieceSelector::PieceSelector(QWidget* parent, const Board& bd, Color color)
    : QWidget(parent),
      m_bd(bd),
      m_color(color)
{
    setMinimumWidth(170);
    setMinimumHeight(30);
    init();
}

void PieceSelector::checkUpdate()
{
    bool disabledStatus[maxColumns][maxRows];
    setDisabledStatus(disabledStatus);
    bool changed = false;
    for (unsigned x = 0; x < m_nuColumns; ++x)
        for (unsigned y = 0; y < m_nuRows; ++y)
            if (! m_piece[x][y].is_null()
                && disabledStatus[x][y] != m_disabledStatus[x][y])
            {
                changed = true;
                break;
            }
    if (changed)
        update();
}

void PieceSelector::findPiecePoints(Piece piece, unsigned x, unsigned y,
                                    PiecePoints& points) const
{
    CoordPoint p(x, y);
    if (x >= m_nuColumns || y >= m_nuRows || m_piece[x][y] != piece
        || points.contains(p))
        return;
    points.push_back(p);
    // This assumes that no Trigon pieces touch at the corners, otherwise
    // we would need to iterate over neighboring CoordPoint's like AdjIterator
    // iterates over neighboring Point's
    findPiecePoints(piece, x + 1, y, points);
    findPiecePoints(piece, x - 1, y, points);
    findPiecePoints(piece, x, y + 1, points);
    findPiecePoints(piece, x, y - 1, points);
}

int PieceSelector::heightForWidth(int width) const
{
    // Use ratio for layout of classic pieces, which has larger relative width
    // because the limiting factor in the right panel of the main window is the
    // width
    return width / 33 * 6;
}

void PieceSelector::init()
{
    BoardType boardType = m_bd.get_board_type();
    auto variant = m_bd.get_variant();
    const string* pieceLayout;
    if (boardType == BoardType::trigon || boardType == BoardType::trigon_3)
    {
        pieceLayout = &pieceLayoutTrigon;
        m_nuColumns = 43;
        m_nuRows = 6;
    }
    else if (variant == Variant::junior)
    {
        pieceLayout = &pieceLayoutJunior;
        m_nuColumns = 34;
        m_nuRows = 6;
    }
    else
    {
        pieceLayout = &pieceLayoutClassic;
        m_nuColumns = 33;
        m_nuRows = 6;
    }
    for (unsigned y = 0; y < m_nuRows; ++y)
        for (unsigned x = 0; x < m_nuColumns; ++x)
        {
            string name = pieceLayout->substr(y * m_nuColumns * 2 + x * 2, 2);
            name = trim(name);
            Piece piece = Piece::null();
            if (name != ".")
            {
                m_bd.get_piece_by_name(name, piece);
                LIBBOARDGAME_ASSERT(! piece.is_null());
            }
            m_piece[x][y] = piece;
        }
    auto& geo = m_bd.get_geometry();
    for (unsigned y = 0; y < m_nuRows; ++y)
        for (unsigned x = 0; x < m_nuColumns; ++x)
        {
            Piece piece = m_piece[x][y];
            if (piece.is_null())
                continue;
            PiecePoints points;
            findPiecePoints(piece, x, y, points);
            type_match_shift(geo, points.begin(), points.end(), 0);
            m_transform[x][y] =
                m_bd.get_piece_info(piece).find_transform(geo, points);
            LIBBOARDGAME_ASSERT(m_transform[x][y] != 0);
        }
    setDisabledStatus(m_disabledStatus);
    update();
}

void PieceSelector::mousePressEvent(QMouseEvent* event)
{
    qreal pixelX = event->x() - 0.5 * (width() - m_selectorWidth);
    qreal pixelY = event->y() - 0.5 * (height() - m_selectorHeight);
    if (pixelX < 0 || pixelX >= m_selectorWidth
        || pixelY < 0 || pixelY >= m_selectorHeight)
        return;
    int x = static_cast<int>(pixelX / m_fieldWidth);
    int y = static_cast<int>(pixelY / m_fieldHeight);
    Piece piece = m_piece[x][y];
    if (piece.is_null() || m_disabledStatus[x][y])
        return;
    update();
    emit pieceSelected(m_color, piece, m_transform[x][y]);
}

void PieceSelector::paintEvent(QPaintEvent*)
{
    setDisabledStatus(m_disabledStatus);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    BoardType boardType = m_bd.get_board_type();
    bool isTrigon =
        (boardType == BoardType::trigon || boardType == BoardType::trigon_3);
    qreal ratio;
    if (isTrigon)
    {
        ratio = 1.732;
        m_fieldWidth = min(qreal(width()) / (m_nuColumns + 1),
                           qreal(height()) / (ratio * m_nuRows));
    }
    else
    {
        ratio = 1;
        m_fieldWidth = min(qreal(width()) / m_nuColumns,
                           qreal(height()) / m_nuRows);
    }
    if (m_fieldWidth > 8)
        // Prefer pixel alignment if piece is not too small
        m_fieldWidth = floor(m_fieldWidth);
    m_fieldHeight = ratio * m_fieldWidth;
    m_selectorWidth = m_fieldWidth * m_nuColumns;
    m_selectorHeight = m_fieldHeight * m_nuRows;
    painter.save();
    painter.translate(0.5 * (width() - m_selectorWidth),
                      0.5 * (height() - m_selectorHeight));
    auto variant = m_bd.get_variant();
    auto& geo = m_bd.get_geometry();
    for (unsigned x = 0; x < m_nuColumns; ++x)
        for (unsigned y = 0; y < m_nuRows; ++y)
        {
            Piece piece = m_piece[x][y];
            if (! piece.is_null() && ! m_disabledStatus[x][y])
            {
                if (isTrigon)
                {
                    bool isUpward =
                        (geo.get_point_type(x, y) == geo.get_point_type(0, 0));
                    Util::paintColorTriangle(painter, variant, m_color,
                                             isUpward, x * m_fieldWidth,
                                             y * m_fieldHeight, m_fieldWidth,
                                             m_fieldHeight);
                }
                else
                    Util::paintColorSquare(painter, variant, m_color,
                                           x * m_fieldWidth, y * m_fieldHeight,
                                           m_fieldWidth);
            }
        }
    painter.restore();
}

void PieceSelector::setDisabledStatus(bool disabledStatus[maxColumns][maxRows])
{
    bool marker[maxColumns][maxRows];
    for (unsigned x = 0; x < m_nuColumns; ++x)
        for (unsigned y = 0; y < m_nuRows; ++y)
        {
            marker[x][y] = false;
            disabledStatus[x][y] = false;
        }
    PieceMap<unsigned> nuInstances;
    nuInstances.fill(0);
    bool isColorUsed = (m_color.to_int() < m_bd.get_nu_colors());
    for (unsigned x = 0; x < m_nuColumns; ++x)
        for (unsigned y = 0; y < m_nuRows; ++y)
        {
            if (marker[x][y])
                continue;
            Piece piece = m_piece[x][y];
            if (piece.is_null())
                continue;
            PiecePoints points;
            findPiecePoints(piece, x, y, points);
            bool disabled = false;
            if (! isColorUsed
                || ++nuInstances[piece] > m_bd.get_nu_left_piece(m_color,
                                                                 piece))
                disabled = true;
            for (auto& p : points)
            {
                disabledStatus[p.x][p.y] = disabled;
                marker[p.x][p.y] = true;
            }
        }
}

//-----------------------------------------------------------------------------
