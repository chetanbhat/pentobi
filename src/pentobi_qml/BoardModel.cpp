//-----------------------------------------------------------------------------
/** @file pentobi_qml/BoardModel.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#include "BoardModel.h"

#include <QDebug>
#include <QSettings>
#include "libboardgame_util/Log.h"

using namespace std;
using libboardgame_util::log;
using libpentobi_base::to_string_id;
using libpentobi_base::BoardType;
using libpentobi_base::Color;
using libpentobi_base::ColorMap;
using libpentobi_base::ColorMove;
using libpentobi_base::CoordPoint;
using libpentobi_base::MovePoints;
using libpentobi_base::Piece;
using libpentobi_base::PieceInfo;
using libpentobi_base::PiecePoints;
using libpentobi_base::Point;

//-----------------------------------------------------------------------------

namespace {

// Game coordinates are fractional because they refer to the center of a piece.
// This function is used to compare game coordinates of moves with the same
// piece, so we could even compare the rounded values (?), but comparing
// against epsilon is also safe.
bool compareGameCoord(const QPointF& p1, const QPointF& p2)
{
    return (p1 - p2).manhattanLength() < 0.01f;
}

bool compareTransform(const PieceInfo& pieceInfo, const Transform* t1,
                      const Transform* t2)
{
    return pieceInfo.get_equivalent_transform(t1) ==
            pieceInfo.get_equivalent_transform(t2);
}

int getNuPiecesLeft(const Board& bd, Color c)
{
    unsigned n = 0;
    for (auto piece : bd.get_pieces_left(c))
        n += bd.get_nu_left_piece(c, piece);
    return static_cast<int>(n);
}

QPointF getGameCoord(const Board& bd, Move mv)
{
    auto& geo = bd.get_geometry();
    auto moveInfo = bd.get_move_info(mv);
    PiecePoints movePoints;
    for (Point p : moveInfo)
        movePoints.push_back(CoordPoint(geo.get_x(p), geo.get_y(p)));
    return PieceModel::findCenter(bd, movePoints, false);
}

const Transform* getTransform(const Board& bd, Move mv)
{
    auto& geo = bd.get_geometry();
    auto moveInfo = bd.get_move_info(mv);
    PiecePoints movePoints;
    for (Point p : moveInfo)
        movePoints.push_back(CoordPoint(geo.get_x(p), geo.get_y(p)));
    auto& pieceInfo = bd.get_piece_info(moveInfo.get_piece());
    return pieceInfo.find_transform(geo, movePoints);
}

} //namespace

//-----------------------------------------------------------------------------

BoardModel::BoardModel(QObject* parent)
    : QObject(parent),
      m_bd(getInitialGameVariant()),
      m_gameVariant(to_string_id(m_bd.get_variant())),
      m_nuColors(m_bd.get_nu_colors()),
      m_nuPieces(m_bd.get_nu_pieces()),
      m_toPlay(0),
      m_altPlayer(0),
      m_points0(0),
      m_points1(0),
      m_points2(0),
      m_points3(0),
      m_nuPiecesLeft0(0),
      m_nuPiecesLeft1(0),
      m_nuPiecesLeft2(0),
      m_nuPiecesLeft3(0),
      m_hasMoves0(true),
      m_hasMoves1(true),
      m_hasMoves2(true),
      m_hasMoves3(true),
      m_isGameOver(false),
      m_isBoardEmpty(true),
      m_canUndo(false)
{
    createPieceModels();
    updateProperties();
}

void BoardModel::autoSave()
{
    QString s;
    if (m_bd.get_nu_moves() > 0)
    {
        s  = to_string_id(m_bd.get_variant());
        for (unsigned i = 0; i < m_bd.get_nu_moves(); ++i)
        {
            ColorMove mv = m_bd.get_move(i);
            s.append(QString(";%1;%2")
                     .arg(mv.color.to_int())
                     .arg(m_bd.to_string(mv.move, false).c_str()));
        }
    }
    QSettings settings;
    settings.setValue("autosave", s);
}

void BoardModel::clearAutoSave()
{
    QSettings settings;
    settings.remove("autosave");
}

void BoardModel::createPieceModels()
{
    m_pieceModels0.clear();
    m_pieceModels1.clear();
    m_pieceModels2.clear();
    m_pieceModels3.clear();
    createPieceModels(Color(0), m_pieceModels0);
    createPieceModels(Color(1), m_pieceModels1);
    if (m_nuColors > 2)
        createPieceModels(Color(2), m_pieceModels2);
    if (m_nuColors > 3)
        createPieceModels(Color(3), m_pieceModels3);
}

void BoardModel::createPieceModels(Color c, QList<PieceModel*>& pieceModels)
{
    pieceModels.clear();
    for (unsigned i = 0; i < m_bd.get_nu_uniq_pieces(); ++i)
    {
        Piece piece(i);
        for (unsigned j = 0; j < m_bd.get_nu_piece_instances(); ++j)
            pieceModels.append(new PieceModel(this, m_bd, piece, c));
    }
}

bool BoardModel::findMove(const PieceModel& piece, QString state,
                          QPointF coord, Move& mv) const
{
    auto& info = m_bd.get_piece_info(piece.getPiece());
    auto transform = piece.getTransform(state);
    PiecePoints piecePoints = info.get_points();
    transform->transform(piecePoints.begin(), piecePoints.end());
    auto boardType = m_bd.get_board_type();
    auto newPointType = transform->get_new_point_type();
    bool pointTypeChanged =
            ((boardType == BoardType::trigon && newPointType == 1)
             || (boardType == BoardType::trigon_3 && newPointType == 0));
    QPointF center(PieceModel::findCenter(m_bd, piecePoints, false));
    // Round y of center to a multiple of 0.5, works better in Trigon
    center.setY(round(2 * center.y()) / 2);
    int offX = static_cast<int>(round(coord.x() - center.x()));
    int offY = static_cast<int>(round(coord.y() - center.y()));
    auto& geo = m_bd.get_geometry();
    MovePoints points;
    for (auto& p : piecePoints)
    {
        int x = p.x + offX;
        int y = p.y + offY;
        if (! geo.is_onboard(CoordPoint(x, y)))
            return false;
        auto pointType = geo.get_point_type(p.x, p.y);
        auto boardPointType = geo.get_point_type(x, y);
        if (! pointTypeChanged && pointType != boardPointType)
            return false;
        if (pointTypeChanged && pointType == boardPointType)
            return false;
        points.push_back(geo.get_point(x, y));
    }
    return m_bd.find_move(points, piece.getPiece(), mv);
}

Variant BoardModel::getInitialGameVariant()
{
    QSettings settings;
    auto variantString = settings.value("variant", "").toString();
    Variant gameVariant;
    if (! parse_variant_id(variantString.toLocal8Bit().constData(),
                           gameVariant))
        gameVariant = Variant::duo;
    return gameVariant;
}

int BoardModel::getLastMoveColor()
{
    auto nuMoves = m_bd.get_nu_moves();
    if (nuMoves == 0)
        return 0;
    return m_bd.get_move(nuMoves - 1).color.to_int();
}

PieceModel* BoardModel::getLastMovePieceModel()
{
    return m_lastMovePieceModel;
}

QList<PieceModel*>& BoardModel::getPieceModels(Color c)
{
    if (c == Color(0))
        return m_pieceModels0;
    else if (c == Color(1))
        return m_pieceModels1;
    else if (c == Color(2))
        return m_pieceModels2;
    else
        return  m_pieceModels3;
}

void BoardModel::initGameVariant(QString gameVariant)
{
    if (m_gameVariant == gameVariant)
        return;
    if (gameVariant == "classic")
        m_bd.init(Variant::classic);
    else if (gameVariant == "classic_2")
        m_bd.init(Variant::classic_2);
    else if (gameVariant == "classic_3")
        m_bd.init(Variant::classic_3);
    else if (gameVariant == "duo")
        m_bd.init(Variant::duo);
    else if (gameVariant == "junior")
        m_bd.init(Variant::junior);
    else if (gameVariant == "trigon")
        m_bd.init(Variant::trigon);
    else if (gameVariant == "trigon_2")
        m_bd.init(Variant::trigon_2);
    else if (gameVariant == "trigon_3")
        m_bd.init(Variant::trigon_3);
    else
    {
        qWarning("BoardModel: invalid game variant");
        return;
    }
    int nuColors = m_bd.get_nu_colors();
    if (nuColors != m_nuColors)
    {
        m_nuColors = nuColors;
        emit nuColorsChanged(nuColors);
    }
    int nuPieces = m_bd.get_nu_pieces();
    createPieceModels();
    if (nuPieces != m_nuPieces)
    {
        m_nuPieces = nuPieces;
        emit nuPiecesChanged(nuPieces);
    }
    m_gameVariant = gameVariant;
    emit gameVariantChanged(gameVariant);
    updateProperties();
    QSettings settings;
    settings.setValue("variant", gameVariant);
}

bool BoardModel::isLegalPos(PieceModel* pieceModel, QString state,
                            QPointF coord) const
{
    Move mv;
    if (! findMove(*pieceModel, state, coord, mv))
        return false;
    Color c(pieceModel->color());
    bool result = m_bd.is_legal(c, mv);
    return result;
}

bool BoardModel::loadAutoSave()
{
    QSettings settings;
    QString s = settings.value("autosave", "").toString();
    if (s.isEmpty())
        return false;
    QStringList l = s.split(';');
    if (l[0] != to_string_id(m_bd.get_variant()))
    {
        qWarning("BoardModel: autosave has wrong game variant");
        return false;
    }
    if (l.length() == 1)
    {
        qWarning("BoardModel: autosave has no moves");
        return false;
    }
    m_bd.init();
    try
    {
        for (int i = 1; i < l.length(); i += 2)
        {
            unsigned colorInt = l[i].toUInt();
            if (colorInt >= m_bd.get_nu_colors())
                throw runtime_error("invalid color");
            Color c(colorInt);
            if (i + 1 >= l.length())
                throw runtime_error("color without move");
            Move mv = m_bd.from_string(l[i + 1].toLocal8Bit().constData());
            if (! m_bd.is_legal(c, mv))
                throw runtime_error("illegal move");
            m_bd.play(c, mv);
        }
    }
    catch (const exception &e)
    {
        qWarning() << "BoardModel: autosave has illegal move: " << e.what();
    }
    updateProperties();
    return true;
}

void BoardModel::newGame()
{
    m_bd.init();
    for (auto pieceModel : m_pieceModels0)
        pieceModel->setState("");
    for (auto pieceModel : m_pieceModels1)
        pieceModel->setState("");
    for (auto pieceModel : m_pieceModels2)
        pieceModel->setState("");
    for (auto pieceModel : m_pieceModels3)
        pieceModel->setState("");
    updateProperties();
}


QQmlListProperty<PieceModel> BoardModel::pieceModels0()
{
    return QQmlListProperty<PieceModel>(this, m_pieceModels0);
}

QQmlListProperty<PieceModel> BoardModel::pieceModels1()
{
    return QQmlListProperty<PieceModel>(this, m_pieceModels1);
}

QQmlListProperty<PieceModel> BoardModel::pieceModels2()
{
    return QQmlListProperty<PieceModel>(this, m_pieceModels2);
}

QQmlListProperty<PieceModel> BoardModel::pieceModels3()
{
    return QQmlListProperty<PieceModel>(this, m_pieceModels3);
}

void BoardModel::play(PieceModel* pieceModel, QPointF coord)
{
    Color c(pieceModel->color());
    Move mv;
    if (! findMove(*pieceModel, pieceModel->state(), coord, mv))
    {
        qWarning("BoardModel::play: illegal move");
        return;
    }
    preparePieceGameCoord(pieceModel, mv);
    pieceModel->setIsPlayed(true);
    preparePieceTransform(pieceModel, mv);
    m_bd.play(c, mv);
    updateProperties();
}

void BoardModel::playMove(int move)
{
    Color c = m_bd.get_effective_to_play();
    Move mv(move);
    m_bd.play(c, mv);
    updateProperties();
}

PieceModel* BoardModel::preparePiece(int color, int move)
{
    Move mv(move);
    Piece piece = m_bd.get_move_info(mv).get_piece();
    for (auto pieceModel : getPieceModels(Color(color)))
        if (pieceModel->getPiece() == piece && ! pieceModel->isPlayed())
        {
            preparePieceTransform(pieceModel, mv);
            preparePieceGameCoord(pieceModel, mv);
            return pieceModel;
        }
    return nullptr;
}

void BoardModel::preparePieceGameCoord(PieceModel* pieceModel, Move mv)
{
    pieceModel->setGameCoord(getGameCoord(m_bd, mv));
}

void BoardModel::preparePieceTransform(PieceModel* pieceModel, Move mv)
{
    auto transform = getTransform(m_bd, mv);
    Piece piece = m_bd.get_move_info(mv).get_piece();
    auto& pieceInfo = m_bd.get_piece_info(piece);
    if (! compareTransform(pieceInfo, pieceModel->getTransform(), transform))
        pieceModel->setTransform(transform);
}

void BoardModel::undo()
{
    if (m_bd.get_nu_moves() == 0)
        return;
    m_bd.undo();
    updateProperties();
}

void BoardModel::updateProperties()
{
    int points0 = m_bd.get_points(Color(0));
    if (m_points0 != points0)
    {
        m_points0 = points0;
        emit points0Changed(points0);
    }

    int points1 = m_bd.get_points(Color(1));
    if (m_points1 != points1)
    {
        m_points1 = points1;
        emit points1Changed(points1);
    }

    int nuPiecesLeft0 = getNuPiecesLeft(m_bd, Color(0));
    if (m_nuPiecesLeft0 != nuPiecesLeft0)
    {
        m_nuPiecesLeft0 = nuPiecesLeft0;
        emit nuPiecesLeft0Changed(nuPiecesLeft0);
    }

    int nuPiecesLeft1 = getNuPiecesLeft(m_bd, Color(1));
    if (m_nuPiecesLeft1 != nuPiecesLeft1)
    {
        m_nuPiecesLeft1 = nuPiecesLeft1;
        emit nuPiecesLeft1Changed(nuPiecesLeft1);
    }

    bool hasMoves0 = m_bd.has_moves(Color(0));
    if (m_hasMoves0 != hasMoves0)
    {
        m_hasMoves0 = hasMoves0;
        emit hasMoves0Changed(hasMoves0);
    }

    bool hasMoves1 = m_bd.has_moves(Color(1));
    if (m_hasMoves1 != hasMoves1)
    {
        m_hasMoves1 = hasMoves1;
        emit hasMoves1Changed(hasMoves1);
    }

    if (m_nuColors > 2)
    {
        int points2 = m_bd.get_points(Color(2));
        if (m_points2 != points2)
        {
            m_points2 = points2;
            emit points2Changed(points2);
        }

        bool hasMoves2 = m_bd.has_moves(Color(2));
        if (m_hasMoves2 != hasMoves2)
        {
            m_hasMoves2 = hasMoves2;
            emit hasMoves2Changed(hasMoves2);
        }

        int nuPiecesLeft2 = getNuPiecesLeft(m_bd, Color(2));
        if (m_nuPiecesLeft2 != nuPiecesLeft2)
        {
            m_nuPiecesLeft2 = nuPiecesLeft2;
            emit nuPiecesLeft2Changed(nuPiecesLeft2);
        }
    }

    if (m_nuColors > 3)
    {
        int points3 = m_bd.get_points(Color(3));
        if (m_points3 != points3)
        {
            m_points3 = points3;
            emit points3Changed(points3);
        }

        bool hasMoves3 = m_bd.has_moves(Color(3));
        if (m_hasMoves3 != hasMoves3)
        {
            m_hasMoves3 = hasMoves3;
            emit hasMoves3Changed(hasMoves3);
        }

        int nuPiecesLeft3 = getNuPiecesLeft(m_bd, Color(3));
        if (m_nuPiecesLeft3 != nuPiecesLeft3)
        {
            m_nuPiecesLeft3 = nuPiecesLeft3;
            emit nuPiecesLeft3Changed(nuPiecesLeft3);
        }
    }

    bool canUndo = (m_bd.get_nu_moves() > 0);
    if (m_canUndo != canUndo)
    {
        m_canUndo = canUndo;
        emit canUndoChanged(canUndo);
    }

    bool isGameOver = true;
    for (Color c : m_bd.get_colors())
        if (m_bd.has_moves(c))
            isGameOver = false;
    if (m_isGameOver != isGameOver)
    {
        m_isGameOver = isGameOver;
        emit isGameOverChanged(isGameOver);
    }

    bool isBoardEmpty = (m_bd.get_nu_onboard_pieces() == 0);
    if (m_isBoardEmpty != isBoardEmpty)
    {
        m_isBoardEmpty = isBoardEmpty;
        emit isBoardEmptyChanged(isBoardEmpty);
    }

    ColorMap<array<bool, Board::max_pieces>> isPlayed;
    for (Color c : m_bd.get_colors())
        isPlayed[c].fill(false);
#if LIBBOARDGAME_DEBUG
    // Does not handle setup yet
    for (Color c : m_bd.get_colors())
        LIBBOARDGAME_ASSERT(m_bd.get_setup().placements[c].empty());
#endif
    m_lastMovePieceModel = nullptr;
    for (unsigned i = 0; i < m_bd.get_nu_moves(); ++i)
    {
        auto mv = m_bd.get_move(i);
        Piece piece = m_bd.get_move_info(mv.move).get_piece();
        auto& pieceInfo = m_bd.get_piece_info(piece);
        auto gameCoord = getGameCoord(m_bd, mv.move);
        auto transform = getTransform(m_bd, mv.move);
        auto& pieceModels = getPieceModels(mv.color);
        PieceModel* pieceModel = nullptr;
        // Prefer piece models already played with the given gameCoord and
        // transform because class Board doesn't make a distinction between
        // instances of the same piece (in Junior) and we want to avoid
        // unwanted piece movement animations to switch instances.
        for (int j = 0; j < pieceModels.length(); ++j)
            if (pieceModels[j]->getPiece() == piece
                    && pieceModels[j]->isPlayed()
                    && compareGameCoord(pieceModels[j]->gameCoord(), gameCoord)
                    && compareTransform(pieceInfo,
                                        pieceModels[j]->getTransform(),
                                        transform))
            {
                pieceModel = pieceModels[j];
                isPlayed[mv.color][j] = true;
                break;
            }
        if (pieceModel == nullptr)
        {
            for (int j = 0; j < pieceModels.length(); ++j)
                if (pieceModels[j]->getPiece() == piece
                        && ! isPlayed[mv.color][j])
                {
                    pieceModel = pieceModels[j];
                    isPlayed[mv.color][j] = true;
                    break;
                }
            // Order is important: isPlayed will trigger an animation to move
            // the piece, so it needs to be set after gameCoord.
            pieceModel->setGameCoord(gameCoord);
            pieceModel->setIsPlayed(true);
            pieceModel->setTransform(transform);
        }
        if (i == m_bd.get_nu_moves() - 1)
            m_lastMovePieceModel = pieceModel;
    }
    for (Color c : m_bd.get_colors())
    {
        auto& pieceModels = getPieceModels(c);
        for (int i = 0; i < pieceModels.length(); ++i)
            if (! isPlayed[c][i])
                pieceModels[i]->setIsPlayed(false);
    }

    int toPlay = m_isGameOver ? 0 : m_bd.get_effective_to_play().to_int();
    if (m_toPlay != toPlay)
    {
        m_toPlay = toPlay;
        emit toPlayChanged(toPlay);
    }

    int altPlayer = (m_bd.get_variant() == Variant::classic_3 ?
                         m_bd.get_alt_player() : 0);
    if (m_altPlayer != altPlayer)
    {
        m_altPlayer = altPlayer;
        emit altPlayerChanged(altPlayer);
    }
}

//-----------------------------------------------------------------------------