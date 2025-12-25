#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <assert.h>
#include <stdint.h> 

#define Board_Size 8
#define TRUE 1
#define FALSE 0
#define MAX_DEPTH 30 // 最大搜尋深度

// --- Bitboard 定義與巨集 ---
typedef uint64_t BitBoard;
BitBoard BB_Black = 0;
BitBoard BB_White = 0;

// 將 (x, y) 轉換為 Bitboard 的位移量 (0-63)
#define POS(x, y) (1ULL << ((y) * 8 + (x)))

// 檢查某個位置是否有特定顏色的棋子
#define IS_SET(bb, x, y) (((bb) >> ((y) * 8 + (x))) & 1ULL)

// 用來防止左右位移時跨越邊界的遮罩
const BitBoard NOT_A_FILE = 0xfefefefefefefefeULL; // 避免 H wrapping 到 A
const BitBoard NOT_H_FILE = 0x7f7f7f7f7f7f7f7fULL; // 避免 A wrapping 到 H

// ------------------------

typedef struct {
    int x;
    int y;
    int score;
} Move;

// --- 全域變數：殺手啟發表 ---
// 用來紀錄引發剪枝的強力棋步，加速後續搜尋
Move KillerMoves[MAX_DEPTH][2];

// --- 函式宣告 ---
void Delay(unsigned int mseconds);
int Read_File( FILE *p, char *c );
char Load_File( void );

void Init();
int Play_a_Move( int x, int y, int write_to_file);
void Show_Board_and_Set_Legal_Moves( void );
int Put_a_Stone( int x, int y, int write_to_file);

int In_Board( int x, int y );
int Check_Cross( int x, int y, int update ); 

int Find_Legal_Moves(int color);
int Check_EndGame( void );
int Compute_Grades(int flag);

int Evaluate_Position();

void Computer_Think( int *x, int *y);
int Search(int myturn, int mylevel);
int search_next(int x, int y, int myturn, int mylevel, int alpha, int beta, int prev_pass);

// 移除 qsort 比較函式，改用 sort_moves
void sort_moves(Move *moves, int count);

int Fix_AI_Move_For_Current_Turn(int *x, int *y);

int Get_Board_Val(int x, int y);
BitBoard Get_Flips(BitBoard my, BitBoard opp, int x, int y);

// --- 全域變數 ---
int Search_Counter;
int Computer_Take;
int Winner;
int Legal_Moves[ Board_Size ][ Board_Size ]; 
int HandNumber;

int Black_Count, White_Count;
int Turn = 0;
int Stones[2]= {1,2};

int LastX, LastY;
int Think_Time=0, Total_Time=0;

int search_deep = 8; 
int alpha_beta_option = TRUE;
int resultX, resultY;
int current_max_depth = 6; 

// 位置權重表
int board_weight[8][8] =
{
    {100, -20,  10,   5,   5,  10, -20, 100},
    {-20, -50,  -2,  -2,  -2,  -2, -50, -20},
    { 10,  -2,  -1,  -1,  -1,  -1,  -2,  10},
    {  5,  -2,  -1,  -1,  -1,  -1,  -2,   5},
    {  5,  -2,  -1,  -1,  -1,  -1,  -2,   5},
    { 10,  -2,  -1,  -1,  -1,  -1,  -2,  10},
    {-20, -50,  -2,  -2,  -2,  -2, -50, -20},
    {100, -20,  10,   5,   5,  10, -20, 100}
};

int AI_Color;
int AI_Turn;

// ---------------------------------------------------------------------------
// 輔助函式：計算 Bit 數量
int CountBits(BitBoard b) {
    #ifdef _MSC_VER
        return (int)__popcnt64(b);
    #else
        return __builtin_popcountll(b);
    #endif
}
// ---------------------------------------------------------------------------

// --- 優化：插入排序 ---
void sort_moves(Move *moves, int count) {
    for (int i = 1; i < count; i++) {
        Move key = moves[i];
        int j = i - 1;
        while (j >= 0 && moves[j].score < key.score) {
            moves[j + 1] = moves[j];
            j--;
        }
        moves[j + 1] = key;
    }
}

int main(int argc, char *argv[] )
{
    char compcolor = 'W', c[10];
    int column_input, row_input ;
    int rx, ry, m=0, n;
    FILE *fp;

    Init();

    if ( argc == 3 )
    {
        compcolor = *argv[1] ;
        if ( atoi(argv[2]) > 0 )
            search_deep = atoi(argv[2]);
        printf("%c, %d\n", compcolor, search_deep);
    }
    else if ( argc == 2 )
    {
        compcolor = *argv[1] ;
    }
    else
    {
        printf("Computer take?(B/W/All/File play as first/file play as Second/Load and play): ");
        scanf("%c", &compcolor);
    }

    if (compcolor == 'B' || compcolor == 'b' || compcolor == 'F') AI_Color = 1;
    else if (compcolor == 'W' || compcolor == 'w' || compcolor == 'S') AI_Color = 2;
    else if (compcolor == 'A' || compcolor == 'a') AI_Color = 2; 
    else AI_Color = 2; 

    Show_Board_and_Set_Legal_Moves();

    if (compcolor == 'L' || compcolor == 'l')
        compcolor = Load_File();

    if ( compcolor == 'B' || compcolor == 'b')
    {
        Computer_Think( &rx, &ry );
        printf("Computer played %c%d\n", rx+97, ry+1);
        Play_a_Move( rx, ry, 1 );
        Show_Board_and_Set_Legal_Moves();
    }

    if ( compcolor == 'A' || compcolor == 'a')
        while ( m++ < 64 )
        {
            Computer_Think( &rx, &ry );
            if ( !Play_a_Move( rx, ry, 1 ))
            {
                printf("Wrong Computer moves %c%d\n", rx+97, ry+1);
                scanf("%d", &n);
                break;
            }
            if ( rx == -1 )
                printf("Computer Pass\n");
            else
                printf("Computer played %c%d\n", rx+97, ry+1);

            if ( Check_EndGame() )
                return 0;
            Show_Board_and_Set_Legal_Moves();
        }

    if (compcolor == 'F')
    {
        printf("First/Black start!\n");
        Computer_Think( &rx, &ry );
        Play_a_Move( rx, ry, 1 );
    }

    while ( m++ < 64 )
    {
        while (1)
        {
            if ( compcolor == 'F' || compcolor == 'S' )
            {
                fp = fopen( "of.txt", "r" );
                if(fp == NULL) { Delay(100); continue; }
                fscanf( fp, "%d", &n );

                char tc[10];
                if (compcolor == 'F' )
                {
                    if ( n % 2 == 0 )
                    {
                        while ( (fscanf( fp, "%s", tc ) ) != EOF ) { c[0] = tc[0]; c[1] = tc[1]; }
                        fclose(fp);
                        if ( c[0] == 'w' ) return 0;
                        if( c[0] != 'p' && Get_Board_Val(c[0]-97, c[1]-49) != 0) {
                            printf("%s is wrong F\n", c);
                            continue;
                        }
                    }
                    else { fclose(fp); Delay(100); continue; }
                }
                else
                {
                    if ( n % 2 == 1 )
                    {
                        while ( (fscanf( fp, "%s", tc ) ) != EOF ) { c[0] = tc[0]; c[1] = tc[1]; }
                        fclose(fp);
                        if ( c[0] == 'w' ) return 0;
                        if( c[0] != 'p' && Get_Board_Val(c[0]-97, c[1]-49) != 0) {
                             printf("%s is wrong S\n", c);
                             continue;
                        }
                    }
                    else { fclose(fp); Delay(100); continue; }
                }
            }

            if ( compcolor == 'B' ) { printf("input White move:(a-h 1-8), or PASS\n"); scanf("%s", c); }
            else if ( compcolor == 'W' ) { printf("input Black move:(a-h 1-8), or PASS\n"); scanf("%s", c); }

            if ( c[0] == 'P' || c[0] == 'p')
                row_input = column_input = -1;
            else if ( c[0] == 'M' || c[0] == 'm' )
            {
                Computer_Think( &rx, &ry );
                if ( !Play_a_Move( rx, ry, 1 )) {
                    printf("Wrong Computer moves %c%d\n", rx+97, ry+1); scanf("%d", &n); break;
                }
                if ( rx == -1 ) printf("Computer Pass");
                else printf("Computer played %c%d\n", rx+97, ry+1);
                if ( Check_EndGame() ) break;
                Show_Board_and_Set_Legal_Moves();
            }
            else { row_input= c[0] - 97; column_input = c[1] - 49; }

            if ( !(c[0] == 'M' || c[0] == 'm') ) {
                if ( !Play_a_Move(row_input, column_input, 0) ) {
                    printf("#%d, %c%d is a Wrong move\n", HandNumber, c[0], column_input+1);
                    continue;
                }
                else break;
            }
        }
        
        if ( Check_EndGame() ) return 0;;
        Show_Board_and_Set_Legal_Moves();

        Computer_Think( &rx, &ry );
        printf("Computer played %c%d\n", rx+97, ry+1);
        Play_a_Move( rx, ry, 1 );
        
        if ( Check_EndGame() ) return 0;;
        Show_Board_and_Set_Legal_Moves();
    }

    printf("Game is over!!");
    printf("\n%d", argc);
    if ( argc <= 1 ) scanf("%d", &n);
    return 0;
}

void Delay(unsigned int mseconds)
{
    clock_t goal = mseconds + clock();
    while (goal > clock());
}

int Get_Board_Val(int x, int y) {
    if (IS_SET(BB_Black, x, y)) return 1;
    if (IS_SET(BB_White, x, y)) return 2;
    return 0;
}

int Read_File( FILE *fp, char *c )
{
    char tc[10];
    while ( (fscanf( fp, "%s", tc ) ) != EOF ) { c[0] = tc[0]; c[1] = tc[1]; }
    fclose(fp);
    if ( c[0] == 'w') return 2;
    return 1;
}

char Load_File( void )
{
    FILE *fp;
    char tc[10];
    int row_input, column_input, n;
    fp = fopen( "of.txt", "r" );
    assert( fp != NULL );
    fscanf( fp, "%d", &n );
    while ( (fscanf( fp, "%s", tc ) ) != EOF )
    {
        row_input= tc[0] - 97;
        column_input = tc[1] - 49;
        if ( !Play_a_Move(row_input, column_input, 0) )
            printf("%c%d is a Wrong move\n", tc[0], column_input+1);
        Show_Board_and_Set_Legal_Moves();
    }
    fclose(fp);
    return ( n%2 == 1 )? 'B' : 'W';
}

void Init()
{
    Total_Time = clock();
    Computer_Take = 0;
    BB_Black = 0;
    BB_White = 0;

    srand(time(NULL));
    BB_White |= POS(3, 3);
    BB_White |= POS(4, 4);
    BB_Black |= POS(3, 4);
    BB_Black |= POS(4, 3);

    HandNumber = 0;
    Turn = 0;
    LastX = LastY = -1;
    Black_Count = White_Count = 0;
    Search_Counter = 0;
    Winner = 0;

    // 清空殺手表
    memset(KillerMoves, 0, sizeof(KillerMoves));
}

int Play_a_Move( int x, int y, int write_to_file)
{
    FILE *fp;
    if ( x == -1 && y == -1)
    {
        if (write_to_file) {
            fp = fopen( "of.txt", "r+" );
            if (fp) {
                fprintf( fp, "%2d\n", HandNumber+1 );
                fclose(fp);
            }
            fp = fopen("of.txt", "a");
            if (fp) {
                fprintf( fp, "p9\n" );
                fclose( fp );
            }
        }
        HandNumber ++;
        Turn = 1 - Turn;
        return 1;
    }
    if ( !In_Board(x,y)) return 0;
    
    Find_Legal_Moves( Stones[Turn] );
    if( Legal_Moves[x][y] == FALSE ) return 0;

    if( Put_a_Stone(x,y, write_to_file) ) {
        Compute_Grades( TRUE );
        return 1;
    }
    else return 0;
    return 1;
}

BitBoard Get_Flips(BitBoard my, BitBoard opp, int x, int y) {
    BitBoard flips = 0;
    BitBoard move = POS(x, y);
    int shifts[8] = {8, -8, 1, -1, 9, 7, -7, -9};
    
    for (int dir = 0; dir < 8; dir++) {
        int shift = shifts[dir];
        BitBoard mask = ~0ULL; 
        if (shift == 1 || shift == 9 || shift == -7) mask = NOT_H_FILE;
        else if (shift == -1 || shift == 7 || shift == -9) mask = NOT_A_FILE;

        BitBoard candidate = 0;
        BitBoard walker = move;
        
        while (1) {
            if ((walker & mask) != walker) break;
            if (shift > 0) walker <<= shift;
            else walker >>= (-shift);
            if (walker == 0) break;

            if (walker & opp) candidate |= walker;
            else if (walker & my) {
                flips |= candidate;
                break;
            } else break; 
        }
    }
    return flips;
}

int Put_a_Stone(int x, int y, int write_to_file)
{
    FILE *fp;
    if (Get_Board_Val(x, y) != 0) return FALSE;

    BitBoard *my_bb = (Turn == 0) ? &BB_Black : &BB_White;
    BitBoard *opp_bb = (Turn == 0) ? &BB_White : &BB_Black;

    BitBoard flips = Get_Flips(*my_bb, *opp_bb, x, y);

    if (flips != 0) {
        if (write_to_file) {
            if (HandNumber == 0 && AI_Color == 1) {
                fp = fopen( "of.txt", "w" );
            } else {
                fp = fopen( "of.txt", "r+" );
            }
            
            if (fp != NULL) {
                fprintf( fp, "%2d\n", HandNumber+1 );
                fclose(fp);
            }

            fp = fopen("of.txt", "a");
            if (fp != NULL) {
                fprintf( fp, "%c%d\n", x+97, y+1 );;
                fclose( fp );
            }
        }
        
        HandNumber ++;
        BitBoard move_bit = POS(x, y);
        *my_bb |= move_bit;
        *my_bb |= flips;
        *opp_bb ^= flips;

        LastX = x; LastY = y;
        Turn = 1 - Turn;
        return TRUE;
    }
    return FALSE;
}

void Show_Board_and_Set_Legal_Moves( void )
{
    int i,j;
    Find_Legal_Moves( Stones[Turn] );
    printf("a b c d e f g h\n");
    for(i=0 ; i<Board_Size ; i++)
    {
        for(j=0 ; j<Board_Size ; j++)
        {
            int val = Get_Board_Val(j, i);
            if( val > 0 ) {
                if ( val == 2 ) printf("O ");
                else printf("X ");
            }
            if (val == 0) {
                if ( Legal_Moves[j][i] == 1) printf("? ");
                else printf(". ");
            }
        }
        printf(" %d\n", i+1);
    }
    printf("\n");
}

int Find_Legal_Moves( int color )
{
    int i, j;
    int legal_count = 0;
    memset(Legal_Moves, 0, sizeof(Legal_Moves));

    BitBoard my_bb, opp_bb;
    if (color == 1) { my_bb = BB_Black; opp_bb = BB_White; } 
    else { my_bb = BB_White; opp_bb = BB_Black; }

    BitBoard occupied = my_bb | opp_bb;

    for( i = 0; i < Board_Size; i++ ) { 
        for( j = 0; j < Board_Size; j++ ) { 
            if (!IS_SET(occupied, i, j)) {
                BitBoard flips = Get_Flips(my_bb, opp_bb, i, j);
                if (flips != 0) {
                    Legal_Moves[i][j] = TRUE;
                    legal_count++;
                }
            }
        }
    }
    return legal_count;
}

int Check_Cross(int x, int y, int update)
{
    BitBoard *my_bb = (Turn == 0) ? &BB_Black : &BB_White;
    BitBoard *opp_bb = (Turn == 0) ? &BB_White : &BB_Black;
    BitBoard flips = Get_Flips(*my_bb, *opp_bb, x, y);
    if (flips == 0) return FALSE;
    if (update) {
        *my_bb |= flips;
        *opp_bb ^= flips;
        *my_bb |= POS(x, y);
    }
    return TRUE;
}

int In_Board(int x, int y)
{
    if( x >= 0 && x < Board_Size && y >= 0 && y < Board_Size ) return TRUE;
    else return FALSE;
}

// --- 優化：無迴圈計算邊界子 ---
int Count_Frontier_Discs(BitBoard my_pieces, BitBoard empty_cells) {
    BitBoard empty_neighbors = 0;
    // 上下
    empty_neighbors |= (empty_cells << 8);
    empty_neighbors |= (empty_cells >> 8);
    // 左右 (防止跨越 H-A 邊界)
    empty_neighbors |= (empty_cells & NOT_H_FILE) << 1; 
    empty_neighbors |= (empty_cells & NOT_A_FILE) >> 1; 
    // 對角
    empty_neighbors |= (empty_cells & NOT_H_FILE) << 9;
    empty_neighbors |= (empty_cells & NOT_A_FILE) >> 9;
    empty_neighbors |= (empty_cells & NOT_A_FILE) << 7;
    empty_neighbors |= (empty_cells & NOT_H_FILE) >> 7;

    return CountBits(my_pieces & empty_neighbors);
}

int Evaluate_Position() {
    int my_color_idx = (AI_Color == 1) ? 0 : 1; 
    int opp_color_idx = 1 - my_color_idx;

    BitBoard my_bb = (my_color_idx == 0) ? BB_Black : BB_White;
    BitBoard opp_bb = (my_color_idx == 0) ? BB_White : BB_Black;
    BitBoard empty_bb = ~(my_bb | opp_bb);
    
    int my_disc_count = CountBits(my_bb);
    int opp_disc_count = CountBits(opp_bb);
    int disc_score = 0;
    if (my_disc_count + opp_disc_count != 0)
        disc_score = 100 * (my_disc_count - opp_disc_count) / (my_disc_count + opp_disc_count);

    int my_mobility = Find_Legal_Moves(Stones[my_color_idx]);
    int opp_mobility = Find_Legal_Moves(Stones[opp_color_idx]);
    int mobility_score = 0;
    if (my_mobility + opp_mobility != 0)
        mobility_score = 100 * (my_mobility - opp_mobility) / (my_mobility + opp_mobility);

    int my_frontier = Count_Frontier_Discs(my_bb, empty_bb);
    int opp_frontier = Count_Frontier_Discs(opp_bb, empty_bb);
    int frontier_score = 0;
    if (my_frontier + opp_frontier != 0)
        frontier_score = 100 * (opp_frontier - my_frontier) / (my_frontier + opp_frontier);

    BitBoard corners = POS(0,0) | POS(0,7) | POS(7,0) | POS(7,7);
    int my_corners = CountBits(my_bb & corners);
    int opp_corners = CountBits(opp_bb & corners);
    int corner_score = 25 * (my_corners - opp_corners);

    double w_disc = 0, w_mob = 0, w_front = 0, w_corner = 0;

    if (HandNumber < 20) {
        w_disc = -5; w_mob = 50; w_front = 20; w_corner = 100; 
    } else if (HandNumber <= 50) {
        w_disc = 5; w_mob = 30; w_front = 10; w_corner = 200; 
    } else {
        w_disc = 50; w_mob = 10; w_front = 0; w_corner = 500;
    }

    double score = (w_disc * disc_score) + (w_mob * mobility_score) + 
                   (w_front * frontier_score) + (w_corner * corner_score);
                   
    return (int)score;
}

int Compute_Grades(int flag)
{
    int B = CountBits(BB_Black);
    int W = CountBits(BB_White);
    int BW = 0, WW = 0;

    for(int i=0; i<Board_Size; i++) {
        for(int j=0; j<Board_Size; j++) {
            if (IS_SET(BB_Black, i, j)) BW += board_weight[i][j];
            else if (IS_SET(BB_White, i, j)) WW += board_weight[i][j];
        }
    }

    if(flag) {
        Black_Count = B; White_Count = W;
        printf("#%d Grade: Black %d, White %d\n", HandNumber, B, W);
        return (BW - WW);
    }
    return (AI_Color == 1 ? BW - WW : WW - BW);
}

int Check_EndGame( void )
{
    int lc1, lc2;
    FILE *fp;
    lc2 = Find_Legal_Moves( Stones[1 - Turn] );
    lc1 = Find_Legal_Moves( Stones[Turn] );

    if ( lc1==0 && lc2==0 )
    {
        fp = fopen("of.txt", "a");
        Total_Time = clock() - Total_Time;
        if (HandNumber%2 == 1) {
            fprintf(fp, "Total used time= %d min. %d sec.\n", Total_Time/60000, (Total_Time%60000)/1000 );
            // fprintf(fp, "Black used time= %d min. %d sec.\n", Think_Time/60000, (Think_Time%60000)/1000 );
        } else {
            fprintf(fp, "Total used time= %d min. %d sec.\n", Total_Time/60000, (Total_Time%60000)/1000 );
            // fprintf(fp, "White used time= %d min. %d sec.\n", Think_Time/60000, (Think_Time%60000)/1000 );
        }

        if(Black_Count > White_Count) {
            printf("Black(F) Win!\n"); fprintf(fp, "wB%d\n", Black_Count - White_Count);
            if(Winner == 0) Winner = 1;
        } else if(Black_Count < White_Count) {
            printf("White(S) Win!\n"); fprintf(fp, "wW%d\n", White_Count - Black_Count);
            if(Winner == 0) Winner = 2;
        } else {
            printf("Draw\n"); fprintf(fp, "wZ%d\n", White_Count - Black_Count);
            Winner = 0;
        }
        fclose(fp);
        Show_Board_and_Set_Legal_Moves();
        printf("Game is over");
        return TRUE;
    }
    return FALSE;
}

int Fix_AI_Move_For_Current_Turn(int *x, int *y)
{
    int i, j, has_any = 0;
    Find_Legal_Moves(Stones[Turn]);

    if (*x >= 0 && *x < Board_Size && *y >= 0 && *y < Board_Size && Legal_Moves[*x][*y] == TRUE)
        return 1;

    for (i = 0; i < Board_Size; i++) {
        for (j = 0; j < Board_Size; j++) {
            if (Legal_Moves[i][j] == TRUE) { *x = i; *y = j; has_any = 1; break; }
        }
        if (has_any) break;
    }
    if (!has_any) { *x = -1; *y = -1; return 0; }
    return 1;
}

void Computer_Think( int *x, int *y )
{
    clock_t clockBegin, clockEnd;
    clock_t tinterval;
    
    clockBegin = clock();
    AI_Turn = Turn;
    Search_Counter = 0;
    *x = -1; *y = -1;

    resultX = -1; 
    resultY = -1;

    for (int d = 6; d <= search_deep; d += 2) { 
        extern int current_max_depth;
        current_max_depth = d;
        Search(Turn, 0); 
        if ((clock() - clockBegin) > 5000 * CLOCKS_PER_SEC / 1000) break;
    }

    *x = resultX; *y = resultY;
    clockEnd = clock();
    tinterval = clockEnd - clockBegin;
    Think_Time += (int)tinterval;

    if ( tinterval < 200 ) Delay( (unsigned int)(200 - tinterval) );
    printf("Total thinking time= %d.%d sec.\n", (int)tinterval/1000, (int)tinterval%1000);
    if (*x != -1 && *y != -1) Fix_AI_Move_For_Current_Turn(x, y);
}

int Search(int myturn, int mylevel)
{
    int i, j, k;
    int alpha = -999999;
    int beta = 999999;
    int g;

    BitBoard save_B = BB_Black;
    BitBoard save_W = BB_White;

    int c = Find_Legal_Moves( Stones[myturn] );
    if(c <= 0) return FALSE;

    Move moves[64];
    int move_count = 0;

    for(i=0; i<Board_Size; i++) {
        for(j=0; j<Board_Size; j++) {
            if(Legal_Moves[i][j] == TRUE) {
                moves[move_count].x = i;
                moves[move_count].y = j;
                moves[move_count].score = board_weight[i][j];
                if (HandNumber < 6) moves[move_count].score += (rand() % 5);
                move_count++;
            }
        }
    }

    // 優化：使用插入排序
    sort_moves(moves, move_count);

    BitBoard *my_bb = (myturn == 0) ? &BB_Black : &BB_White;
    BitBoard *opp_bb = (myturn == 0) ? &BB_White : &BB_Black;

    for (k = 0; k < move_count; k++) {
        i = moves[k].x;
        j = moves[k].y;

        BB_Black = save_B;
        BB_White = save_W;
        BitBoard flips = Get_Flips(*my_bb, *opp_bb, i, j);
        *my_bb |= POS(i, j); *my_bb |= flips; *opp_bb ^= flips;

        g = search_next(i, j, 1 - myturn, mylevel + 1, alpha, beta, FALSE);

        if (g > alpha || k == 0) { // 強制記錄第一步，避免 resultX 是空的
            if (g > alpha) alpha = g;
            
            // 只有在根節點 (mylevel==0) 才需要更新全域的最佳步
            if (mylevel == 0) {
                resultX = i;
                resultY = j;
            }
        }
    }

    BB_Black = save_B; BB_White = save_W;
    return TRUE;
}

int search_next( int x, int y, int myturn, int mylevel, int alpha, int beta, int prev_pass )
{
    Search_Counter ++;
    extern int current_max_depth;
    if( mylevel >= current_max_depth ) return Evaluate_Position();

    BitBoard save_B = BB_Black;
    BitBoard save_W = BB_White;

    int c = Find_Legal_Moves( Stones[myturn] );
    if(c <= 0) {
        if (prev_pass) return Evaluate_Position();
        return search_next(x, y, 1 - myturn, mylevel + 1, alpha, beta, TRUE);
    }

    Move moves[64];
    int move_count = 0;

    for(int i=0; i<Board_Size; i++) {
        for(int j=0; j<Board_Size; j++) {
            if(Legal_Moves[i][j] == TRUE) {
                moves[move_count].x = i;
                moves[move_count].y = j;
                moves[move_count].score = board_weight[i][j]; 
                
                // --- 優化：Killer Heuristic ---
                // 如果這一步是該層的殺手步，給予極高分，讓它排在最前面優先搜尋
                if (mylevel < MAX_DEPTH) {
                    if ((KillerMoves[mylevel][0].x == i && KillerMoves[mylevel][0].y == j) ||
                        (KillerMoves[mylevel][1].x == i && KillerMoves[mylevel][1].y == j)) {
                        moves[move_count].score += 10000;
                    }
                }
                // -----------------------------

                move_count++;
            }
        }
    }

    // 優化：使用插入排序
    sort_moves(moves, move_count);

    int g;
    BitBoard *my_bb = (myturn == 0) ? &BB_Black : &BB_White;
    BitBoard *opp_bb = (myturn == 0) ? &BB_White : &BB_Black;

    for (int k = 0; k < move_count; k++) {
        int i = moves[k].x;
        int j = moves[k].y;

        BB_Black = save_B;
        BB_White = save_W;
        BitBoard flips = Get_Flips(*my_bb, *opp_bb, i, j);
        *my_bb |= POS(i, j); *my_bb |= flips; *opp_bb ^= flips;

        g = search_next(i, j, 1 - myturn, mylevel+1, alpha, beta, FALSE);

        if(myturn == AI_Turn) { // Max node
            if(g > alpha) alpha = g;
        } else { // Min node
            if(g < beta) beta = g;
        }

        if( alpha >= beta ) {
            // --- 優化：更新殺手步 ---
            // 發生剪枝，這一步是對手無法忍受的好棋，記下來
            if (mylevel < MAX_DEPTH) {
                // 將舊的殺手步往後推，新的放第一位
                KillerMoves[mylevel][1] = KillerMoves[mylevel][0];
                KillerMoves[mylevel][0].x = i;
                KillerMoves[mylevel][0].y = j;
            }
            break; // Cut-off
        }
    }

    BB_Black = save_B;
    BB_White = save_W;

    if (myturn == AI_Turn) return alpha;
    else return beta;
}