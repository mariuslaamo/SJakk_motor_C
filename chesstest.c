#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <io.h>

typedef struct {
    char board[64];
    int moved[6];
    int en_passant[2];
    int move_number;
    int w_king;
    int b_king;
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

typedef struct { int from; int to; char promo; } Move;
typedef struct { Move moves[256]; int count; } MoveList;

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

void init_position(Position *pos)
{
    memcpy(pos->board, start_board, 64);
    memset(pos->moved, 0, sizeof(pos->moved));
    pos->en_passant[0] = 0;
    pos->en_passant[1] = 0;
    pos->move_number = 0;
    pos->b_king = 4;
    pos->w_king = 60;
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

void en_passant_move_change(Position *pos, char move[], char piece)
{
    int index[2];
    move_to_index(move, index);
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

void move_push(Position *pos, char move[])
{
    int index[2];
    move_to_index(move, index);

    char piece = pos->board[index[0]];

    if (en_passant(pos, index) == 1)
    {
        // fjerner bare bonden hvis trekket faktisk er en passant
        // (går til ruta bak bonden som gikk to frem)
        if (isupper(piece) && index[1] + 8 == pos->en_passant[1])
        {
            pos->board[index[1]+8] = '.';
        }
        if (islower(piece) && index[1] - 8 == pos->en_passant[1])
        {
            pos->board[index[1]-8] = '.';
        }
    }

    if (piece == 'K' && index[0] == 60 && index[1] == 62) {
        pos->board[63] = '.';
        pos->board[61] = 'R';
    }
    if (piece == 'K' && index[0] == 60 && index[1] == 58) {
        pos->board[56] = '.';
        pos->board[59] = 'R';
    }
    if (piece == 'k' && index[0] == 4 && index[1] == 6) {
        pos->board[7] = '.';
        pos->board[5] = 'r';
    }
    if (piece == 'k' && index[0] == 4 && index[1] == 2) {
        pos->board[0] = '.';
        pos->board[3] = 'r';
    }

    if (index[0] == 56 || index[1] == 56) pos->moved[0] = 1;
    if (index[0] == 63 || index[1] == 63) pos->moved[1] = 1;
    if (index[0] == 60 || index[1] == 60) pos->moved[2] = 1;
    if (index[0] == 0 || index[1] == 0) pos->moved[3] = 1;
    if (index[0] == 4 || index[1] == 4) pos->moved[4] = 1;
    if (index[0] == 7 || index[1] == 7) pos->moved[5] = 1;

    if (piece == 'K') pos->w_king = index[1];
    if (piece == 'k') pos->b_king = index[1];

    pos->board[index[0]] = '.';
    pos->board[index[1]] = piece;

    if ((piece == 'P' && index[1] <= 7) || (piece == 'p' && index[1] >= 56))
    {
        char promo = tolower(move[4]);
        if (promo != 'q' && promo != 'r' && promo != 'b' && promo != 'n') promo = 'q';
        pos->board[index[1]] = isupper(piece) ? toupper(promo) : promo;
    }
}

void make_move(Position *pos, char move[])
{
    int index[2];
    move_to_index(move, index);
    char piece = pos->board[index[0]];
    move_push(pos, move);
    en_passant_move_change(pos, move, piece);
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
int try_move(const Position *pos, char move[5], int color)
{
    Position temp = *pos;
    move_push(&temp, move);
    return check_check(&temp, color);
}

void castle(const Position *pos, int indexes[], int legalmoves_out[], int *idx)
{
    char piece = pos->board[indexes[0]];
    int color;
    if (isupper(piece)) {color = -1;}
    else {color = 1;}

    char movemove[5];
    index_to_move(movemove, indexes);

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
    int pass[2] = {indexes[0], indexes[0] + step};
    char pass_move[5];
    index_to_move(pass_move, pass);
    if (try_move(pos, pass_move, color)) return;

    if (try_move(pos, movemove, color)) return;

    legalmoves_out[(*idx)++] = indexes[1];
}

// Sjekker om det er en brikke der man vil flytte fra.
int legal(const char board[], char move[]) {
    int indexes[2];
    move_to_index(move, indexes);

    if (board[indexes[0]] != '.')
    {
        return 1;
    }

    return 0;
}

// skal finne lovlige trekk for brikken
int piecemove(const Position *pos, char move[], int legalmoves_out[]) {
    if (!legal(pos->board, move)) {
        printf("trekket er ulovlighehe");
        return 0;
    }

    int indexes[2];
    move_to_index(move, indexes);
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

        int count = 0;
        for (int i = 0; i < idx; i++)
        {
            char test_move[5];
            int pair[2] = {indexes[0], legalmoves_out[i]};
            index_to_move(test_move, pair);
            if (!try_move(pos, test_move, color)) legalmoves_out[count++] = legalmoves_out[i];
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
    list->count++;
}

void move_generator(const Position *pos, MoveList *list)
{
    list->count = 0;
    int mover_col = (pos->move_number % 2 == 1) ? 1 : -1;
    for (int i = 0; i < 64; i++)
    {   
        if (pos->board[i] == '.') continue;

        if (mover_col == -1)
        {
            if (pos->board[i] == 'P' || pos->board[i] == 'R' || pos->board[i] == 'K' || pos->board[i] == 'B' ||
            pos->board[i] == 'N' || pos->board[i] == 'Q') continue;
        }
        if (mover_col == 1)
        {
            if (pos->board[i] == 'p' || pos->board[i] == 'r' || pos->board[i] == 'k' || pos->board[i] == 'b' ||
            pos->board[i] == 'n' || pos->board[i] == 'q') continue;
        }

        if ((pos->board[i] == 'k' && i == 4) || (pos->board[i] == 'K' && i == 60))
        {
            char *castles[2];
            if (i == 4) { castles[0] = "e8c8"; castles[1] = "e8g8"; }
            else { castles[0] = "e1c1"; castles[1] = "e1g1"; }

            for (int c = 0; c < 2; c++)
            {
                int tmp[32];
                int n = piecemove(pos, castles[c], tmp);
                int target = (c == 0) ? i - 2 : i + 2;
                for (int k = 0; k < n; k++)
                    if (tmp[k] == target) add_move(list, i, target, 0);
            }
        }

        int legalmoves[32];
        int index[2] = {i, 32};
        char move[5];
        index_to_move(move, index);
        int n_moves = piecemove(pos, move, legalmoves);
        for (int k = 0; k < n_moves; k++)
        {
            if (pos->board[i] == 'P' && legalmoves[k] <= 7)
            {
                add_move(list, i, legalmoves[k], 'q');
                add_move(list, i, legalmoves[k], 'b');
                add_move(list, i, legalmoves[k], 'n');
                add_move(list, i, legalmoves[k], 'r');
            }
            else if (pos->board[i] == 'p' && 55 <= legalmoves[k])
            {
                add_move(list, i, legalmoves[k], 'q');
                add_move(list, i, legalmoves[k], 'b');
                add_move(list, i, legalmoves[k], 'n');
                add_move(list, i, legalmoves[k], 'r');
            }
            add_move(list, i, legalmoves[k], 0);
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

        make_move(&pos, move);
        if (game_over_check(&pos)) { board_print(&pos); break; }

        move_generator(&pos, &list);
        if (list.count == 0) break;
        int r = rand() % list.count;
        int pair[2] = {list.moves[r].from, list.moves[r].to};
        char bot_move[6];
        index_to_move(bot_move, pair);
        make_move(&pos, bot_move);
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
    move_generator(pos, list);
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

        int indexes[2];
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
                char piece = pos.board[indexes[0]];
                move_push(&pos, move);
                en_passant_move_change(&pos, move, piece);
                pos.move_number++;
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

    move_generator(pos, &list);
    for (int k = 0; k < list.count; k++)
    {
        Position temp = *pos;
        int pair[2] = {list.moves[k].from, list.moves[k].to};
        char text[5];
        index_to_move(text, pair);
        make_move(&temp, text);
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

int evaluate(Position *pos)
{
    //int endgame = is_endgame(pos) //TODO
    int w_mat = 0;
    int b_mat = 0;
    for (int i = 0; i < 64; i++)
    {
        char piece = pos->board[i];
        if (piece != '.')
        {
            if (isupper(piece)) w_mat += piece_value(piece);
            else b_mat += piece_value(piece);

            if (isupper(piece)) w_mat += pst_value(piece, i, 0);
            else b_mat += pst_value(piece, i, 0);
        }

    }

    return (pos->move_number % 2 == 0) ? (w_mat - b_mat) : (b_mat - w_mat);

}


int negamax(Position *pos, int depth, int alpha, int beta)
{
    if (depth == 0)
    {
        return evaluate(pos);
    }

    MoveList list; 
    move_generator(pos, &list);

    if (list.count == 0)
    {
        int color = (pos->move_number % 2 == 0) ? -1 : 1;
        if (check_check(pos, color)) return -(100000 + depth);
        return 0;
    }
    
    int best_score = -100000;
    for (int move_n = 0; move_n < list.count; move_n++)
    {
        char text[6];
        move_to_string(list.moves[move_n], text);
        Position temp = *pos;
        make_move(&temp, text);
        int score = -negamax(&temp, depth - 1, -beta, -alpha);
        if (score > best_score)
        {
            best_score = score;
        }
        if (best_score > alpha) alpha = best_score;
        if (alpha >= beta) break;

    }
    return best_score;
}


Move best_move(Position *pos, int depth)
{
    MoveList list;
    move_generator(pos, &list);

    Move best = list.moves[0];
    int best_score = -100000;


    int alpha = -100000;
    int beta = 100000;
    for (int i = 0; i < list.count; i++)
    {
        char text[6];
        move_to_string(list.moves[i], text);
        Position temp = *pos;
        make_move(&temp, text);
        int score = -negamax(&temp, depth - 1, -beta, -alpha);
        if (score > best_score)
        {
            best_score = score;
            best = list.moves[i];
        }
        if (best_score > alpha) alpha = best_score;
    }
    return best;
}

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
                    make_move(&pos, tok);
                    tok = strtok(NULL, " ");
                }
            }
        }
        else if (strncmp(line, "go", 2) == 0)
        {
            Move m = best_move(&pos, 4);
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
    init_position(&pos);
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

        make_move(&pos, move);
        if (game_over_check(&pos)) { board_print(&pos); break; }

        Move move;
        move = best_move(&pos, 4);
        char best[6];
        move_to_string(move, best);
        make_move(&pos, best);
        board_print(&pos);
    }
}

void time_it(void)
{
    int start1 = clock();
    int depth1 = 8;
    Position pos1;
    init_kiwipete(&pos1);
    best_move(&pos1, depth1);
    int end1 = clock() - start1;
    printf("\nTid med dybde %d: %f", depth1, (float)end1/CLOCKS_PER_SEC);
}

int main(int argc, char *argv[])
{
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