#ifndef CHESS_UTILS_HPP
#define CHESS_UTILS_HPP

#include <array>
#include <vector>
#include <unordered_map>
#include "../extern/chess.hpp"

template<typename T, std::size_t _size> 
using arr_t = std::array<T, _size>;

template<typename T>
using vec_t = std::vector<T>;

template<typename K, typename V>
using map_t = std::unordered_map<K, V>;

template<typename T1, typename T2>
using pair_t = std::pair<T1, T2>;

namespace Constants{
    const arr_t<std::string, 12> piece_symbol = {
        "K", "k", "Q", "q", "R", "r", "B", "b", "N", "n", "P", "p"
    };
    const arr_t<std::string, 6> piecetype_symbol = {
        "k", "q", "r", "b", "n", "p"
    };
    const arr_t<float, 12> piece_weight = {
        0.0, 0.0, 10.0, -10.0, 5.0, -5.0, 3.0, -3.0, 3.0, -3.0, 1.0, -1.0
    };
    const arr_t<float, 6> piecetype_weight = {
        0.0, 10.0, 5.0, 3.0, 3.0, 1.0
    };
    const map_t<std::string, float> piece_weight_map = {
        {piece_symbol[0], piece_weight[0]},
        {piece_symbol[1], piece_weight[1]},
        {piece_symbol[2], piece_weight[2]},
        {piece_symbol[3], piece_weight[3]},
        {piece_symbol[4], piece_weight[4]},
        {piece_symbol[5], piece_weight[5]},
        {piece_symbol[6], piece_weight[6]},
        {piece_symbol[7], piece_weight[7]},
        {piece_symbol[8], piece_weight[8]},
        {piece_symbol[9], piece_weight[9]},
        {piece_symbol[10], piece_weight[10]},
        {piece_symbol[11], piece_weight[11]},
    };
    const map_t<std::string, float> piecetype_weight_map = {
        {piecetype_symbol[0], piecetype_weight[0]},
        {piecetype_symbol[1], piecetype_weight[1]},
        {piecetype_symbol[2], piecetype_weight[2]},
        {piecetype_symbol[3], piecetype_weight[3]},
        {piecetype_symbol[4], piecetype_weight[4]},
        {piecetype_symbol[5], piecetype_weight[5]},
    };
    const float max_mb = 40;
    const std::uint64_t black_half = 0xFFFFFFFF00000000ULL;
    const std::uint64_t white_half = 0xFFFFFFFFULL;
}

/**
 * Evaluate move based on the movetype (captures (enpassant), promotions, castling, etc)
 * and return a single value weighted by the by piece type
 */
float evaluate_move(chess::Board &board, chess::Move &move){
    float val = 0;
    chess::Color color(board.sideToMove());
    uint16_t move_type(move.typeOf());
    chess::Square from(move.from());
    chess::Square to(move.to());
    chess::Piece attacker(board.at<chess::Piece>(from));
    chess::Piece victim(board.at<chess::Piece>(to));
    
    if(victim != chess::Piece::underlying::NONE && move_type != chess::Move::CASTLING) {
        if(move_type == chess::Move::ENPASSANT) {
            val += 1.5;
        }else{
            float attacker_w = Constants::piecetype_weight_map.at(std::string(attacker.type()));
            float victim_w = Constants::piecetype_weight_map.at(std::string(victim.type()));
            float diff = attacker_w - victim_w;
            val += diff > 1.0 ? diff : 1.0;
        }
    }
    if(move_type == chess::Move::PROMOTION) {
        val += Constants::piecetype_weight_map.at(std::string(move.promotionType()));
    }
    if(move_type == chess::Move::CASTLING) {val += 2.0;}
    if(attacker.type() == chess::PieceType::PAWN){
        if((board.us(color) | board.them(color)).count() < 10){val += 1;}
    }
    if(board.isAttacked(to, ~color)){
        val -= Constants::piece_weight_map.at(std::string(attacker.type()));
    }
    return val;
}

/**
 * Sort moves based in ascending or descending order, based on how good they are
 */
void sort_moves(chess::Board &board, chess::Movelist &moves, bool ascending=false){
    std::sort(
        moves.begin(), 
        moves.end(), 
    [&board, &ascending](chess::Move &move_a, chess::Move &move_b){
        if(!ascending){
            return evaluate_move(board, move_a) > evaluate_move(board, move_b);
        }else{
            return evaluate_move(board, move_a) < evaluate_move(board, move_b);
        }
    });
}

inline bool is_white(chess::Board &board){
    return board.sideToMove() == chess::Color::WHITE;
}

/**
 * This function evaluates the material balance of the board, assigning each unique
 * weight values to different pieces. In this function, positive values indicate that
 * the game is probably in favour of white, and negative indicates that the game is
 * probably in favour of black.
 *
 * Range: 
 * --------
 * (-40 ≤ x ≤ 40)
 */
float evaluate_material_balance(chess::Board &board){
    float result = 0;
    chess::Piece p;
    std::for_each(
        // std::execution::par_unseq,
        Constants::piece_symbol.begin(), 
        Constants::piece_symbol.end(), 
        [&](const std::string &piece_str){
            p = chess::Piece(piece_str);
            result += (
                Constants::piece_weight_map.at(piece_str) * 
                board.pieces(p.type(), p.color()).count()
            );
        }
    );
    return result;
}

float evaluate_king_safety(chess::Board &board, chess::Color &color, chess::Bitboard &us){
    float val = 0;
    chess::Bitboard attacks;
    chess::Square square(board.kingSq(color));
    chess::Bitboard king_surr = chess::attacks::king(square) & ~us;
    king_surr |= chess::Bitboard(1ULL << square.index());
    for(int i=0; i < 64; i++){
        if(!king_surr.check(i)){continue;}
        val += board.isAttacked(chess::Square(i), ~color);
    }
    return -val;
}

float evaluate_space_advantage(chess::Board &board, chess::Color color){
    float val = 0;
    chess::Bitboard occupancy(board.occ());
    chess::Bitboard opp_half;
    if(color == chess::Color{"w"}){
        opp_half = Constants::black_half;
    }else{
        opp_half = Constants::white_half;
    }
    chess::Piece piece;
    chess::Square square;
    chess::PieceType piece_type;
    chess::Bitboard piece_space;
    for(int i=0; i < 64; i++){
        if(!occupancy.check(i)){continue;}
        square = chess::Square(i);
        piece = board.at<chess::Piece>(square);
        if(piece.color() != color){continue;}
        piece_type = piece.type();

        if(piece_type == chess::PieceType::PAWN){
            piece_space = chess::attacks::pawn(piece.color(), square);
            int _idx = square.index();
            if(piece.color() == chess::Color("w")){
                piece_space |= chess::Bitboard(!occupancy.check(_idx + 8) ? (1ULL << (_idx + 8)) : 0ULL);
            }else{
                piece_space |= chess::Bitboard(!occupancy.check(_idx - 8) ? (1ULL << (_idx - 8)) : 0ULL);
            }
        }
        else if(piece_type == chess::PieceType:: KNIGHT){
            piece_space = chess::attacks::knight(square);
        }
        else if(piece_type == chess::PieceType:: ROOK){
            piece_space = chess::attacks::rook(square, occupancy);
        }
        else if(piece_type == chess::PieceType:: BISHOP){
            piece_space = chess::attacks::bishop(square, occupancy);
        }
        else if(piece_type == chess::PieceType:: QUEEN){
            piece_space = chess::attacks::queen(square, occupancy);
        }
        else{continue;}
        val += Constants::piecetype_weight_map.at(static_cast<std::string>(piece_type))
        * (opp_half & (chess::Bitboard(1ULL << square.index()) | piece_space)).count();
    }
    return val;
}

/**
 * This function evaluates the freedom of movement of the piece on a given square.
 * It does so by getting all possible paths that the piece can go / attack, and masking
 * out locations on the board occupied by other pieces, as well as the locations beyond
 * those pieces along that path. If an opponent piece blocks its path, the location of 
 * that opponent is is still mapped as a valid square (indicating that it can be consumed),
 * after which it invalidates / unsets the squares beyond it.
 * 
 * NOTE: This operation is somewhat expensive
 * 
 * Ranges:
 * --------
 * Pawn: (0 ≤ x ≤ 3)
 * King: (0 ≤ x ≤ 8)
 * Knight: (0 ≤ x ≤ 8)
 * Bishop: (0 ≤ x ≤ 13)
 * Rook: (0 ≤ x ≤ 14)
 * Queen: (0 ≤ x ≤ 27)
 * None: 0
 */
float evaluate_freedom(
    chess::Board &board, 
    chess::Square square,
    chess::Piece &piece,
    bool filter_friends=true
){
    assert(square != chess::Square::underlying::NO_SQ);
    chess::Bitboard attacks;
    chess::PieceType piece_type(piece.type());
    chess::Color color(piece.color());
    chess::Bitboard occupancy(board.occ());
    chess::Bitboard us(board.us(color));
    float val;
    
    if(piece_type == chess::PieceType::underlying::PAWN){
        attacks = chess::attacks::pawn(color, square);
    }
    else if (piece_type == chess::PieceType::underlying::KING){
        attacks = chess::attacks::king(square);
    }
    else if (piece_type == chess::PieceType::underlying::KNIGHT){
        attacks = chess::attacks::knight(square);
    }
    else if (piece_type == chess::PieceType::underlying::ROOK){
        attacks = chess::attacks::rook(square, occupancy);
    }
    else if (piece_type == chess::PieceType::underlying::BISHOP){
        attacks = chess::attacks::bishop(square, occupancy);
    }
    else if (piece_type == chess::PieceType::underlying::QUEEN){
        attacks = chess::attacks::queen(square, occupancy);
    }
    if(filter_friends){attacks &= ~us;}
    val = attacks.count();
    return val;
}

/**
 * This function evaluates the freedom of all pieces of the specified underlying
 * Eg: If there are 2 rooks on board, it aggregates the freedom of both rooks into
 * a single value.
 * 
 *NOTE: This operation is somewhat expensive
 * 
 * Ranges:
 * --------
 * Pawn: (0 ≤ x ≤ 24)
 * King: (0 ≤ x ≤ 8)
 * Knight: (0 ≤ x ≤ 16)
 * Bishop: (0 ≤ x ≤ 26)
 * Rook: (0 ≤ x ≤ 28)
 * Queen: (0 ≤ x ≤ 27)
 * None: 0
 */
float evaluate_piece_freedom(chess::Board &board, chess::Piece &piece, bool filter_friends=true){
    assert(piece != chess::Piece::underlying::NONE);
    chess::Square square;
    chess::Color color(piece.color());
    chess::PieceType piece_type(piece.type());
    float val = 0;
    if(board.pieces(piece_type, color).count() == 0){
        return 0;
    }
    chess::Bitboard piecies_bb = board.pieces(piece.type(), piece.color());
    for(int i=0; i < 64; i++){
        if(!piecies_bb.check(i)){continue;}
        val += evaluate_freedom(board, chess::Square(i), piece, filter_friends);
    }
    return val;
}

float evaluate_board_state(chess::Board &board){
    float mb = evaluate_material_balance(board);
    chess::Color white("w");
    chess::Color black = ~white;
    chess::Bitboard white_occ(board.us(white));
    chess::Bitboard black_occ(board.us(black));
    float ks = 0;
    ks += evaluate_king_safety(board, white, white_occ);
    ks -= evaluate_king_safety(board, black, black_occ);
    float sa = 0;
    sa += evaluate_space_advantage(board, white);
    sa -= evaluate_space_advantage(board, black);
    return (1.0 * mb) + (0.5 * ks) + (0.5 * sa);
}
#endif