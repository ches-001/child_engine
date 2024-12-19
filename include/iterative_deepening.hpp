#ifndef ITERATIVE_DEEPENING_HPP
#define ITERATIVE_DEEPENING_HPP

#include <algorithm>
#include <math.h>
#include <numeric>
#include <chrono>
#include "chess_utils.hpp"
#include "../extern/chess.hpp"

class IterativeDeepening{
    private:
        chess::Board board_;
        bool max_is_first_;
        int current_search_depth_;
        bool timeout_;
        std::uint64_t timelimit_ms_;
        std::chrono::steady_clock::time_point start_;
        chess::Movelist sorted_root_moves_;
        pair_t<float, chess::Move> best_ret_;

        pair_t<float, chess::Move> _negamax(
            int color, 
            int depth, 
            float alpha, 
            float beta){

            _set_timeout();
            pair_t<float, chess::Move> ret;
            chess::Movelist legal_moves = _legal_moves(depth);
            if(legal_moves.empty() || board_.isHalfMoveDraw()){
                ret.second = chess::Move(chess::Move::NO_MOVE);
                if(board_.inCheck()){
                    ret.first = color * 1000;
                    return ret;
                }
                ret.first = 0;
                return ret;
            }
            if(board_.isRepetition() || board_.isInsufficientMaterial()){
                ret.first = 0;
                ret.second = chess::Move(chess::Move::NO_MOVE);
                return ret;
            }
            if(depth == 0){
                ret.first = color * evaluate_board_state(board_);
                ret.second = chess::Move(chess::Move::NO_MOVE);
                return ret;
            }

            chess::Move best_move;
            float bestval = -INFINITY;
            int first_color = max_is_first_ ? 1 : -1;

            for(chess::Move &move : legal_moves){
                board_.makeMove(move);
                ret = _negamax(-color, depth-1, -beta, -alpha);
                board_.unmakeMove(move);
                ret.first = -ret.first;
                if(ret.first > bestval){
                    bestval = ret.first;
                    best_move = move;
                }
                if(bestval > alpha){
                    alpha = ret.first;
                }
                if(depth == current_search_depth_ && color==first_color){
                        move.setScore(ret.first);
                        sorted_root_moves_.add(move);
                    }
                if(beta <= alpha){
                    break;
                }
            }
            return {bestval, best_move};
        }

        chess::Movelist _legal_moves(int depth){
            chess::Movelist legal_moves;
            if(depth == current_search_depth_ && sorted_root_moves_.size() > 0){
                std::function<bool(chess::Move&, chess::Move&)> comperator = [&](chess::Move &left, chess::Move &right){
                    return max_is_first_ ? (left.score() > right.score()) : (left.score() < right.score());
                };
                if(!std::is_sorted(sorted_root_moves_.begin(), sorted_root_moves_.end(), comperator)){
                    std::sort(sorted_root_moves_.begin(), sorted_root_moves_.end(), comperator);
                };
                legal_moves = sorted_root_moves_;
                sorted_root_moves_.clear();
                return legal_moves;
            }
            chess::movegen::legalmoves(legal_moves, board_);
            if(depth != current_search_depth_){
                sort_moves(board_, legal_moves);
            }
            return legal_moves;
        }

        void _set_timeout(){
            std::chrono::duration<double>time_limit(timelimit_ms_ / 1000);
            timeout_ = (std::chrono::steady_clock::now() - start_) >= time_limit;
        }

    public:
        IterativeDeepening(
            chess::Board &board, 
            bool max_is_first, 
            int start_depth)
        :board_(board), 
        max_is_first_(max_is_first), 
        current_search_depth_(start_depth){
            assert(start_depth > 0);
            timeout_ = false;
        };

        pair_t<float, chess::Move> iterative_deepening(int depth_increment, std::uint64_t timelimit_ms){
            timelimit_ms_ = timelimit_ms;
            start_ = std::chrono::steady_clock::now();
            int color = max_is_first_ ? 1 : -1;
            while(!timeout_){
                best_ret_ = _negamax(color, current_search_depth_, -INFINITY, INFINITY);
                current_search_depth_ += depth_increment;
            }
            return best_ret_;
        }

        int max_depth_searched(){return current_search_depth_;}
};

pair_t<float, std::string> iterative_deepening_agent(
    std::string fen_pos, 
    int start_depth, 
    int depth_increment,
    std::uint64_t timelimit_ms){

    chess::Board board = chess::Board(fen_pos);
    chess::Movelist legal_moves;
    chess::movegen::legalmoves(legal_moves, board);
    for(chess::Move &move : legal_moves){
        board.makeMove(move);
        if(board.isGameOver().first == chess::GameResultReason::CHECKMATE){
            return {0, chess::uci::moveToUci(move)};
        }
        if(move.typeOf() == chess::Move::PROMOTION){
            if(move.promotionType() == chess::PieceType::underlying::QUEEN){
                return {0, chess::uci::moveToUci(move)};
            }
        }
        board.unmakeMove(move);
    }
    bool is_white = board.sideToMove() == chess::Color("w");
    pair_t<float, chess::Move> ret;
    IterativeDeepening id_agent = IterativeDeepening(board, is_white, start_depth);
    ret = id_agent.iterative_deepening(depth_increment, timelimit_ms);
    // std::cout << "search depth reached: " << id_agent.max_depth_searched() << std::endl;
    return {ret.first, chess::uci::moveToUci(ret.second)};
}

#endif