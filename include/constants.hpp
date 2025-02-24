#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include "types.hpp"
#include "../extern/chess.hpp"

inline constexpr int32_t make_score(int16_t mg, int16_t eg){
    return static_cast<int32_t>(static_cast<int32_t>(eg) << 16) + mg;
}

inline constexpr int16_t mg_score(int32_t score){
    return static_cast<int16_t>(static_cast<int32_t>(score));
}

inline constexpr int16_t eg_score(int32_t score){
    return static_cast<int16_t>((static_cast<int32_t>(score) + 0x8000) >> 16);
}

inline constexpr int32_t max(int32_t a, int32_t b){
    return a > b ? a : b;
}

inline constexpr int32_t min(int32_t a, int32_t b){
    return a < b ? a : b;
}

inline constexpr int clamp(int val, int min_val, int max_val){
    return val < min_val ? min_val : (val > max_val) ? max_val : val;
}

inline bool is_quiet_move(chess::Board &board, chess::Move move){
    return !board.isCapture(move) 
         && move.typeOf() != chess::Move::PROMOTION 
         && move.typeOf() != chess::Move::CASTLING;
}


namespace Constants{
    // If you decide to increase this, ensure to change the TTEntry::TTEntryType enum struct to 
    // accomodate for integer size.
    const int MAX_SEARCH_DEPTH = 255;

    const arr_t<int32_t, 6> PIECETYPE_WEIGHT = {{
        make_score(  82, 144), make_score( 426, 475), make_score( 441, 510), 
        make_score( 627, 803), make_score(1292,1623), make_score(   0,   0)
    }};

    const arr_t<int32_t, 12> PIECE_WEIGHT = {{
        PIECETYPE_WEIGHT[ 0], PIECETYPE_WEIGHT[ 1], PIECETYPE_WEIGHT[ 2], 
        PIECETYPE_WEIGHT[ 3], PIECETYPE_WEIGHT[ 4], PIECETYPE_WEIGHT[ 5], 
        -PIECETYPE_WEIGHT[0], -PIECETYPE_WEIGHT[1], -PIECETYPE_WEIGHT[2], 
        -PIECETYPE_WEIGHT[3], -PIECETYPE_WEIGHT[4], -PIECETYPE_WEIGHT[5]
    }};

    const int16_t DRAW_SCORE      = 0;
    const int16_t CHECKMATE_SCORE = 32000;
    const int16_t MAX_AB_VAL      = 32000;
    
    const uint64_t WHITE_SQUARES       = 0x55aa55aa55aa55aaULL;
    const uint64_t BLACK_SQUARES       = ~WHITE_SQUARES;
    const uint64_t MIDDLE_SQUARES_4    = 0x1818000000ULL;
    const uint64_t MIDDLE_SQUARES_16   = 0x00003C3C3C3C0000ULL;
    const uint64_t LEFT_FLANK          = 0xf0f0f0f0f0f0f0fULL;
    const uint64_t RIGHT_FLANK         = 0xf0f0f0f0f0f0f0f0ULL;
    const uint64_t CENTER_SQUARES      = 0x0000001818000000ULL;
    
    const arr_t<uint64_t, 2> OUTPOST_RANKS = {{0xffffff000000ULL, 0xffffff0000ULL}};

    const arr_t<uint64_t, 15> MASK_DIAGONAL = {{
        0x80ULL,               0x8040ULL,             0x804020ULL, 
        0x20402010ULL,         0x8040201008ULL,       0x804020100804ULL, 
        0x80402010080402ULL,   0x8040201008040201ULL, 0x4020100804020100ULL, 
        0x2010080402010000ULL, 0x1008040201000000ULL, 0x804020100000000ULL, 
        0x402010000000000ULL,  0x201000000000000ULL,  0x100000000000000ULL
    }};

    const arr_t<uint64_t, 15> MASK_ANTIDIAGONAL = {{
        0x1ULL,                0x102ULL,              0x10204ULL, 
        0x1020408ULL,          0x102040810ULL,        0x10204081020ULL, 
        0x1020408102040ULL,    0x102040810204080ULL,  0x204081020408000ULL, 
        0x408102040800000ULL,  0x810204080000000ULL,  0x1020408000000000ULL, 
        0x2040800000000000ULL, 0x4080000000000000ULL, 0x8000000000000000ULL
    }};
    
    const arr_t<arr_t<uint64_t, 8>, 2> FORWARD_RANKS = {{
        {
            0xffffffffffffffffULL, 0xffffffffffffff00ULL, 0xffffffffffff0000ULL, 0xffffffffff000000ULL,
            0xffffffff00000000ULL, 0xffffff0000000000ULL, 0xffff000000000000ULL, 0xff00000000000000ULL
        },
        {
            0xffffffffffffffffULL, 0xffffffffffffffULL,   0xffffffffffffULL,  0xffffffffffULL,
            0xffffffffULL,         0xffffffULL,           0xffffULL,          0xffULL
        }
    }};

    const arr_t<uint64_t, 8> ADJACENT_FILES = {{
        0x202020202020202ULL,  0x505050505050505ULL,  0xa0a0a0a0a0a0a0aULL,  0x1414141414141414ULL,
        0x2828282828282828ULL, 0x5050505050505050ULL, 0xa0a0a0a0a0a0a0a0ULL, 0x4040404040404040ULL
    }};

    arr_t<int32_t, 64> _PAWN_PSQT = {
        make_score(   0,   0), make_score(   0,   0), make_score(   0,   0), make_score(   0,   0),
        make_score(   0,   0), make_score(   0,   0), make_score(   0,   0), make_score(   0,   0),
        make_score( -13,   7), make_score(  -4,   0), make_score(   1,   4), make_score(   6,   1),
        make_score(   3,  10), make_score(  -9,   4), make_score(  -9,   3), make_score( -16,   7),
        make_score( -21,   5), make_score( -17,   6), make_score(  -1,  -6), make_score(  12, -14),
        make_score(   8, -10), make_score(  -4,  -5), make_score( -15,   7), make_score( -24,  11),
        make_score( -14,  16), make_score( -21,  17), make_score(   9, -10), make_score(  10, -24),
        make_score(   4, -22), make_score(   4, -10), make_score( -20,  17), make_score( -17,  18),
        make_score( -15,  18), make_score( -18,  11), make_score( -16,  -8), make_score(   4, -30),
        make_score(  -2, -24), make_score( -18,  -9), make_score( -23,  13), make_score( -17,  21),
        make_score( -20,  48), make_score(  -9,  44), make_score(   1,  31), make_score(  17,  -9),
        make_score(  36,  -6), make_score(  -9,  31), make_score(  -6,  45), make_score( -23,  49),
        make_score( -33, -70), make_score( -66,  -9), make_score( -16, -22), make_score(  65, -23),
        make_score(  41, -18), make_score(  39, -14), make_score( -47,   4), make_score( -62, -51),
        make_score(   0,   0), make_score(   0,   0), make_score(   0,   0), make_score(   0,   0),
        make_score(   0,   0), make_score(   0,   0), make_score(   0,   0), make_score(   0,   0),
    };

    arr_t<int32_t, 64> _KNIGHT_PSQT = {
        make_score( -31, -38), make_score(  -6, -24), make_score( -20, -22), make_score( -16,  -1),
        make_score( -11,  -1), make_score( -22, -19), make_score(  -8, -20), make_score( -41, -30),
        make_score(   1,  -5), make_score( -11,   3), make_score(  -6, -19), make_score(  -1,  -2),
        make_score(   0,   0), make_score(  -9, -16), make_score(  -8,  -3), make_score(  -6,   1),
        make_score(   7, -21), make_score(   8,  -5), make_score(   7,   2), make_score(  10,  19),
        make_score(  10,  19), make_score(   4,   2), make_score(   8,  -4), make_score(   3, -19),
        make_score(  16,  21), make_score(  17,  30), make_score(  23,  41), make_score(  27,  50),
        make_score(  24,  53), make_score(  23,  41), make_score(  19,  28), make_score(  13,  26),
        make_score(  13,  30), make_score(  23,  30), make_score(  37,  51), make_score(  30,  70),
        make_score(  26,  67), make_score(  38,  50), make_score(  22,  33), make_score(  14,  28),
        make_score( -24,  25), make_score(  -5,  37), make_score(  25,  56), make_score(  22,  60),
        make_score(  27,  55), make_score(  29,  55), make_score(  -1,  32), make_score( -19,  25),
        make_score(  13,  -2), make_score( -11,  18), make_score(  27,  -2), make_score(  37,  24),
        make_score(  41,  24), make_score(  40,  -7), make_score( -13,  16), make_score(   2,  -2),
        make_score(-167,  -5), make_score( -91,  12), make_score(-117,  41), make_score( -38,  17),
        make_score( -18,  19), make_score(-105,  48), make_score(-119,  24), make_score(-165, -17),
    };

    arr_t<int32_t, 64> _BISHOP_PSQT = {
        make_score(   5, -21), make_score(   1,   1), make_score(  -1,   5), make_score(   1,   5),
        make_score(   2,   8), make_score(  -6,  -2), make_score(   0,   1), make_score(   4, -25),
        make_score(  26, -17), make_score(   2, -31), make_score(  15,  -2), make_score(   8,   8),
        make_score(   8,   8), make_score(  13,  -3), make_score(   9, -31), make_score(  26, -29),
        make_score(   9,   3), make_score(  22,   9), make_score(  -5,  -3), make_score(  18,  19),
        make_score(  17,  20), make_score(  -5,  -6), make_score(  20,   4), make_score(  15,   8),
        make_score(   0,  12), make_score(  10,  17), make_score(  17,  32), make_score(  20,  32),
        make_score(  24,  34), make_score(  12,  30), make_score(  15,  17), make_score(   0,  14),
        make_score( -20,  34), make_score(  13,  31), make_score(   1,  38), make_score(  21,  45),
        make_score(  12,  46), make_score(   6,  38), make_score(  13,  33), make_score( -14,  37),
        make_score( -13,  31), make_score( -11,  45), make_score(  -7,  23), make_score(   2,  40),
        make_score(   8,  38), make_score( -21,  34), make_score(  -5,  46), make_score(  -9,  35),
        make_score( -59,  38), make_score( -49,  22), make_score( -13,  30), make_score( -35,  36),
        make_score( -33,  36), make_score( -13,  33), make_score( -68,  21), make_score( -55,  35),
        make_score( -66,  18), make_score( -65,  36), make_score(-123,  48), make_score(-107,  56),
        make_score(-112,  53), make_score( -97,  43), make_score( -33,  22), make_score( -74,  15),
    };

    arr_t<int32_t, 64> _ROOK_PSQT = {
        make_score( -26,  -1), make_score( -21,   3), make_score( -14,   4), make_score(  -6,  -4),
        make_score(  -5,  -4), make_score( -10,   3), make_score( -13,  -2), make_score( -22, -14),
        make_score( -70,   5), make_score( -25, -10), make_score( -18,  -7), make_score( -11, -11),
        make_score(  -9, -13), make_score( -15, -15), make_score( -15, -17), make_score( -77,   3),
        make_score( -39,   3), make_score( -16,  14), make_score( -25,   9), make_score( -14,   2),
        make_score( -12,   3), make_score( -25,   8), make_score(  -4,   9), make_score( -39,   1),
        make_score( -32,  24), make_score( -21,  36), make_score( -21,  36), make_score(  -5,  26),
        make_score(  -8,  27), make_score( -19,  34), make_score( -13,  33), make_score( -30,  24),
        make_score( -22,  46), make_score(   4,  38), make_score(  16,  38), make_score(  35,  30),
        make_score(  33,  32), make_score(  10,  36), make_score(  17,  31), make_score( -14,  43),
        make_score( -33,  60), make_score(  17,  41), make_score(   0,  54), make_score(  33,  36),
        make_score(  29,  35), make_score(   3,  52), make_score(  33,  32), make_score( -26,  56),
        make_score( -18,  41), make_score( -24,  47), make_score(  -1,  38), make_score(  15,  38),
        make_score(  14,  37), make_score(  -2,  36), make_score( -24,  49), make_score( -12,  38),
        make_score(  33,  55), make_score(  24,  63), make_score(  -1,  73), make_score(   9,  66),
        make_score(  10,  67), make_score(   0,  69), make_score(  34,  59), make_score(  37,  56),
    };

    arr_t<int32_t, 64> _QUEEN_PSQT = {
        make_score(  20, -34), make_score(   4, -26), make_score(   9, -34), make_score(  17, -16),
        make_score(  18, -18), make_score(  14, -46), make_score(   9, -28), make_score(  22, -44),
        make_score(   6, -15), make_score(  15, -22), make_score(  22, -42), make_score(  13,   2),
        make_score(  17,   0), make_score(  22, -49), make_score(  18, -29), make_score(   3, -18),
        make_score(   6,  -1), make_score(  21,   7), make_score(   5,  35), make_score(   0,  34),
        make_score(   2,  34), make_score(   5,  37), make_score(  24,   9), make_score(  13, -15),
        make_score(   9,  17), make_score(  12,  46), make_score(  -6,  59), make_score( -19, 109),
        make_score( -17, 106), make_score(  -4,  57), make_score(  18,  48), make_score(   8,  33),
        make_score( -10,  42), make_score(  -8,  79), make_score( -19,  66), make_score( -32, 121),
        make_score( -32, 127), make_score( -23,  80), make_score(  -8,  95), make_score( -10,  68),
        make_score( -28,  56), make_score( -23,  50), make_score( -33,  66), make_score( -18,  70),
        make_score( -17,  71), make_score( -19,  63), make_score( -18,  65), make_score( -28,  76),
        make_score( -16,  61), make_score( -72, 108), make_score( -19,  65), make_score( -52, 114),
        make_score( -54, 120), make_score( -14,  59), make_score( -69, 116), make_score( -11,  73),
        make_score(   8,  43), make_score(  19,  47), make_score(   0,  79), make_score(   3,  78),
        make_score(  -3,  89), make_score(  13,  65), make_score(  18,  79), make_score(  21,  56),
    };

    arr_t<int32_t, 64> _KING_PSQT = {
        make_score(  87, -77), make_score(  67, -49), make_score(   4,  -7), make_score(  -9, -26),
        make_score( -10, -27), make_score(  -8,  -1), make_score(  57, -50), make_score(  79, -82),
        make_score(  35,   3), make_score( -27,  -3), make_score( -41,  16), make_score( -89,  29),
        make_score( -64,  26), make_score( -64,  28), make_score( -25,  -3), make_score(  30,  -4),
        make_score( -44, -19), make_score( -16, -19), make_score(  28,   7), make_score(   0,  35),
        make_score(  18,  32), make_score(  31,   9), make_score( -13, -18), make_score( -36, -13),
        make_score( -48, -44), make_score(  98, -39), make_score(  71,  12), make_score( -22,  45),
        make_score(  12,  41), make_score(  79,  10), make_score( 115, -34), make_score( -59, -38),
        make_score(  -6, -10), make_score(  95, -39), make_score(  39,  14), make_score( -49,  18),
        make_score( -27,  19), make_score(  35,  14), make_score(  81, -34), make_score( -50, -13),
        make_score(  24, -39), make_score( 123, -22), make_score( 105,  -1), make_score( -22, -21),
        make_score( -39, -20), make_score(  74, -15), make_score( 100, -23), make_score( -17, -49),
        make_score(   0, -98), make_score(  28, -21), make_score(   7, -18), make_score(  -3, -41),
        make_score( -57, -39), make_score(  12, -26), make_score(  22, -24), make_score( -15,-119),
        make_score( -16,-153), make_score(  49, -94), make_score( -21, -73), make_score( -19, -32),
        make_score( -51, -55), make_score( -42, -62), make_score(  53, -93), make_score( -58,-133),
    };

    const arr_t<arr_t<int32_t, 64>*, 6> PSQT = {{
        &_PAWN_PSQT, &_KNIGHT_PSQT, &_BISHOP_PSQT, &_ROOK_PSQT, &_QUEEN_PSQT, &_KING_PSQT
    }};


    const arr_t<int32_t, 28> QUEEN_MOBILITY_SCORES = {{
        make_score(-111,-273), make_score(-253,-401), make_score(-127,-228), make_score( -46,-236),
        make_score( -20,-173), make_score(  -9, -86), make_score(  -1, -35), make_score(   2,  -1),
        make_score(   8,   8), make_score(  10,  31), make_score(  15,  37), make_score(  17,  55),
        make_score(  20,  46), make_score(  23,  57), make_score(  22,  58), make_score(  21,  64),
        make_score(  24,  62), make_score(  16,  65), make_score(  13,  63), make_score(  18,  48),
        make_score(  25,  30), make_score(  38,   8), make_score(  34, -12), make_score(  28, -29),
        make_score(  10, -44), make_score(   7, -79), make_score( -42, -30), make_score( -23, -50),
    }};

    const arr_t<int32_t, 15> ROOK_MOBILITY_SCORES = {{
        make_score(-127,-148), make_score( -56,-127), make_score( -25, -85), make_score( -12, -28),
        make_score( -10,   2), make_score( -12,  27), make_score( -11,  42), make_score(  -4,  46),
        make_score(   4,  52), make_score(   9,  55), make_score(  11,  64), make_score(  19,  68),
        make_score(  19,  73), make_score(  37,  60), make_score(  97,  15),
    }};

    const arr_t<int32_t, 14> BISHOP_MOBILITY_SCORES = {{
        make_score( -99,-186), make_score( -46,-124), make_score( -16, -54), make_score(  -4, -14),
        make_score(   6,   1), make_score(  14,  20), make_score(  17,  35), make_score(  19,  39),
        make_score(  19,  49), make_score(  27,  48), make_score(  26,  48), make_score(  52,  32),
        make_score(  55,  47), make_score(  83,   2),
    }};

    const arr_t<int32_t, 9> KNIGHT_MOBILITY_SCORES = {{
        make_score(-104,-139), make_score( -45,-114), make_score( -22, -37), make_score(  -8,   3),
        make_score(   6,  15), make_score(  11,  34), make_score(  19,  38), make_score(  30,  37),
        make_score(  43,  17),
    }};

    const arr_t<arr_t<int32_t, 8>, 2> PAWN_CANDIDATE_PASSER_SCORES = {{
    {
        make_score(   0,   0), make_score( -11, -18), make_score( -16,  18), make_score( -18,  29),
        make_score( -22,  61), make_score(  21,  59), make_score(   0,   0), make_score(   0,   0)},
    {
        make_score(   0,   0), make_score( -12,  21), make_score(  -7,  27), make_score(   2,  53),
        make_score(  22, 116), make_score(  49,  78), make_score(   0,   0), make_score(   0,   0)},
    }};

    const arr_t<int32_t, 8> PAWN_ISOLATED_SCORES = {{
        make_score( -13, -12), make_score(  -1, -16), make_score(   1, -16), make_score(   3, -18),
        make_score(   7, -19), make_score(   3, -15), make_score(  -4, -14), make_score(  -4, -17),
    }};

    const arr_t<arr_t<int32_t, 8>, 2> PAWN_STACKED_SCORES = {{
    {
        make_score(  10, -29), make_score(  -2, -26), make_score(   0, -23), make_score(   0, -20),
        make_score(   3, -20), make_score(   5, -26), make_score(   4, -30), make_score(   8, -31)},
    {
        make_score(   3, -14), make_score(   0, -15), make_score(  -6,  -9), make_score(  -7, -10),
        make_score(  -4,  -9), make_score(  -2, -10), make_score(   0, -13), make_score(   0, -17)},
    }};

    const arr_t<arr_t<int32_t, 8>, 2> PAWN_BACKWARDS_SCORES = {{
    {
        make_score(   0,   0), make_score(   0,  -7), make_score(   7,  -7), make_score(   6, -18),
        make_score(  -4, -29), make_score(   0,   0), make_score(   0,   0), make_score(   0,   0)},
    {
        make_score(   0,   0), make_score(  -9, -32), make_score(  -5, -30), make_score(   3, -31),
        make_score(  29, -41), make_score(   0,   0), make_score(   0,   0), make_score(   0,   0)},
    }};

    const arr_t<int32_t, 32> CONNECTED_PAWN_SCORES_32 = {
        make_score(   0,   0), make_score(   0,   0), make_score(   0,   0), make_score(   0,   0),
        make_score(  -1, -11), make_score(  12,  -4), make_score(   0,  -2), make_score(   6,   8),
        make_score(  14,   0), make_score(  20,  -6), make_score(  19,   3), make_score(  17,   8),
        make_score(   6,  -1), make_score(  20,   1), make_score(   6,   3), make_score(  14,  10),
        make_score(   8,  14), make_score(  21,  17), make_score(  31,  23), make_score(  25,  18),
        make_score(  45,  40), make_score(  36,  64), make_score(  58,  74), make_score(  64,  88),
        make_score( 108,  35), make_score( 214,  45), make_score( 216,  70), make_score( 233,  61),
        make_score(   0,   0), make_score(   0,   0), make_score(   0,   0), make_score(   0,   0),
    };

    /** NOTE: index 0 for semi-opened and index 2 for opened file */
    const arr_t<int32_t, 2> ROOK_OPEN_FILE_SCORES = {{
        make_score(10, 9), 
        make_score(34, 8)
    }};

    const arr_t<arr_t<int32_t, 2>, 2> KNIHT_OUTPOST_SCORES = {{
        {make_score(12, -32), make_score(40, 0)},
        {make_score( 7, -24), make_score(21,-3)},
    }};

    const arr_t<arr_t<int32_t, 2>, 2> BISHOP_OUTPOST_SCORES = {{
        {make_score(16, -16), make_score(50, -3)},
        {make_score( 9,  -9), make_score(-4, -4)},
    }};

    const arr_t<int32_t, 4> KNIGHT_IN_SIBERIA_SCORES = {{
        make_score(  -9,  -6), make_score( -12, -20), 
        make_score( -27, -20), make_score( -47, -19),
    }};

    /** NOTE: King area is 12 */
    const arr_t<int32_t, 12> KING_DEFENDERS_SCORES = {{
        make_score( -37,  -3), make_score( -17,   2), make_score(   0,   6), make_score(  11,   8),
        make_score(  21,   8), make_score(  32,   0), make_score(  38, -14), make_score(  10,  -5),
        make_score(  12,   6), make_score(  12,   6), make_score(  12,   6), make_score(  12,   6),
    }};

    const arr_t<arr_t<arr_t<int32_t, 8>, 2>, 2> PASSED_PAWN_SCORES = {{
        {{
            {make_score(   0,   0), make_score( -39,  -4), make_score( -43,  25), make_score( -62,  28),
            make_score(   8,  19), make_score(  97,  -4), make_score( 162,  46), make_score(   0,   0)},

            {make_score(   0,   0), make_score( -28,  13), make_score( -40,  42), make_score( -56,  44),
            make_score(  -2,  56), make_score( 114,  54), make_score( 193,  94), make_score(   0,   0)}
        }},
        {{
            {make_score(   0,   0), make_score( -28,  29), make_score( -47,  36), make_score( -60,  54),
            make_score(   8,  65), make_score( 106,  76), make_score( 258, 124), make_score(   0,   0)},

            {make_score(   0,   0), make_score( -28,  23), make_score( -40,  35), make_score( -55,  60),
            make_score(   8,  89), make_score(  95, 166), make_score( 124, 293), make_score(   0,   0)}
        }}
    }};

    const arr_t<int32_t, 8> PASSED_FRIENDLY_DISTANCE_SCORES = {{
        make_score(   0,   0), make_score(  -3,   1), make_score(   0,  -4), make_score(   5, -13),
        make_score(   6, -19), make_score(  -9, -19), make_score(  -9,  -7), make_score(   0,   0),
    }};

    const arr_t<int32_t, 8> PASSED_ENEMY_DISTANCE_SCORES = {{
        make_score(   0,   0), make_score(   5,  -1), make_score(   7,   0), make_score(   9,  11),
        make_score(   0,  25), make_score(   1,  37), make_score(  16,  37), make_score(   0,   0),
    }};
    
    const arr_t<int32_t, 9> CLOSEDNESS_KNIGHT_ADJUSTMENT = {{
        make_score(  -7,  10), make_score(  -7,  29), make_score(  -9,  37), make_score(  -5,  37),
        make_score(  -3,  44), make_score(  -1,  36), make_score(   1,  33), make_score( -10,  51),
        make_score(  -7,  30),
    }};

    const arr_t<int32_t, 9> CLOSEDNESS_ROOK_ADJUSTMENT = {{
        make_score(  42,  43), make_score(  -6,  80), make_score(   3,  59), make_score(  -5,  47),
        make_score(  -7,  41), make_score(  -3,  23), make_score(  -6,  11), make_score( -17,  11),
        make_score( -34, -12),
    }};

    const arr_t<arr_t<int32_t, 8>, 2> SAFETY_SHELTER_SCORES = {{
        {
            make_score(  -2,   7), make_score(  -1,  13), make_score(   0,   8), make_score(   4,   7),
            make_score(   6,   2), make_score(  -1,   0), make_score(   2,   0), make_score(   0, -13)
        },
        {
            make_score(   0,   0), make_score(  -2,  13), make_score(  -2,   9), make_score(   4,   5),
            make_score(   3,   1), make_score(  -3,   0), make_score(  -2,   0), make_score(  -1,  -9)
        },
    }};

    const arr_t<arr_t<int32_t, 8>, 2> SAFETY_STORM_SCORES = {{
        {
            make_score(  -4,  -1), make_score(  -8,   3), make_score(   0,   5), make_score(   1,  -1),
            make_score(   3,   6), make_score(  -2,  20), make_score(  -2,  18), make_score(   2, -12)
        },
        {
            make_score(   0,   0), make_score(   1,   0), make_score(  -1,   4), make_score(   0,   0),
            make_score(   0,   5), make_score(  -1,   1), make_score(   1,   0), make_score(   1,   0)
        },
    }};

    const arr_t<int32_t, 8> KING_PAWN_FILE_PROXIMITY_SCORES = {{
        make_score(  36,  46), make_score(  22,  31), make_score(  13,  15), make_score(  -8, -22),
        make_score(  -5, -62), make_score(  -3, -75), make_score( -15, -81), make_score( -12, -75),
    }};

    const arr_t<arr_t<arr_t<int32_t, 8>, 8>, 2> KING_SHELTER_SCORES = {{
        {{
            {
                make_score(  -5,  -5), make_score(  17, -31), make_score(  26,  -3), make_score(  24,   8),
                make_score(   4,   1), make_score( -12,   4), make_score( -16, -33), make_score( -59,  24)
            },
            {
                make_score(  11,  -6), make_score(   3, -15), make_score(  -5,  -2), make_score(   5,  -4),
                make_score( -11,   7), make_score( -53,  70), make_score(  81,  82), make_score( -19,   1)
            },
            {
                make_score(  38,  -3), make_score(   5,  -6), make_score( -34,   5), make_score( -17, -15),
                make_score(  -9,  -5), make_score( -26,  12), make_score(  11,  73), make_score( -16,  -1)
            },
            {
                make_score(  18,  11), make_score(  25, -18), make_score(   0, -14), make_score(  10, -21),
                make_score(  22, -34), make_score( -48,   9), make_score(-140,  49), make_score(  -5,  -5)
            },
            {
                make_score( -11,  15), make_score(   1,  -3), make_score( -44,   6), make_score( -28,  10),
                make_score( -24,  -2), make_score( -35,  -5), make_score(  40, -24), make_score( -13,   3)
            },
            {
                make_score(  51, -14), make_score(  15, -14), make_score( -24,   5), make_score( -10, -20),
                make_score(  10, -34), make_score(  34, -20), make_score(  48, -38), make_score( -21,   1)
            },
            {
                make_score(  40, -17), make_score(   2, -24), make_score( -31,  -1), make_score( -24,  -8),
                make_score( -31,   2), make_score( -20,  29), make_score(   4,  49), make_score( -16,   3)
            },
            {
                make_score(  10, -20), make_score(   4, -24), make_score(  10,   2), make_score(   2,  16),
                make_score( -10,  24), make_score( -10,  44), make_score(-184,  81), make_score( -17,  17)
            }
        }},
        {{   
            {
                make_score(   0,   0), make_score( -15, -39), make_score(   9, -29), make_score( -49,  14),
                make_score( -36,   6), make_score(  -8,  50), make_score(-168,  -3), make_score( -59,  19)
            },
            {
                make_score(   0,   0), make_score(  17, -18), make_score(   9, -11), make_score( -11,  -5),
                make_score(  -1, -24), make_score(  26,  73), make_score(-186,   4), make_score( -32,  11)
            },
            {
                make_score(   0,   0), make_score(  19,  -9), make_score(   1, -11), make_score(   9, -26),
                make_score(  28,  -5), make_score( -92,  56), make_score( -88, -74), make_score(  -8,   1)
            },
            {
                make_score(   0,   0), make_score(   0,   3), make_score(  -6,  -6), make_score( -35,  10),
                make_score( -46,  13), make_score( -98,  33), make_score(  -7, -45), make_score( -35,  -5)
            },
            {
                make_score(   0,   0), make_score(  12,  -3), make_score(  17, -15), make_score(  17, -15),
                make_score(  -5, -14), make_score( -36,   5), make_score(-101, -52), make_score( -18,  -1)
            },
            {
                make_score(   0,   0), make_score(  -8,  -5), make_score( -22,   1), make_score( -16,  -6),
                make_score(  25, -22), make_score( -27,  10), make_score(  52,  39), make_score( -14,  -2)
            },
            {
                make_score(   0,   0), make_score(  32, -22), make_score(  19, -15), make_score(  -9,  -6),
                make_score( -29,  13), make_score(  -7,  23), make_score( -50, -39), make_score( -27,  18)
            },
            {
                make_score(   0,   0), make_score(  16, -57), make_score(  17, -32), make_score( -18,  -7),
                make_score( -31,  24), make_score( -11,  24), make_score(-225, -49), make_score( -30,   5)
            }
        }},
    }};

    const arr_t<arr_t<arr_t<int32_t, 8>, 4>, 2> KING_STORM_SCORES = {{
        {{
            {
                make_score(  -6,  36), make_score( 144,  -4), make_score( -13,  26), make_score(  -7,   1),
                make_score( -12,  -3), make_score(  -8,  -7), make_score( -19,   8), make_score( -28,  -2)
            },
            {
                make_score( -17,  60), make_score(  64,  17), make_score(  -9,  21), make_score(   8,  12),
                make_score(   3,   9), make_score(   6,  -2), make_score(  -5,   2), make_score( -16,   8)
            },
            {
                make_score(   2,  48), make_score(  15,  30), make_score( -17,  20), make_score( -13,  10),
                make_score(  -1,   6), make_score(   7,   3), make_score(   8,  -7), make_score(   7,   8)
            },
            {
                make_score(  -1,  25), make_score(  15,  22), make_score( -31,  10), make_score( -22,   1),
                make_score( -15,   4), make_score(  13, -10), make_score(   3,  -5), make_score( -20,   8)
            }
        }},
        {{
            {
                make_score(   0,   0), make_score( -18, -16), make_score( -18,  -2), make_score(  27, -24),
                make_score(  10,  -6), make_score(  15, -24), make_score(  -6,   9), make_score(   9,  30)
            },
            {
                make_score(   0,   0), make_score( -15, -42), make_score(  -3, -15), make_score(  53, -17),
                make_score(  15,  -5), make_score(  20, -28), make_score( -12, -17), make_score( -34,   5)
            },
            {
                make_score(   0,   0), make_score( -34, -62), make_score( -15, -13), make_score(   9,  -6),
                make_score(   6,  -2), make_score(  -2, -17), make_score(  -5, -21), make_score(  -3,   3)
            },
            {
                make_score(   0,   0), make_score(  -1, -26), make_score( -27, -19), make_score( -21,   4),
                make_score( -10,  -6), make_score(   7, -35), make_score(  66, -29), make_score(  11,  25)
            }
        }},
    }};


    // MVV LVA (Most valuable victim, least valuable attacker) row index represents 
    // victims (P, N, B, R, Q, K), column indexes represent attackers (P, N, B, R, Q, K).
    // King cannot be captured (that would simply be checkmate and gameover, so king row is all 0s)
    const arr_t<arr_t<int16_t, 6>, 6> MVV_LVA_SCORES = {{
        {15, 14, 13, 12, 11, 10},
        {25, 24, 23, 22, 21, 20},
        {35, 34, 33, 32, 31, 30},
        {45, 44, 43, 42, 41, 40},
        {55, 54, 53, 52, 51, 50},
        { 0,  0,  0,  0,  0,  0},
    }};
    // Promotion scores (P, N, B, R, Q)
    const arr_t<int16_t, 5> PROMOTION_SCORES = {{-10, 10, 20, 30, 40}};
    
    // The indexes of TT_MOVE_SCORES correspond to Exact, Upperbound and Lowebound nodes
    // Although, looking at the logic of the search algorithms, you would notice that the
    // Exact nodes are not used for move sorting, because if an Exact node is encountered
    // It automatically returns its result from the TT, without having to do any further
    // search.
    const arr_t<int16_t, 3> TT_MOVE_SCORES   = {{150, -10, 250}};
    const int16_t PV_MOVE_SCORE     = 300;
    const int16_t CASTLING_SCORE    =  50;
    const int16_t KILLER_MOVE_SCORE =  10;
    const int16_t MAX_HISTORY_SCORE =  1000;


    // bonus or penalty values for various scenarios
    const int32_t QUEEN_RELATIVE_PIN_PENALTY           = make_score( -22, -13);
    const int32_t ROOK_BONUS_ON_SEVENTH                = make_score(  -1,  42);
    const int32_t BISHOP_PAIR_BONUS                    = make_score(  22,  88);
    const int32_t BISHOP_RAMMED_PAWNS_PENALTY          = make_score(  -8, -17);
    const int32_t BISHOP_BEHIND_PAWN_BONUS             = make_score(   4,  24);
    const int32_t BISHOP_LONG_DIAG_CENTER_SQUARE_BONUS = make_score(  26,  20);
    const int32_t KNIGHT_BEHIND_PAWN_BONUS             = make_score(   3,  28);
    // bonus for if file path to promotion is uncontested / not obstructed or attacked
    const int32_t PASSED_SAFE_PROMOTION_PATH_BONUS     = make_score( -49,  57);

    // respective weight values for pieces that attack king
    const int32_t KNIGHT_ATTACK_WEIGHT                 = make_score(  48,  41);
    const int32_t BISHOP_ATTACK_WEIGHT                 = make_score(  24,  35);
    const int32_t ROOK_ATTACK_WEIGHT                   = make_score(  36,   8);
    const int32_t QUEEN_ATTACK_WEIGHT                  = make_score(  30,   6);

    // multipliers for respective threats to king, the `SAFE_` in these variable
    // names imply that the threat to our king is safe from us
    const int32_t THREAT_ATTACK_VALUES                 = make_score(  45,  34);
    const int32_t THREAT_WEAK_SQUARES                  = make_score(  42,  41);
    const int32_t THREAT_NO_ENEMY_QUEEN                = make_score(-237,-259);
    const int32_t THREAT_SAFE_QUEEN_CHECK              = make_score(  93,  83);
    const int32_t THREAT_SAFE_ROOK_CHECK               = make_score(  90,  98);
    const int32_t THREAT_SAFE_BISHOP_CHECK             = make_score(  59,  59);
    const int32_t THREAT_SAFE_KNIGHT_CHECK             = make_score( 112, 117);
    const int32_t THREAT_ADJUSTMENT                    = make_score( -74, -26);

    // Complexity evaluation values
    const int32_t COMPLEXITY_TOTAL_PAWNS               = make_score(   0,   8);
    const int32_t COMPLEXITY_PAWN_FLANKS               = make_score(   0,  82);
    const int32_t COMPLEXITY_PAWN_ENDGAME              = make_score(   0,  76);
    const int32_t COMPLEXITY_ADJUSTMENT                = make_score(   0,-157);

     // Space evaluation
    const int32_t SPACE_RESTRICTED_PIECE               = make_score(  -4,  -1);
    const int32_t SPACE_RESTRICTED_EMPTY               = make_score(  -4,  -2);
    const int32_t SPACE_CENTER_CONTROL                 = make_score(   3,   0);

    // multipliers for respective threat to other pieces, major and minorss
    const int32_t THREAT_WEAK_PAWN                     = make_score( -11, -38);
    const int32_t THREAT_MINOR_ATTACKED_BY_PAWNS       = make_score( -55, -83);
    const int32_t THREAT_MINOR_ATTACKED_BY_MINORS      = make_score( -25, -45);
    const int32_t THREAT_MINOR_ATTACKED_BY_MAJORS      = make_score( -30, -55);
    const int32_t THREAT_ROOK_ATTACKED_BY_LESSER       = make_score( -48, -28);
    const int32_t THREAT_MINOR_ATTACKED_BY_KING        = make_score( -43, -21);
    const int32_t THREAT_ROOK_ATTACKED_BY_KING         = make_score( -33, -18);
    const int32_t THREAT_QUEEEN_ATTACKED_BY_ANYONE     = make_score( -50,  -7);
    const int32_t THREAT_OVERLOAD_PIECES               = make_score(  -7, -16);
    const int32_t THREAT_BY_OUR_PAWN_PUSH              = make_score(  15,  32);

    // endgame scale factors for different scenarios
    const int32_t SCALE_DRAW                 =   0;
    const int32_t SCALE_OCB_BISHOPS_ONLY     =  64;
    const int32_t SCALE_OCB_ONE_KNIGHT       = 106;
    const int32_t SCALE_OCB_ONE_ROOK         =  96;
    const int32_t SCALE_LONE_QUEEN           =  88;
    const int32_t SCALE_NORMAL               = 128;
    const int32_t SCALE_LARGE_PAWN_ADVANCE   = 144;
}

#endif