#ifndef ID_PV_SEARCH_HPP
#define ID_PV_SEARCH_HPP

#include <algorithm>
#include <chrono>
#include "evaluation.hpp"
#include "../extern/chess.hpp"


class IterativeDeepeningPVSearch{

    private:
        bool timedout_;

        int current_search_depth_;

        chess::Board board_;

        uint64_t timelimit_ms_;
        
        std::chrono::steady_clock::time_point start_;

        PVLine pv_moves_;

        chess::Move best_move_;

        int16_t best_score_;

        map_t<uint64_t, TTEntry> *tt_;
        
        kmt_t km_table_;

        /** 
         * The technique for implementing PV search with a transposition table can be found in the reference below;
         * REFERENCE: https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables 
         * REFERENCE: https://www.chessprogramming.org/Node_Types#CUT
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
            
            int16_t alpha_orig = alpha;
            uint64_t hash = board_.hash();
            chess::Move tt_move = chess::Move::NO_MOVE;

            if(tt_){
                if(tt_->find(hash) != tt_->end()){
                    TTEntry tt_entry = tt_->at(hash);
                    tt_move = tt_entry.tt_move;

                    if(tt_entry.depth >= depth){
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
            chess::Movelist legal_moves = _legal_moves(depth, is_pv_node);

            if(legal_moves.empty() || board_.isHalfMoveDraw()){
                if(board_.inCheck()){
                    search_result.score = color *  Constants::CHECKMATE_SCORE;
                    return search_result;
                }
                search_result.score = Constants::DRAW_SCORE;
                return search_result;
            }
            if(depth == 0 || timedout_){
                search_result.score = color * evaluate_board_state(board_);
                return search_result;
            }
            
            chess::Move child_move(chess::Move::NO_MOVE);
            int16_t best_score = -Constants::MAX_AB_VAL;
            PVLine child_pv_moves;

            for(int i=0; i < legal_moves.size(); i++){
                select_move(legal_moves, i);
                child_move = legal_moves[i];
                board_.makeMove(child_move);
                
                search_result = _search(-color, depth-1, -beta, -alpha, child_pv_moves, i==0 ? is_pv_node : false);
                search_result.score = -search_result.score;

                board_.unmakeMove(child_move);

                // set best score to beta if beta cutoff occurs, making this node a fail-high / lowerbound node
                // The intuition here is that if the score >= beta, it means the maximizer has found a move that
                // is too good, such that the minimizer will avoid said branch, as such there is no need to 
                // explore further since ideally, the minimizer will not be playing this branch. If the minimizing
                // player does not play this node, then the search result along this node is inconsequential to the
                // maximizer, as such the branch is pruned from the tree
                if(search_result.score >= beta){
                    best_score = search_result.score;
                    best_move = child_move;
                    // Store killer moves, this will be used for sorting silent moves (moves that are not captures
                    // or promotions). The killer move heuristics suggests that a move that caused a beta-cutoff at a
                    // given ply will probably be a good move at that ply, as such we can order it to be one of the 
                    // first few moves to be explored by the algorithm.
                    if(
                        !board_.isCapture(best_move) 
                        && best_move.typeOf() != chess::Move::PROMOTION
                        && best_move.typeOf() != chess::Move::CASTLING){
                        store_killer_move(&km_table_, best_move, current_search_depth_-depth);
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
                    pv_moves.moves[0] = child_move;
                    std::copy(
                        child_pv_moves.moves.begin(), 
                        child_pv_moves.moves.begin() + child_pv_moves.size, 
                        pv_moves.moves.begin() + 1
                    );
                    pv_moves.size = child_pv_moves.size + 1;
                }
            }
            if(tt_){
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
                tt_->insert({hash, TTEntry(depth, child_move, search_result.score, entry_type)});
            }
            return SearchResult(best_move, best_score);
        }

        chess::Movelist _legal_moves(int depth, bool is_pv_node, chess::Move tt_move=chess::Move::NO_MOVE){
            // Here we compute the legal moves, check if this node is along a pv_path (left most part of the game tree)
            // and also check if pv_idx is within range of the current pv_moves_ move-list, then we score moves based on
            // various conditions, like whether the move is a PV move or a move from the transposition table, or whether
            // move is a capture or whatever...
            chess::Movelist legal_moves;
            chess::movegen::legalmoves(legal_moves, board_);
            if(legal_moves.empty()){
                return legal_moves;
            }
            chess::Move pv_move = chess::Move::NO_MOVE;
            pv_move.setScore(-Constants::MAX_AB_VAL);
            int ply = current_search_depth_ - depth;

            if(is_pv_node && ply < pv_moves_.size){
                pv_move = pv_moves_.moves[ply];
            }
            score_moves(board_, legal_moves, tt_move, pv_move, &(km_table_).at(ply));
            return legal_moves;
        }

        void _set_timedout(){
            std::chrono::duration<double>time_limit(timelimit_ms_ / 1000);
            timedout_ = (std::chrono::steady_clock::now() - start_) >= time_limit;
        }

    public:
        int n_nodes;
        int level_n_nodes;
        
        IterativeDeepeningPVSearch(
            chess::Board &board, 
            map_t<uint64_t, TTEntry> *tt=nullptr
        ):board_(board), current_search_depth_(1), n_nodes(0), level_n_nodes(0), timedout_(false), tt_(tt){

            best_move_.setScore(-Constants::MAX_AB_VAL);
            km_table_ = {{chess::Move::NO_MOVE}};
        };

        pair_t<chess::Move, int16_t> run(uint64_t timelimit_ms, bool log=true){
            
            start_        = std::chrono::steady_clock::now();
            timelimit_ms_ = timelimit_ms;
            int16_t color = board_.sideToMove() == chess::Color("w") ? 1 : -1;
            SearchResult search_result;

            while(!timedout_){
                PVLine pv_moves;
                search_result = _search(color, current_search_depth_, -Constants::MAX_AB_VAL, Constants::MAX_AB_VAL, pv_moves);
                // an exemption to this if-block would imply that the _search algorithm
                // timed out prematurely.
                if(!timedout_ && pv_moves.size > pv_moves_.size){
                    best_move_  = search_result.move;
                    best_score_ = search_result.score;
                    pv_moves_   = pv_moves;
                }
                if(log){
                    std::cout << "INFO-: score " << best_score_
                              << " |depth " << current_search_depth_ << " |nodes " 
                              << level_n_nodes << " |pv moves ";
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

        int max_depth_searched(){return current_search_depth_;}
};

pair_t<std::string, int16_t> id_pv_search_agent(
    std::string fen_pos, 
    uint64_t timelimit_ms,
    map_t<uint64_t, TTEntry> *tt=nullptr, 
    bool log=true){

    chess::Board board = chess::Board(fen_pos);
    
    pair_t<chess::Move, int16_t> search_result;
    IterativeDeepeningPVSearch search = IterativeDeepeningPVSearch(board, tt);
    search_result = search.run(timelimit_ms, log);
    return {chess::uci::moveToUci(search_result.first), search_result.second};
}

#endif