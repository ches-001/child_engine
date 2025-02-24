#ifndef TYPES_HPP
#define TYPES_HPP

#include <array>
#include <vector>
#include <unordered_map>
#include <tuple>
#include "../extern/chess.hpp"

namespace Constants{
    const int MAX_KILLER_MOVES_PLY = 64;

    const int NUM_KILLER_MOVES     =  2;

    const std::size_t DEFAULT_MAX_TT_SIZE  = 100000;
}

template<typename T, std::size_t _size> 
using arr_t = std::array<T, _size>;

template<typename T>
using vec_t = std::vector<T>;

template<typename K, typename V>
using map_t = std::unordered_map<K, V>;

template<typename T1, typename T2>
using pair_t = std::pair<T1, T2>;

// Killer Move Table template: (killermoves[ply][idx])
using kmt_t = arr_t<arr_t<chess::Move, Constants::NUM_KILLER_MOVES>, Constants::MAX_KILLER_MOVES_PLY>;

// History Table template: (history[piece][to])
using history_t = arr_t<arr_t<int16_t, 64>, 12>;

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
        arr_t<chess::Move, 64> moves;
        std::size_t size;
        
        PVLine():size(0), moves({{ }}){}
};

struct TTEntry{
    public:
        /** REFERENCE: https://www.chessprogramming.org/Node_Types#CUT */
        enum struct TTEntryType : uint8_t{
            EXACT,      // Also known as PV Nodes (alpha < score < beta)
            UPPERBOUND, // Also known as All-nodes or fail-low nodes (score <= alpha)
            LOWERBOUND, // Also known as Cut-nodes or fail-high nodes (score > beta)
            NONE        // No Type (merely a placeholder)
        };

        uint8_t depth;
        chess::Move tt_move;
        int16_t tt_score;
        TTEntry::TTEntryType entry_type;

        TTEntry():
            depth(0),
            tt_move(chess::Move::NO_MOVE),
            tt_score(0),
            entry_type(TTEntryType::NONE){}

        TTEntry(
            uint8_t depth, 
            chess::Move tt_move,
            int16_t tt_score,
            TTEntry::TTEntryType entry_type
        ):depth(depth), 
        tt_move(tt_move), 
        tt_score(tt_score), 
        entry_type(entry_type){}
};

struct TTMove{
    public:
        chess::Move move;
        TTEntry::TTEntryType type;

    TTMove():move(chess::Move::NO_MOVE),type(TTEntry::TTEntryType::NONE){}
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

struct TranspositionTable{
    private:
        map_t<uint64_t, TTEntry> tt_;
        vec_t<uint64_t> order_;
        std::size_t current_size_;
        int index_to_pop_;
        std::size_t max_tt_size_;

    public:
        TranspositionTable(std::size_t max_tt_size=Constants::DEFAULT_MAX_TT_SIZE)
            :current_size_(0), index_to_pop_(0), max_tt_size_(max_tt_size){

            assert(max_tt_size > 0);
            max_tt_size_ = max_tt_size_;
            order_.resize(max_tt_size_);
        }

        void insert(pair_t<uint64_t, TTEntry> item){
            if(!tt_.insert(item).second){
                return;
            }

            if(current_size_ == max_tt_size_){
                int idx = index_to_pop_ % max_tt_size_;
                tt_.erase(order_[idx]);
                order_[idx] = item.first;
                index_to_pop_ += 1;
            }

            else if(current_size_ > max_tt_size_){
                // This should not happen
                throw std::length_error("Size of transposition table exceeds the allowed limit!");
            }
            
            else{
                order_[current_size_] = item.first;
                current_size_ += 1;
            }
        }

        bool is_exist(uint64_t key){
            return tt_.find(key) != tt_.end();
        }

        TTEntry unchecked_get(uint64_t key){
            return tt_.at(key);
        }

        TTEntry get(uint64_t key){
            map_t<uint64_t, TTEntry>::iterator it = tt_.find(key);
            if(it != tt_.end()){
                return it->second;
            }
           throw std::out_of_range("Key not found in the transposition table.");
        }

        std::size_t size(){
            return current_size_;
        }

        void clear(){
            tt_.clear();
            current_size_ = 0;
            index_to_pop_ = 0;
            order_.clear();
        }
};

#endif