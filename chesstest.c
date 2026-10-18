#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <io.h>
#include <stdint.h>


#define TT_SIZE (1 << 20)
#define TT_EXACT 0
#define TT_LOWER 1
#define TT_UPPER 2
#define MAX_PLY 128

typedef struct {
    uint64_t key;
    int8_t depth;
    int32_t score;
    int8_t flag;
    int8_t from, to;
    char promo;
} TTEntry;

static TTEntry tt[TT_SIZE];

typedef struct {
    char board[64];
    int moved[6];
    int en_passant[2];
    int move_number;
    int w_king;
    int b_king;
    uint64_t hash;
} Position;

int piece_value(char piece)
{
    switch (toupper(piece))
    {
    case 'P': return 100;
    case 'R': return 500;
    case 'B': return 330;
    case 'N': return 320;
    case 'Q': return 900;
    default: return 0;
    }
}

//hard koda verdier for hvor bra det er å være for brikker (mulig å gjøre om med nevrale nettverk)
static const int pawn_pst[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

static const int knight_pst[64] = {
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50
};

static const int bishop_pst[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -20,-10,-10,-10,-10,-10,-10,-20
};

static const int rook_pst[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  -5,  0,  5,  5,  0,  -5,  0
};

static const int queen_pst[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20
};

static const int king_mid_pst[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};

static const int king_end_pst[64] = {
   -50,-40,-30,-20,-20,-30,-40,-50,
   -30,-20,-10,  0,  0,-10,-20,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-30,  0,  0,  0,  0,-30,-30,
   -50,-30,-30,-30,-30,-30,-30,-50
};

static uint64_t zob_piece[12][64];
static uint64_t zob_side;
static uint64_t zob_castle[6];
static uint64_t zob_ep[8];

static uint64_t rand64(void)
{
    static uint64_t s = 0x9E3779B97F4A7C15ULL;
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
}

static int piece_index(char p)
{
    switch (p) {
    case 'P': return 0; case 'N': return 1; case 'B': return 2;
    case 'R': return 3; case 'Q': return 4; case 'K': return 5;
    case 'p': return 6; case 'n': return 7; case 'b': return 8;
    case 'r': return 9; case 'q': return 10; case 'k': return 11;
    default:  return -1;
    }
}

void init_zobrist(void)
{
    for (int p = 0; p < 12; p++)
    {
        for (int s = 0; s < 64; s++) zob_piece[p][s] = rand64();
    }
    zob_side = rand64();
    for (int i = 0; i < 6; i++) zob_castle[i] = rand64();
    for (int i = 0; i < 8; i++) zob_ep[i] = rand64();
}

static int pst_value(char piece, int sq, int endgame)
{
    int idx = isupper(piece) ? sq : (sq ^ 56);
    switch (toupper(piece))
    {
        case 'P': return pawn_pst[idx];
        case 'N': return knight_pst[idx];
        case 'B': return bishop_pst[idx];
        case 'R': return rook_pst[idx];
        case 'Q': return queen_pst[idx];
        case 'K': return endgame ? king_end_pst[idx] : king_mid_pst[idx];
        default:  return 0;
    }
}

typedef struct { int from; int to; char promo; int score; } Move;
typedef struct { Move moves[256]; int count; } MoveList;

static Move killers[MAX_PLY][2];
static int history[2][64][64];
static int root_move_number;

const char start_board[64] = {
    'r','n','b','q','k','b','n','r', // black
    'p','p','p','p','p','p','p','p',
    '.','.','.','.','.','.','.','.',
    '.','.','.','.','.','.','.','.',
    '.','.','.','.','.','.','.','.',
    '.','.','.','.','.','.','.','.',
    'P','P','P','P','P','P','P','P', // white
    'R','N','B','Q','K','B','N','R'
};

uint64_t compute_hash(const Position *pos)
{
    uint64_t h = 0;
    for (int s = 0; s < 64; s++)
    {
        int p = piece_index(pos->board[s]);
        if (p >= 0) h ^= zob_piece[p][s];
    }
    if (pos->move_number % 2 == 1) h ^= zob_side;
    for (int i = 0; i < 6; i++) if (pos->moved[i]) h ^= zob_castle[i];
    if (pos->en_passant[1] != 0) h ^= zob_ep[pos->en_passant[1] % 8];
    return h;
}


void init_position(Position *pos)
{
    memcpy(pos->board, start_board, 64);
    memset(pos->moved, 0, sizeof(pos->moved));
    pos->en_passant[0] = 0;
    pos->en_passant[1] = 0;
    pos->move_number = 0;
    pos->b_king = 4;
    pos->w_king = 60;
    pos->hash = compute_hash(pos);
}

void init_kiwipete(Position *pos) // typisk midtspill posisjon
{
    const char kiwipete[64] = {
        'r','.','.','.','k','.','.','r',
        'p','.','p','p','q','p','b','.',
        'b','n','.','.','p','n','p','.',
        '.','.','.','P','N','.','.','.',
        '.','p','.','.','P','.','.','.',
        '.','.','N','.','.','Q','.','p',
        'P','P','P','B','B','P','P','P',
        'R','.','.','.','K','.','.','R'
    };
    memcpy(pos->board, kiwipete, 64);
    memset(pos->moved, 0, sizeof(pos->moved));
    pos->en_passant[0] = 0;
    pos->en_passant[1] = 0;
    pos->move_number = 0;
    pos->b_king = 4;
    pos->w_king = 60;
    pos->hash = compute_hash(pos);
}

void init_rook_endgame(Position *pos)
{
    const char rook_endgame[64] = {
        '.','.','.','.','.','.','.','.',
        '.','.','.','.','.','.','.','.',
        '.','.','.','k','.','K','.','.',
        '.','.','.','p','.','.','.','.',
        '.','.','.','.','.','.','.','.',
        '.','.','R','.','.','.','.','.',
        '.','.','.','.','r','.','.','.',
        '.','.','.','.','.','.','.','.'
    };
    memcpy(pos->board, rook_endgame, 64);
    for (int i = 0; i < 6; i++) pos->moved[i] = 1;
    pos->en_passant[0] = 0;
    pos->en_passant[1] = 0;
    pos->move_number = 0;
    pos->b_king = 19;
    pos->w_king = 21;
}


int is_enemy(char target_piece, int my_color) {
    if (target_piece == '.')
    {
        return 0;
    }
    int target_color;
    if (isupper(target_piece)) {
        target_color = -1;
    }
    else
    {
        target_color = 1;
    }
    return target_color != my_color;
}

void move_to_index(char move[], int out[2]) {
    out[0] = (move[0] - 'a') + 8 * (8 - (move[1] - '0'));
    out[1] = (move[2] - 'a') + 8 * (8 - (move[3] - '0'));
}

int valid_input(const char move[])
{
    if (strlen(move) < 4) return 0;
    if (move[0] < 'a' || move[0] > 'h') return 0;
    if (move[1] < '1' || move[1] > '8') return 0;
    if (move[2] < 'a' || move[2] > 'h') return 0;
    if (move[3] < '1' || move[3] > '8') return 0;
    return 1;
}

void index_to_move(char move[5], int in[2]) { // gjør om en index til et trekk (e2e4)
    int from_file = in[0] % 8;
    int from_rank = 8 - (in[0]/8);
    int to_file = in[1] % 8;
    int to_rank = 8 - (in[1]/8);

    move[0] = 'a' + from_file;
    move[1] = '0' + from_rank;
    move[2] = 'a' + to_file;
    move[3] = '0' + to_rank;
    move[4] = '\0';
}

void move_to_string(Move m, char out[6])
{
    int pair[2] = {m.from, m.to};
    index_to_move(out, pair);
    if (m.promo) {out[4] = m.promo; out[5] = '\0';}
}


void board_print(const Position *pos)
{
    for (int i = 0; i < 64; i++)
    {
        if (i % 8 == 0)
        {
            printf("%d ", 8 - (i / 8));
            printf(" ");
        }

        printf("%c", pos->board[i]);

        if (i % 8 == 7)
        {
            printf("\n");
        }
    }
    printf("\n");
    printf("   ");
    for (int k = 0; k < 8; k++)
    {
        printf("%c", 'a' + k);
    }
    printf("\n");
}

void en_passant_move_change(Position *pos, int index[3], char piece)
{
    if ((piece == 'p' && index[1] - index[0] == 16)
        || (piece == 'P' && index[0] - index[1] == 16))
    {
        pos->en_passant[0] = index[0];
        pos->en_passant[1] = index[1];
    }
    else
    {
        pos->en_passant[0] = 0;
        pos->en_passant[1] = 0;
    }
}

int en_passant(const Position *pos, int index[])
{
    if (pos->en_passant[1] != 0)
    {
        int beside_pawn = (index[0] % 8 != 0 && index[0] - 1 == pos->en_passant[1])
                       || (index[0] % 8 != 7 && index[0] + 1 == pos->en_passant[1]);
        if (pos->board[index[0]] == 'P' && beside_pawn && pos->board[pos->en_passant[1]] == 'p')
        {
            return 1;
        }
        if (pos->board[index[0]] == 'p' && beside_pawn && pos->board[pos->en_passant[1]] == 'P')
        {
            return 1;
        }
    }
    return 0;
}

static void set_sq(Position *pos, int sq, char piece)
{
    int old = piece_index(pos->board[sq]);
    if (old >= 0) pos->hash ^= zob_piece[old][sq];
    int nw = piece_index(piece);
    if (nw >= 0) pos->hash ^= zob_piece[nw][sq];
    pos->board[sq] = piece;
}

void move_push(Position *pos, int index[3])
{
    char piece = pos->board[index[0]];

    if (en_passant(pos, index) == 1)
    {
        // fjerner bare bonden hvis trekket faktisk er en passant
        // (går til ruta bak bonden som gikk to frem)
        if (isupper(piece) && index[1] + 8 == pos->en_passant[1])
        {
            set_sq(pos, index[1] + 8, '.');
        }
        if (islower(piece) && index[1] - 8 == pos->en_passant[1])
        {
            set_sq(pos, index[1] - 8, '.');
        }
    }

    if (piece == 'K' && index[0] == 60 && index[1] == 62) {
        set_sq(pos, 63, '.');
        set_sq(pos, 61, 'R');
    }
    if (piece == 'K' && index[0] == 60 && index[1] == 58) {
        set_sq(pos, 56, '.');
        set_sq(pos, 59, 'R');
    }
    if (piece == 'k' && index[0] == 4 && index[1] == 6) {
        set_sq(pos, 7, '.');
        set_sq(pos, 5, 'r');
    }
    if (piece == 'k' && index[0] == 4 && index[1] == 2) {
        set_sq(pos, 0, '.');
        set_sq(pos, 3, 'r');
    }

    if (index[0] == 56 || index[1] == 56) pos->moved[0] = 1;
    if (index[0] == 63 || index[1] == 63) pos->moved[1] = 1;
    if (index[0] == 60 || index[1] == 60) pos->moved[2] = 1;
    if (index[0] == 0 || index[1] == 0) pos->moved[3] = 1;
    if (index[0] == 4 || index[1] == 4) pos->moved[4] = 1;
    if (index[0] == 7 || index[1] == 7) pos->moved[5] = 1;

    if (piece == 'K') pos->w_king = index[1];
    if (piece == 'k') pos->b_king = index[1];

    set_sq(pos, index[0], '.');
    set_sq(pos, index[1], piece);

    if ((piece == 'P' && index[1] <= 7) || (piece == 'p' && index[1] >= 56))
    {
        char promo = tolower(index[2]);
        if (promo != 'q' && promo != 'r' && promo != 'b' && promo != 'n') promo = 'q';
        char piecee = isupper(piece) ? toupper(promo) : promo;
        set_sq(pos, index[1], piecee);
    }
}

void make_move(Position *pos, int from, int to, int promo)
{
    int index[3] = {from, to, promo};
    char piece = pos->board[index[0]];
    int old_moved[6];
    memcpy(old_moved, pos->moved, sizeof old_moved);
    if (pos->en_passant[1] != 0) pos->hash ^= zob_ep[pos->en_passant[1] % 8];

    move_push(pos, index);
    en_passant_move_change(pos, index, piece);

    for (int i = 0; i < 6; i++) if (old_moved[i] != pos->moved[i]) pos->hash ^= zob_castle[i];
    if (pos->en_passant[1] != 0) pos->hash ^= zob_ep[pos->en_passant[1] % 8];
    pos->hash ^= zob_side;
    pos->move_number++;
}

int castle_rights(const Position *pos, int king_index, int rook_index)
{
    int king_flag = -1;
    int rook_flag = -1;

    if (king_index == 60) king_flag = 2;
    if (king_index == 4)  king_flag = 4;

    if (rook_index == 56) rook_flag = 0;
    if (rook_index == 63) rook_flag = 1;
    if (rook_index == 0)  rook_flag = 3;
    if (rook_index == 7)  rook_flag = 5;

    if (pos->moved[king_flag] == 1) return 1;
    if (pos->moved[rook_flag] == 1) return 1;
    return 0;
}

void pawn_move(const Position *pos, int color, int indexes[], int *idx, int legalmoves_out[])
{
    if (color == -1) {
        if (48 <= indexes[0] && indexes[0] <= 55
            && pos->board[indexes[0] - 8] == '.' && pos->board[indexes[0] - 16] == '.')
        {
            legalmoves_out[(*idx)++] = indexes[0] - 8;
            legalmoves_out[(*idx)++] = indexes[0] - 16;
        }
        else
        {
            if (indexes[0] - 8 >= 0 && pos->board[indexes[0] - 8] == '.')
            {
                legalmoves_out[(*idx)++] = indexes[0] - 8;
            }
        }
        if (indexes[0] - 7 >= 0 && indexes[0] % 8 != 7
            && is_enemy(pos->board[indexes[0] - 7], color))
        {
            legalmoves_out[(*idx)++] = indexes[0] - 7;
        }
        if (indexes[0] - 9 >= 0 && indexes[0] % 8 != 0
            && is_enemy(pos->board[indexes[0] - 9], color))
        {
            legalmoves_out[(*idx)++] = indexes[0] - 9;
        }
        if (indexes[0] - 1 >= 0 && indexes[0] % 8 != 0 && indexes[0] - 1 == pos->en_passant[1]
            && is_enemy(pos->board[indexes[0] - 1], color) && en_passant(pos, indexes))
        {
            legalmoves_out[(*idx)++] = indexes[0] - 9;
        }
        if (indexes[0] + 1 <= 63 && indexes[0] % 8 != 7 && indexes[0] + 1 == pos->en_passant[1]
            && is_enemy(pos->board[indexes[0] + 1], color) && en_passant(pos, indexes))
        {
            legalmoves_out[(*idx)++] = indexes[0] - 7;
        }
    }
    else
    {
        if (8 <= indexes[0] && indexes[0] <= 15
            && pos->board[indexes[0] + 8] == '.' && pos->board[indexes[0] + 16] == '.')
        {
            legalmoves_out[(*idx)++] = indexes[0] + 8;
            legalmoves_out[(*idx)++] = indexes[0] + 16;
        }
        else
        {
            if (indexes[0] + 8 <= 63 && pos->board[indexes[0] + 8] == '.')
            {
                legalmoves_out[(*idx)++] = indexes[0] + 8;
            }
        }
        if (indexes[0] + 7 <= 63 && indexes[0] % 8 != 0
            && is_enemy(pos->board[indexes[0] + 7], color))
        {
            legalmoves_out[(*idx)++] = indexes[0] + 7;
        }
        if (indexes[0] + 9 <= 63 && indexes[0] % 8 != 7
            && is_enemy(pos->board[indexes[0] + 9], color))
        {
            legalmoves_out[(*idx)++] = indexes[0] + 9;
        }
        if (indexes[0] - 1 >= 0 && indexes[0] % 8 != 0 && indexes[0] - 1 == pos->en_passant[1]
            && is_enemy(pos->board[indexes[0] - 1], color) && en_passant(pos, indexes))
        {
            legalmoves_out[(*idx)++] = indexes[0] + 7;
        }
        if (indexes[0] + 1 <= 63 && indexes[0] % 8 != 7 && indexes[0] + 1 == pos->en_passant[1]
            && is_enemy(pos->board[indexes[0] + 1], color) && en_passant(pos, indexes))
        {
            legalmoves_out[(*idx)++] = indexes[0] + 9;
        }
    }
}

void knight_move(const Position *pos, int color, int indexes[], int *idx, int legalmoves_out[])
{
    if (indexes[0] - 17 >= 0 && indexes[0] % 8 != 0 && pos->board[indexes[0] - 17] == '.')
        legalmoves_out[(*idx)++] = indexes[0] - 17;
    if (indexes[0] - 17 >= 0 && indexes[0] % 8 != 0) {
        if (is_enemy(pos->board[indexes[0] - 17], color)) {
            legalmoves_out[(*idx)++] = indexes[0] - 17;
        }
    }

    if (indexes[0] - 15 >= 0 && indexes[0] % 8 != 7 && pos->board[indexes[0] - 15] == '.')
        legalmoves_out[(*idx)++] = indexes[0] - 15;
    if (indexes[0] - 15 >= 0 && indexes[0] % 8 != 7) {
        if (is_enemy(pos->board[indexes[0] - 15], color)) {
            legalmoves_out[(*idx)++] = indexes[0] - 15;
        }
    }

    if (indexes[0] + 17 <= 63 && indexes[0] % 8 != 7 && pos->board[indexes[0] + 17] == '.')
        legalmoves_out[(*idx)++] = indexes[0] + 17;
    if (indexes[0] + 17 <= 63 && indexes[0] % 8 != 7) {
        if (is_enemy(pos->board[indexes[0] + 17], color)) {
            legalmoves_out[(*idx)++] = indexes[0] + 17;
        }
    }

    if (indexes[0] + 15 <= 63 && indexes[0] % 8 != 0 && pos->board[indexes[0] + 15] == '.')
        legalmoves_out[(*idx)++] = indexes[0] + 15;
    if (indexes[0] + 15 <= 63 && indexes[0] % 8 != 0) {
        if (is_enemy(pos->board[indexes[0] + 15], color)) {
            legalmoves_out[(*idx)++] = indexes[0] + 15;
        }
    }

    if (indexes[0] - 10 >= 0 && indexes[0] % 8 >= 2 && pos->board[indexes[0] - 10] == '.')
        legalmoves_out[(*idx)++] = indexes[0] - 10;
    if (indexes[0] - 10 >= 0 && indexes[0] % 8 >= 2) {
        if (is_enemy(pos->board[indexes[0] - 10], color)) {
            legalmoves_out[(*idx)++] = indexes[0] - 10;
        }
    }

    if (indexes[0] + 6 <= 63 && indexes[0] % 8 >= 2 && pos->board[indexes[0] + 6] == '.')
        legalmoves_out[(*idx)++] = indexes[0] + 6;
    if (indexes[0] + 6 <= 63 && indexes[0] % 8 >= 2) {
        if (is_enemy(pos->board[indexes[0] + 6], color)) {
            legalmoves_out[(*idx)++] = indexes[0] + 6;
        }
    }

    if (indexes[0] + 10 <= 63 && indexes[0] % 8 <= 5 && pos->board[indexes[0] + 10] == '.')
        legalmoves_out[(*idx)++] = indexes[0] + 10;
    if (indexes[0] + 10 <= 63 && indexes[0] % 8 <= 5) {
        if (is_enemy(pos->board[indexes[0] + 10], color)) {
            legalmoves_out[(*idx)++] = indexes[0] + 10;
        }
    }

    if (indexes[0] - 6 >= 0 && indexes[0] % 8 <= 5 && pos->board[indexes[0] - 6] == '.')
        legalmoves_out[(*idx)++] = indexes[0] - 6;
    if (indexes[0] - 6 >= 0 && indexes[0] % 8 <= 5) {
        if (is_enemy(pos->board[indexes[0] - 6], color)) {
            legalmoves_out[(*idx)++] = indexes[0] - 6;
        }
    }
}

void rook_move(const Position *pos, int color, int indexes[], int *idx, int legalmoves_out[])
{
    int uprank, downrank, rightrank, leftrank;
    int filecount = indexes[0] % 8;

    for (uprank = 0;
         indexes[0] - 8 * (uprank + 1) >= 0 && pos->board[indexes[0] - 8 * (uprank + 1)] == '.';
         uprank++) {}
    if (indexes[0] - 8 * (uprank + 1) >= 0) {
        if (is_enemy(pos->board[indexes[0] - 8 * (uprank + 1)], color)) {uprank++;}
    }

    for (downrank = 0;
         indexes[0] + 8 * (downrank + 1) <= 63
         && pos->board[indexes[0] + 8 * (downrank + 1)] == '.';
         downrank++) {}
    if (indexes[0] + 8 * (downrank + 1) <= 63) {
        if (is_enemy(pos->board[indexes[0] + 8 * (downrank + 1)], color)) {downrank++;}
    }

    for (rightrank = 0;
         (filecount + rightrank + 1) <= 7 && pos->board[indexes[0] + (rightrank + 1)] == '.';
         rightrank++) {}
    if ((filecount + rightrank + 1) <= 7) {
        if (is_enemy(pos->board[(indexes[0] + rightrank + 1)], color)) {rightrank++;}
    }

    for (leftrank = 0;
         (filecount - leftrank - 1) >= 0 && pos->board[indexes[0] - (leftrank + 1)] == '.';
         leftrank++) {}
    if ((filecount - leftrank - 1) >= 0) {
        if (is_enemy(pos->board[indexes[0] - (leftrank + 1)], color)) {leftrank++;}
    }

    for (int i = 0; i < uprank; i++)    legalmoves_out[(*idx)++] = indexes[0] - 8 * (i + 1);
    for (int i = 0; i < downrank; i++)  legalmoves_out[(*idx)++] = indexes[0] + 8 * (i + 1);
    for (int i = 0; i < rightrank; i++) legalmoves_out[(*idx)++] = indexes[0] + (i + 1);
    for (int i = 0; i < leftrank; i++)  legalmoves_out[(*idx)++] = indexes[0] - (i + 1);
}

void bishop_move(const Position *pos, int color, int indexes[], int *idx, int legalmoves_out[])
{
    int nw, ne, se, sw;

    for (nw = 0;
         indexes[0] - 9 * (nw + 1) >= 0 && (indexes[0] - 9 * nw) % 8 != 0
         && pos->board[indexes[0] - 9 * (nw + 1)] == '.';
         nw++) {}
    if (indexes[0] - 9 * (nw + 1) >= 0 && (indexes[0] - 9 * nw) % 8 != 0) {
        if (is_enemy(pos->board[indexes[0] - 9 * (nw + 1)], color)) {nw++;}
    }

    for (ne = 0;
         indexes[0] - 7 * (ne + 1) >= 0 && (indexes[0] - 7 * ne) % 8 != 7
         && pos->board[indexes[0] - 7 * (ne + 1)] == '.';
         ne++) {}
    if (indexes[0] - 7 * (ne + 1) >= 0 && (indexes[0] - 7 * ne) % 8 != 7) {
        if (is_enemy(pos->board[indexes[0] - 7 * (ne + 1)], color)) {ne++;}
    }

    for (se = 0;
         indexes[0] + 9 * (se + 1) <= 63 && (indexes[0] + 9 * se) % 8 != 7
         && pos->board[indexes[0] + 9 * (se + 1)] == '.';
         se++) {}
    if (indexes[0] + 9 * (se + 1) <= 63 && (indexes[0] + 9 * se) % 8 != 7) {
        if (is_enemy(pos->board[indexes[0] + 9 * (se + 1)], color)) {se++;}
    }

    for (sw = 0;
         indexes[0] + 7 * (sw + 1) <= 63 && (indexes[0] + 7 * sw) % 8 != 0
         && pos->board[indexes[0] + 7 * (sw + 1)] == '.';
         sw++) {}
    if (indexes[0] + 7 * (sw + 1) <= 63 && (indexes[0] + 7 * sw) % 8 != 0) {
        if (is_enemy(pos->board[indexes[0] + 7 * (sw + 1)], color)) {sw++;}
    }

    for (int i = 0; i < nw; i++) legalmoves_out[(*idx)++] = indexes[0] - 9 * (i + 1);
    for (int i = 0; i < ne; i++) legalmoves_out[(*idx)++] = indexes[0] - 7 * (i + 1);
    for (int i = 0; i < se; i++) legalmoves_out[(*idx)++] = indexes[0] + 9 * (i + 1);
    for (int i = 0; i < sw; i++) legalmoves_out[(*idx)++] = indexes[0] + 7 * (i + 1);
}

void queen_move(const Position *pos, int color, int indexes[], int *idx, int legalmoves_out[])
{
    int nw, ne, se, sw;
    int uprank, downrank, rightrank, leftrank;
    int filecount = indexes[0] % 8;

    for (nw = 0;
         indexes[0] - 9 * (nw + 1) >= 0 && (indexes[0] - 9 * nw) % 8 != 0
         && pos->board[indexes[0] - 9 * (nw + 1)] == '.';
         nw++) {}
    if (indexes[0] - 9 * (nw + 1) >= 0 && (indexes[0] - 9 * nw) % 8 != 0) {
        if (is_enemy(pos->board[indexes[0] - 9 * (nw + 1)], color)) {nw++;}
    }

    for (ne = 0;
         indexes[0] - 7 * (ne + 1) >= 0 && (indexes[0] - 7 * ne) % 8 != 7
         && pos->board[indexes[0] - 7 * (ne + 1)] == '.';
         ne++) {}
    if (indexes[0] - 7 * (ne + 1) >= 0 && (indexes[0] - 7 * ne) % 8 != 7) {
        if (is_enemy(pos->board[indexes[0] - 7 * (ne + 1)], color)) {ne++;}
    }

    for (se = 0;
         indexes[0] + 9 * (se + 1) <= 63 && (indexes[0] + 9 * se) % 8 != 7
         && pos->board[indexes[0] + 9 * (se + 1)] == '.';
         se++) {}
    if (indexes[0] + 9 * (se + 1) <= 63 && (indexes[0] + 9 * se) % 8 != 7) {
        if (is_enemy(pos->board[indexes[0] + 9 * (se + 1)], color)) {se++;}
    }

    for (sw = 0;
         indexes[0] + 7 * (sw + 1) <= 63 && (indexes[0] + 7 * sw) % 8 != 0
         && pos->board[indexes[0] + 7 * (sw + 1)] == '.';
         sw++) {}
    if (indexes[0] + 7 * (sw + 1) <= 63 && (indexes[0] + 7 * sw) % 8 != 0) {
        if (is_enemy(pos->board[indexes[0] + 7 * (sw + 1)], color)) {sw++;}
    }

    for (uprank = 0;
         indexes[0] - 8 * (uprank + 1) >= 0 && pos->board[indexes[0] - 8 * (uprank + 1)] == '.';
         uprank++) {}
    if (indexes[0] - 8 * (uprank + 1) >= 0) {
        if (is_enemy(pos->board[indexes[0] - 8 * (uprank + 1)], color)) {uprank++;}
    }

    for (downrank = 0;
         indexes[0] + 8 * (downrank + 1) <= 63
         && pos->board[indexes[0] + 8 * (downrank + 1)] == '.';
         downrank++) {}
    if (indexes[0] + 8 * (downrank + 1) <= 63) {
        if (is_enemy(pos->board[indexes[0] + 8 * (downrank + 1)], color)) {downrank++;}
    }

    for (rightrank = 0;
         (filecount + rightrank + 1) <= 7 && pos->board[indexes[0] + (rightrank + 1)] == '.';
         rightrank++) {}
    if ((filecount + rightrank + 1) <= 7) {
        if (is_enemy(pos->board[(indexes[0] + rightrank + 1)], color)) {rightrank++;}
    }

    for (leftrank = 0;
         (filecount - leftrank - 1) >= 0 && pos->board[indexes[0] - (leftrank + 1)] == '.';
         leftrank++) {}
    if ((filecount - leftrank - 1) >= 0) {
        if (is_enemy(pos->board[indexes[0] - (leftrank + 1)], color)) {leftrank++;}
    }

    for (int i = 0; i < uprank; i++)    legalmoves_out[(*idx)++] = indexes[0] - 8 * (i + 1);
    for (int i = 0; i < downrank; i++)  legalmoves_out[(*idx)++] = indexes[0] + 8 * (i + 1);
    for (int i = 0; i < rightrank; i++) legalmoves_out[(*idx)++] = indexes[0] + (i + 1);
    for (int i = 0; i < leftrank; i++)  legalmoves_out[(*idx)++] = indexes[0] - (i + 1);
    for (int i = 0; i < nw; i++)        legalmoves_out[(*idx)++] = indexes[0] - 9 * (i + 1);
    for (int i = 0; i < ne; i++)        legalmoves_out[(*idx)++] = indexes[0] - 7 * (i + 1);
    for (int i = 0; i < se; i++)        legalmoves_out[(*idx)++] = indexes[0] + 9 * (i + 1);
    for (int i = 0; i < sw; i++)        legalmoves_out[(*idx)++] = indexes[0] + 7 * (i + 1);
}

void king_move(const Position *pos, int color, int indexes[], int *idx, int legalmoves_out[])
{
    if (indexes[0] - 9 >= 0 && indexes[0] % 8 != 0
        && (pos->board[indexes[0] - 9] == '.' || is_enemy(pos->board[indexes[0] - 9], color)))
        legalmoves_out[(*idx)++] = indexes[0] - 9;
    if (indexes[0] - 8 >= 0
        && (pos->board[indexes[0] - 8] == '.' || is_enemy(pos->board[indexes[0] - 8], color)))
        legalmoves_out[(*idx)++] = indexes[0] - 8;
    if (indexes[0] - 7 >= 0 && indexes[0] % 8 != 7
        && (pos->board[indexes[0] - 7] == '.' || is_enemy(pos->board[indexes[0] - 7], color)))
        legalmoves_out[(*idx)++] = indexes[0] - 7;
    if (indexes[0] - 1 >= 0 && indexes[0] % 8 != 0
        && (pos->board[indexes[0] - 1] == '.' || is_enemy(pos->board[indexes[0] - 1], color)))
        legalmoves_out[(*idx)++] = indexes[0] - 1;
    if (indexes[0] + 1 <= 63 && indexes[0] % 8 != 7
        && (pos->board[indexes[0] + 1] == '.' || is_enemy(pos->board[indexes[0] + 1], color)))
        legalmoves_out[(*idx)++] = indexes[0] + 1;
    if (indexes[0] + 7 <= 63 && indexes[0] % 8 != 0
        && (pos->board[indexes[0] + 7] == '.' || is_enemy(pos->board[indexes[0] + 7], color)))
        legalmoves_out[(*idx)++] = indexes[0] + 7;
    if (indexes[0] + 8 <= 63
        && (pos->board[indexes[0] + 8] == '.' || is_enemy(pos->board[indexes[0] + 8], color)))
        legalmoves_out[(*idx)++] = indexes[0] + 8;
    if (indexes[0] + 9 <= 63 && indexes[0] % 8 != 7
        && (pos->board[indexes[0] + 9] == '.' || is_enemy(pos->board[indexes[0] + 9], color)))
        legalmoves_out[(*idx)++] = indexes[0] + 9;
}


int check_check(const Position *pos, int defender_color)
{
    static const int knight_row_change[8] = {-2, -2, -1, -1, 1, 1, 2, 2}; // for å sjekke hest
    static const int knight_col_change[8] = {-1, 1, -2, 2, -2, 2, -1, 1};

    static const int dir_row_change[8] = {-1, -1, -1, 0, 0, 1, 1, 1}; // for å sjekke retning
    static const int dir_col_change[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

    int target_square = 0;
    if (defender_color == -1) target_square = pos->w_king;
    else if (defender_color == 1) target_square = pos->b_king;


    int target_row = target_square / 8;
    int target_col = target_square % 8;

    for (int i = 0; i < 8; i++)
    {
        int row = target_row + knight_row_change[i];
        int col = target_col + knight_col_change[i];
        if (row < 0 || row > 7 || col < 0 || col > 7) continue;

        char piece = pos->board[row * 8 + col];
        if (is_enemy(piece, defender_color) && (piece == 'n' || piece == 'N')) return 1;
    }

   if (defender_color == -1)
   {
       if (target_row >= 1)
       {
           if (target_col != 7 && pos->board[target_square - 7] == 'p') return 1;
           if (target_col != 0 && pos->board[target_square - 9] == 'p') return 1;
       }
   }
   else
   {
       if (target_row <= 6)
       {
           if (target_col != 0 && pos->board[target_square + 7] == 'P') return 1;
           if (target_col != 7 && pos->board[target_square + 9] == 'P') return 1;
       }
   }
    
    for (int dir = 0; dir < 8; dir++)
    {
        int is_straight_line = (dir_row_change[dir] == 0 || dir_col_change[dir] == 0);

        for (int distance = 1; ; distance++)
        {
            int row = target_row + dir_row_change[dir] * distance;
            int col = target_col + dir_col_change[dir] * distance;
            if (row < 0 || row > 7 || col < 0 || col > 7) break;

            char piece = pos->board[row*8 + col];
            if (piece == '.') continue;

            if (is_enemy(piece, defender_color))
            {
                char type = toupper(piece);
                if (type == 'Q') return 1;
                if (type == 'R' && is_straight_line) return 1;
                if (type == 'B' && !is_straight_line) return 1;
                if (type == 'K' && distance == 1) return 1;
            }
        break;
        }
    }
    return 0;
}


// sjekker om man er i sjakk etter et trekk
int try_move(const Position *pos, int index[3], int color)
{
    Position temp = *pos;
    move_push(&temp, index);
    return check_check(&temp, color);
}

void castle(const Position *pos, int indexes[], int legalmoves_out[], int *idx)
{
    char piece = pos->board[indexes[0]];
    int color;
    if (isupper(piece)) {color = -1;}
    else {color = 1;}

    int step = (indexes[1] > indexes[0]) ? 1 : -1; // 1 = kort rokade, -1 = lang rokade
    int rook_index = (step == 1) ? indexes[0] + 3 : indexes[0] - 4;
    char rook = (color == -1) ? 'R' : 'r';

    if (pos->board[rook_index] != rook) return;
    if (castle_rights(pos, indexes[0], rook_index) == 1) {return;}

    // alle ruter mellom konge og tårn må være tomme
    for (int sq = indexes[0] + step; sq != rook_index; sq += step)
    {
        if (pos->board[sq] != '.') return;
    }

    // kan ikke rokere ut av sjakk eller gjennom en angrepet rute
    if (check_check(pos, color)) return;
    int pass[3] = {indexes[0], indexes[0] + step, 0};
    if (try_move(pos, pass, color)) return;

    int full[3] = {indexes[0], indexes[1], 0};
    if (try_move(pos, full, color)) return;

    legalmoves_out[(*idx)++] = indexes[1];
}

// Sjekker om det er en brikke der man vil flytte fra.
int legal(const char board[], int indexes[]) {
    if (board[indexes[0]] != '.')
    {
        return 1;
    }

    return 0;
}

int on_line_with_king(int king_square, int piece_square)
{
    int row_diff = piece_square / 8 - king_square / 8;
    int col_diff = piece_square % 8 - king_square % 8;
    return row_diff == 0 || col_diff == 0 || row_diff == col_diff || row_diff == -col_diff;
}

// skal finne lovlige trekk for brikken
int piecemove(const Position *pos, char move[], int legalmoves_out[]) {
    int indexes[3];
    move_to_index(move, indexes);
    char piece = pos->board[indexes[0]];

    if (!legal(pos->board, indexes)) {
        printf("trekket er ulovlighehe");
        return 0;
    }

    int color;
    int idx = 0;

    if (isupper(piece)) {
        color = -1;
    } else {
        color = 1;
    }

    if ((pos->move_number % 2 == 0 && isupper(piece))
        || (pos->move_number % 2 == 1 && islower(piece)))
    {
        if (piece == 'k' && indexes[1] == 2 && indexes[0] == 4)
            castle(pos, indexes, legalmoves_out, &idx);
        if (piece == 'k' && indexes[1] == 6 && indexes[0] == 4)
            castle(pos, indexes, legalmoves_out, &idx);
        if (piece == 'K' && indexes[1] == 58 && indexes[0] == 60)
            castle(pos, indexes, legalmoves_out, &idx);
        if (piece == 'K' && indexes[1] == 62 && indexes[0] == 60)
            castle(pos, indexes, legalmoves_out, &idx);

        if (toupper(piece) == 'P') pawn_move(pos, color, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'N') knight_move(pos, color, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'R') rook_move(pos, color, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'B') bishop_move(pos, color, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'Q') queen_move(pos, color, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'K') king_move(pos, color, indexes, &idx, legalmoves_out);

        int count = 0;
        for (int i = 0; i < idx; i++)
        {
            int test[3] = {indexes[0], legalmoves_out[i], 0};
            if (!try_move(pos, test, color)) legalmoves_out[count++] = legalmoves_out[i];
        }

        return count;
    }
    return 0;
}

int piecemove_fast(const Position *pos, int indexes[], int legalmoves_out[], int in_check, char pinned[], int captures_only) {
    if (!legal(pos->board, indexes)) {
        printf("trekket er ulovlighehe");
        return 0;
    }
    char piece = pos->board[indexes[0]];

    int color;
    int idx = 0;

    if (isupper(piece)) {
        color = -1;
    } else {
        color = 1;
    }

    if ((pos->move_number % 2 == 0 && isupper(piece))
        || (pos->move_number % 2 == 1 && islower(piece)))
    {
        if (piece == 'k' && indexes[1] == 2 && indexes[0] == 4)
            castle(pos, indexes, legalmoves_out, &idx);
        if (piece == 'k' && indexes[1] == 6 && indexes[0] == 4)
            castle(pos, indexes, legalmoves_out, &idx);
        if (piece == 'K' && indexes[1] == 58 && indexes[0] == 60)
            castle(pos, indexes, legalmoves_out, &idx);
        if (piece == 'K' && indexes[1] == 62 && indexes[0] == 60)
            castle(pos, indexes, legalmoves_out, &idx);

        if (toupper(piece) == 'P') pawn_move(pos, color, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'N') knight_move(pos, color, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'R') rook_move(pos, color, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'B') bishop_move(pos, color, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'Q') queen_move(pos, color, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'K') king_move(pos, color, indexes, &idx, legalmoves_out);

        int is_king = (toupper(piece) == 'K');

        int count = 0;
        for (int i = 0; i < idx; i++)
        {
            int to = legalmoves_out[i];

            if (captures_only && pos->board[to] == '.') continue;

            int is_en_passant = (toupper(piece) == 'P'
                                && to % 8 != indexes[0] % 8
                                && pos->board[to] == '.');

            int needs_testing = is_king || in_check || pinned[indexes[0]] || is_en_passant;

            if (needs_testing)
            {
                int test[3] = {indexes[0], to, 0};
                if (try_move(pos, test, color)) continue;
            }
            legalmoves_out[count++] = to;
        }
        return count;
    }
    return 0;
}

void add_move(MoveList *list, int from, int to, char promo)
{
    if (list->count >= 256) return;
    list->moves[list->count].from = from;
    list->moves[list->count].to = to;
    list->moves[list->count].promo = promo;
    list->moves[list->count].score = 0;
    list->count++;
}

void find_pin(const Position *pos, int color, char pinned[64])
{
    static const int dir_row_change[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    static const int dir_col_change[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

    memset(pinned, 0, 64);
    int king_square = (color == -1) ? pos->w_king : pos->b_king;
    int king_row = king_square / 8;
    int king_col = king_square % 8;    

    for (int dir = 0; dir < 8; dir++)
    {
        int is_straight_line = (dir_row_change[dir] == 0 || dir_col_change[dir] == 0);
        int own_piece = -1;

        for (int distance = 1; ; distance++)
        {
            int row = king_row + dir_row_change[dir] * distance;
            int col = king_col + dir_col_change[dir] * distance;
            if (row < 0 || row > 7 || col < 0 || col > 7) break;

            int square = row * 8 + col;
            char piece = pos->board[square];
            if (piece == '.') continue;

            if (!is_enemy(piece, color))
            {
                if (own_piece != -1) break;
                own_piece = square;
                continue;
            }

            if (own_piece != -1)
            {
                char type = toupper(piece);
                if ((type == 'Q')
                    || (type == 'R' && is_straight_line)
                    || (type == 'B' && !is_straight_line))
                    pinned[own_piece] = 1;
            }
        break;
        }
    }
}

void scoring(Position *pos, MoveList *list)
{
    for (int i = 0; i < list->count; i++)
    {
        Move *m = &list->moves[i];
        char mover = pos->board[m->from];
        char victim = pos->board[m->to];

        if (victim != '.')
        {
            m->score = 10000 + 10 * piece_value(victim) - piece_value(mover);
        }
        else
        {
            m->score = pst_value(mover, m->to, 0) - pst_value(mover, m->from, 0);
        }
        if (m->promo == 'q') m->score += 900;
    }
}

int comp_moves(const void *a, const void *b)
{
    return ((const Move *)b)->score - ((const Move *)a)->score;
}

void move_generator(const Position *pos, MoveList *list, int captures_only)
{
    list->count = 0;
    int mover_col = (pos->move_number % 2 == 1) ? 1 : -1;
    int in_check;
    char pinned[64];
    find_pin(pos, mover_col, pinned);
    in_check = check_check(pos, mover_col);
    for (int i = 0; i < 64; i++)
    {
        if (pos->board[i] == '.') continue;

        if (mover_col == 1)
        {
            if (pos->board[i] == 'P' || pos->board[i] == 'R' || pos->board[i] == 'K' || pos->board[i] == 'B' ||
            pos->board[i] == 'N' || pos->board[i] == 'Q') continue;
        }
        if (mover_col == -1)
        {
            if (pos->board[i] == 'p' || pos->board[i] == 'r' || pos->board[i] == 'k' || pos->board[i] == 'b' ||
            pos->board[i] == 'n' || pos->board[i] == 'q') continue;
        }

        if (!captures_only && !in_check && ((pos->board[i] == 'k' && i == 4) || (pos->board[i] == 'K' && i == 60)))
        {
            for (int c = 0; c < 2; c++)
            {
                int target = (c == 0) ? i - 2 : i + 2;
                int indexes[2] = {i, target};
                int tmp[1];
                int n = 0;
                castle(pos, indexes, tmp, &n);
                if (n) add_move(list, i, target, 0);
            }
        }

        int legalmoves[32];
        int index[3] = {i, 32, 0};
        int n_moves = piecemove_fast(pos, index, legalmoves, in_check, pinned, captures_only);
        for (int k = 0; k < n_moves; k++)
        {
            if (captures_only && pos->board[legalmoves[k]] == '.') continue;

            if (pos->board[i] == 'P' && legalmoves[k] <= 7)
            {
                add_move(list, i, legalmoves[k], 'q');
                add_move(list, i, legalmoves[k], 'b');
                add_move(list, i, legalmoves[k], 'n');
                add_move(list, i, legalmoves[k], 'r');
            }
            else if (pos->board[i] == 'p' && 56 <= legalmoves[k])
            {
                add_move(list, i, legalmoves[k], 'q');
                add_move(list, i, legalmoves[k], 'b');
                add_move(list, i, legalmoves[k], 'n');
                add_move(list, i, legalmoves[k], 'r');
            }
            else
            {
            add_move(list, i, legalmoves[k], 0);
            }
        }
    }
}

void print_moves(const MoveList *list)
{
    for (int i = 0; i < list->count; i++)
    {
        char text[6];
        move_to_string(list->moves[i], text);
        printf("%s\n", text);
    }
}

void random_move(const Position *pos, char move[5])
{
    int pieces[20];
    int idx = 0;
    for (int i = 0; i < 64; i++)
    {
        if (pos->board[i] != '.')
        {
            if (isupper(pos->board[i]))
            {
            pieces[idx++] = i;
            }
            if (islower(pos->board[i]))
            {
            pieces[idx++] = i;
            }
        }
    }
    int legalmovesb[32];
    int count = 0;
    int pt;
    int selected_piece_index[2];
    selected_piece_index[1] = 31;   

    while (count == 0)
    {
        pt = rand() % idx;
        selected_piece_index[0] = pieces[pt];
        index_to_move(move, selected_piece_index);
        count = piecemove(pos, move, legalmovesb);
    }

    int select_move = rand() % count;
    int indexes[2] = {selected_piece_index[0], legalmovesb[select_move]};

    index_to_move(move, indexes);
}

int count_all_legal_moves(const Position *pos, int color)
{
    int total = 0;
    int legalmoves_out[32];

    for (int i = 0; i < 64; i++)
    {
        char piece = pos->board[i];
        if (piece == '.') continue;

        int piece_color = isupper(piece) ? -1 : 1;
        if (piece_color != color) continue;

        char move[5];
        int pair[2] = {i, 32};
        index_to_move(move, pair);

        int count = piecemove(pos, move, legalmoves_out);
        total += count;
    }

    return total;
}

int game_status(const Position *pos, int color)
{
    int in_check = check_check(pos, color);
    int total_moves = count_all_legal_moves(pos, color);

    if (total_moves == 0)
    {
        if (in_check) return 1; // checkmate
        return 2;               // stalemate
    }

    return 0; // game continues
}

int game_over_check(const Position *pos)
{
    int next_color = (pos->move_number % 2 == 0) ? -1 : 1;
    int status = game_status(pos, next_color);
    if (status == 1) { printf("Checkmate!\n"); return 1; }
    if (status == 2) { printf("Stalemate!\n"); return 1; }
    return 0;
}

void play_random(void)
{
    Position pos;
    init_position(&pos);
    MoveList list;
    char move[10] = "";

    board_print(&pos);
    while (1)
    {
        printf("\nEnter move: ");
        scanf("%9s", move);
        if (strcmp(move, "stopp") == 0) break;
        if (!valid_input(move))
        {
            printf("Write a move like e2e4\n");
            continue;
        }

        int indexes[3];
        move_to_index(move, indexes);
        int legalmoves[32];
        int count = piecemove(&pos, move, legalmoves);

        int found = 0;
        for (int i = 0; i < count; i++)
        {
            if (legalmoves[i] == indexes[1]) found = 1;
        }
        if (!found)
        {
            printf("Illegal move\n");
            continue;
        }

        make_move(&pos, indexes[0], indexes[1], move[4]);
        if (game_over_check(&pos)) { board_print(&pos); break; }

        move_generator(&pos, &list, 0);
        if (list.count == 0) break;
        int r = rand() % list.count;
        int pair[3] = {list.moves[r].from, list.moves[r].to, list.moves[r].promo};
        make_move(&pos, pair[0], pair[1], pair[2]);
        board_print(&pos);
        if (game_over_check(&pos)) break;
    }
}

void set_castle_test(Position *pos) // kun brukt for å teste om generatoren fungerer med rokkade
{
    memset(pos->board, '.', 64);
    pos->board[0] = 'r'; pos->board[4] = 'k'; pos->board[7] = 'r';
    pos->board[56] = 'R'; pos->board[60] = 'K'; pos->board[63] = 'R';
    memset(pos->moved, 0, sizeof(pos->moved));
    pos->en_passant[0] = 0;
    pos->en_passant[1] = 0;
    pos->move_number = 0;
}

void set_promo_test(Position *pos, MoveList *list) //kun brukt for å teste om generatoren fungerer med forskjellige promotions
{
    memset(pos->board, '.', 64);
    pos->board[8]  = 'P';
    pos->board[1]  = 'r';
    pos->board[7]  = 'k';
    pos->board[60] = 'K';
    memset(pos->moved, 0, sizeof(pos->moved));
    pos->en_passant[0] = 0;
    pos->en_passant[1] = 0;
    pos->move_number = 0;
    move_generator(pos, list, 0);
}

void play(void)
{
    Position pos;
    init_position(&pos);
    char move[10] = "";
    int game_over = 0;
    while (strcmp(move, "stopp") != 0 && !game_over)
    {
        printf("\nEnter move: ");
        scanf("%9s", move);
        if (strcmp(move, "stopp") == 0) break;

        int indexes[3];
        move_to_index(move, indexes);

        int legalmoves[32];
        int count = piecemove(&pos, move, legalmoves);

        char moves[5];
        for (int i = 0; i < count; i++)
        {
            int pair[2] = {indexes[0], legalmoves[i]};
            index_to_move(moves, pair);
            if (legalmoves[i] == indexes[1])
            {
                make_move(&pos, indexes[0], indexes[1], move[4]);
                int next_color = (pos.move_number % 2 == 0) ? -1 : 1;
                int status = game_status(&pos, next_color);
                if (status == 1) {
                    printf("Checkmate!\n");
                    board_print(&pos);
                    game_over = 1;
                    break;
                }
                if (status == 2) {
                    printf("Stalemate!\n");
                    board_print(&pos);
                    game_over = 1;
                    break;
                }
                board_print(&pos);
            }
        }
    }
}

int perft(Position *pos, int depth)
{
    int sum = 0;
    if (depth == 0)
    {
        return 1;
    }
    MoveList list;
    move_generator(pos, &list, 0);
    for (int k = 0; k < list.count; k++)
    {
        Position temp = *pos;
        make_move(&temp, list.moves[k].from, list.moves[k].to, list.moves[k].promo);
        sum += perft(&temp, depth-1);
    }
    return sum;
}

void per(void) //for å sjekke at den finner trekk riktig
{
    int start = clock();
    int depth = 6;
    Position pos;
    init_position(&pos);
    int nodes = perft(&pos, depth);
    printf("\nTotalt trekk med %d i dybde: %d", depth, nodes);
    int end = clock() - start;
    printf("Tid: %f", (float)end/CLOCKS_PER_SEC);
}

// Faktisk motoren som skal bestemme trekk

static const int passed_mg[8] = {0,  5, 10, 15, 25,  40,  60, 0}; // bonus for bønder lenger frem
static const int passed_eg[8] = {0, 10, 20, 40, 70, 120, 200, 0}; // tenkt til å bruke i endgames

static int sq_distance(int a, int b) // antall kongetrekk mellom to ruter
{
    int dr = abs(a / 8 - b / 8);
    int dc = abs(a % 8 - b % 8);
    return dr > dc ? dr : dc;
}

int evaluate(Position *pos)
{
    int phase = 0;
    int w_mat = 0;
    int b_mat = 0;

    int w_pawns[8], b_pawns[8];
    int n_wp = 0, n_bp = 0;
    int max_w_row[8], min_b_row[8];

    for (int f = 0; f < 8; f++) {max_w_row[f] = -1; min_b_row[f] = 8;}


    for (int i = 0; i < 64; i++)
    {
        char piece = pos->board[i];
        if (piece != '.')
        {
            switch (piece)
            {
            case 'N': case 'n': case 'B': case 'b': phase += 1; break;
            case 'R': case 'r': phase +=2; break;
            case 'Q': case 'q': phase +=4; break;
            case 'P':
                w_pawns[n_wp++] = i;
                if (i / 8 > max_w_row[i % 8]) max_w_row[i % 8] = i / 8;
                break;
            case 'p':
                b_pawns[n_bp++] = i;
                if (i / 8 < min_b_row[i % 8]) min_b_row[i % 8] = i / 8;
                break;            
            }

            if (piece == 'K' || piece == 'k') continue;

            if (isupper(piece)) w_mat += piece_value(piece) + pst_value(piece, i, 0);
            else b_mat += piece_value(piece) + pst_value(piece, i, 0);
        }
    }
    if (phase > 24) phase = 24;
    int eg_weight = 24 - phase;

    int king_mid = pst_value('K', pos->w_king, 0) - pst_value('k', pos->b_king, 0);
    int king_end = pst_value('K', pos->w_king, 1) - pst_value('k', pos->b_king, 1);
    int king = (king_mid * phase + king_end * (24 - phase)) / 24;
    
    int passed = 0;

    for (int k = 0; k < n_wp; k++)
    {
        int sq = w_pawns[k];
        int row = sq / 8;
        int file = sq % 8;
        int is_passed = 1;

        for (int f = file - 1; f <= file + 1; f++)
        {
            if (f >= 0 && f <= 7 && min_b_row[f] < row) {is_passed = 0; break;}
        }
        if (!is_passed) continue;

        int rel = 7 - row;
        int front = sq - 8;
        int bonus = (passed_mg[rel] * phase + passed_eg[rel] * eg_weight) / 24;
        int prox = (4 * sq_distance(pos->b_king, front) - 2 * sq_distance(pos->w_king, front)) * rel;
        passed += bonus + prox * eg_weight / 24;
    }

    for (int k = 0; k < n_bp; k++)
    {
        int sq = b_pawns[k];
        int row = sq / 8;
        int file = sq % 8;
        int is_passed = 1;

        for (int f = file - 1; f <= file + 1; f++)
        {
            if (f >= 0 && f <= 7 && max_w_row[f] > row) {is_passed = 0; break;}
        }
        if (!is_passed) continue;

        int rel = row;
        int front = sq + 8;
        int bonus = (passed_mg[rel] * phase + passed_eg[rel] * eg_weight) / 24;
        int prox = (4 * sq_distance(pos->w_king, front) - 2 * sq_distance(pos->b_king, front)) * rel;
        passed -= bonus + prox * eg_weight / 24;
    }


    int score = w_mat - b_mat + king + passed;
    return (pos->move_number % 2 == 0) ? score : -score;

}

static clock_t search_start;
static int hard_limit_ms = 2000000000;
static int can_stop = 0;
static int stop_search = 0;
static long long nodes = 0;
static int root_score = 0;

static int elapsed_ms(void)
{
    return (int)((clock() - search_start) * 1000 / CLOCKS_PER_SEC);
}



int quiesce(Position *pos, int alpha, int beta)
{
    nodes++;
    if (can_stop && (nodes & 2047) == 0 && elapsed_ms() >= hard_limit_ms) stop_search = 1;
    if (stop_search) return 0;
    int stand_pat = evaluate(pos);
    if (stand_pat >= beta) return stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;

    MoveList list;
    move_generator(pos, &list, 1);
    scoring(pos, &list);

    for (int n = 0; n < list.count; n++)
    {
        int best_index = n;
        for (int j = n + 1; j < list.count; j++)
        {
            if (list.moves[j].score > list.moves[best_index].score) best_index = j;
        }

        Move tmp = list.moves[n];
        list.moves[n] = list.moves[best_index];
        list.moves[best_index] = tmp;
        Move m = list.moves[n];

        if (m.score < 10000) break;

        Position temp = *pos;
        make_move(&temp, m.from, m.to, m.promo);
        int score = -quiesce(&temp, -beta, -alpha);
        if (stop_search) return 0;

        if (score >= beta) return score;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int has_non_pawn(Position *pos, int color)
{
    for (int i = 0; i < 64; i++)
    {
        char p = pos->board[i];
        if (p == '.' || p == 'P' || p == 'p' || p == 'K' || p == 'k') continue;
        if ((color == -1) == (isupper(p) != 0)) return 1; 
    }
    return 0;
}


int negamax(Position *pos, int depth, int alpha, int beta, int allow_null)
{
    nodes++;
    if (can_stop && (nodes & 2047) == 0 && elapsed_ms() >= hard_limit_ms) stop_search = 1;
    if (stop_search) return 0;
    int color = (pos->move_number % 2 == 0) ? -1 : 1;

    int ply = pos->move_number - root_move_number;
    if (ply >= MAX_PLY) ply = MAX_PLY - 1;
    int side = pos->move_number % 2;

    int in_check = check_check(pos, color);
    if (in_check) depth++;

    if (depth == 0)
    {
        return quiesce(pos, alpha, beta);
    }

    int alpha_orig = alpha;
    TTEntry *e = &tt[pos->hash & (TT_SIZE - 1)];
    int has_tt_move = 0;

    if (e->key == pos->hash)
    {
        has_tt_move = 1;
        if (e->depth >= depth)
        {
            if (e->flag == TT_EXACT) return e->score;
            if (e->flag == TT_LOWER && e->score >= beta) return e->score;
            if (e->flag == TT_UPPER && e->score <= alpha) return e->score;
        }
    }


    if (allow_null && !in_check && depth >= 3 && has_non_pawn(pos, color))
    {
        Position temp = *pos;
        if (temp.en_passant[1] != 0) temp.hash ^=zob_ep[temp.en_passant[1] % 8];
        temp.en_passant[0] = 0;
        temp.en_passant[1] = 0;
        temp.move_number++;
        temp.hash ^= zob_side;

        int R = 2;
        int score = -negamax(&temp, depth - 1 - R, -beta, -beta + 1, 0);
        if (stop_search) return 0;
        if (score >= beta) return beta;
    }


    MoveList list; 
    move_generator(pos, &list, 0);

    scoring(pos, &list);

    if (list.count == 0)
    {
        if (in_check) return -(100000 + depth);
        return 0;
    }

    if (has_tt_move)
    {
        for (int i = 0; i < list.count; i++)
        {
            if (list.moves[i].from == e->from && list.moves[i].to == e->to
                && list.moves[i].promo == e->promo)
                list.moves[i].score = 1000000;
        }
    }

    for (int i = 0; i < list.count; i++)
    {
        Move *mv = &list.moves[i];
        if (mv->score >= 10000) continue;
        if (pos->board[mv->to] != '.' || mv->promo) continue;

        if (mv->from == killers[ply][0].from && mv->to == killers[ply][0].to) mv->score = 9000;
        else if (mv->from == killers[ply][1].from && mv->to == killers[ply][1].to) mv->score = 8000;
        else mv->score += history[side][mv->from][mv->to];
    }
    
    int best_score = -10000000;
    Move best_m = {0, 0, 0, 0};

    for (int move_n = 0; move_n < list.count; move_n++)
    {
        int best_index = move_n;
        for (int j = move_n + 1; j < list.count; j++)
        {
            if (list.moves[j].score > list.moves[best_index].score) best_index = j;
        }
        Move tmp = list.moves[move_n];
        list.moves[move_n] = list.moves[best_index];
        list.moves[best_index] = tmp;

        Move m = list.moves[move_n];
        int is_capture = (pos->board[m.to] != '.');

        Position temp = *pos;
        make_move(&temp, m.from, m.to, m.promo);

        int score;
        if (move_n >= 3 && depth >= 3 && !in_check && !is_capture && !m.promo)
        {
            score = -negamax(&temp, depth - 2, -alpha - 1, -alpha, 1);

            if (score > alpha)
            {
                score = -negamax(&temp, depth - 1, -beta, -alpha, 1);
            }
        }
        else
        {
            score = -negamax(&temp, depth - 1, -beta, -alpha, 1);
        }
        if (stop_search) return 0;
        if (score > best_score)
        {
            best_score = score;
            best_m = m;
        }
        if (best_score > alpha) alpha = best_score;
        if (alpha >= beta)
        {
            if (!is_capture && !m.promo && killers[ply][0].to == m.to)
            {
                killers[ply][1] = killers[ply][0];
                killers[ply][0] = m;
            }

            history[side][m.from][m.to] += depth * depth;
            if (history[side][m.from][m.to] > 7000)
            {
                for (int f = 0; f < 64; f++)
                {
                    for (int t = 0; t < 64; t++) history[side][f][t] /= 2;
                }
            }
            break;
        }

    }
    e->key = pos->hash;
    e->depth = depth;
    e->score = best_score;
    e->flag = (best_score <= alpha_orig) ? TT_UPPER
            : (best_score >= beta) ? TT_LOWER
            : TT_EXACT;
    e->from = best_m.from;
    e->to = best_m.to;
    e->promo = best_m.promo;
    return best_score;
}


Move best_move(Position *pos, int depth, Move first)
{
    MoveList list;
    move_generator(pos, &list, 0);

    int best_score = -10000000;

    scoring(pos, &list);

    for (int i = 0; i < list.count; i++)
    {
        if (list.moves[i].from == first.from && list.moves[i].to == first.to
            && list.moves[i].promo == first.promo) list.moves[i].score = 100000;
    }

    qsort(list.moves, list.count, sizeof(Move), comp_moves);
    Move best = list.moves[0];

    int alpha = -10000000;
    int beta = 10000000;
    for (int i = 0; i < list.count; i++)
    {
        Position temp = *pos;
        make_move(&temp, list.moves[i].from, list.moves[i].to, list.moves[i].promo);
        int score = -negamax(&temp, depth - 1, -beta, -alpha, 1);
        if (stop_search) break;
        if (score > best_score)
        {
            best_score = score;
            best = list.moves[i];
        }
        if (best_score > alpha) alpha = best_score;
    }
    root_score = best_score;
    return best;
}

Move think(Position *pos, int soft_ms, int hard_ms, int max_depth)
{
    search_start = clock();
    hard_limit_ms = hard_ms;
    stop_search = 0;
    nodes = 0;
    Move best = {0, 0, 0, 0};

    root_move_number = pos->move_number;
    memset(killers, 0, sizeof killers);
    for (int s = 0; s < 2; s++)
    {
        for (int f = 0; f < 64; f++)
        {
            for (int t = 0; t < 64; t++)
            {
                history[s][f][t] /= 2;
            }
        }
    }

    for (int depth = 1; depth <= max_depth; depth++)
    {
        can_stop = (depth > 1);
        best = best_move(pos, depth, best);
        if (stop_search) break;

        int used_ms = elapsed_ms();
        printf("info depth %d score cp %d nodes %lld time %d\n", depth, root_score, nodes, used_ms);
        fflush(stdout);

        if (used_ms >= soft_ms) break;
    }
    can_stop = 0;
    hard_limit_ms = 5000;
    return best;
}

//claude som ga for å spille i cute chess engine
static void parse_go(const char *line, int white_to_move, int *soft_ms, int *hard_ms, int *max_depth)
{
    int wtime = -1, btime = -1, winc = 0, binc = 0, movestogo = 0, movetime = -1, depth = 0;
    const char *p;
    if ((p = strstr(line, "wtime")))     sscanf(p + 5, "%d", &wtime);
    if ((p = strstr(line, "btime")))     sscanf(p + 5, "%d", &btime);
    if ((p = strstr(line, "winc")))      sscanf(p + 4, "%d", &winc);
    if ((p = strstr(line, "binc")))      sscanf(p + 4, "%d", &binc);
    if ((p = strstr(line, "movestogo"))) sscanf(p + 9, "%d", &movestogo);
    if ((p = strstr(line, "movetime")))  sscanf(p + 8, "%d", &movetime);
    if ((p = strstr(line, "depth")))     sscanf(p + 5, "%d", &depth);

    *max_depth = 64;
    int time = white_to_move ? wtime : btime;
    int inc  = white_to_move ? winc : binc;

    if (depth > 0)
    {
        *max_depth = depth;
        *soft_ms = *hard_ms = 2000000000;
    }
    else if (movetime > 0)
    {
        int t = movetime - 30;
        if (t < 10) t = 10;
        *soft_ms = t / 2;
        *hard_ms = t;
    }
    else if (time >= 0)
    {
        int mtg = (movestogo > 0) ? movestogo : 30;
        int budget = time / mtg + inc * 3 / 4;
        int hard = budget * 3;
        if (hard > time / 2) hard = time / 2;
        if (hard < 10) hard = 10;
        if (budget > hard) budget = hard;
        *soft_ms = budget / 2;
        *hard_ms = hard;
    }
    else
    {
        *soft_ms = 2500;
        *hard_ms = 5000;
    }
}

//claude igjen
void uci_loop(void)
{
    Position pos;
    init_position(&pos);
    char line[2048];

    while (fgets(line, sizeof line, stdin))
    {
        line[strcspn(line, "\r\n")] = '\0';

        if (strcmp(line, "uci") == 0)
        {
            printf("id name SjakkMotor\n");
            printf("id author Marius\n");
            printf("uciok\n");
        }
        else if (strcmp(line, "isready") == 0)
        {
            printf("readyok\n");
        }
        else if (strcmp(line, "ucinewgame") == 0)
        {
            init_position(&pos);
        }
        else if (strncmp(line, "position", 8) == 0)
        {
            init_position(&pos);
            char *moves = strstr(line, "moves");
            if (moves != NULL)
            {
                char *tok = strtok(moves + 5, " ");
                while (tok != NULL)
                {
                    int idx[2];
                    move_to_index(tok, idx);
                    make_move(&pos, idx[0], idx[1], tok[4]);
                    tok = strtok(NULL, " ");
                }
            }
        }
        else if (strncmp(line, "go", 2) == 0)
        {
            int soft_ms, hard_ms, max_depth;
            parse_go(line, pos.move_number % 2 == 0, &soft_ms, &hard_ms, &max_depth);
            Move m = think(&pos, soft_ms, hard_ms, max_depth);

            char text[6];
            move_to_string(m, text);
            printf("bestmove %s\n", text);
        }
        else if (strcmp(line, "quit") == 0)
        {
            break;
        }
        fflush(stdout);
    }
}

void motor(void)
{
    Position pos;
    init_rook_endgame(&pos);
    char move[10] = "";

    board_print(&pos);
    while (1)
    {
        printf("\nEnter move: ");
        scanf("%9s", move);
        if (strcmp(move, "stopp") == 0) break;
        if (!valid_input(move))
        {
            printf("Write a move like e2e4\n");
            continue;
        }

        int indexes[2];
        move_to_index(move, indexes);
        int legalmoves[32];
        int count = piecemove(&pos, move, legalmoves);

        int found = 0;
        for (int i = 0; i < count; i++)
        {
            if (legalmoves[i] == indexes[1]) found = 1;
        }
        if (!found)
        {
            printf("Illegal move\n");
            continue;
        }

        make_move(&pos, indexes[0], indexes[1], move[4]);
        if (game_over_check(&pos)) { board_print(&pos); break; }

        Move move;
        move = think(&pos, 1250, 2500, 64);
        make_move(&pos, move.from, move.to, move.promo);
        board_print(&pos);
    }
}

void time_it(void)
{
    int start1 = clock();
    int depth1 = 9;
    Position pos1;
    init_kiwipete(&pos1);
    think(&pos1, 200000, 200000, 9);
    int end1 = clock() - start1;
    printf("\nTid med dybde %d: %f", depth1, (float)end1/CLOCKS_PER_SEC);
}

int main(int argc, char *argv[])
{
    init_zobrist();
    srand(time(NULL));
    if ((argc > 1 && strcmp(argv[1], "uci") == 0) || !_isatty(_fileno(stdin)))
    {
        uci_loop();
        return 0;
    }

    char valg[10];
    printf("\nMotoren (m), vanlig (v), random (r) eller perft (p): ");
    scanf("%9s", valg);
    if (strcmp(valg, "v") == 0) play();
    else if (strcmp(valg, "r") == 0) play_random();
    else if (strcmp(valg, "p") == 0) per();
    else if (strcmp(valg, "m") == 0) motor();
    else if (strcmp(valg, "t") == 0) time_it();
    else printf("Choose v, r, p or m\n");

    return 0;
}