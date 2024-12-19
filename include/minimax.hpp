#ifndef MINIMAX_HPP
#define MINIMAX_HPP

#include <algorithm>
#include <math.h>
#include <numeric>
#include "chess_utils.hpp"
#include "../extern/chess.hpp"

pair_t<float, chess::Move> negamax(
    chess::Board &board, 
    int color, 
    int depth, 
    float alpha, 
    float beta){

    chess::Movelist legal_moves;
    chess::movegen::legalmoves(legal_moves, board);

    pair_t<float, chess::Move> ret;
    if(legal_moves.empty() || board.isHalfMoveDraw()){
        ret.second = chess::Move(chess::Move::NO_MOVE);
        if(board.inCheck()){
            ret.first = color * 1000;
            return ret;
        }
        ret.first = 0;
        return ret;
    }
    if(board.isRepetition() || board.isInsufficientMaterial()){
        ret.first = 0;
        ret.second = chess::Move(chess::Move::NO_MOVE);
        return ret;
    }
    if(depth == 0){
        ret.first = color * evaluate_board_state(board);
        ret.second = chess::Move(chess::Move::NO_MOVE);
        return ret;
    }
    sort_moves(board, legal_moves);
    chess::Move best_move;
    float bestval = -INFINITY;

    for(chess::Move &move : legal_moves){
        board.makeMove(move);
        ret = negamax(board, -color, depth-1, -beta, -alpha);
        board.unmakeMove(move);
        ret.first = -ret.first;
        if(ret.first > bestval){
            bestval = ret.first;
            best_move = move;
        }
        if(bestval > alpha){
            alpha = ret.first;
        }
        if(beta <= alpha){
            break;
        }
    }
    return {bestval, best_move};
}

pair_t<float, std::string> minimax_agent(std::string fen_pos, int depth){
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
    ret = negamax(board, (is_white) ? 1 : -1, depth, -INFINITY, INFINITY);
    return {ret.first, chess::uci::moveToUci(ret.second)};
}
#endif