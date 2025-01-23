#ifndef EVALUATION_HPP
#define EVALUATION_HPP

#include "constants.hpp"
#include "../extern/chess.hpp"

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

void score_moves(
    chess::Board &board, 
    chess::Movelist &movelist,
    chess::Move tt_move,
    chess::Move pv_move,
    arr_t<chess::Move, Constants::NUM_KILLER_MOVES> *killer_moves
){
    
    chess::Color color(board.sideToMove());
    int16_t score;
    chess::Square from;
    chess::Square to;
    chess::PieceType attacker;
    chess::PieceType victim;

    for(chess::Move &move : movelist){
        score = 0;
        from = move.from();
        to = move.to();
        attacker = board.at<chess::PieceType>(from);
        victim = board.at<chess::PieceType>(to);

        // Move priority:
        // 1. PV moves
        // 2. TT moves
        // 2. Captures / Promotion / Castling
        // 3. History heuristics for quite moves

        // It is possible for a PV move to be equal to a TT move as the transposition table (TT)
        // stores all EXACT types (alpha < score < beta), however, I do like to differentiate
        // because it is possible for an EXACT type to not be a PV node even though ideally, this
        // should not be the case
        if(pv_move.move() != chess::Move::NO_MOVE && move == pv_move){
            score += Constants::PV_MOVE_SCORE;
        }
        else if(tt_move.move() != chess::Move::NO_MOVE && move == tt_move){
            score += Constants::TT_MOVE_SCORE;
        }
        else{
            bool is_capture = board.isCapture(move);
            bool is_promotion = move.typeOf() == chess::Move::PROMOTION;
            bool is_castling = move.typeOf() == chess::Move::CASTLING;
            bool is_enpassent = move.typeOf() == chess::Move::ENPASSANT;
            if(is_capture){
                score += is_enpassent
                    ? Constants::MVV_LVA_SCORES.at((int)chess::PieceType::PAWN).at(attacker)
                    : Constants::MVV_LVA_SCORES.at(victim).at(attacker);
            }
            if(is_promotion){
                score += Constants::PROMOTION_SCORES[move.promotionType()];
            }
            if(is_castling){
                score += Constants::CASTLING_SCORE;
            }
            if(!is_capture && !is_promotion && !is_castling){
                if(killer_moves){
                    for(chess::Move &killer_move : *killer_moves){
                        if(killer_move == move){
                            score += Constants::KILLER_MOVE_SCORE;
                        }
                    }
                }
            }
        }
        move.setScore(score);
    }
}

void score_moves(
    chess::Board &board, 
    chess::Movelist &movelist, 
    chess::Move tt_move, 
    chess::Move pv_move){
    score_moves(board, movelist, tt_move, pv_move, nullptr);
}

void score_moves(
    chess::Board &board, 
    chess::Movelist &movelist,
    chess::Move tt_move, 
    arr_t<chess::Move, Constants::NUM_KILLER_MOVES> *killer_moves){
    score_moves(board, movelist, tt_move, chess::Move::NO_MOVE, killer_moves);
}

void score_moves(chess::Board &board, chess::Movelist &movelist, chess::Move tt_move){
    score_moves(board, movelist, tt_move, chess::Move::NO_MOVE, nullptr);
}

void score_moves(chess::Board &board, chess::Movelist &movelist){
    score_moves(board, movelist, chess::Move::NO_MOVE, chess::Move::NO_MOVE, nullptr);
}

void select_move(chess::Movelist &movelist, int start_idx){
    // instead of sorting the movelist array, I instead opt for moving the most valuable move at a
    // given time to the left-side of the array (at start_idx), although this by itself is less efficient
    // than physically sorting the array, it is more efficient in a context where beta-cutoff occurs very
    // frequently. So inotherwords the efficiency of this technique hinges on how good the move scoring
    // and static evaluation methods are, because a good move scoring technique will trigger more beta
    // cut-offs.
    chess::Move temp_move;
    for(int i = start_idx + 1; i < movelist.size(); i++){
        if(movelist.at(i).score() > movelist.at(start_idx).score()){
            temp_move = movelist.at(i);
            movelist[i] = movelist[start_idx];
            movelist[start_idx] = temp_move;
        }
    }
}

void store_killer_move(kmt_t *km_table, chess::Move move, int ply){
    if(move == chess::Move::NO_MOVE){return;}
    arr_t<chess::Move, Constants::NUM_KILLER_MOVES> &killers = (*km_table).at(ply);

    for(chess::Move &killer_move : killers){
        if(move == killer_move){return;}
    }
    for(int i=1; i < killers.size(); i++){
        killers[i] = killers[i-1];
    }
    killers[0] = move;
}

chess::Bitboard get_pawn_advance(
    chess::Bitboard occ, 
    chess::Bitboard pawns, 
    chess::Color color, 
    int advance_by=1){
    assert(advance_by >= 1 && advance_by <= 8);
    int forward = 8 * advance_by;
    return ~occ & (color == chess::Color::WHITE ? pawns >> forward : pawns << forward);
}

int get_mirror_file(int file){
    const static arr_t<int, 8> mirror_files = {{0, 1, 2, 3, 3, 2, 1, 0}};
    return mirror_files.at(file);
}

chess::Bitboard get_king_areas(chess::Color color, chess::Square king_sq){
    chess::Bitboard king_attacks = chess::attacks::king(king_sq);
    chess::Bitboard area_bb = king_attacks  
        | (color == chess::Color::WHITE ? king_attacks << 8 : king_attacks >> 8)
        | (1ULL << king_sq.index());
    area_bb |= king_sq.file() == chess::File::FILE_A ? area_bb << 1 : 0ULL;
    area_bb |= king_sq.file() == chess::File::FILE_H ? area_bb >> 1 : 0ULL;
    return area_bb;
}

chess::Bitboard get_all_pawn_attacks(
    chess::Bitboard pawns, 
    chess::Color color, 
    bool spanned=true
    ){
    chess::Bitboard left_attacks = color == chess::Color::WHITE 
        ? chess::attacks::pawnLeftAttacks<chess::Color::WHITE>(pawns)
        : chess::attacks::pawnLeftAttacks<chess::Color::BLACK>(pawns);

    chess::Bitboard right_attacks = color == chess::Color::WHITE 
        ? chess::attacks::pawnRightAttacks<chess::Color::WHITE>(pawns)
        : chess::attacks::pawnRightAttacks<chess::Color::BLACK>(pawns);
        
    // if spanned == false, it means we are getting only squares attacked by two pawns
    chess::Bitboard attacks = spanned ? (left_attacks | right_attacks) : (left_attacks & right_attacks);
    return attacks;
}

int32_t get_distance_between(chess::Square sq1, chess::Square sq2){
    return max(abs(sq1.rank() - sq2.rank()), abs(sq1.file() - sq2.file()));
}

chess::Bitboard get_rammed_pawns(
    chess::Bitboard our_pawns, 
    chess::Bitboard enemy_pawns, 
    chess::Color color){
    return (color == chess::Color::WHITE) 
        ? ((our_pawns << 8) & enemy_pawns) >> 8    
        : ((our_pawns >> 8) & enemy_pawns) << 8;
}

chess::Bitboard get_matching_color_square(int sq_idx){
    
    return ((1ULL << sq_idx) & Constants::WHITE_SQUARES) 
        ? Constants::WHITE_SQUARES 
        : Constants::BLACK_SQUARES;
}

int16_t get_discovered_attacks(
    chess::Bitboard us,
    chess::Bitboard enemy_rooks,
    chess::Bitboard enemy_bishops, 
    chess::Bitboard enemy_queens,
    chess::Color color, 
    chess::Square square){

    int16_t val = 0;
    chess::Bitboard sliding_ops = enemy_rooks | enemy_queens;
    chess::Bitboard occ = sliding_ops | us;
    chess::Bitboard vmob = chess::attacks::rook(square, occ) & ~us;
    val += (sliding_ops & vmob).count();

    sliding_ops ^= enemy_rooks;
    sliding_ops |= enemy_bishops;
    occ = sliding_ops | us;
    vmob = chess::attacks::bishop(square, occ) & ~us;
    val += (sliding_ops & vmob).count();
    return val;
}

chess::Bitboard get_passed_squares(chess::Square sq, chess::Color color){
    return Constants::FORWARD_RANKS.at(~color).at(sq.rank()) 
        & (Constants::ADJACENT_FILES.at(sq.file()) | chess::attacks::MASK_FILE[sq.file()]);
}

int32_t evaluate_pawns(chess::Board &board, chess::Color color, EvalInfo &eval_info){
    int sq_idx;
    int flag;
    int forward = color == chess::Color::WHITE ? 8 : -8;
    chess::Square sq;
    chess::Square rel_sq;
    chess::Bitboard pawns_bb;
    chess::Bitboard our_pawns;
    chess::Bitboard enemy_pawns;
    chess::Bitboard neighbors;
    chess::Bitboard backups;
    chess::Bitboard stoppers;
    chess::Bitboard threats;
    chess::Bitboard supports;
    chess::Bitboard push_threats;
    chess::Bitboard push_supports;
    chess::Bitboard leftovers;
    our_pawns = eval_info.pieces.at(color).at((int)chess::PieceType::PAWN);
    pawns_bb = our_pawns;
    enemy_pawns = eval_info.pieces.at(~color).at((int)chess::PieceType::PAWN);

    const static std::function<chess::Bitboard(chess::Color, chess::Square)> connected_squares = [
    ](chess::Color color, chess::Square sq){
        chess::Bitboard mask = (
              chess::attacks::MASK_FILE[max(0, sq.file() - 1)]
            | chess::attacks::MASK_FILE[min(7, sq.file() + 1)]
        ) & ~chess::attacks::MASK_FILE[sq.file()];
        mask &= chess::attacks::MASK_RANK[sq.rank()];
        mask |= color == chess::Color::WHITE ? mask >> 8 : mask << 8;
        return mask;
    };

    eval_info.double_attacks.at(color) |= (eval_info.pawn_attacks.at(color) & eval_info.attacks.at(color));
    eval_info.piece_attacks.at(color).at((int)chess::PieceType::PAWN) |= eval_info.pawn_attacks.at(color);
    eval_info.attacks.at(color) |= eval_info.pawn_attacks.at(color);
    eval_info.king_attacks_count.at(~color) = (
        eval_info.pawn_attacks.at(color) & eval_info.king_areas.at(~color)
    ).count();

    while(pawns_bb){
        sq_idx = pawns_bb.pop();
        sq = chess::Square(sq_idx);
        rel_sq = sq.relative_square(color);

        neighbors = our_pawns & Constants::ADJACENT_FILES.at(sq.file());
        backups = our_pawns & (
            ~Constants::FORWARD_RANKS.at(color).at(sq.rank()) 
            & (
                Constants::ADJACENT_FILES.at(sq.file()) 
                | chess::attacks::MASK_FILE[sq.file()]
            )
        );
        stoppers = enemy_pawns & (
            ~Constants::FORWARD_RANKS.at(~color).at(sq.rank())
            & (
                Constants::ADJACENT_FILES.at(sq.file()) 
                | chess::attacks::MASK_FILE[sq.file()]
            )
        );
        threats = enemy_pawns & chess::attacks::pawn(color, sq_idx);
        supports = our_pawns & chess::attacks::pawn(~color, sq_idx);
        push_threats = enemy_pawns & chess::attacks::pawn(color, sq_idx + forward);
        push_supports = our_pawns & chess::attacks::pawn(~color, sq_idx + forward);
        leftovers = stoppers ^ threats ^ push_threats;

        // pawn is considered a passed pawn if there are no enemy pawns on the relatively
        // ranks above that can block or attack it if it advances, in otherwords no stoppers
        if(!stoppers){
            eval_info.passed_pawns |= (1ULL << sq_idx);
        }
        
        // Apply bonus to pawns that will be passed by advancing a square
        if(!leftovers && push_supports.count() > push_threats.count()){
            flag = supports.count() >= threats.count();
            eval_info.pk_eval.at(color) += Constants::PAWN_CANDIDATE_PASSER_SCORES.at(flag).at(rel_sq.rank());
        }

        // Apply a penalty for isolated pawns (pawns with no threats nor neighbors)
        if(!neighbors && !threats){
            eval_info.pk_eval.at(color) += Constants::PAWN_ISOLATED_SCORES.at(sq.file());
        }

        // Apply penalty when pawn is stacked (in otherwords, multiple pawns on immediate
        // ranks on one file). The score is adjusted if pawn has the potential to be unstacked
        // In otherwords, if: 1.) the pawn has stopper (a pawn along its left or right immediate 
        // diagonal or its front) and it either has a threat or neighbor OR .2) The pawn has enemy
        // pawns along its adjacent diagonals. This way we can check if the pawn has the potential
        // to get unstacked either by getting captured by enemy threat or by the enemy blocker getting
        // captured by the pawn neighbors.
        if((chess::attacks::MASK_FILE[sq.file()] & our_pawns).count() > 1){
            flag = (stoppers && (threats || neighbors)) 
                || (stoppers & ~(
                    Constants::FORWARD_RANKS.at(color).at(sq.rank()) 
                    & chess::attacks::MASK_FILE[sq.file()]
                ));
            eval_info.pk_eval.at(color) += Constants::PAWN_STACKED_SCORES.at(flag).at(sq.file());
        }

        // Apply a penalty if pawn is backward, a pawn is backward if it has no support or backup.
        // We also ensure that the penalty assigned here does not affect isolated pawns by checking if
        // it has least one neighbor and atleast one push threat (enemy pawns that threaten the
        // rank above the pawn along that file)
        if (neighbors && push_threats && !backups) {
            flag = !(chess::attacks::MASK_FILE[sq.file()] & enemy_pawns);
            eval_info.pk_eval.at(color) += Constants::PAWN_BACKWARDS_SCORES.at(flag).at(rel_sq.rank());
        }
        // Apply bonus if any of the connected squares has a pawn, If so it means a pawn lever is formed
        // given a square, connected squares are immediate squares on adjacent files that are on the left
        // and right of the square rank and the rank below (for white) or above (for black)
        
        // NOTE: You can use Constants::CONNECTED_SQUARES.at(color).at(sq_idx)  in place of connected_squares(color, sq_idx)
        // as long as you uncomment the CONNECTED_SQUARES array in the constants.cpp file
        else if (connected_squares(color, sq_idx) & our_pawns) {
            eval_info.pk_eval.at(color) += 
                Constants::CONNECTED_PAWN_SCORES_32.at(4 * rel_sq.rank() + get_mirror_file(rel_sq.file()));
        }
    }
    return 0;
}

int32_t evaluate_knights(chess::Board &board, chess::Color color, EvalInfo &eval_info){
    int sq_idx;
    int outside;
    int defended;
    int king_dist;
    bool behind_pawn;
    uint32_t eval = 0;
    chess::Bitboard attacks;
    chess::Square sq;
    chess::Square king_sq = eval_info.king_sqs.at(color);
    chess::Square enemy_king_sq = eval_info.king_sqs.at(~color);
    chess::Bitboard our_pawns = eval_info.pieces.at(color).at((int)chess::PieceType::PAWN);
    chess::Bitboard enemy_pawns = eval_info.pieces.at(~color).at((int)chess::PieceType::PAWN);
    chess::Bitboard pawns = our_pawns | enemy_pawns;
    chess::Bitboard knight_bb = eval_info.pieces.at(color).at((int)chess::PieceType::KNIGHT);

    while(knight_bb){
        sq_idx = knight_bb.pop();
        sq = chess::Square(sq_idx);
        attacks = chess::attacks::knight(sq);

        eval_info.double_attacks.at(color) |= (attacks & eval_info.attacks.at(color));
        eval_info.piece_attacks.at(color).at((int)chess::PieceType::KNIGHT) |= attacks;
        eval_info.attacks.at(color) |= attacks;

        // Evaluate knight mobility and apply a bonus or penalty, a knight can have a max
        // mobility value of 8, with an extra value 0 for when it is completely immobile,
        // hence we index the BISHOP_MOBILITY_SCORES array with a moblity value ranging from
        // 0 - 8 to get the assigned mobility score.
        eval += Constants::KNIGHT_MOBILITY_SCORES.at(attacks.count());

        // Apply bonus if knight is on an outpost and cannot be attacked by enemy pawns. An outpost
        // are usually squares in the 4th, 5th and 6th rank for white and 5th, 4th and 3rd rank for
        // black, the bonus is increased if the knight in question is defended by a pawn.
        // Do note that here, outside refers to files A and files H, and defended refers to whether
        // knight is defended by pawn.
        if(     (Constants::OUTPOST_RANKS.at(color) & (1ULL << sq_idx))
            && !(eval_info.pawn_attacks.at(~color) & (1ULL << sq_idx))){
            outside = (bool)((chess::attacks::MASK_FILE[0] | chess::attacks::MASK_FILE[7]) & (1ULL << sq_idx));
            defended = (bool)(eval_info.pawn_attacks.at(color) & (1ULL << sq_idx));
            eval += Constants::KNIHT_OUTPOST_SCORES.at(outside).at(defended);
        }

        // Apply bonus if knight is behind an enemy pawn
        behind_pawn = (color == chess::Color::WHITE)
            ? (bool)((1ULL << sq_idx) & (enemy_pawns << 8))
            : (bool)((1ULL << sq_idx) & (enemy_pawns >> 8));
        if(behind_pawn){
            eval += Constants::KNIGHT_BEHIND_PAWN_BONUS;
        }

        // Apply a penalty if knight is far from both kings
        king_dist = min(get_distance_between(sq, king_sq), get_distance_between(sq, enemy_king_sq));
        if(king_dist >= 4){
            eval += Constants::KNIGHT_IN_SIBERIA_SCORES.at(king_dist - 4);
        }
        
        // Update opponent king safety based on whether they're being attacked by a knight
        if(attacks &= eval_info.king_areas.at(~color) & ~eval_info.double_pawn_attacks.at(~color)){
            eval_info.king_attackers_count.at(~color) += 1;
            eval_info.king_attacks_count.at(~color) += attacks.count();
            eval_info.king_attackers_weight.at(~color) += Constants::KNIGHT_ATTACK_WEIGHT;
        }
    }
    return eval;
}

int32_t evaluate_bishops(chess::Board &board, chess::Color color, EvalInfo &eval_info){
    int sq_idx;
    int outside;
    int defended;
    bool behind_pawn;
    bool controls_middle;
    uint32_t eval = 0;
    chess::Bitboard attacks;
    chess::Square sq;
    chess::Bitboard occ = eval_info.sides.at(color) | eval_info.sides.at(~color);
    chess::Bitboard long_diags = Constants::MASK_DIAGONAL.at(7) | Constants::MASK_ANTIDIAGONAL.at(7);
    chess::Bitboard our_pawns = eval_info.pieces.at(color).at((int)chess::PieceType::PAWN);
    chess::Bitboard enemy_pawns = eval_info.pieces.at(~color).at((int)chess::PieceType::PAWN);
    chess::Bitboard pawns = our_pawns | enemy_pawns;
    chess::Bitboard bishop_bb = eval_info.pieces.at(color).at((int)chess::PieceType::BISHOP);

    // Apply bonus if you have two or more bishops that occupy white and black squares.
    if((bishop_bb & Constants::WHITE_SQUARES) && (bishop_bb & Constants::BLACK_SQUARES)){
        eval += Constants::BISHOP_PAIR_BONUS;
    }
    while(bishop_bb){
        sq_idx = bishop_bb.pop();
        sq = chess::Square(sq_idx);

        attacks = chess::attacks::bishop(sq, occ);
        eval_info.double_attacks.at(color) |= (attacks & eval_info.attacks.at(color));
        eval_info.piece_attacks.at(color).at((int)chess::PieceType::BISHOP) |= attacks;
        eval_info.attacks.at(color) |= attacks;

        // Evaluate bishop mobility and apply a bonus or penalty, a bishop can have a max
        // mobility value of 13, with an extra value 0 for when it is completely immobile,
        // hence we index the BISHOP_MOBILITY_SCORES array with a moblity value ranging from
        // 0 - 13 to get the assigned mobility score.
        eval += Constants::BISHOP_MOBILITY_SCORES.at(attacks.count());

        // apply a penalty based on the number of rammed pawns are in the same square color that
        // this bishop occupies. A rammed pawn is a pawn that has directly been blocked by an
        // enemy pawn, ramming is a mutual obstruction, so it also affects the opponent just as
        // it affects us.
        eval += (
            (get_matching_color_square(sq_idx) & eval_info.rammed_pawns.at(color)).count()
            * Constants::BISHOP_RAMMED_PAWNS_PENALTY
        );

        // Apply bonus if bishop is on an outpost and cannot be attacked by enemy pawns. An outpost
        // are usually squares in the 4th, 5th and 6th rank for white and 5th, 4th and 3rd rank for
        // black, the bonus is increased if the bishop in question is defended by a pawn.
        // Do note that here, outside refers to files A and files H, and defended refers to whether
        // bishop is defended by pawn.
        if(     (Constants::OUTPOST_RANKS.at(color) & (1ULL << sq_idx))
            && !(eval_info.pawn_attacks.at(~color) & (1ULL << sq_idx))
        ){
            outside = (bool)((chess::attacks::MASK_FILE[0] | chess::attacks::MASK_FILE[7]) & (1ULL << sq_idx));
            defended = (bool)(eval_info.pawn_attacks.at(color) & (1ULL << sq_idx));
            eval += Constants::BISHOP_OUTPOST_SCORES.at(outside).at(defended);
        }

        // Apply bonus if bishop is behind an enemy pawn
        behind_pawn = (color == chess::Color::WHITE)
            ? (bool)((1ULL << sq_idx) & (enemy_pawns << 8))
            : (bool)((1ULL << sq_idx) & (enemy_pawns >> 8));
        if(behind_pawn){
            eval += Constants::BISHOP_BEHIND_PAWN_BONUS;
        }

        // Apply bonus if bishop controls the two middle squares along the longest diagonals
        // or longest anti-diagonals
        controls_middle = (
            (bool)((long_diags & ~Constants::MIDDLE_SQUARES_4) & (1ULL << sq_idx))
             && (chess::attacks::bishop(sq, pawns) & Constants::MIDDLE_SQUARES_4).count() > 1);
        if(controls_middle){
            eval += Constants::BISHOP_LONG_DIAG_CENTER_SQUARE_BONUS;
        }
        
        // Update opponent king safety based on whether they're being attacked by a bishop
        if(attacks &= eval_info.king_areas.at(~color) & ~eval_info.double_pawn_attacks.at(~color)){
            eval_info.king_attackers_count.at(~color) += 1;
            eval_info.king_attacks_count.at(~color) += attacks.count();
            eval_info.king_attackers_weight.at(~color) += Constants::BISHOP_ATTACK_WEIGHT;
        }
    }
    return eval;
}

int32_t evaluate_rooks(chess::Board &board, chess::Color color, EvalInfo &eval_info){
    int sq_idx;
    int opened;
    uint32_t eval = 0;
    chess::Bitboard attacks;
    chess::Square sq;
    chess::Bitboard occ = eval_info.sides.at(color) | eval_info.sides.at(~color);
    chess::Square enemy_king_sq = eval_info.king_sqs.at(~color);
    chess::Bitboard our_pawn = eval_info.pieces.at(color).at((int)chess::PieceType::PAWN);
    chess::Bitboard enemy_pawn = eval_info.pieces.at(~color).at((int)chess::PieceType::PAWN);
    chess::Bitboard rook_bb = eval_info.pieces.at(color).at((int)chess::PieceType::ROOK);

    while(rook_bb){
        sq_idx = rook_bb.pop();
        sq = chess::Square(sq_idx);
        
        attacks = chess::attacks::rook(sq, occ);
        eval_info.double_attacks.at(color) |= (attacks & eval_info.attacks.at(color));
        eval_info.piece_attacks.at(color).at((int)chess::PieceType::ROOK) |= attacks;
        eval_info.attacks.at(color) |= attacks;

        // Evaluate rook mobility and apply a bonus or penalty, a rook can have a max
        // mobility value of 14, with an extra value 0 for when it is completely immobile,
        // hence we index the ROOK_MOBILITY_SCORES array with a moblity value ranging from
        // 0 - 14 to get the assigned mobility score.
        eval += Constants::ROOK_MOBILITY_SCORES.at(attacks.count());

        // Apply bonus if rook is on the 7th rank relative to enemy king as long as enemy 
        // king is on rank 8
        if( sq.relative_square(color).rank() == chess::Rank::RANK_7 
            && enemy_king_sq.relative_square(color).rank() == chess::Rank::RANK_8){
            eval += Constants::ROOK_BONUS_ON_SEVENTH;
        }

        // Assign bonus if rook is on opened file or semi-opened file. A file is closed
        // if any of our pawns is on said file, a file is semi-opened if enemy pawns is
        // on said files and opened if otherwise.
        if(!(our_pawn & chess::attacks::MASK_FILE[sq.file()])){
            opened = !(enemy_pawn & chess::attacks::MASK_FILE[sq.file()]);
            eval += Constants::ROOK_OPEN_FILE_SCORES.at(opened);
        }

        // Update opponent king safety based on whether they're being attacked by a rook
        if(attacks &= eval_info.king_areas.at(~color) & ~eval_info.double_pawn_attacks.at(~color)){
            eval_info.king_attackers_count.at(~color) += 1;
            eval_info.king_attacks_count.at(~color) += attacks.count();
            eval_info.king_attackers_weight.at(~color) += Constants::ROOK_ATTACK_WEIGHT;
        }
    }
    return eval;
}

int32_t evaluate_queens(chess::Board &board, chess::Color color, EvalInfo &eval_info){
    int sq_idx;
    chess::Square sq;
    uint32_t eval = 0;
    chess::Bitboard attacks;
    chess::Bitboard us = eval_info.sides.at(color);
    chess::Bitboard them = eval_info.sides.at(~color);
    chess::Bitboard enemy_rooks = eval_info.pieces.at(~color).at((int)chess::PieceType::ROOK);
    chess::Bitboard enemy_bishops = eval_info.pieces.at(~color).at((int)chess::PieceType::BISHOP);
    chess::Bitboard enemy_queens = eval_info.pieces.at(~color).at((int)chess::PieceType::QUEEN);
    chess::Bitboard occ = us | them;
    chess::Bitboard enemy_king = 1ULL << eval_info.king_sqs.at(~color).index();
    chess::Bitboard queen_bb = eval_info.pieces.at(color).at((int)chess::PieceType::QUEEN);

    while(queen_bb){
        sq_idx = queen_bb.pop();
        sq = chess::Square(sq_idx);
        attacks = chess::attacks::queen(sq, occ);
        eval_info.double_attacks.at(color) |= (attacks & eval_info.attacks.at(color));
        eval_info.piece_attacks.at(color).at((int)chess::PieceType::QUEEN) |= attacks;
        eval_info.attacks.at(color) |= attacks;
        
        // Evaluate queen mobility, queen mobility is a combination of Rook and bishop 
        // mobility, as such the max queen mobility is 27. with an extra moblilty of 0
        // for when it is completely immobile, hence we index the QUEEN_MOBILITY_SCORES
        // array with a mobility value ranging from 0 - 27 to get the assigned mobility 
        // scores.
        eval += Constants::QUEEN_MOBILITY_SCORES.at(attacks.count());

        // Apply a penalty for discovered attacks against queen
        if(get_discovered_attacks(us, enemy_rooks, enemy_bishops, enemy_queens, color, sq) > 0){
            eval += Constants::QUEEN_RELATIVE_PIN_PENALTY;
        }

        // Update opponent king safety based on whether they're being attacked by a queen
        if(attacks &= eval_info.king_areas.at(~color) & ~eval_info.double_pawn_attacks.at(~color)){
            eval_info.king_attackers_count.at(~color) += 1;
            eval_info.king_attacks_count.at(~color) += attacks.count();
            eval_info.king_attackers_weight.at(~color) += Constants::QUEEN_ATTACK_WEIGHT;
        }
    }
    return eval;
}

int32_t evaluate_king(chess::Board &board, chess::Color color, EvalInfo &eval_info){
    int sq_idx;
    int32_t eval = 0;
    int32_t king_threat;
    int16_t mg;
    int16_t eg;
    chess::Square sq;
    chess::Square king_sq = eval_info.king_sqs.at(color);
    chess::Bitboard weak_to_us;
    chess::Bitboard safe_from_us;
    chess::Bitboard defenders = (
        eval_info.pieces.at(color).at((int)chess::PieceType::PAWN)
        | eval_info.pieces.at(color).at((int)chess::PieceType::KNIGHT)
        | eval_info.pieces.at(color).at((int)chess::PieceType::BISHOP)
    );
    chess::Bitboard enemy_queens = eval_info.pieces.at(~color).at((int)chess::PieceType::QUEEN);
    chess::Bitboard king_bb = 1ULL << sq_idx;

    // In the endgame, the king should more or less have as much mobility as possible to avoid checks
    // So the defense count only really matters in the middle game, if not this would have been
    // eval += make_score(defense_count, defense_count) instead of eval += defense_count
    eval += (eval_info.king_areas.at(color) & defenders).count();

    if(    eval_info.king_attackers_count.at(color) >= 2
        || (eval_info.king_attackers_count.at(color) == 1 && enemy_queens.count() >= 1)){
        // Weak squares are defended only once and by the queen or king, 
        // and attacked by the opponent
        weak_to_us = eval_info.attacks.at(~color)
            & ~eval_info.double_attacks.at(color)
            & (
                ~eval_info.attacks.at(color) 
                | eval_info.piece_attacks.at(color).at((int)chess::PieceType::KING)
                | eval_info.piece_attacks.at(color).at((int)chess::PieceType::QUEEN)
            );

        // safe squares in this case are empty squares that are safe from us (we do not attack 
        // or threaten) that the enemy can occupy
        safe_from_us = ~eval_info.sides.at(~color) & (
            ~eval_info.attacks.at(color) | (weak_to_us & eval_info.double_attacks.at(~color))
        );

        int scaled_attack_counts = 9 * (
            eval_info.king_attackers_count.at(color) / eval_info.king_areas.at(color).count()
        );

        // threats of each piece to king (usually for opponents)
        chess::Bitboard occ = board.occ();
        chess::Bitboard knight_threats = chess::attacks::knight(king_sq);
        chess::Bitboard bishop_threats = chess::attacks::bishop(king_sq, occ);
        chess::Bitboard rook_threats = chess::attacks::rook(king_sq, occ);
        chess::Bitboard queen_threats = bishop_threats | rook_threats;

        // Identify squares that the enemy can move to to check our king
        chess::Bitboard knight_check = knight_threats 
            & safe_from_us & eval_info.piece_attacks.at(~color).at((int)chess::PieceType::KNIGHT);
        chess::Bitboard bishop_check = bishop_threats 
            & safe_from_us & eval_info.piece_attacks.at(~color).at((int)chess::PieceType::BISHOP);
        chess::Bitboard rook_check = rook_threats 
            & safe_from_us & eval_info.piece_attacks.at(~color).at((int)chess::PieceType::ROOK);
        chess::Bitboard queen_check = queen_threats 
            & safe_from_us & eval_info.piece_attacks.at(~color).at((int)chess::PieceType::QUEEN);

        king_threat = eval_info.king_attackers_weight.at(color);

        king_threat += (Constants::THREAT_ATTACK_VALUES  * scaled_attack_counts)
                    + (Constants::THREAT_WEAK_SQUARES   * (weak_to_us & eval_info.king_areas.at(color)).count())
                    + (Constants::THREAT_NO_ENEMY_QUEEN * !enemy_queens)
                    + (Constants::THREAT_SAFE_QUEEN_CHECK  * queen_check.count())
                    + (Constants::THREAT_SAFE_ROOK_CHECK   * rook_check.count())
                    + (Constants::THREAT_SAFE_BISHOP_CHECK * bishop_check.count())
                    + (Constants::THREAT_SAFE_KNIGHT_CHECK * knight_check.count())
                    + eval_info.pk_safety.at(color);
                    + Constants::THREAT_ADJUSTMENT;

        mg = mg_score(king_threat);
        eg = eg_score(king_threat);
        eval += make_score(-mg * max(0, mg) / 720, -max(0, eg) / 20);
    }

    return eval;
}

int32_t evaluate_king_pawns(chess::Board &board, chess::Color color, EvalInfo &eval_info){
    int our_dist;
    int their_dist;
    int blocked;
    chess::Bitboard ours;
    chess::Bitboard theirs;
    chess::Square king_sq = eval_info.king_sqs.at(color);
    chess::File king_file = king_sq.file();
    chess::Bitboard our_pawns = eval_info.pieces.at(color).at((int)chess::PieceType::PAWN);
    chess::Bitboard enemy_pawns = eval_info.pieces.at(~color).at((int)chess::PieceType::PAWN);
    chess::Bitboard pawns = our_pawns | enemy_pawns;

    const static std::function<chess::Rank(chess::Bitboard, chess::Color)> backmost_rank = [
    ](chess::Bitboard bb, chess::Color color){
        return chess::Square(color == chess::Color::WHITE ? bb.lsb() : bb.msb()).rank();
    };

    // evaluate pawn file proximity based on the distance between the king file and the nearest
    // pawn file, regardless of color of pawns. If there are no pawns, this score is the same
    // for both kings
    for(int dist=1; dist <= 7; dist++){
        if(    (pawns & chess::attacks::MASK_FILE[min(7, king_file + dist)]) 
            || (pawns & chess::attacks::MASK_FILE[max(0, king_file - dist)])){
                eval_info.pk_eval.at(color) += Constants::KING_PAWN_FILE_PROXIMITY_SCORES.at(dist);
                break;
        }
        if(dist == 7){
            eval_info.pk_eval.at(color) += Constants::KING_PAWN_FILE_PROXIMITY_SCORES.at(dist);
        }
    }
    
    for(int file_idx=max(0, king_file-1); file_idx <= min(7, king_file+1); file_idx++){
        // compute distance between our king and the our rank-wise closest pawn. 
        // If none of our pawns are present in the above ranks, the distance is set
        // to 7 (the max possible distance in this context)
        ours = our_pawns 
             & chess::attacks::MASK_FILE[file_idx] 
             & Constants::FORWARD_RANKS.at(color).at(king_sq.rank());
        our_dist = !ours ? 7 : abs(king_sq.rank() - backmost_rank(ours, color));

        // compute distance between our king and closest rank-wise enemy pawn.
        theirs = enemy_pawns 
               & chess::attacks::MASK_FILE[file_idx]
               & Constants::FORWARD_RANKS.at(color).at(king_sq.rank());
        their_dist = !theirs ? 7 : abs(king_sq.rank() - backmost_rank(theirs, color));

        // king shelter is evaluated based on whether the file and the rank-wise distance between king
        // and friendly pawns
        eval_info.pk_eval.at(color) += Constants::KING_SHELTER_SCORES.at(file_idx==king_file).at(file_idx).at(our_dist);

        // shelter safety is also evaluated based on whether the file is same as the king file and the
        // distance between king and friendly pawns 
        eval_info.pk_safety.at(color) += Constants::SAFETY_SHELTER_SCORES.at(file_idx==king_file).at(our_dist);
        
        // we also evaluate king storm (a threat to our king by enemy pawns) in a similar fashion, using the
        // enemy pawns rank-wise distance. Here, `blocked` symbolises whether our pawns are being blocked 
        // by enemy pawns one rank ahead, the evaluation depends this status.
        blocked = (our_dist != 7) && (our_dist == their_dist - 1);
        eval_info.pk_eval.at(color) += Constants::KING_STORM_SCORES.at(blocked).at(get_mirror_file(file_idx)).at(their_dist);

        // storm safety is evaluated based on the blocked status and the distance of enemy pawns from us.
        eval_info.pk_safety.at(color) += Constants::SAFETY_STORM_SCORES.at(blocked).at(their_dist);
    }
    
    return 0;
}

int32_t evaluate_passed_pawns(chess::Board &board, chess::Color color, EvalInfo &eval_info){
    int sq_idx;
    int can_advance;
    int safe_to_advance;
    int safe_advance;
    bool several_passers_on_file;
    int32_t eval = 0;
    chess::Square sq;
    chess::Square rel_sq;
    chess::Square our_king_sq = eval_info.king_sqs.at(color);
    chess::Square enemy_king_sq = eval_info.king_sqs.at(~color);
    chess::Bitboard forward_file_mask;
    chess::Bitboard advanced_pawn;
    chess::Bitboard us = eval_info.sides.at(color);
    chess::Bitboard them = eval_info.sides.at(~color);
    chess::Bitboard occ = us | them;
    chess::Bitboard passed_pawns_bb = eval_info.passed_pawns 
        & eval_info.pieces.at(color).at((int)chess::PieceType::PAWN);
    chess::Bitboard temp_bb = passed_pawns_bb;

    while(temp_bb){
        sq_idx = temp_bb.pop();
        sq = chess::Square(sq_idx);
        rel_sq = sq.relative_square(color);

        advanced_pawn = color == chess::Color::WHITE ? (1ULL << sq_idx) << 8 : (1ULL << sq_idx) >> 8;
        
        // you may be wondering why we are evaluating if a pawn can advance and if it is safe to advance
        // despite already establishing that fact already in the pawn evaluation function.
        // Remember from the pawn evaluation function that an advanced pawn is a pawn that has no enemy
        // pawns blocking it or capable of attacking it in the ranks relatively above it, that being said
        // a passed pawn can still be blocked by a friendly pawn (which was not accounted for prior), it
        // is also possible for the advance squares to be attacked by other pieces that are not pawns.
        can_advance = !(advanced_pawn & occ);
        safe_to_advance = !(advanced_pawn & eval_info.attacks.at(~color));
        eval += Constants::PASSED_PAWN_SCORES.at(can_advance).at(safe_to_advance).at(rel_sq.rank());

        // if several passed pawns are on the same file, skip further the evaluations
        forward_file_mask = Constants::FORWARD_RANKS.at(color).at(sq.rank()) & chess::attacks::MASK_FILE[sq.file()];
        several_passers_on_file = (forward_file_mask & passed_pawns_bb).count() > 1;
        if(several_passers_on_file){
            continue;
        }

        // apply bonus if path to promotion is uncontested, by uncontested, we mean not piece is along the 
        // file our pawn is about to take to promote, or no piece can attack along that file
        if(!(forward_file_mask & (them | eval_info.attacks.at(~color)))){
            eval += Constants::PASSED_SAFE_PROMOTION_PATH_BONUS;
        }

        // Evaluate based on distance from our king
        eval += get_distance_between(sq, our_king_sq) 
            * Constants::PASSED_FRIENDLY_DISTANCE_SCORES.at(rel_sq.rank());

        // Evaluate based on distance from enemy king
        eval += get_distance_between(sq, enemy_king_sq) 
            * Constants::PASSED_ENEMY_DISTANCE_SCORES.at(rel_sq.rank());
    }
    return eval;
}

int32_t evaluate_threats(chess::Board &board, chess::Color color, EvalInfo &eval_info){
    int32_t eval = 0;

    chess::Bitboard rel_rank_3 = color == chess::Color::WHITE 
        ? chess::attacks::MASK_RANK[2] 
        : chess::attacks::MASK_RANK[5];

    chess::Bitboard us = eval_info.sides.at(color);
    chess::Bitboard them = eval_info.sides.at(~color);
    chess::Bitboard occ = us | them;

    int white = (int)chess::Color::WHITE;
    int black = (int)chess::Color::BLACK;

    chess::Bitboard pawns = eval_info.pieces.at(white).at((int)chess::PieceType::PAWN) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::PAWN);

    chess::Bitboard knights = eval_info.pieces.at(white).at((int)chess::PieceType::KNIGHT) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::KNIGHT);

    chess::Bitboard bishops = eval_info.pieces.at(white).at((int)chess::PieceType::BISHOP) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::BISHOP);

    chess::Bitboard rooks = eval_info.pieces.at(white).at((int)chess::PieceType::ROOK) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::ROOK);

    chess::Bitboard queens = eval_info.pieces.at(white).at((int)chess::PieceType::QUEEN) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::QUEEN);

    chess::Bitboard attacks_by_pawns = eval_info.piece_attacks.at(~color).at((int)chess::PieceType::PAWN);
    chess::Bitboard attacks_by_minors = (
        eval_info.piece_attacks.at(~color).at((int)chess::PieceType::KNIGHT) 
        | eval_info.piece_attacks.at(~color).at((int)chess::PieceType::BISHOP)
    );
    chess::Bitboard attacks_by_majors = (
        eval_info.piece_attacks.at(~color).at((int)chess::PieceType::ROOK) 
        | eval_info.piece_attacks.at(~color).at((int)chess::PieceType::QUEEN)
    );

    chess::Bitboard poorly_defended = (
        eval_info.attacks.at(~color) & ~eval_info.attacks.at(color)
        | (
               eval_info.double_attacks.at(~color) 
            & ~eval_info.double_attacks.at(color) 
            & ~eval_info.piece_attacks.at(color).at((int)chess::PieceType::PAWN)
        )
    );

    chess::Bitboard weak_minors = (knights | bishops) & poorly_defended;

    chess::Bitboard overloaded = (
          (eval_info.attacks.at(color) & ~eval_info.double_attacks.at(color)) 
        & (eval_info.attacks.at(~color) & ~eval_info.double_attacks.at(~color))
        & (knights | bishops | rooks | queens)
    );

    // push threats are threats that we cause to our enemies as we advance the given file from
    // relative ranks 2 to 3 to 4.
    // Here we do not consider pieces that we already attack prior to computing the push threat
    // nor enemy pieces that are defended by pawns.
    chess::Bitboard push_threats = get_pawn_advance(occ, pawns, color, 1);
    push_threats |= get_pawn_advance(occ, (push_threats & ~attacks_by_pawns & rel_rank_3), color, 1);
    push_threats &= ~attacks_by_pawns & (eval_info.attacks.at(color) | eval_info.attacks.at(~color));
    if(color == chess::Color::WHITE){
        push_threats = (
              (them & ~eval_info.piece_attacks.at(color).at((int)chess::PieceType::PAWN)) 
            & (chess::attacks::pawnLeftAttacks<chess::Color::WHITE>(push_threats) 
                | chess::attacks::pawnRightAttacks<chess::Color::WHITE>(push_threats))
        );
    }
    else{
        push_threats = (
              (them & ~eval_info.piece_attacks.at(color).at((int)chess::PieceType::PAWN)) 
            & (chess::attacks::pawnLeftAttacks<chess::Color::BLACK>(push_threats) 
                | chess::attacks::pawnRightAttacks<chess::Color::BLACK>(push_threats))
        );
    }

    // apply penalty for poorly supported pawns
    eval += (pawns & ~attacks_by_pawns & poorly_defended).count() * Constants::THREAT_WEAK_PAWN;

    // apply penalty for pawn threats against our minor
    eval += ((knights | bishops) & attacks_by_pawns).count() * Constants::THREAT_MINOR_ATTACKED_BY_PAWNS;

    // apply penalty for minor threats against our minors
    eval += ((knights | bishops) & attacks_by_minors).count() * Constants::THREAT_MINOR_ATTACKED_BY_MINORS;

    // apply penalty for major threats against our poorly supported / weak minors
    eval += (weak_minors & attacks_by_majors).count() * Constants::THREAT_MINOR_ATTACKED_BY_MAJORS;

    // apply penalty for pawn and minor threats against our rook
    eval += (rooks & (attacks_by_pawns | attacks_by_minors)).count() * Constants::THREAT_ROOK_ATTACKED_BY_LESSER;

    // apply penalty for king threats against our poorly defended minors
    eval += (weak_minors & eval_info.piece_attacks.at(~color).at((int)chess::PieceType::KING)).count()
         * Constants::THREAT_MINOR_ATTACKED_BY_KING;

    // apply penalty for king threats against our poorly defended rooks
    eval += ((rooks & poorly_defended) & eval_info.piece_attacks.at(~color).at((int)chess::PieceType::KING)).count()
         * Constants::THREAT_ROOK_ATTACKED_BY_KING;

    // apply penalty for any threat against our queen
    eval += (queens & eval_info.attacks.at(~color)).count() * Constants::THREAT_QUEEEN_ATTACKED_BY_ANYONE;

    // apply penalty for any overloaded majors or minors
    eval += overloaded.count() * Constants::THREAT_OVERLOAD_PIECES;

    // apply bonus for threatening enemy by our safe pawn push
    eval += push_threats.count() * Constants::THREAT_BY_OUR_PAWN_PUSH;

    return eval;    
}

int32_t evaluate_space(chess::Board &board, chess::Color color, EvalInfo &eval_info){
    int32_t eval = 0;

    chess::Bitboard us = eval_info.sides.at(color);
    chess::Bitboard them = eval_info.sides.at(~color);

    int white = (int)chess::Color::WHITE;
    int black = (int)chess::Color::BLACK;

    chess::Bitboard knights = eval_info.pieces.at(white).at((int)chess::PieceType::KNIGHT) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::KNIGHT);

    chess::Bitboard bishops = eval_info.pieces.at(white).at((int)chess::PieceType::BISHOP) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::BISHOP);

    chess::Bitboard rooks = eval_info.pieces.at(white).at((int)chess::PieceType::ROOK) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::ROOK);

    chess::Bitboard queens = eval_info.pieces.at(white).at((int)chess::PieceType::QUEEN) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::QUEEN);
    
    // uncontrolled squares are squares that we attack once and have more enemy attackers,
    // for these squares we do not attack twice and we do not attack with pawns either.
    chess::Bitboard uncontrolled_sqs = eval_info.double_attacks.at(~color) 
            & eval_info.attacks.at(color)
            & ~eval_info.double_attacks.at(color) 
            & ~eval_info.piece_attacks.at(color).at((int)chess::PieceType::PAWN);

    // for the uncontrolled squares, it would be threatening if our pieces were on those 
    // squares as they could get captured, it could also be threatening if enemy pieces 
    // are on those squares as they are defended by the enemy, hence a penalty is assigned
    // based on how many of our pieces or enemy pieces are on these uncontrolled squares.
    eval += (uncontrolled_sqs & (us | them)).count() * Constants::SPACE_RESTRICTED_PIECE;

    // Apply penalty (lesser than prior) for spaces that are neither occupied by us nor the
    // enemy, because even if they are not occupied, the fact that we have less control over
    // those squares than the enemy still puts us at a disadvantage.
    eval += (uncontrolled_sqs & (~us & ~them)).count() * Constants::SPACE_RESTRICTED_EMPTY;
    
    // Apply bonus for the number of uncontested central squares, these are squares in the 
    // 16 central that are not attacked by enemy and are eiher occupied or attacked by our piece. 
    // This is a mostly a beginning and middle game metric, so we simply include this evaluation
    // until number of pieces is less than some threshold
    if((knights | bishops).count() + (2 * (rooks | queens).count()) > 12){
        eval += (
            ~eval_info.attacks.at(~color)
            & (eval_info.attacks.at(color) | us) 
            & Constants::MIDDLE_SQUARES_16
        ).count() * Constants::SPACE_CENTER_CONTROL;
    }
    return eval;
}

int32_t evaluate_closedness(chess::Board &board, EvalInfo &eval_info){
    int count;
    int closedness;
    int32_t eval = 0;
    int white = (int)chess::Color::WHITE;
    int black = (int)chess::Color::BLACK;

    chess::Bitboard pawns = eval_info.pieces.at(white).at((int)chess::PieceType::PAWN)
                          | eval_info.pieces.at(black).at((int)chess::PieceType::PAWN);

    const static std::function<int(chess::Bitboard)> count_free_files = [](chess::Bitboard pawns){
        pawns |= pawns << 8;
        pawns |= pawns << 16;
        pawns |= pawns << 32;
        return (~pawns & chess::attacks::MASK_RANK[7]).count();
    };
    
    // The idea of CLOSEDNESS refers to how squares are blocked by panws in a
    // given position. In a very closed position, knights thrive better than rooks
    // because they can jump over obstructive pawns, on the other hand, rooks thrive
    // better than knights in an opened position. This evaluation function calculates the
    // closedness values and retrieves the corresponding knights and rooks adjustments
    // based on how closed the position is. We expect that the more closed a position
    // is, the more the closedness adjustment value for the knight and less for the rook,
    // and the less closed the position is, the more the adjustment value for the rook is...

    // The more the pawns and the more rammed the pawns are in a given position, the more closed
    // that position is. Remember that ramming is a mutual obstruction, so we need not evaluate 
    // the rammed status for both sides, just one side is enough, since we are just counting the 
    // number of ramming obstructions. The more files that unoccupied by pawns, the less closed
    // or more opened said position is, hence the reason the `closedness` value is computed this way
    closedness = (1 * pawns.count())
               + (3 * eval_info.rammed_pawns.at((int)chess::Color::WHITE).count())
               - (4 * count_free_files(pawns));
    closedness = max(0, min(8, closedness / 3));

    count = eval_info.pieces.at(white).at((int)chess::PieceType::KNIGHT).count()
          - eval_info.pieces.at(black).at((int)chess::PieceType::KNIGHT).count();
    eval += count * Constants::CLOSEDNESS_KNIGHT_ADJUSTMENT.at(closedness);

    count = eval_info.pieces.at(white).at((int)chess::PieceType::ROOK).count()
          - eval_info.pieces.at(black).at((int)chess::PieceType::ROOK).count();
    eval += count * Constants::CLOSEDNESS_ROOK_ADJUSTMENT.at(closedness);

    return eval;
}

int32_t evaluate_complexity(chess::Board &board, EvalInfo &eval_info, int32_t eval){
    // this function computes a metric that describes how likely the side with the advantage
    // is to win / turn the game.
    int32_t complexity;
    int16_t eg = eg_score(eval);

    // this sign indicates who has the advantage in the endgame, white has the advantage if positive
    // else black has the advantage. If both sides are evenly matched at the end game, the sign is 0
    // hence this entire function will evaluate to 0 at endgame
    int sign = (eg > 0) - (eg < 0);

    int white = (int)chess::Color::WHITE;
    int black = (int)chess::Color::BLACK;

    chess::Bitboard pawns = eval_info.pieces.at(white).at((int)chess::PieceType::PAWN) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::PAWN);

    chess::Bitboard knights = eval_info.pieces.at(white).at((int)chess::PieceType::KNIGHT) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::KNIGHT);

    chess::Bitboard bishops = eval_info.pieces.at(white).at((int)chess::PieceType::BISHOP) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::BISHOP);

    chess::Bitboard rooks = eval_info.pieces.at(white).at((int)chess::PieceType::ROOK) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::ROOK);

    chess::Bitboard queens = eval_info.pieces.at(white).at((int)chess::PieceType::QUEEN) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::QUEEN);

    int pawn_on_flanks = (pawns & Constants::LEFT_FLANK) && (pawns & Constants::RIGHT_FLANK);

    complexity  = (Constants::COMPLEXITY_TOTAL_PAWNS * pawns.count())
                + (Constants::COMPLEXITY_PAWN_FLANKS * pawn_on_flanks)
                + (Constants::COMPLEXITY_PAWN_ENDGAME * !(knights | bishops | rooks | queens))
                + (Constants::COMPLEXITY_ADJUSTMENT);
    
    // to ensure that the evaluation does not overly get penalized for cases where pawns are few to none
    // and / or no side has pawns on both flanks, and / or side has major or minor pieces, we cap the 
    // complexity to have a lower bound of -abs(eg)
    eval = sign * max(eg_score(complexity), -abs(eg));
    
    return make_score(0, eval);
}

int32_t evaluate_scalefactor(chess::Board &board, EvalInfo &eval_info, int32_t eval){

    int16_t eg = eg_score(eval);
    int white = (int)chess::Color::WHITE;
    int black = (int)chess::Color::BLACK;

    chess::Bitboard pawns = eval_info.pieces.at(white).at((int)chess::PieceType::PAWN) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::PAWN);

    chess::Bitboard knights = eval_info.pieces.at(white).at((int)chess::PieceType::KNIGHT) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::KNIGHT);

    chess::Bitboard bishops = eval_info.pieces.at(white).at((int)chess::PieceType::BISHOP) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::BISHOP);

    chess::Bitboard rooks = eval_info.pieces.at(white).at((int)chess::PieceType::ROOK) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::ROOK);

    chess::Bitboard queens = eval_info.pieces.at(white).at((int)chess::PieceType::QUEEN) 
                          | eval_info.pieces.at(black).at((int)chess::PieceType::QUEEN);

    chess::Bitboard minors = knights | bishops;
    chess::Bitboard t_pieces = minors | rooks;

    chess::Bitboard white_pieces = eval_info.sides.at((int)chess::Color::WHITE);
    chess::Bitboard black_pieces = eval_info.sides.at((int)chess::Color::WHITE);

    chess::Bitboard weak_side = eg < 0 ? white_pieces : black_pieces;
    chess::Bitboard strong_side = eg < 0 ? black_pieces : white_pieces;

    // check if both black and white sides have one bishops each and them lye on 
    // opposite colored squares (OCB = Opposite Colored Bishops)
    if(    (white_pieces & bishops).count() == 1
        && (black_pieces & bishops).count() == 1
        && (bishops & Constants::WHITE_SQUARES).count() == 1){

        // scale factor if both sides have only one knight each with no rooks and queens
        if(   !(rooks | queens) 
            && (white_pieces & knights).count() == 1 
            && (black_pieces & knights).count() == 1){
            return Constants::SCALE_OCB_ONE_KNIGHT;
        }

        // scale factor if both sides have only one rook each with no knight and queens
        else if(!(knights | queens) 
            && (white_pieces & rooks).count() == 1 
            && (black_pieces & rooks).count() == 1){
            return Constants::SCALE_OCB_ONE_ROOK;
        }

        // scale factor if not major or minor pieces except the opposite colored bishops 
        // (and maybe pawns)
        else if(!(knights | rooks | queens)){
            return Constants::SCALE_OCB_BISHOPS_ONLY;;
        }
    }

    // scale factor if there is only one queen on the board, with multiple other pieces (knights, bishops 
    // and / or rooks) belonging to the weaker side (based on static evaluation). The intuition here is
    // that lone queens are weak against multiple enemy major and minor pieces (even if the enemy does not
    // necessarily have a queen)
    else if(queens.count() == 1 && t_pieces.count() > 1 && t_pieces == (weak_side & t_pieces)){
        return Constants::SCALE_LONE_QUEEN;
    }

    // scale factor if the strong side has only two pieces and one of those pieces is a minor (knight or bishop)
    // (with the other obviously being a king), then the weak side would have only a king and at least one pawn,
    // (in otherwords king + at least one pawn vs king + lone minor). In such cases, it is safe to say that the 
    // game will end in a draw.
    else if((strong_side & minors) && strong_side.count() == 2){
        return Constants::SCALE_DRAW;
    }

    // scale factor if bothsides have no queens and bothsides have at most one non-pawn piece (asides the king)
    // and the strong side has three or more pawns than the weak side (in otherwords a battle of lone non-queen 
    // pieces and pawn advances) with the weak side being at a pawn disadvantage.
    else if(!queens
        && (t_pieces & white_pieces).count() <= 1 
        && ((t_pieces & black_pieces).count() <= 1)
        && ((strong_side & pawns).count() - (weak_side & pawns).count() > 2)){
        return Constants::SCALE_LARGE_PAWN_ADVANCE;
    }

    // scale down as the number of pawns of the strong side diminishes, but ensure that the scale value does not
    // go below a certain bound.
    return min(Constants::SCALE_NORMAL, 96 + ((pawns & strong_side).count() * 8));
}

int32_t evaluate_material_balance(chess::Board &board){
    int32_t eval = 0;
    chess::Piece p;
    chess::Bitboard occ = board.occ();

    for(int i = 0; i < 64; i++){
        if(!occ.check(i)){continue;}
        eval += Constants::PIECE_WEIGHT.at(board.at<chess::Piece>(chess::Square(i)));
    }
    return eval;
}

void init_eval_info(chess::Board &board, EvalInfo &eval_info){
    chess::Color white("w");
    chess::Color black("b");

    eval_info.sides.at(white) = board.us(white);
    eval_info.sides.at(black) = board.us(black);

    int pawn_ = (int)chess::PieceType::PAWN;
    int knight_ = (int)chess::PieceType::KNIGHT;
    int bishop_ = (int)chess::PieceType::BISHOP;
    int rook_ = (int)chess::PieceType::ROOK;
    int queen_ = (int)chess::PieceType::QUEEN;
    int king_ = (int)chess::PieceType::KING;

    eval_info.pieces.at(white).at(pawn_) = board.pieces(chess::PieceType::PAWN, white);
    eval_info.pieces.at(white).at(knight_) = board.pieces(chess::PieceType::KNIGHT, white);
    eval_info.pieces.at(white).at(bishop_) = board.pieces(chess::PieceType::BISHOP, white);
    eval_info.pieces.at(white).at(rook_) = board.pieces(chess::PieceType::ROOK, white);
    eval_info.pieces.at(white).at(queen_) = board.pieces(chess::PieceType::QUEEN, white);
    eval_info.pieces.at(white).at(king_) = board.pieces(chess::PieceType::KING, white);

    eval_info.pieces.at(black).at(pawn_) = board.pieces(chess::PieceType::PAWN, black);
    eval_info.pieces.at(black).at(knight_) = board.pieces(chess::PieceType::KNIGHT, black);
    eval_info.pieces.at(black).at(bishop_) = board.pieces(chess::PieceType::BISHOP, black);
    eval_info.pieces.at(black).at(rook_) = board.pieces(chess::PieceType::ROOK, black);
    eval_info.pieces.at(black).at(queen_) = board.pieces(chess::PieceType::QUEEN, black);
    eval_info.pieces.at(black).at(king_) = board.pieces(chess::PieceType::KING, black);

    eval_info.king_sqs.at(white) = board.kingSq(white);
    eval_info.king_sqs.at(black) = board.kingSq(black);

    eval_info.rammed_pawns.at(white) = get_rammed_pawns(
        eval_info.pieces.at(white).at(pawn_), 
        eval_info.pieces.at(black).at(pawn_), 
        white
    );
    eval_info.rammed_pawns.at(black) = get_rammed_pawns(
        eval_info.pieces.at(black).at(pawn_), 
        eval_info.pieces.at(white).at(pawn_), 
        black
    );

    eval_info.pawn_attacks.at(white) = get_all_pawn_attacks(eval_info.pieces.at(white).at(pawn_), white, true);
    eval_info.pawn_attacks.at(black) = get_all_pawn_attacks(eval_info.pieces.at(black).at(pawn_), black, true);

    eval_info.double_pawn_attacks.at(white) = get_all_pawn_attacks(eval_info.pieces.at(white).at(pawn_), white, false);
    eval_info.double_pawn_attacks.at(black) = get_all_pawn_attacks(eval_info.pieces.at(black).at(pawn_), black, false);

    eval_info.king_areas.at(white) = get_king_areas(white, eval_info.king_sqs.at(white));
    eval_info.king_areas.at(black) = get_king_areas(black, eval_info.king_sqs.at(black));

    eval_info.pk_eval.at(white) = 0;
    eval_info.pk_eval.at(black) = 0;

    eval_info.pk_safety.at(white) = 0;
    eval_info.pk_safety.at(black) = 0;

    eval_info.king_attacks_count.at(white) = 0;
    eval_info.king_attacks_count.at(black) = 0;

    eval_info.king_attackers_count.at(white) = 0;
    eval_info.king_attackers_count.at(black) = 0;

    eval_info.king_attackers_weight.at(white) = 0;
    eval_info.king_attackers_weight.at(black) = 0;

    eval_info.piece_attacks.at(white).at(king_) = chess::attacks::king(eval_info.king_sqs.at(white));
    eval_info.piece_attacks.at(black).at(king_) = chess::attacks::king(eval_info.king_sqs.at(black));

    eval_info.passed_pawns = 0ULL;
}

int16_t evaluate_board_state(chess::Board &board){
    int8_t phase;
    int32_t factor;
    int32_t eval = 0;
    EvalInfo eval_info;
    init_eval_info(board, eval_info);

    int pawn_ = (int)chess::PieceType::PAWN;
    int knight_ = (int)chess::PieceType::KNIGHT;
    int bishop_ = (int)chess::PieceType::BISHOP;
    int rook_ = (int)chess::PieceType::ROOK;
    int queen_ = (int)chess::PieceType::QUEEN;
    int king_ = (int)chess::PieceType::KING;

    chess::Color white("w");
    chess::Color black("b");

    eval += evaluate_pawns(board, white, eval_info) - evaluate_pawns(board, black, eval_info);  
    // // king pawns needs to be done after pawn evaluation and before king evaluation
    eval += evaluate_king_pawns(board, white, eval_info) - evaluate_king_pawns(board, black, eval_info); 
    eval += evaluate_knights(board, white, eval_info) - evaluate_knights(board, black, eval_info);   
    eval += evaluate_bishops(board, white, eval_info) - evaluate_bishops(board, black, eval_info);
    eval += evaluate_rooks(board, white, eval_info) - evaluate_rooks(board, black, eval_info);  
    eval += evaluate_queens(board, white, eval_info) - evaluate_queens(board, black, eval_info); 
    eval += evaluate_king(board, white, eval_info) - evaluate_king(board, black, eval_info);
    eval += evaluate_passed_pawns(board, white, eval_info) - evaluate_passed_pawns(board, black, eval_info); 
    eval += evaluate_threats(board, white, eval_info) - evaluate_threats(board, black, eval_info);
    eval += evaluate_space(board, white, eval_info) - evaluate_space(board, black, eval_info);

    eval += (eval_info.pk_eval.at(white) - eval_info.pk_eval.at(black)) + evaluate_material_balance(board);

    eval += evaluate_closedness(board, eval_info);

    eval += evaluate_complexity(board, eval_info, eval);

    factor = evaluate_scalefactor(board, eval_info, eval);

    // calculate game phase with Fruit method
    phase =  4 * (eval_info.pieces.at(white).at(queen_) | eval_info.pieces.at(black).at(queen_)).count();
    phase += 2 * (eval_info.pieces.at(white).at(rook_) | eval_info.pieces.at(black).at(rook_)).count();
    phase += 1 * ((eval_info.pieces.at(white).at(knight_) | eval_info.pieces.at(black).at(knight_))
               | (eval_info.pieces.at(white).at(bishop_)  | eval_info.pieces.at(black).at(bishop_))).count();

    eval = ((mg_score(eval) * phase)
         + eg_score(eval) * (24 - phase) * (factor / Constants::SCALE_NORMAL)) / 24;
         
    return eval;
}
#endif