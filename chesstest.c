#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

char board[64] = {
    'r','n','b','q','k','b','n','r', // black
    'p','p','p','p','p','p','p','p',
    '.','.','.','.','.','.','.','.',
    '.','.','.','.','.','.','.','.',
    '.','.','.','.','.','.','.','.',
    '.','.','.','.','.','.','.','.',
    'P','P','P','P','P','P','P','P', // white
    'R','N','B','Q','K','B','N','R'
};

int moved[6] = {0, 0, 0, 0, 0, 0}; 

char files[8] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'
};

char brikker[6] = {
    'p', 'n', 'b', 'r', 'q', 'k'
};


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

void move_to_index(char move[], int out[2]) { // gjør om et trekk (e2e4) til en index 
    char from_file = move[0];
    char from_rank = move[1];
    char to_file = move[2];
    char to_rank = move[3];
    int filecount;

    for (filecount = 0; from_file != files[filecount]; filecount++){}
    int rank = from_rank - '0';
    out[0] = filecount + 8*(8 - rank);

    for (filecount = 0; to_file != files[filecount]; filecount++){}
    int rank2 = to_rank - '0';
    out[1] = filecount + 8*(8 - rank2);
}

void index_to_move(char move[5], int in[2]) { // gjør om en index til et trekk (e2e4)
    int from_file = in[0] % 8;
    int from_rank = 8 - (in[0]/8);
    int to_file = in[1] % 8;
    int to_rank = 8 - (in[1]/8);

    move[0] = files[from_file];
    move[1] = '0' + from_rank;
    move[2] = files[to_file];
    move[3] = '0' + to_rank;
    move[4] = '\0';    
}


void board_print()
{
    for (int i = 0; i < 64; i++) 
    {
        if (i % 8 == 0) 
        {
            printf("%d ", 8 - (i / 8));
            printf(" ");
        }

        printf("%c", board[i]);

        if (i % 8 == 7) 
        {
            printf("\n");
        }
    }   
    printf("\n");
    printf("   ");
    for (int k = 0; k < 8; k++) 
    {
        printf("%c", files[k]);
    }
    printf("\n");
}

void move_push(char board[], char move[])
{
    int index[2];
    move_to_index(move, index);

    if (index[0] == 56) moved[0] = 1;
    if (index[0] == 63) moved[1] = 1;
    if (index[0] == 60) moved[2] = 1;
    if (index[0] == 0) moved[3] = 1;
    if (index[0] == 4) moved[4] = 1;
    if (index[0] == 7) moved[5] = 1;

    char piece = board[index[0]];
    board[index[0]] = '.';
    board[index[1]] = piece;
}

int castle_rights(char board[], int king_index, int rook_index)
{
    int king_flag = -1;
    int rook_flag = -1;

    if (king_index == 60) king_flag = 2;
    if (king_index == 4)  king_flag = 4;

    if (rook_index == 56) rook_flag = 0;
    if (rook_index == 63) rook_flag = 1;
    if (rook_index == 0)  rook_flag = 3;
    if (rook_index == 7)  rook_flag = 5;

    if (moved[king_flag] == 1) return 1;
    if (moved[rook_flag] == 1) return 1;
    return 0;
}

void pawn_move(int color, char board[], int indexes[], int *idx, int legalmoves_out[])
{
    if (color == -1) {
                if (48 <= indexes[0] && indexes[0] <= 55 && board[indexes[0] - 8] == '.' && board[indexes[0] - 16] == '.') 
                {
                    legalmoves_out[(*idx)++] = indexes[0] - 8;
                    legalmoves_out[(*idx)++] = indexes[0] - 16;
                } 
                else 
                {
                    if (indexes[0] - 8 >= 0 && board[indexes[0] - 8] == '.')
                    {
                    legalmoves_out[(*idx)++] = indexes[0] - 8;
                    }
                }
                if (indexes[0] - 7 >= 0 && indexes[0] % 8 != 7 && is_enemy(board[indexes[0] - 7], color))
                {
                    legalmoves_out[(*idx)++] = indexes[0] - 7;
                }
                if (indexes[0] - 9 >= 0 && indexes[0] % 8 != 0 && is_enemy(board[indexes[0] - 9], color))
                {
                    legalmoves_out[(*idx)++] = indexes[0] - 9;
                }


            }
            
            else 
            {
                if (8 <= indexes[0] && indexes[0] <= 15 && board[indexes[0] + 8] == '.' && board[indexes[0] + 16] == '.') 
                {
                    legalmoves_out[(*idx)++] = indexes[0] + 8;
                    legalmoves_out[(*idx)++] = indexes[0] + 16;
                } else 
                {
                    if (indexes[0] + 8 <= 63 && board[indexes[0] + 8] == '.')
                    {
                    legalmoves_out[(*idx)++] = indexes[0] + 8;
                    }
                }
                if (indexes[0] + 7 >= 0 && indexes[0] % 8 != 7 && is_enemy(board[indexes[0] + 7], color))
                {
                    legalmoves_out[(*idx)++] = indexes[0] + 7;
                }
                if (indexes[0] + 9 >= 0 && indexes[0] % 8 != 7 && is_enemy(board[indexes[0] + 9], color))
                {
                    legalmoves_out[(*idx)++] = indexes[0] + 9;
                }

            }
}

void knight_move(int color, char board[], int indexes[], int *idx, int legalmoves_out[])
{
    if (indexes[0] - 17 >= 0 && indexes[0] % 8 != 0 && board[indexes[0] - 17] == '.') legalmoves_out[(*idx)++] = indexes[0] - 17;
            if (indexes[0] - 17 >= 0 && indexes[0] % 8 != 0) {if (is_enemy(board[indexes[0] - 17], color)) {legalmoves_out[(*idx)++] = indexes[0] - 17;}}

            if (indexes[0] - 15 >= 0 && indexes[0] % 8 != 7 && board[indexes[0] - 15] == '.') legalmoves_out[(*idx)++] = indexes[0] - 15;
            if (indexes[0] - 15 >= 0 && indexes[0] % 8 != 7) {if (is_enemy(board[indexes[0] - 15], color)) {legalmoves_out[(*idx)++] = indexes[0] - 15;}}

            if (indexes[0] + 17 <= 63 && indexes[0] % 8 != 7 && board[indexes[0] + 17] == '.') legalmoves_out[(*idx)++] = indexes[0] + 17;
            if (indexes[0] + 17 <= 63 && indexes[0] % 8 != 7) {if (is_enemy(board[indexes[0] + 17], color)) {legalmoves_out[(*idx)++] = indexes[0] + 17;}}

            if (indexes[0] + 15 <= 63 && indexes[0] % 8 != 0 && board[indexes[0] + 15] == '.') legalmoves_out[(*idx)++] = indexes[0] + 15;
            if (indexes[0] + 15 <= 63 && indexes[0] % 8 != 0) {if (is_enemy(board[indexes[0] + 15], color)) {legalmoves_out[(*idx)++] = indexes[0] + 15;}}


            if (indexes[0] - 10 >= 0 && indexes[0] % 8 >= 2 && board[indexes[0] - 10] == '.') legalmoves_out[(*idx)++] = indexes[0] - 10;
            if (indexes[0] - 10 >= 0 && indexes[0] % 8 >= 2) {if (is_enemy(board[indexes[0] - 10], color)) {legalmoves_out[(*idx)++] = indexes[0] - 10;}

            if (indexes[0] + 6 <= 63 && indexes[0] % 8 >= 2 && board[indexes[0] + 6] == '.') legalmoves_out[(*idx)++] = indexes[0] + 6;
            if (indexes[0] + 6 <= 63 && indexes[0] % 8 >= 2) {if (is_enemy(board[indexes[0] + 6], color)) {legalmoves_out[(*idx)++] = indexes[0] + 6;}}

            if (indexes[0] + 10 <= 63 && indexes[0] % 8 >= 5 && board[indexes[0] + 10] == '.') legalmoves_out[(*idx)++] = indexes[0] + 10;
            if (indexes[0] + 10 <= 63 && indexes[0] % 8 >= 5) {if (is_enemy(board[indexes[0] + 10], color)) {legalmoves_out[(*idx)++] = indexes[0] + 10;}}

            if (indexes[0] - 6 >= 0 && indexes[0] % 8 >= 5 && board[indexes[0] - 6] == '.') legalmoves_out[(*idx)++] = indexes[0] - 6;
            if (indexes[0] - 6 >= 0 && indexes[0] % 8 >= 5) {if (is_enemy(board[indexes[0] - 6], color)) {legalmoves_out[(*idx)++] = indexes[0] - 6;}}

        
}}

void rook_move(int color, char board[], int indexes[], int *idx, int legalmoves_out[])
{
    int uprank, downrank, rightrank, leftrank;
    int filecount = indexes[0] % 8;

    for (uprank = 0; indexes[0] - 8 * (uprank + 1) >= 0 && board[indexes[0] - 8 * (uprank + 1)] == '.'; uprank++) {}
    if (indexes[0] - 8 * (uprank + 1) >= 0) {if (is_enemy(board[indexes[0] - 8 * (uprank + 1)], color)) {uprank++;}}

    for (downrank = 0; indexes[0] + 8 * (downrank + 1) <= 63 && board[indexes[0] + 8 * (downrank + 1)] == '.'; downrank++) {}
    if (indexes[0] + 8 * (downrank + 1) <= 63) {if (is_enemy(board[indexes[0] + 8 * (downrank + 1)], color)) {downrank++;}}

    for (rightrank = 0; (filecount + rightrank + 1) <= 7 && board[indexes[0] + (rightrank + 1)] == '.'; rightrank++) {}
    if ((filecount + rightrank + 1) <= 7) {if (is_enemy(board[(indexes[0] + rightrank + 1)], color)) {rightrank++;}}

    for (leftrank = 0; (filecount - leftrank - 1) >= 0 && board[indexes[0] - (leftrank + 1)] == '.'; leftrank++) {}
    if ((filecount - leftrank - 1) >= 0) {if (is_enemy(board[indexes[0] - (leftrank + 1)], color)) {leftrank++;}}

    for (int i = 0; i < uprank; i++)    legalmoves_out[(*idx)++] = indexes[0] - 8 * (i + 1);
    for (int i = 0; i < downrank; i++)  legalmoves_out[(*idx)++] = indexes[0] + 8 * (i + 1);
    for (int i = 0; i < rightrank; i++) legalmoves_out[(*idx)++] = indexes[0] + (i + 1);
    for (int i = 0; i < leftrank; i++)  legalmoves_out[(*idx)++] = indexes[0] - (i + 1);
}

void bishop_move(int color, char board[], int indexes[], int *idx, int legalmoves_out[])
{
            int nw, ne, se, sw;

            for (nw = 0; indexes[0] - 9 * (nw + 1) >= 0 && (indexes[0] - 9 * nw) % 8 != 0 && board[indexes[0] - 9 * (nw + 1)] == '.'; nw++) {}
            if (indexes[0] - 9 * (nw + 1) >= 0 && (indexes[0] - 9 * nw) % 8 != 0) {if (is_enemy(board[indexes[0] - 9 * (nw + 1)], color)) {nw++;}}

            for (ne = 0; indexes[0] - 7 * (ne + 1) >= 0 && (indexes[0] - 7 * ne) % 8 != 7 && board[indexes[0] - 7 * (ne + 1)] == '.'; ne++) {}
            if (indexes[0] - 7 * (ne + 1) >= 0 && (indexes[0] - 7 * ne) % 8 != 7) {if (is_enemy(board[indexes[0] - 7 * (ne + 1)], color)) {ne++;}}

            for (se = 0; indexes[0] + 9 * (se + 1) <= 63 && (indexes[0] + 9 * se) % 8 != 7 && board[indexes[0] + 9 * (se + 1)] == '.'; se++) {}
            if (indexes[0] + 9 * (se + 1) <= 63 && (indexes[0] + 9 * se) % 8 != 7) {if (is_enemy(board[indexes[0] + 9 * (se + 1)], color)) {se++;}}

            for (sw = 0; indexes[0] + 7 * (sw + 1) <= 63 && (indexes[0] + 7 * sw) % 8 != 0 && board[indexes[0] + 7 * (sw + 1)] == '.'; sw++) {}
            if (indexes[0] + 7 * (sw + 1) <= 63 && (indexes[0] + 7 * sw) % 8 != 0) {if (is_enemy(board[indexes[0] + 7 * (sw + 1)], color)) {sw++;}}

            for (int i = 0; i < nw; i++) legalmoves_out[(*idx)++] = indexes[0] - 9 * (i + 1);
            for (int i = 0; i < ne; i++) legalmoves_out[(*idx)++] = indexes[0] - 7 * (i + 1);
            for (int i = 0; i < se; i++) legalmoves_out[(*idx)++] = indexes[0] + 9 * (i + 1);
            for (int i = 0; i < sw; i++) legalmoves_out[(*idx)++] = indexes[0] + 7 * (i + 1);
}

void queen_move(int color, char board[], int indexes[], int *idx, int legalmoves_out[])
{
            int nw, ne, se, sw;
            int uprank, downrank, rightrank, leftrank;
            int filecount = indexes[0] % 8;

            for (nw = 0; indexes[0] - 9 * (nw + 1) >= 0 && (indexes[0] - 9 * nw) % 8 != 0 && board[indexes[0] - 9 * (nw + 1)] == '.'; nw++) {}
            if (indexes[0] - 9 * (nw + 1) >= 0 && (indexes[0] - 9 * nw) % 8 != 0) {if (is_enemy(board[indexes[0] - 9 * (nw + 1)], color)) {nw++;}}

            for (ne = 0; indexes[0] - 7 * (ne + 1) >= 0 && (indexes[0] - 7 * ne) % 8 != 7 && board[indexes[0] - 7 * (ne + 1)] == '.'; ne++) {}
            if (indexes[0] - 7 * (ne + 1) >= 0 && (indexes[0] - 7 * ne) % 8 != 7) {if (is_enemy(board[indexes[0] - 7 * (ne + 1)], color)) {ne++;}}

            for (se = 0; indexes[0] + 9 * (se + 1) <= 63 && (indexes[0] + 9 * se) % 8 != 7 && board[indexes[0] + 9 * (se + 1)] == '.'; se++) {}
            if (indexes[0] + 9 * (se + 1) <= 63 && (indexes[0] + 9 * se) % 8 != 7) {if (is_enemy(board[indexes[0] + 9 * (se + 1)], color)) {se++;}}

            for (sw = 0; indexes[0] + 7 * (sw + 1) <= 63 && (indexes[0] + 7 * sw) % 8 != 0 && board[indexes[0] + 7 * (sw + 1)] == '.'; sw++) {}
            if (indexes[0] + 7 * (sw + 1) <= 63 && (indexes[0] + 7 * sw) % 8 != 0) {if (is_enemy(board[indexes[0] + 7 * (sw + 1)], color)) {sw++;}}

            for (uprank = 0; indexes[0] - 8 * (uprank + 1) >= 0 && board[indexes[0] - 8 * (uprank + 1)] == '.'; uprank++) {}
            if (indexes[0] - 8 * (uprank + 1) >= 0) {if (is_enemy(board[indexes[0] - 8 * (uprank + 1)], color)) {uprank++;}}

            for (downrank = 0; indexes[0] + 8 * (downrank + 1) <= 63 && board[indexes[0] + 8 * (downrank + 1)] == '.'; downrank++) {}
            if (indexes[0] + 8 * (downrank + 1) <= 63) {if (is_enemy(board[indexes[0] + 8 * (downrank + 1)], color)) {downrank++;}}

            for (rightrank = 0; (filecount + rightrank + 1) <= 7 && board[indexes[0] + (rightrank + 1)] == '.'; rightrank++) {}
            if ((filecount + rightrank + 1) <= 7) {if (is_enemy(board[(indexes[0] + rightrank + 1)], color)) {rightrank++;}}

            for (leftrank = 0; (filecount - leftrank - 1) >= 0 && board[indexes[0] - (leftrank + 1)] == '.'; leftrank++) {}
            if ((filecount - leftrank - 1) >= 0) {if (is_enemy(board[indexes[0] - (leftrank + 1)], color)) {leftrank++;}}

            for (int i = 0; i < uprank; i++)    legalmoves_out[(*idx)++] = indexes[0] - 8 * (i + 1);
            for (int i = 0; i < downrank; i++)  legalmoves_out[(*idx)++] = indexes[0] + 8 * (i + 1);
            for (int i = 0; i < rightrank; i++) legalmoves_out[(*idx)++] = indexes[0] + (i + 1);
            for (int i = 0; i < leftrank; i++)  legalmoves_out[(*idx)++] = indexes[0] - (i + 1);
            for (int i = 0; i < nw; i++)        legalmoves_out[(*idx)++] = indexes[0] - 9 * (i + 1);
            for (int i = 0; i < ne; i++)        legalmoves_out[(*idx)++] = indexes[0] - 7 * (i + 1);
            for (int i = 0; i < se; i++)        legalmoves_out[(*idx)++] = indexes[0] + 9 * (i + 1);
            for (int i = 0; i < sw; i++)        legalmoves_out[(*idx)++] = indexes[0] + 7 * (i + 1);
}

void king_move(int color, char board[], int indexes[], int *idx, int legalmoves_out[])
{
            if (indexes[0] - 9 >= 0 && indexes[0] % 8 != 0 && (board[indexes[0] - 9] == '.' || is_enemy(board[indexes[0] - 9], color))) legalmoves_out[(*idx)++] = indexes[0] - 9;
            if (indexes[0] - 8 >= 0 && (board[indexes[0] - 8] == '.' || is_enemy(board[indexes[0] - 8], color))) legalmoves_out[(*idx)++] = indexes[0] - 8;
            if (indexes[0] - 7 >= 0 && indexes[0] % 8 != 7 && (board[indexes[0] - 7] == '.' || is_enemy(board[indexes[0] - 7], color))) legalmoves_out[(*idx)++] = indexes[0] - 7;
            if (indexes[0] - 1 >= 0 && indexes[0] % 8 != 0 && (board[indexes[0] - 1] == '.' || is_enemy(board[indexes[0] - 1], color))) legalmoves_out[(*idx)++] = indexes[0] - 1;
            if (indexes[0] + 1 <= 63 && indexes[0] % 8 != 7 && (board[indexes[0] + 1] == '.' || is_enemy(board[indexes[0] + 1], color))) legalmoves_out[(*idx)++] = indexes[0] + 1;
            if (indexes[0] + 7 <= 63 && indexes[0] % 8 != 0 && (board[indexes[0] + 7] == '.' || is_enemy(board[indexes[0] + 7], color))) legalmoves_out[(*idx)++] = indexes[0] + 7;
            if (indexes[0] + 8 <= 63 && (board[indexes[0] + 8] == '.' || is_enemy(board[indexes[0] + 8], color))) legalmoves_out[(*idx)++] = indexes[0] + 8;
            if (indexes[0] + 9 <= 63 && indexes[0] % 8 != 7 && (board[indexes[0] + 9] == '.' || is_enemy(board[indexes[0] + 9], color))) legalmoves_out[(*idx)++] = indexes[0] + 9;
}

int check_check(char board[], int king_color)
{
    int legalmoves_out[192];
    int idx = 0;
    int enemy_color;
    if (king_color == -1) {
        enemy_color = 1;
    } else {
        enemy_color = -1;
    }
        for (int i = 0; i < 64; i++)
        {
            char piece = board[i];
            if (piece == '.') continue;

            int piece_color;
            if (isupper(piece)) piece_color = -1;
            if (islower(piece)) piece_color = 1;

            if (piece_color != enemy_color) continue;

            int indexes[2] = {i, 32};
            if (toupper(piece) == 'P') pawn_move(enemy_color, board, indexes, &idx, legalmoves_out);
        
            if (toupper(piece) == 'N') knight_move(enemy_color, board, indexes, &idx, legalmoves_out);

            if (toupper(piece) == 'R') rook_move(enemy_color, board, indexes, &idx, legalmoves_out);

            if (toupper(piece) == 'B') bishop_move(enemy_color, board, indexes, &idx, legalmoves_out);

            if (toupper(piece) == 'Q') queen_move(enemy_color, board, indexes, &idx, legalmoves_out);

            if (toupper(piece) == 'K') king_move(enemy_color, board, indexes, &idx, legalmoves_out);
        }
        char king_char;
        if (king_color == -1) {
            king_char = 'K';
        } else {
            king_char = 'k';
        }
        for (int k = 0; k < idx; k++) {
            if (board[legalmoves_out[k]] == king_char) return 1;
        }
    
    return 0;
}

int try_move(char board[], char move[5], int color) // sjekker om man er i sjakk etter et trekk
{
    char temp_board[64];
    memcpy(temp_board, board, 64);

    move_push(temp_board, move);

    if (check_check(temp_board, color)) {
    return 1;
    }

    return 0;
}

void castle(char board[], int indexes[], int legalmoves_out[], int *idx)
{
    char piece = board[indexes[0]];
    int color;
    if (isupper(piece)) {color = -1;}
    else {color = -1;}

    char movemove[5];
    index_to_move(movemove, indexes);

    int rook_index[2];
    if (indexes[0] == 4 && indexes[1] == 2) {rook_index[0] = 0; rook_index[1] = 3;}
    if (indexes[0] == 4 && indexes[1] == 6) {rook_index[0] = 7; rook_index[1] = 5;}
    if (indexes[0] == 60 && indexes[1] == 62) {rook_index[0] = 63; rook_index[1] = 61;}
    if (indexes[0] == 60 && indexes[1] == 58) {rook_index[0] = 56; rook_index[1] = 59;}

    if (castle_rights(board, indexes[0], rook_index[0]) == 1) {return;}
    if (try_move(board, movemove, color)) return;

    legalmoves_out[(*idx)++] = indexes[1];
    char move[5];
    index_to_move(move, rook_index);
    move_push(board, move);
}
int legal(char board[], char move[]) { //Sjekker om det er en brikke der man vil flytte fra. 
    int indexes[2];
    move_to_index(move, indexes);

    if (board[indexes[0]] != '.')
    {
        return 1;
    }

    return 0;
};

int piecemove(char board[], char move[], int legalmoves_out[], int *move_number) { // skal finne lovlige trekk for brikken
    if (!legal(board, move)) {
        printf("trekket er ulovlighehe");
        return 0;
    }
    
    int indexes[2];
    move_to_index(move, indexes);
    char piece = board[indexes[0]];

    int color;
    int idx = 0;

    if (isupper(piece)) {
        color = -1;
    } else {
        color = 1;
    }

    if ((*move_number % 2 == 0 && isupper(piece)) || (*move_number % 2 == 1 && islower(piece)))
    {
        if (piece == 'k' && indexes[1] == 2 && indexes[0] == 4) castle(board, indexes, legalmoves_out, &idx);
        if (piece == 'k' && indexes[1] == 6 && indexes[0] == 4) castle(board, indexes, legalmoves_out, &idx);
        if (piece == 'K' && indexes[1] == 58 && indexes[0] == 60) castle(board, indexes, legalmoves_out, &idx);
        if (piece == 'K' && indexes[1] == 62 && indexes[0] == 60) castle(board, indexes, legalmoves_out, &idx);

        if (try_move(board, move, color)) return 0;

        if (toupper(piece) == 'P') pawn_move(color, board, indexes, &idx, legalmoves_out);
            
        if (toupper(piece) == 'N') knight_move(color, board, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'R') rook_move(color, board, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'B') bishop_move(color, board, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'Q') queen_move(color, board, indexes, &idx, legalmoves_out);

        if (toupper(piece) == 'K') king_move(color, board, indexes, &idx, legalmoves_out);

        return idx;
    }
    return 0;
}

void random_move(char board[], char move[5], int *move_number)
{
    srand(time(NULL));
    int black_pieces[20];
    int idx = 0;
    for (int i = 0; i < 64; i++)
    {
        if (board[i] != '.' && islower(board[i]))
        {
            black_pieces[idx++] = i;
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
        selected_piece_index[0] = black_pieces[pt];
        index_to_move(move, selected_piece_index);
        count = piecemove(board, move, legalmovesb, move_number);
    }


    int select_move = rand() % count;
    int indexes[2] = {selected_piece_index[0], legalmovesb[select_move]};

    index_to_move(move, indexes);
}

int count_all_legal_moves(char board[], int color, int *move_number)
{
    int total = 0;
    int legalmoves_out[32];

    for (int i = 0; i < 64; i++)
    {
        char piece = board[i];
        if (piece == '.') continue;

        int piece_color = isupper(piece) ? -1 : 1;
        if (piece_color != color) continue;

        char move[5];
        int pair[2] = {i, 32};
        index_to_move(move, pair);

        int count = piecemove(board, move, legalmoves_out, move_number);
        total += count;
    }

    return total;
}

int game_status(char board[], int color, int *move_number)
{
    int in_check = check_check(board, color);
    int total_moves = count_all_legal_moves(board, color, move_number);

    if (total_moves == 0)
    {
        if (in_check) return 1; // checkmate
        return 2;               // stalemate
    }

    return 0; // game continues
}



int main() {
    char move[10];
    int move_number = 0;
    int game_over = 0;
    while (strcmp(move, "stopp") != 0 && !game_over)
    {
        printf("\nEnter move: ");
        scanf("%s", move);

        int indexes[2];
        move_to_index(move, indexes);

        int legalmoves[32];
        int count = piecemove(board, move, legalmoves, &move_number);

        char moves[5];
        for (int i = 0; i < count; i++)
        {
            int pair[2] = {indexes[0], legalmoves[i]};
            index_to_move(moves, pair);
            if (legalmoves[i] == indexes[1])
            {
                move_push(board, move);
                move_number++;
                int next_color = (move_number % 2 == 0) ? -1 : 1;
                int status = game_status(board, next_color, &move_number);
                if (status == 1) { printf("Checkmate!\n"); board_print(); game_over = 1; break; }
                if (status == 2) { printf("Stalemate!\n"); board_print(); game_over = 1; break; }
                board_print();
            }
        }
    }
    return 0;
}

/*
                char black_move[5];
                random_move(board, black_move, &move_number);
                move_push(board, black_move);
                move_number++;
*/