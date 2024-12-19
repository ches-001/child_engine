#ifndef MCTS_HPP
#define MCTS_HPP

#include <algorithm>
#include <math.h>
#include <memory>
#include <type_traits>
#include <random>
#include <ctime>
#include "chess_utils.hpp"
#include "../extern/chess.hpp"

struct Node{
    chess::Board state;
    std::weak_ptr<Node> parent_ptr;
    chess::Move move;
    int depth;
    vec_t<std::shared_ptr<Node>> children_ptrs;
    int n;
    float w;

    Node() = default;
    Node(
        chess::Board state, 
        const std::weak_ptr<Node> &parent_ptr,
        chess::Move move,
        int depth):state(state), parent_ptr(parent_ptr), move(move), depth(depth){
            this->n = 0;
            this->w = 0;
            this->_init = true;
    }
    bool isInit(){return this->_init;}
    private:
        bool _init = false;
};

float evaluate_ucb(
    Node &node, 
    float c, 
    int N){
    
    if(node.n == 0){
        return INFINITY;
    }
    return (node.w / node.n) + (c * sqrt(log(N) / node.n));
}

std::shared_ptr<Node> select(
    std::shared_ptr<Node> node_ptr, 
    float c, 
    int N){
    auto max_comperator = [&c, &N](
        std::shared_ptr<Node> &left,
        std::shared_ptr<Node> &right){
        return evaluate_ucb(*left, c, N) < evaluate_ucb(*right, c, N);
    };
    while(node_ptr->children_ptrs.size() > 0){
        node_ptr = *std::max_element(
            node_ptr->children_ptrs.begin(), node_ptr->children_ptrs.end(), max_comperator
        );
    }
    return node_ptr;
}

void expand(std::shared_ptr<Node> &node_ptr){
    if(node_ptr->children_ptrs.size() > 0){
        return;
    }
    chess::Movelist legal_moves;
    chess::movegen::legalmoves(legal_moves, node_ptr->state);
    node_ptr->children_ptrs.resize(legal_moves.size());
    int i = 0;
    for(chess::Move &move : legal_moves){
        node_ptr->state.makeMove(move);
        std::shared_ptr<Node> childnode = std::make_shared<Node>(
            node_ptr->state, node_ptr, move, node_ptr->depth+1
        );
        node_ptr->state.unmakeMove(move);
        node_ptr->children_ptrs[i] = childnode;
        i++;
    }
}

pair_t<chess::Color, float> rollout(Node node, int rollout_depth, std::mt19937 rng){
    pair_t<chess::GameResultReason, chess::GameResult> go_status;
    chess::Move move;
    chess::Movelist legal_moves;
    chess::movegen::legalmoves(legal_moves, node.state);
    chess::Color winner = chess::Color::NONE;
    float score = 0;
    float static_eval = 0;
    map_t<chess::Color::underlying, chess::Color::underlying> opp_map = {
        {chess::Color::WHITE, chess::Color::BLACK}, 
        {chess::Color::BLACK, chess::Color::WHITE}
    };
    for(int i=0; i < rollout_depth; i++){
        if(legal_moves.empty() || node.state.isHalfMoveDraw()){
            if(node.state.inCheck()){
                winner = opp_map[node.state.sideToMove().internal()];
                score = 1.0;
            }else{score = 0.5;}
            break;
        }
        if(node.state.isRepetition() || node.state.isInsufficientMaterial()){
            score = 0.5;
            break;
        }
        std::uniform_int_distribution<> dist(0, legal_moves.size()-1);
        move = legal_moves[dist(rng)];
        node.state.makeMove(move);
        chess::movegen::legalmoves(legal_moves, node.state);
    }

    if(winner == chess::Color::NONE && score == 0){
        static_eval = evaluate_board_state(node.state);
        if(static_eval == 0) {
            score = 0.5;
            return {winner, score};
        };
        score = abs(static_eval);
        winner = static_eval < 0 ? chess::Color::BLACK : chess::Color::WHITE;
        return {winner, score};
    }
    return {winner, score};
}

void backpropagation(Node &node, chess::Color winner, float score){
    node.n += 1;
    if(winner == chess::Color::NONE){
        node.w += 0.5;
    }
    else if(node.state.sideToMove() == winner){
        node.w += score;
    }
    std::shared_ptr<Node> parent_ptr = node.parent_ptr.lock();
    if(parent_ptr && parent_ptr->isInit()){
        backpropagation(*parent_ptr, winner, score);
    };
}

pair_t<float, chess::Move> mcts(
    chess::Board &board, 
    float c, 
    int rollout_depth, 
    int max_iter){

    int global_N = 0;
    std::shared_ptr<Node> parent_ptr = std::make_shared<Node>();
    std::shared_ptr<Node> root_ptr = std::make_shared<Node>(
        board, parent_ptr, chess::Move::NO_MOVE, 0
    );
    std::time_t time_now;
    std::mt19937 rng;
    pair_t<chess::Color, float> rollout_ret;
    std::shared_ptr<Node> selected_node_ptr;

    while(global_N < max_iter){
        time_now = std::time(0);
        rng = std::mt19937((static_cast<std::uint32_t>(time_now)));
        selected_node_ptr = select(root_ptr, c, global_N);
        if(selected_node_ptr->children_ptrs.size() != 0 || global_N == 0){
            expand(selected_node_ptr);
            std::uniform_int_distribution<> dist(0, selected_node_ptr->children_ptrs.size()-1);
            selected_node_ptr = selected_node_ptr->children_ptrs[dist(rng)];
        }
        rollout_ret = rollout(*selected_node_ptr, rollout_depth, rng);
        backpropagation(*selected_node_ptr, rollout_ret.first, rollout_ret.second);
        global_N += 1;
    }
    auto max_comperator = [&global_N](
        std::shared_ptr<Node> &left, 
        std::shared_ptr<Node> &right){
        return evaluate_ucb(*left, 0.0, global_N) < evaluate_ucb(*right, 0.0, global_N);
    };
    std::shared_ptr<Node> bestnode_ptr = *std::max_element(
        root_ptr->children_ptrs.begin(), root_ptr->children_ptrs.end(), max_comperator
    );
    float best_ucb = evaluate_ucb(*bestnode_ptr, 0.0, global_N);
    return {best_ucb, bestnode_ptr->move};
}

pair_t<float, std::string> mcts_agent(
    std::string fen_pos, 
    float c, 
    int rollout_depth, 
    int max_iter){

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
    pair_t<float, chess::Move> ret = mcts(board, c, rollout_depth, max_iter);
    return {ret.first, chess::uci::moveToUci(ret.second)};
}
#endif