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
    TranspositionTable *tt=nullptr,
    kmt_t *km_table=nullptr,
    history_t *history_table=nullptr){
    
    _NODE_COUNT += 1;

    int ply = _SEARCH_DEPTH - depth;

    // mate distance pruning at NonRoot Node
    if(depth != _SEARCH_DEPTH){
        alpha = max(alpha, -Constants::CHECKMATE_SCORE + ply);
        beta  = min(beta, Constants::CHECKMATE_SCORE - (ply + 1));
        if (alpha >= beta){
            return SearchResult(chess::Move::NO_MOVE, alpha);
        }
    }

    int16_t alpha_orig = alpha;
    uint64_t hash = board.hash();
    TTMove tt_move;
    TTEntry tt_entry;

    if(tt){
        if(tt->is_exist(hash)){
            tt_entry = tt->unchecked_get(hash);
            
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
        return SearchResult(best_move, color * Constants::DRAW_SCORE);
    }
    if(depth == 0){
        return SearchResult(best_move, color * evaluate_board_state(board));
    }

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
    bool is_quiet;

    for(int i = 0; i < legal_moves.size(); i++){
        select_move(legal_moves, i);
        child_move = legal_moves[i];
        is_quiet = is_quiet_move(board, child_move);
        board.makeMove(child_move);
        
        if(i > 2 && depth >= 3 && !board.inCheck() && is_quiet){
            int reduction = 1;

            // we use .to instead of .from in the first index because the move has not been unmade
            int16_t hist = (*history_table).at(
                board.at<chess::Piece>(child_move.to())
            ).at(child_move.to().index());

            reduction += max(0, reduction - hist / Constants::MAX_HISTORY_SCORE);

            if(    tt 
                && tt->is_exist(hash) 
                && tt->unchecked_get(hash).entry_type == TTEntry::TTEntryType::LOWERBOUND){
                reduction += 2;
            }

            for(chess::Move &killer_move : (*km_table).at(ply)){
                if(child_move == killer_move){
                    reduction -= 2;
                    break;
                }
            }
            int reduced_depth = clamp(depth - reduction, 0, depth - 1);
            search_result = negamax(board, -color, reduced_depth, -beta, -alpha, tt, km_table, history_table);

            if(-search_result.score > alpha && reduction > 1){
                search_result = negamax(board, -color, depth-1, -beta, -alpha, tt, km_table, history_table);
            }
        }else{
            search_result = negamax(board, -color, depth-1, -beta, -alpha, tt, km_table, history_table);
        }

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
        if (best_score <= alpha_orig){
            entry_type = TTEntry::TTEntryType::UPPERBOUND;
        }
        else if (best_score >= beta){
            entry_type = TTEntry::TTEntryType::LOWERBOUND;
        }
        else{
            entry_type = TTEntry::TTEntryType::EXACT;
        }
        tt->insert({hash, TTEntry(depth, best_move, best_score, entry_type)});
    }
    // Store killer moves and update history, this will be used for sorting silent moves 
    // (moves that are not captures or promotions). The killer move heuristics suggests
    // that a move that caused a beta-cutoff at a given ply will probably be a good move
    // at that ply, as such we can order it to be one of the first few moves to be explored
    // by the algorithm.
    if(is_quiet_move(board, best_move)){
        int16_t history_bonus = 100 * depth;
        
        if(best_score >= beta){
            history_bonus *= 4;
            store_killer_move(km_table, best_move, ply);
        }
        else if (best_score > alpha_orig){
            history_bonus *= 2;
        }
        update_history(history_table, board.at<chess::Piece>(best_move.from()), best_move.to(), history_bonus);
    }
    return SearchResult(best_move, best_score);
}

pair_t<std::string, int16_t> negamax_agent(
    std::string fen_pos, 
    int depth, 
    TranspositionTable *tt=nullptr,
    bool log=true){
        
    assert(depth <= Constants::MAX_SEARCH_DEPTH);
    
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