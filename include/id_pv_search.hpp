#ifndef ID_PV_SEARCH_HPP
#define ID_PV_SEARCH_HPP

#include <algorithm>
#include <chrono>
#include "types.hpp"
#include "evaluation.hpp"
#include "../extern/chess.hpp"


class ID_PVSearch{

    private:
        bool timedout_;

        int current_search_depth_;

        uint64_t timelimit_ms_;
        
        std::chrono::steady_clock::time_point start_time_;

        PVLine pv_moves_;

        chess::Move best_move_;

        int16_t best_score_;
        
        chess::Board *board_;

        TranspositionTable *tt_;
        
        kmt_t km_table_;

        history_t history_table_;

        /** 
         * The technique for implementing PV search with a transposition table can be found in the reference below;
         * REFERENCE: https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables 
         * 
         * NOTE: There are some differences between this implementation and the one in the reference, this one
         *  is implemented to accomodate the iterative deepening.
         * */
        SearchResult _search(
            int16_t color, 
            int depth, 
            int16_t alpha, 
            int16_t beta, 
            PVLine &pv_moves, 
            bool is_pv_node=true){

            level_n_nodes += 1;
            _set_timedout();
            int ply = current_search_depth_ - depth;

            // mate distance pruning at NonRoot node
            if(depth != current_search_depth_){
                alpha = max(alpha, -Constants::CHECKMATE_SCORE + ply);
                beta  = min(beta, Constants::CHECKMATE_SCORE - (ply + 1));
                if (alpha >= beta){
                    return SearchResult(chess::Move::NO_MOVE, alpha);
                }
            }
            
            int16_t alpha_orig = alpha;
            uint64_t hash = board_->hash();
            TTMove tt_move;
            TTEntry tt_entry;

            if(tt_){
                if(!is_pv_node && tt_->is_exist(hash)){
                    tt_entry = tt_->unchecked_get(hash);

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
            
            SearchResult search_result;
            chess::Move best_move(chess::Move::NO_MOVE);
            search_result.move = best_move;
            chess::Movelist legal_moves = _legal_moves(depth, is_pv_node, tt_move);

            if(legal_moves.empty() || board_->isHalfMoveDraw()){
                if(board_->inCheck()){
                    search_result.score = color *  Constants::CHECKMATE_SCORE;
                    return search_result;
                }
                search_result.score = color * Constants::DRAW_SCORE;
                return search_result;
            }
            if(depth == 0 || timedout_){
                search_result.score = color * evaluate_board_state(*board_);
                return search_result;
            }
            
            chess::Move child_move(chess::Move::NO_MOVE);
            int16_t best_score = -Constants::MAX_AB_VAL;
            PVLine child_pv_moves;
            bool is_quiet;

            for(int i=0; i < legal_moves.size(); i++){
                select_move(legal_moves, i);
                child_move = legal_moves[i];
                is_quiet = is_quiet_move(*board_, child_move);
                board_->makeMove(child_move);
                
                if(i > 2 && depth >= 3 && !board_->inCheck() && is_quiet){
                    int reduction = 1;

                    // we use .to instead of .from in the first index because the move has not been unmade
                    int16_t hist = history_table_.at(
                        board_->at<chess::Piece>(child_move.to())
                    ).at(child_move.to().index());

                    reduction += max(0, reduction - hist / Constants::MAX_HISTORY_SCORE);

                    if(    tt_
                        && tt_->is_exist(hash) 
                        && tt_->unchecked_get(hash).entry_type == TTEntry::TTEntryType::LOWERBOUND){
                        reduction += 2;
                    }

                    for(chess::Move &killer_move : km_table_.at(ply)){
                        if(child_move == killer_move){
                            reduction -= 2;
                            break;
                        }
                    }
                    int reduced_depth = clamp(depth - reduction, 0, depth - 1);
                    int pv_size = child_pv_moves.size;
                    search_result = _search(-color, reduced_depth, -beta, -alpha, child_pv_moves, false);

                    if(-search_result.score > alpha && reduction > 1){
                        int rollback = child_pv_moves.size - pv_size;
                        int last_idx = child_pv_moves.size - 1;
                        for(int i=last_idx; i > (last_idx - rollback); i--){
                            child_pv_moves.moves[i] = chess::Move::NO_MOVE;
                        }
                        child_pv_moves.size = pv_size;
                        search_result = _search(-color, depth-1, -beta, -alpha, child_pv_moves, false);
                    }
                }else{
                    search_result = _search(-color, depth-1, -beta, -alpha, child_pv_moves, i==0 ? is_pv_node : false);
                }
                search_result.score = -search_result.score;

                board_->unmakeMove(child_move);

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
                    best_score = search_result.score;
                    best_move = child_move;

                    if(search_result.score > alpha){
                        alpha = search_result.score;
                        pv_moves.moves[0] = child_move;
                        std::copy(
                            child_pv_moves.moves.begin(), 
                            child_pv_moves.moves.begin() + child_pv_moves.size, 
                            pv_moves.moves.begin() + 1
                        );
                        pv_moves.size = child_pv_moves.size + 1;
                    }
                }
            }
            if(tt_){
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
                tt_->insert({hash, TTEntry(depth, best_move, best_score, entry_type)});
            }

            // Store killer moves and update history, this will be used for sorting silent moves 
            // (moves that are not captures or promotions). The killer move heuristics suggests
            // that a move that caused a beta-cutoff at a given ply will probably be a good move
            // at that ply, as such we can order it to be one of the first few moves to be explored
            // by the algorithm.
            if(is_quiet_move(*board_, best_move)){
                int ply = current_search_depth_ - depth;
                int16_t history_bonus = 100 * depth;

                if(best_score >= beta){
                    history_bonus *= 4;
                    store_killer_move(&km_table_, best_move, ply);
                }
                else if (best_score > alpha_orig){
                    history_bonus *= 2;
                }
                update_history(&history_table_, board_->at<chess::Piece>(best_move.from()), best_move.to(), history_bonus);
            }
            return SearchResult(best_move, best_score);
        }

        chess::Movelist _legal_moves(int depth, bool is_pv_node, TTMove tt_move){
            // Here we compute the legal moves, check if this node is along a pv_path (left most part of the game tree)
            // and also check if pv_idx is within range of the current pv_moves_ move-list, then we score moves based on
            // various conditions, like whether the move is a PV move or a move from the transposition table, or whether
            // move is a capture or whatever...
            chess::Movelist legal_moves;
            chess::movegen::legalmoves(legal_moves, *board_);
            if(legal_moves.empty()){
                return legal_moves;
            }
            chess::Move pv_move = chess::Move::NO_MOVE;
            pv_move.setScore(-Constants::MAX_AB_VAL);
            int ply = current_search_depth_ - depth;

            if(is_pv_node && ply < pv_moves_.size){
                pv_move = pv_moves_.moves[ply];
            }
            score_moves(*board_, legal_moves, tt_move, pv_move, &(km_table_).at(ply), &history_table_);
            return legal_moves;
        }

        void _set_timedout(){
            std::chrono::duration<double>time_limit(timelimit_ms_ / 1000);
            timedout_ = (std::chrono::steady_clock::now() - start_time_) >= time_limit;
        }

    public:
        int n_nodes;
        int level_n_nodes;
        
        ID_PVSearch(TranspositionTable *tt=nullptr)
        :n_nodes(0), 
        level_n_nodes(0),
        current_search_depth_(1),  
        best_score_(-Constants::MAX_AB_VAL), 
        timedout_(false), 
        tt_(tt){
            
            best_move_.setScore(-Constants::MAX_AB_VAL);
            km_table_      = {{ }};
            history_table_ = {{ }};
        };

        pair_t<chess::Move, int16_t> run(chess::Board &board, uint64_t timelimit_ms, bool log=true){
            board_        = &board;
            start_time_   = std::chrono::steady_clock::now();
            timelimit_ms_ = timelimit_ms;
            int16_t color = board_->sideToMove() == chess::Color("w") ? 1 : -1;
            SearchResult search_result;

            while(!timedout_  && current_search_depth_ <= Constants::MAX_SEARCH_DEPTH){
                PVLine pv_moves;
                search_result = _search(color, current_search_depth_, -Constants::MAX_AB_VAL, Constants::MAX_AB_VAL, pv_moves);
                // an exemption to this if-block would imply that the _search algorithm
                // timed out prematurely.
                if(!timedout_){
                    // it is possible for the current PV moves to be less than the previous PV moves
                    // due to encountering an EXACT position in the transposition table. For those cases
                    // array is completed with the remaining PV moves from before.
                    if(pv_moves.size < pv_moves_.size){
                        std::copy(
                            pv_moves.moves.begin(),
                            pv_moves.moves.begin() + pv_moves.size,
                            pv_moves_.moves.begin()
                        );
                    }
                    else{
                        pv_moves_ = pv_moves;
                    }
                    best_move_  = search_result.move;
                    best_score_ = search_result.score;
                }
                if(log){
                    std::cout << "INFO-: score: " << best_score_
                              << " |depth: " << current_search_depth_ << " |nodes: " 
                              << level_n_nodes << " |pv moves: ";
                    for(int i = 0; i < pv_moves_.size; i++){
                        std::cout << chess::uci::moveToUci(pv_moves_.moves[i]) << ", ";
                    }
                    std::cout <<std::endl;
                }
                n_nodes += level_n_nodes;
                level_n_nodes = 0;
                current_search_depth_ += 1;
            }
            return {best_move_, best_score_};
        }

        inline int max_depth_searched(){
            return current_search_depth_;
        }

        inline PVLine pv_moves(){
            return pv_moves_;
        }

        SearchResult get_result(){
            return SearchResult(best_move_, best_score_);
        }

        void reset(bool clear_tt=true){
            timedout_             = false;
            current_search_depth_ = 1;
            timelimit_ms_         = 0;
            pv_moves_.moves       = {{ }};
            pv_moves_.size        = 0;
            best_move_            = chess::Move::NO_MOVE;
            best_score_           = -Constants::MAX_AB_VAL;
            km_table_             = {{ }};
            history_table_        = {{ }};
            n_nodes               = 0;
            level_n_nodes         = 0;

            best_move_.setScore(-Constants::MAX_AB_VAL);
            if(clear_tt){tt_->clear();}
        }
};

pair_t<pair_t<std::string, int16_t>, int> id_pv_search_agent(
    std::string fen_pos, 
    uint64_t timelimit_ms,
    TranspositionTable *tt=nullptr, 
    bool log=true){

    chess::Board board = chess::Board(fen_pos);
    
    pair_t<chess::Move, int16_t> search_result;
    ID_PVSearch search = ID_PVSearch(tt);
    search_result = search.run(board, timelimit_ms, log);
    return {{chess::uci::moveToUci(search_result.first), search_result.second}, search.max_depth_searched()};
}

#endif