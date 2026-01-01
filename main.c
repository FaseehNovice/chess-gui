/*
* Name: main.c
* Purpose: A complete chess implementation with all standard rules including
*          en passant and castling, using Raylib for graphics rendering.
*          Inspire from Chess.com
* Author: Faseeh-Ur-Rehman
* Roll Number: FA25-BCS-019

* Key Features:
* - Complete FIDE (Fédération Internationale des Échecs) chess rules implementation
* - En passant and castling with proper validation
* - Check and checkmate detection
* - Graphical interface
* - Restart Functionality

* Features that can and will be added Later:
* - Choice to promote pawn to other pieces ( right now its Queen only )
* - Choice to rotate the board after each turn
* - Choice for piece and board design
* - Algebraic notation panel at the side bar to replay the game
*/

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<math.h>

#include "raylib.h"

#define TILE_SIZE 80
#define BOARD_SIZE 8
#define BUFFER_SIZE 128
#define MOVE_CIRCLE_RADIUS 10
#define CAPTURE_CIRCLE_RADIUS (((TILE_SIZE) / 2) - 5)

#define TILE_LIGHT GetColor(0xEEEED2FF)
#define TILE_DARK  GetColor(0x769656FF)
#define MOVE_CIRCLE_COLOR Fade(GetColor(0x252525FF), 0.5f)
#define CHECK_COLOR Fade(RED,0.6f)
#define SELECTED_TILE Fade(YELLOW, 0.4f)

/**
 * PieceType enum: represents all possible chess pieces
 */
typedef enum PieceType{
    EMPTY,
    PAWN,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING
} PieceType;

/**
 * PieceColor enum: represents piece Colors. White moves First
 */
typedef enum PieceColor{
    WHITE_PIECE,
    BLACK_PIECE,
    NONE_PIECE
} PieceColor;

/**
 * Piece struct: Complete representation of a chess piece
 *
 * bool moved: checks eligibility for castling and pawn double-move
 * bool enPassant: flag for enPassant move
 *
 * en passant is special pawn capture in chess where a pawn
 * can capture an opponent's pawn that has just moved two squares
 * forward from its starting position, landing right beside it,
 * as if the opponent's pawn had only moved one square
 */
typedef struct Piece{
    PieceType type;
    PieceColor color;
    bool moved;
    bool enPassant;
} Piece;

Piece board[BOARD_SIZE][BOARD_SIZE];
int selectedRow = -1;
int selectedCol = -1;
PieceColor turn = WHITE_PIECE;
bool gameOver = false;

char gameResult[BUFFER_SIZE] = {0};

// en passant tracking
int enPassantTargetRow = -1;
int enPassantTargetCol = -1;
PieceColor enPassantPawnColor = NONE_PIECE;

// array to store Textures (I will be using pngs as piece models from the web)
// Texture array[color][type];
Texture2D pieceTextures[2][7];

/*============= Core Game Functions =================*/
void InitBoard(void);
void LoadAssets(void);
void UnloadAssets(void);
void DrawBoard(void);
void DrawPieces(void);
void HandleInput(void);

/*============ Move Validations ======================*/
bool IsValidMove(int source_row, int source_column, int destination_row, int destination_column);
bool MovePiece(int source_row, int source_column, int destination_row, int destination_column);
bool IsInCheck(PieceColor color);
bool HasAnyValidMove(PieceColor color);
bool IsCheckMate(PieceColor Color);
bool IsPathClear(int source_row, int source_column, int destination_row, int destination_column);
bool TestMoveForCheck(PieceColor color, int source_row, int source_column, int destination_row, int destination_column);
void ResetEnPassant(void);

int main(void) {

    InitWindow(BOARD_SIZE * TILE_SIZE + 240, BOARD_SIZE * TILE_SIZE, 
               "Chess - Faseeh Ur Rehman");
    SetTargetFPS(60);

    LoadAssets();
    InitBoard();

    while (!WindowShouldClose()) {

        HandleInput();
        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));
        DrawBoard();
        DrawPieces();

        int sideX = BOARD_SIZE * TILE_SIZE;

        DrawRectangle(sideX, 0, 240, BOARD_SIZE * TILE_SIZE, GetColor(0x252525FF));
        DrawRectangle(sideX, 0, 5, BOARD_SIZE * TILE_SIZE, GetColor(0x333333FF));

        // Title section
        DrawText("PROJECT CHESS", sideX + 30, 30, 22, GetColor(0x69923EFF));
        DrawRectangle(sideX + 40, 60, 140, 2, DARKGRAY);


        DrawText("CURRENT MOVE", sideX + 30, 100, 14, LIGHTGRAY);

        Color cardColor = (turn == WHITE_PIECE) ? RAYWHITE : GetColor(0x383838FF);
        Color textColor = (turn == WHITE_PIECE) ? BLACK : RAYWHITE;

        DrawRectangleRounded((Rectangle){sideX + 25, 125, 190, 80}, 0.2, 10, 
                           Fade(BLACK, 0.3f));
        DrawRectangleRounded((Rectangle){sideX + 20, 120, 190, 80}, 0.2, 10, cardColor);

        const char* turnText = (turn == WHITE_PIECE) ? "WHITE" : "BLACK";
        int tw = MeasureText(turnText, 28);
        DrawText(turnText, sideX + 20 + (190 - tw) / 2, 145, 28, textColor);


        if (IsInCheck(turn)) {

            float pulse = (sinf(GetTime() * 10.0f) * 0.5f) + 0.5f;
            DrawRectangleRounded((Rectangle){sideX + 50, 215, 130, 30}, 0.5, 10, Fade(RED, 0.2f + (pulse * 0.3f)));
            DrawText("KING IN CHECK", sideX + 65, 224, 12, RED);
        }

        if (gameOver) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));

            Rectangle resBox = { sideX - 450, 200, 400, 200 };
            DrawRectangleRounded(resBox, 0.1, 10, GetColor(0x202020FF));
            DrawRectangleRoundedLines(resBox, 0.1, 10, TILE_DARK);

            DrawText("GAME OVER", resBox.x + 110, resBox.y + 30, 30, TILE_DARK);

            int resW = MeasureText(gameResult, 20);
            DrawText(gameResult, resBox.x + (400 - resW) / 2, resBox.y + 80, 20, RAYWHITE);

            Rectangle btn = { resBox.x + 100, resBox.y + 130, 200, 45 };
            bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
            DrawRectangleRounded(btn, 0.2, 10, hover ? TILE_DARK : DARKGRAY);
            DrawText("PLAY AGAIN", btn.x + 45, btn.y + 12, 18, hover ? BLACK : RAYWHITE);

            if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                InitBoard();
                turn = WHITE_PIECE;
                gameOver = false;
                selectedRow = -1;
                ResetEnPassant();
            }
        } else {
            DrawText("L-Click: Select/Move", sideX + 35, BOARD_SIZE * TILE_SIZE - 60, 14, WHITE);
            DrawText("R-Click: Deselect", sideX + 45, BOARD_SIZE * TILE_SIZE - 40, 14, WHITE);
        }

        EndDrawing();
    }

    UnloadAssets();
    CloseWindow();
    return 0;
}

//===========================================================================
// ASSET MANAGEMENT FUNCTIONS
//===========================================================================

/**
 * @brief Loads all piece textures from /assets/PNG
 * File Path follow pattern: assets/PNG/{color}_{piece}.png
 */
void LoadAssets(){
    const char* names[] = { "", "pawn", "rook", "knight", "bishop", "queen", "king" };
    char path[128];

    for (int i = 1; i <= 6; i++) {
        // White pieces
        sprintf(path, "assets/PNG/white_%s.png", names[i]);
        pieceTextures[0][i] = LoadTexture(path);

        // Black pieces
        sprintf(path, "assets/PNG/black_%s.png", names[i]);
        pieceTextures[1][i] = LoadTexture(path);
    }
}

/**
 * @brief Releases all Loaded Textures.
 * Unloads texture from pieceTexture 2D array
 */
void UnloadAssets(void){
    for(int i = 0 ; i < 7 ; i++){
        UnloadTexture(pieceTextures[0][i]);
        UnloadTexture(pieceTextures[1][i]);
    }
}

//===========================================================================
// BOARD INITIALIZATION FUNCTION
//===========================================================================

/**
 * @brief Set Up Standard chess Sarting position
 *
 * Chess Board Setup:
 *  0 | r n b q k b n r
 *  1 | p p p p p p p p
 *  2 |
 *  3 |
 *  4 |
 *  5 |
 *  6 | P P P P P P P P
 *  7 | R N B Q K B N R
 */
void InitBoard(void){
    memset(board, EMPTY, sizeof(board));

    board[0][0] = (Piece){ROOK, BLACK_PIECE, false, false};
    board[0][1] = (Piece){KNIGHT, BLACK_PIECE, false, false};
    board[0][2] = (Piece){BISHOP, BLACK_PIECE, false, false};
    board[0][3] = (Piece){QUEEN, BLACK_PIECE, false, false};
    board[0][4] = (Piece){KING, BLACK_PIECE, false, false};
    board[0][5] = (Piece){BISHOP, BLACK_PIECE, false, false};
    board[0][6] = (Piece){KNIGHT, BLACK_PIECE, false, false};
    board[0][7] = (Piece){ROOK, BLACK_PIECE, false, false};

    board[7][0] = (Piece){ROOK, WHITE_PIECE, false, false};
    board[7][1] = (Piece){KNIGHT, WHITE_PIECE, false, false};
    board[7][2] = (Piece){BISHOP, WHITE_PIECE, false, false};
    board[7][3] = (Piece){QUEEN, WHITE_PIECE, false, false};
    board[7][4] = (Piece){KING, WHITE_PIECE, false, false};
    board[7][5] = (Piece){BISHOP, WHITE_PIECE, false, false};
    board[7][6] = (Piece){KNIGHT, WHITE_PIECE, false, false};
    board[7][7] = (Piece){ROOK, WHITE_PIECE, false, false};


    for(int col = 0 ; col < BOARD_SIZE ; col++){
        board[1][col] = (Piece){PAWN, BLACK_PIECE, false, false};
        board[6][col] = (Piece){PAWN, WHITE_PIECE, false, false};
    }

    for(int i = 2; i < BOARD_SIZE - 2 ; i++){
        for(int j = 0 ; j < BOARD_SIZE ; j++){
            board[i][j] = (Piece){EMPTY, NONE_PIECE, false, false};
        }
    }

    ResetEnPassant();
}

//===========================================================================
// RENDERING FUNCTIONS
//===========================================================================

/**
 * @brief Renders the Chess Board with selection Highlights
 * Renders 64 squares with alternating colors and visual feedback for:
 * - Selected Square (Yellow Highlights)
 * - Valid Moves (circles for empty squares, rings for captures)
 */
void DrawBoard(void){

    for(int i = 0 ; i < BOARD_SIZE ; i++){
        for(int j = 0 ; j < BOARD_SIZE ; j++){

            Color tile_color = !((i + j) % 2) ? TILE_LIGHT : TILE_DARK;
            DrawRectangle(i * TILE_SIZE, j * TILE_SIZE, TILE_SIZE, TILE_SIZE, tile_color);

            if(selectedRow != -1 && IsValidMove(selectedRow, selectedCol, i , j)){
                if(!TestMoveForCheck(board[selectedRow][selectedCol].color, selectedRow, selectedCol, i , j)){

                    int positionX = (i * TILE_SIZE) + TILE_SIZE / 2;
                    int positionY = (i * TILE_SIZE) + TILE_SIZE / 2;

                    if(board[i][j].type == EMPTY)
                        DrawCircle(positionX, positionY, MOVE_CIRCLE_RADIUS, MOVE_CIRCLE_COLOR);
                    else
                        DrawCircleLines(positionX,positionY,CAPTURE_CIRCLE_RADIUS, MOVE_CIRCLE_COLOR);
                }
            }

            if(i == selectedRow && j == selectedCol)
                DrawRectangle(i * TILE_SIZE, j * TILE_SIZE, TILE_SIZE, TILE_SIZE, SELECTED_TILE);
        }
    }
}

/**
 * @brief Render all pieces on the board
 * Incudes special highlighting for kings in Check
 */
void DrawPieces(void){
    bool wCheck = IsInCheck(WHITE_PIECE);
    bool bCheck = IsInCheck(BLACK_PIECE);

    for(int i = 0 ; i < BOARD_SIZE ; i++){
        for(int j = 0 ; j < BOARD_SIZE ; j++){
            Piece p = board[i][j];

            if(p.type == EMPTY)
                continue;

            if(p.type == KING){

                if((p.color == WHITE_PIECE && wCheck) || (p.color == BLACK_PIECE && bCheck)){
                    DrawRectangle(j * TILE_SIZE, i * TILE_SIZE, TILE_SIZE, TILE_SIZE, CHECK_COLOR);
                }
            }


            int colorID = (p.color == WHITE_PIECE) ? 0 : 1;
            Texture2D texture = pieceTextures[colorID][p.type];

            float scale = (float)TILE_SIZE / (float)texture.width * 0.85f;

            Vector2 pos = {
                i * TILE_SIZE + (TILE_SIZE - texture.width * scale) / 2,
                j * TILE_SIZE + (TILE_SIZE - texture.width * scale) / 2
            };

            DrawTextureEx(texture, pos, 0, scale, WHITE);


        }
    }
}

//===========================================================================
// INPUT HANDLING FUNCTION
//===========================================================================

/**
 * @brief Process mouse input for piece selection and movement
 * Implements two-step interaction: select piece -> select destination
 */
void HandleInput(void){

    if(gameOver)
        return;

    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){

        int col = GetMouseX() / TILE_SIZE;
        int row = GetMouseY() / TILE_SIZE;

        if (col >= 8 || row >= 8) {
            selectedRow = -1;
            return;
        }

        if (selectedRow == -1) {

            if (board[row][col].color == turn) {
                selectedRow = row;
                selectedCol = col;
            }
        }
        else{

            if(board[row][col].color == turn){

                selectedRow = row;
                selectedCol = col;
            }
            else if(MovePiece(selectedRow, selectedCol, row, col)){

                turn = (turn == WHITE_PIECE) ? BLACK_PIECE : WHITE_PIECE;

                if (IsCheckMate(turn)) {

                    gameOver = true;
                    sprintf(gameResult, "Checkmate! %s Wins",
                            (turn == BLACK_PIECE ? "White" : "Black"));
                }
                else if(!HasAnyValidMove(turn)){

                    gameOver = true;
                    strcpy(gameResult, "Stalemate! Draw");
                }

                selectedRow = -1;
            }
            else
                selectedRow = -1;

        }

    }

}

//===========================================================================
// MOVE VALIDATION FUNCTIONS
//===========================================================================

/**
 * @brief Validates move according to chess piece rules (FIDE rules)
 * Performs basic validation without considering King Safety
 *
 * @param sr Source Row
 * @param sc Source Column
 * @param dr Destination Row
 * @param dc Destination Column
 *
 * @return true if move follows piece movement rules
 */
bool IsValidMove(int sr, int sc, int dr, int dc){

    if(dr < 0 || dr >= BOARD_SIZE || dc < 0 || dc >= BOARD_SIZE)
        return false;

    if(sr == dr && sc == dc)
        return false;

    Piece p = board[sr][sc];
    if(board[dr][dc].color == p.color)
        return false;

    int rDiff = dr - sr;
    int cDiff = dc - sc;

    switch(p.type){
        case PAWN:{
            int dir = (p.color == WHITE_PIECE) ? -1 : 1;

            if(!cDiff && rDiff == dir && board[dr][dc].type == EMPTY)
                return true;

            if(!p.moved && !cDiff && rDiff == 2 * dir && board[dr][dc].type == EMPTY && board[sr + dir][sc].type == EMPTY )
                return true;

            if(abs(cDiff) == 1 && rDiff == dir){

                if(board[dr][dc].type != EMPTY)
                    return true;

                if (dr == enPassantTargetRow && dc == enPassantTargetCol && p.color != enPassantPawnColor){

                    int pawnRow = (p.color == WHITE_PIECE) ? dr + 1 : dr - 1;
                    if (pawnRow >= 0 && pawnRow < BOARD_SIZE && board[pawnRow][dc].type == PAWN && board[pawnRow][dc].color != p.color){
                        return true;

                    }
                }
            }

            return false;
            ;}
        case ROOK:{
            return (sr == dr || sc == dc && IsPathClear(sr, sc, dr, dc));
            }
        case KNIGHT:{
            return ((abs(rDiff) == 2) && (abs(cDiff) == 1)) || ((abs(rDiff) == 1) && (abs(cDiff) == 2));
        }
        case BISHOP:{
            return (abs(rDiff) == abs(cDiff) && IsPathClear(sr, sc, dr, dc));
        }
        case QUEEN:{
            return (sr == dr || sc == dc || abs(rDiff) == abs(cDiff)) && (IsPathClear(sr, sc, dr, dc));
        }
        case KING:{
            if(abs(rDiff) <= 1 && abs(cDiff) <= 1)
                return true;


            if(!p.moved && !rDiff && abs(cDiff) == 2){
                int rookCol = (cDiff > 0) ? 7 : 0;

                /* Castling Conditions
                1_ Rook Hasn't moved
                2_ Path is clear
                3_ King not currently in check
                4_ King doesn't pass through attacked squares
                */

                if(board[sr][rookCol].type == ROOK && !board[sr][rookCol].moved && IsPathClear(sr, sc, sr, rookCol)){

                    if(IsInCheck(p.color)) return false;

                    return !TestMoveForCheck(p.color, sr, sc, sr, sc + (cDiff > 0 ? 1 : -1));
                }
            }
            return false;

            }
        default:
            return false;
    }
}

/**
 * @brief Executes a move with all side effects
 * Performs safety check, handles special moves, updates board states
 *
 * @param sr Source Row
 * @param sc Source Column
 * @param dr Destination Row
 * @param dc Destination Column
 *
 * @returns true if move was successfully executed
 */
bool MovePiece(int sr, int sc, int dr, int dc){

    if(!IsValidMove(sr, sc, dr, dc)) return false;

    Piece p = board[sr][sc];

    if(TestMoveForCheck(p.color, sr, sc, dr, dc)) return false;

    bool isEnPassantCapture = (p.type == PAWN && dr == enPassantTargetRow && dc == enPassantTargetCol && p.color != enPassantPawnColor);

    if(isEnPassantCapture){
        int captureRow = (p.color == WHITE_PIECE) ? dr + 1: dr - 1;
        board[captureRow][dc] = (Piece){EMPTY, NONE_PIECE, false, false};
    }

    if(p.type == PAWN && abs(dr - sr) == 2){
        enPassantTargetRow = (sr + dr) / 2;
        enPassantTargetCol = sc;
        enPassantPawnColor = p.color;
    }
    else{
        ResetEnPassant();
    }

    if(p.type == PAWN){
        if((p.color == WHITE_PIECE && dr == 0) || (p.color == BLACK_PIECE && dr == 7))
            p.type = QUEEN;
    }

    if(p.type == KING && abs(dc - sc) == 2){

        int rookCol = (dc > sc) ? 7 : 0;
        int newRookCol = (dc > sc) ? dc - 1 : dc + 1;

        board[dr][newRookCol] = board[sr][rookCol];
        board[dr][newRookCol].moved = true;

        board[sr][rookCol] = (Piece){EMPTY, NONE_PIECE, false, false};
    }

    p.moved = true;
    board[dr][dc] = p;
    board[sr][sc] = (Piece){EMPTY, NONE_PIECE, false, false};

    return true;
}
/**
 * @brief Determine if specified king is in check
 *
 * @param color color of king to check
 *
 * @return true if king is under attack
 */
bool IsInCheck(PieceColor color){
    int kr = -1, kc = -1;

    for(int i = 0; i < 8 ; i++){
        for(int j = 0 ; j < 8 ; j++){
            if(board[i][j].type == KING && board[i][j].color == color){
                kr = i;
                kc = j;
                break;
            }
        }
        if(kr != -1) break;
    }

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Piece p = board[i][j];

            if (p.type == EMPTY || p.color == color) continue;

            if (p.type == PAWN) {
                int dir = (p.color == WHITE_PIECE) ? -1 : 1;

                if(i + dir == kr && abs(j - kc) == 1)
                    return true;
            }
            else if(IsValidMove(i, j, kr, kc)){
                return true;
            }
        }
    }

    return false;

}

/**
 * @brief Check if player has any legal moves
 *
 * @param color Player color to check
 *
 * @return true if at least one legal move exists
 */
bool HasAnyValidMove(PieceColor color){

    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++){
            if(board[i][j].color == color){

                for(int dr = 0; dr < 8; dr++){
                    for(int dc = 0; dc < 8; dc++){

                        if (IsValidMove(i, j, dr, dc) && !TestMoveForCheck(color, i, j, dr, dc)){
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

/**
 * @brief Determine if player is in check
 *
 * @param color Player color to check
 *
 * @return true if player is checkmated
 */
bool IsCheckMate(PieceColor color){
    return IsInCheck(color) && !HasAnyValidMove(color);
}

/**
 * @brief  Check if path between two squares is unobstructed
 *
 * @param sr Source row
 * @param sc Source column
 * @param dr Destination row
 * @param dc Destination column
 *
 * @return true if all intermediate squares are empty
 */
bool IsPathClear(int sr, int sc, int dr, int dc){

    int rs = (dr > sr) ? 1 : (dr < sr) ? -1 : 0;
    int cs = (dc > sc) ? 1 : (dc < sc) ? -1 : 0;

    for (int r = sr + rs, c = sc + cs; r != dr || c != dc; r += rs, c += cs) {
        if (board[r][c].type != EMPTY)
            return false;
    }

    return true;
}

/**
 * @brief Simulate move to check if it leaves king in check
 * Uses temporary board modification to test move consequences
 *
 * @param color Color of moving Player
 * @param sr Source Row
 * @param sc Source Column
 * @param dr Destination Row
 * @param dc Destination Column
 *
 * @return true if move would leave king in check
 */
bool TestMoveForCheck(PieceColor color, int sr, int sc, int dr, int dc){
    Piece src = board[sr][sc];
    Piece dest = board[dr][dc];

    Piece capturedPawn = {EMPTY, NONE_PIECE, false, false};

    int capturedRow = -1, capturedCol = -1;

    bool isEP = (src.type == PAWN && dr == enPassantTargetRow && dc == enPassantTargetCol && src.color != enPassantPawnColor);

    if(isEP){
        capturedRow = (src.color == WHITE_PIECE) ? dr + 1 : dr - 1;
        capturedCol = dc;
        capturedPawn = board[capturedRow][capturedCol];
        board[capturedRow][capturedCol] = (Piece){EMPTY, NONE_PIECE, false, false};
    }

    board[dr][dc] = src;
    board[sr][sc] = (Piece){EMPTY, NONE_PIECE, false, false};

    bool inCheck = IsInCheck(color);

    board[sr][sc] = src;
    board[dr][dc] = dest;

    if(isEP){
        board[capturedRow][capturedCol] = capturedPawn;
    }

    return inCheck;

}

/**
 * @brief Clear en Passant tracking variables
 * Called after each move (except when a pawn moves two squares)
 */
void ResetEnPassant(void){

    enPassantTargetRow = -1;
    enPassantTargetCol = -1;
    enPassantPawnColor = NONE_PIECE;

}