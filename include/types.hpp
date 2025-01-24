#ifndef TYPES_HPP
#define TYPES_HPP

#include <unordered_map>
#include "../extern/chess.hpp"

namespace Constants{
    const int MAX_KILLER_MOVES_PLY = 64;
    const int NUM_KILLER_MOVES     =  2;
}

template<typename T, std::size_t _size> 
using arr_t = std::array<T, _size>;

template<typename T>
using vec_t = std::vector<T>;

template<typename K, typename V>
using map_t = std::unordered_map<K, V>;

template<typename T1, typename T2>
using pair_t = std::pair<T1, T2>;

// Killer Move Table template
using kmt_t = arr_t<arr_t<chess::Move, Constants::NUM_KILLER_MOVES>, Constants::MAX_KILLER_MOVES_PLY>;

struct SearchResult{
    public:
        chess::Move move;
        int16_t score;

        SearchResult() = default;
        SearchResult(
            chess::Move move,
            int16_t score
        ):move(move), score(score){};
};

struct PVLine{
    public:
        arr_t<chess::Move, 64> moves = {{chess::Move::NO_MOVE}};
        int size;
        
        PVLine():size(0){}
};

struct TTEntry{
    public:
        enum struct TTEntryType{
            EXACT,      // Also known as PV Nodes (alpha < score < beta)
            UPPERBOUND, // Also known as All-nodes or fail-low nodes (score <= alpha)
            LOWERBOUND  // Also known as Cut-nodes or fail-high nodes (score > beta)
        };

        int depth;
        chess::Move tt_move;
        int16_t tt_score;
        TTEntry::TTEntryType entry_type;

        TTEntry(
            int depth, 
            chess::Move tt_move,
            int16_t tt_score,
            TTEntry::TTEntryType entry_type
        ):depth(depth), 
        tt_move(tt_move), 
        tt_score(tt_score), 
        entry_type(entry_type){}
};

struct EvalInfo{
    public:
        arr_t<chess::Bitboard, 2> sides;
        arr_t<arr_t<chess::Bitboard, 6>, 2> pieces;

        arr_t<chess::Bitboard, 2> rammed_pawns;
        arr_t<chess::Bitboard, 2> pawn_attacks;
        arr_t<chess::Bitboard, 2> double_pawn_attacks;
        arr_t<chess::Bitboard, 2> king_areas;
        arr_t<chess::Bitboard, 2> attacks;
        arr_t<chess::Bitboard, 2> double_attacks;
        arr_t<arr_t<chess::Bitboard, 6>, 2> piece_attacks;

        arr_t<int32_t, 2> king_attacks_count;
        arr_t<int32_t, 2> king_attackers_count;
        arr_t<int32_t, 2> king_attackers_weight;

        arr_t<int32_t, 2> pk_eval;
        arr_t<int32_t, 2> pk_safety;

        arr_t<chess::Square, 2> king_sqs;
        
        chess::Bitboard passed_pawns;
};

#endif