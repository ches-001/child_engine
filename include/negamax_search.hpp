#ifndef MINIMAX_HPP
#define MINIMAX_HPP

#include <algorithm>
#include "types.hpp"
#include "evaluation.hpp"
#include "../extern/chess.hpp"

int _NODE_COUNT = 0;
int _SEARCH_DEPTH = 0;

SearchResult negamax(
    chess::Board &board, 
    int16_t color, 
    int depth,
    int16_t alpha, 
    int16_t beta,
    map_t<uint64_t, TTEntry> *tt=nullptr,
    kmt_t *km_table=nullptr,
    history_t *history_table=nullptr){
    
    _NODE_COUNT += 1;

    int16_t alpha_orig = alpha;
    uint64_t hash = board.hash();
    TTMove tt_move;

    if(tt){
        if(tt->find(hash) != tt->end()){
            TTEntry tt_entry = tt->at(hash);
            
            if(tt_entry.depth >= depth){
                tt_move.move = tt_entry.tt_move;
                tt_move.type = tt_entry.entry_type;

                if(tt_entry.entry_type == TTEntry::TTEntryType::EXACT){
                    return SearchResult(tt_entry.tt_move, tt_entry.tt_score);
                }
                if(tt_entry.entry_type == TTEntry::TTEntryType::LOWERBOUND){
                    alpha = max(alpha, tt_entry.tt_score);
                }
                else if(tt_entry.entry_type == TTEntry::TTEntryType::UPPERBOUND){
                    beta = min(beta, tt_entry.tt_score);
                }
                if(beta <= alpha){
                    return SearchResult(tt_entry.tt_move, tt_entry.tt_score);
                }
            }
        }
    }

    chess::Move best_move(chess::Move::NO_MOVE);
    chess::Movelist legal_moves;
    chess::movegen::legalmoves(legal_moves, board);
    
    if(legal_moves.empty() || board.isHalfMoveDraw()){
        if(board.inCheck()){
            return SearchResult(best_move, color * Constants::CHECKMATE_SCORE);
        }
        return SearchResult(best_move, Constants::DRAW_SCORE);
    }
    if(depth == 0){
        return SearchResult(best_move, color * evaluate_board_state(board));
    }

    int ply = _SEARCH_DEPTH - depth;
    if(km_table){
        arr_t<chess::Move, Constants::NUM_KILLER_MOVES> &killer_moves = (*km_table).at(ply);
        score_moves(board, legal_moves, tt_move, chess::Move::NO_MOVE, &killer_moves, history_table);
    }
    else{
        score_moves(board, legal_moves, tt_move, chess::Move::NO_MOVE, nullptr, history_table);
    }
    SearchResult search_result;
    chess::Move child_move(chess::Move::NO_MOVE);
    int16_t best_score = -Constants::MAX_AB_VAL;

    for(int i = 0; i < legal_moves.size(); i++){
        select_move(legal_moves, i);
        child_move = legal_moves[i];
        board.makeMove(child_move);
        
        search_result = negamax(board, -color, depth-1, -beta, -alpha, tt, km_table, history_table);
        search_result.score = -search_result.score;

        board.unmakeMove(child_move);

        // set best score to beta if beta cutoff occurs, making this node a fail-high / lowerbound node
        // The intuition here is that if the score >= beta, it means the maximizer has found a move that
        // is too good, such that the minimizer will avoid said branch, as such there is no need to 
        // explore further since ideally, the minimizer will not be playing this branch. If the minimizing
        // player does not play this node, then the search result along this node is inconsequential to the
        // maximizer, as such the branch is pruned from the tree
        if(search_result.score >= beta){
            best_score = search_result.score;
            best_move = child_move;
            // Store killer moves and update history, this will be used for sorting silent moves 
            // (moves that are not captures or promotions).
            // The killer move heuristics suggests that a move that caused a beta-cutoff at a
            // given ply will probably be a good move at that ply, as such we can order it to be one of the 
            // first few moves to be explored by the algorithm.
            if(
                km_table 
                && !board.isCapture(best_move) 
                && best_move.typeOf() != chess::Move::PROMOTION
                && best_move.typeOf() != chess::Move::CASTLING){
                store_killer_move(km_table, best_move, ply);
                update_history(history_table, board.at<chess::Piece>(best_move.from()), best_move.to(), ply);
            }
            break;
        }
        // do note that the score assigned to a move with the `scoreMove` method is different from 
        // the score assigned to a move after search (search_score). 
        // It is possible that search_result.score may be greater than the current best score, but 
        // not greater than alpha (fail-low), in such cases, regardless of whether we return alpha
        // as it is or the exact score which could be less than or equal to alpha, it will make no
        // difference hence we return the best score found in this search rather than alpha
        if(search_result.score > best_score){
            alpha = max(alpha, search_result.score);
            best_score = search_result.score;
            best_move = child_move;
        }        
    }
    if(tt){
        TTEntry::TTEntryType entry_type;
        if (search_result.score <= alpha_orig){
            entry_type = TTEntry::TTEntryType::UPPERBOUND;
        }
        else if (search_result.score >= beta){
            entry_type = TTEntry::TTEntryType::LOWERBOUND;
        }
        else{
            entry_type = TTEntry::TTEntryType::EXACT;
        }
        tt->insert({hash, TTEntry(depth, child_move, search_result.score, entry_type)});
    }
    return SearchResult(best_move, best_score);
}

pair_t<std::string, int16_t> minimax_agent(
    std::string fen_pos, 
    int depth, 
    map_t<uint64_t, TTEntry> *tt=nullptr,
    bool log=true){
        
    _NODE_COUNT             = 0;
    _SEARCH_DEPTH           = depth;
    chess::Board board      = chess::Board(fen_pos);
    int color               = board.sideToMove() == chess::Color("w") ? 1 : -1;
    kmt_t km_table          = {{ }};
    history_t history_table = {{ }};
    
    std::string best_move;
    int16_t score;
    SearchResult search_result;

    search_result = negamax(
        board, color, depth, -Constants::MAX_AB_VAL, Constants::MAX_AB_VAL, tt, &km_table, &history_table
    );
    best_move = chess::uci::moveToUci(search_result.move);
    score = search_result.score;
    if (log){
        std::cout << "INFO-: best move " << best_move << " |score: " << score 
                  << " |depth " << depth << " |nodes " << _NODE_COUNT << std::endl;
    }
    _NODE_COUNT   = 0;
    _SEARCH_DEPTH = 0;
    return {best_move, score};
}
#endif