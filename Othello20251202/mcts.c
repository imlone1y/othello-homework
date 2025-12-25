#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <assert.h>
#include <math.h>


#define Board_Size 8
#define TRUE 1
#define FALSE 0

void Delay(unsigned int mseconds);
int Read_File(FILE *p, char *c);
char Load_File(void);

void Init();
int Play_a_Move(int x, int y);
void Show_Board_and_Set_Legal_Moves(void);
int Put_a_Stone(int x, int y);

int In_Board(int x, int y);
int Check_Cross(int x, int y, int update);
int Check_Straight_Army(int x, int y, int d, int update);

int Find_Legal_Moves(int color);
int Check_EndGame(void);
int Compute_Grades(int flag);

void Computer_Think(int *x, int *y);
int Fix_AI_Move_For_Current_Turn(int *x, int *y);

// Monte Carlo 模擬用 helper（不動到真正的 Now_Board）
int sim_Check_Straight_Army(int board[Board_Size][Board_Size], int x, int y, int d, int color, int do_flip);
int sim_Check_Cross(int board[Board_Size][Board_Size], int x, int y, int color, int do_flip);
int sim_Find_Legal_Moves(int board[Board_Size][Board_Size], int color, int moves_x[], int moves_y[]);
int simulate_random_game_from_move(int move_x, int move_y, int turn_index);

int Search_Counter;
int Computer_Take;
int Winner;
int Now_Board[Board_Size][Board_Size];
int Legal_Moves[Board_Size][Board_Size];
int HandNumber;
int sequence[100];

int Black_Count, White_Count;
int Turn = 0;
int Stones[2] = {1, 2};   // Stones[0] = Black(1), Stones[1] = White(2)
int DirX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
int DirY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

int LastX, LastY;
int Think_Time = 0, Total_Time = 0;

// 雖然不再用深度搜尋，但保留變數，避免改 main() 邏輯
int search_deep = 6;
int alpha_beta_option = TRUE;


// 智能權重表（Compute_Grades 用，真實對局輸出分數）
int board_weight[8][8] = {
    {100, -20, 10, 5, 5, 10, -20, 100},
    {-20, -50, -2, -2, -2, -2, -50, -20},
    {10, -2, -1, -1, -1, -1, -2, 10},
    {5, -2, -1, -1, -1, -1, -2, 5},
    {5, -2, -1, -1, -1, -1, -2, 5},
    {10, -2, -1, -1, -1, -1, -2, 10},
    {-20, -50, -2, -2, -2, -2, -50, -20},
    {100, -20, 10, 5, 5, 10, -20, 100}
};

int AI_Color;   // 1 = Black, 2 = White
int AI_Turn;    // 0 或 1，對應 Stones[]

//---------------------------------------------------------------------------

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
            search_deep = atoi(argv[2]);  // 雖然沒用到，但保留參數
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
    else if (compcolor == 'A' || compcolor == 'a') AI_Color = 2; // arbitrary for all mode
    else AI_Color = 2; // default

    Show_Board_and_Set_Legal_Moves();

    if (compcolor == 'L' || compcolor == 'l')
        compcolor = Load_File();

    if ( compcolor == 'B' || compcolor == 'b')
    {
        Computer_Think( &rx, &ry );
        printf("Computer played %c%d\n", rx+97, ry+1);
        Play_a_Move( rx, ry );

        Show_Board_and_Set_Legal_Moves();
    }

    if ( compcolor == 'A' || compcolor == 'a')
        while ( m++ < 64 )
        {
            Computer_Think( &rx, &ry );
            if ( !Play_a_Move( rx, ry ))
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
        Play_a_Move( rx, ry );
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
                        while ( (fscanf( fp, "%s", tc ) ) != EOF )
                        {
                            c[0] = tc[0];
                            c[1] = tc[1];
                        }
                        fclose(fp);

                        if ( c[0] == 'w' ) return 0;
                        if( c[0] != 'p' && Now_Board[c[0]-97][c[1]-49] != 0)
                        {
                            printf("%s is wrong F\n", c);
                            continue;
                        }
                    }
                    else
                    {
                        fclose(fp);
                        Delay(100);
                        continue;
                    }
                }
                else
                {
                    if ( n % 2 == 1 )
                    {
                        while ( (fscanf( fp, "%s", tc ) ) != EOF )
                        {
                            c[0] = tc[0];
                            c[1] = tc[1];
                        }
                        fclose(fp);
                        if ( c[0] == 'w' ) return 0;
                        if( c[0] != 'p' && Now_Board[c[0]-97][c[1]-49] != 0)
                        {
                             printf("%s is wrong S\n", c);
                             continue;
                        }
                    }
                    else
                    {
                        fclose(fp);
                        Delay(100);
                        continue;
                    }
                }
            }

            if ( compcolor == 'B' )
            {
                printf("input White move:(a-h 1-8), or PASS\n");
                scanf("%s", c);
            }
            else if ( compcolor == 'W' )
            {
                printf("input Black move:(a-h 1-8), or PASS\n");
                scanf("%s", c);
            }

            if ( c[0] == 'P' || c[0] == 'p')
                row_input = column_input = -1;
            else if ( c[0] == 'M' || c[0] == 'm' )
            {
                Computer_Think( &rx, &ry );
                if ( !Play_a_Move( rx, ry ))
                {
                    printf("Wrong Computer moves %c%d\n", rx+97, ry+1);
                    scanf("%d", &n);
                    break;
                }
                if ( rx == -1 )
                    printf("Computer Pass");
                else
                    printf("Computer played %c%d\n", rx+97, ry+1);
                if ( Check_EndGame() )
                    break;
                Show_Board_and_Set_Legal_Moves();
            }
            else
            {
                row_input= c[0] - 97;
                column_input = c[1] - 49;
            }

            if ( !Play_a_Move(row_input, column_input) )
            {
                printf("#%d, %c%d is a Wrong move\n", HandNumber, c[0], column_input+1);
                continue;
            }

            else
                break;
        }
        if ( Check_EndGame() )
            return 0;;
        Show_Board_and_Set_Legal_Moves();

        Computer_Think( &rx, &ry );
        printf("Computer played %c%d\n", rx+97, ry+1);
        Play_a_Move( rx, ry );
        if ( Check_EndGame() )
            return 0;;
        Show_Board_and_Set_Legal_Moves();

    }

    printf("Game is over!!");
    printf("\n%d", argc);
    if ( argc <= 1 )
        scanf("%d", &n);

    return 0;
}

//---------------------------------------------------------------------------

void Delay(unsigned int mseconds)
{
    clock_t goal = mseconds + clock();
    while (goal > clock());
}

//---------------------------------------------------------------------------

int Read_File( FILE *fp, char *c )
{
    char tc[10];
    while ( (fscanf( fp, "%s", tc ) ) != EOF )
    {
        c[0] = tc[0];
        c[1] = tc[1];
    }
    fclose(fp);
    if ( c[0] == 'w')
        return 2;
    return 1;
}

//---------------------------------------------------------------------------

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
        if ( !Play_a_Move(row_input, column_input) )
            printf("%c%d is a Wrong move\n", tc[0], column_input+1);

        Show_Board_and_Set_Legal_Moves();
    }
    fclose(fp);
    return ( n%2 == 1 )? 'B' : 'W';
}

//---------------------------------------------------------------------------

void Init()
{
    Total_Time = clock();

    Computer_Take = 0;
    memset(Now_Board, 0, sizeof(int) * Board_Size * Board_Size);

    srand(time(NULL));
    Now_Board[3][3] = Now_Board[4][4] = 2;
    Now_Board[3][4] = Now_Board[4][3] = 1;

    HandNumber = 0;
    memset(sequence, -1, sizeof(int) * 100);
    Turn = 0;

    LastX = LastY = -1;
    Black_Count = White_Count = 0;
    Search_Counter = 0;
    Winner = 0;
}

//---------------------------------------------------------------------------

int Play_a_Move( int x, int y)
{
    FILE *fp;

    if ( x == -1 && y == -1)
    {
        fp = fopen( "of.txt", "r+" );

        fprintf( fp, "%2d\n", HandNumber+1 );
        fclose(fp);

        fp = fopen("of.txt", "a");
        fprintf( fp, "p9\n" );
        fclose( fp );

        sequence[HandNumber] = -1;
        HandNumber ++;
        Turn = 1 - Turn;
        return 1;
    }

    if ( !In_Board(x,y))
        return 0;
    Find_Legal_Moves( Stones[Turn] );
    if( Legal_Moves[x][y] == FALSE )
        return 0;

    if( Put_a_Stone(x,y) )
    {
        Check_Cross(x,y,1);

        Compute_Grades( TRUE );
        return 1;
    }
    else
        return 0;
}

//---------------------------------------------------------------------------

int Put_a_Stone(int x, int y)
{
    FILE *fp;

    if( Now_Board[x][y] == 0)
    {
        sequence[HandNumber] = Turn;
        if (HandNumber == 0)
            fp = fopen( "of.txt", "w" );
        else
            fp = fopen( "of.txt", "r+" );
        fprintf( fp, "%2d\n", HandNumber+1 );
        HandNumber ++;
        fclose(fp);

        Now_Board[x][y] = Stones[Turn];
        fp = fopen("of.txt", "a");
        fprintf( fp, "%c%d\n", x+97, y+1 );;
        fclose( fp );

        LastX = x;
        LastY = y;

        Turn = 1 - Turn;

        return TRUE;
    }
    return FALSE;
}

//---------------------------------------------------------------------------

void Show_Board_and_Set_Legal_Moves( void )
{
    int i,j;

    Find_Legal_Moves( Stones[Turn] );

    printf("a b c d e f g h\n");
    for(i=0 ; i<Board_Size ; i++)
    {
        for(j=0 ; j<Board_Size ; j++)
        {
            if( Now_Board[j][i] > 0 )
            {
                if ( Now_Board[j][i] == 2 )
                    printf("O ");
                else
                    printf("X ");
            }

            if (Now_Board[j][i] == 0)
            {
                if ( Legal_Moves[j][i] == 1)
                    printf("? ");
                else printf(". ");
            }
        }
        printf(" %d\n", i+1);
    }
    printf("\n");
}

//---------------------------------------------------------------------------

int Find_Legal_Moves( int color )
{
    int i,j;
    int me = color;
    int legal_count = 0;

    for( i = 0; i < Board_Size; i++ )
        for( j = 0; j < Board_Size; j++ )
            Legal_Moves[i][j] = 0;

    for( i = 0; i < Board_Size; i++ )
        for( j = 0; j < Board_Size; j++ )
            if( Now_Board[i][j] == 0 )
            {
                Now_Board[i][j] = me;
                if( Check_Cross(i,j,FALSE) == TRUE )
                {
                    Legal_Moves[i][j] = TRUE;
                    legal_count ++;
                }
                Now_Board[i][j] = 0;
            }

    return legal_count;
}

//---------------------------------------------------------------------------

int Check_Cross(int x, int y, int update)
{
    int k;
    int dx, dy;

    if( ! In_Board(x,y) || Now_Board[x][y] == 0)
        return FALSE;

    int army = 3 - Now_Board[x][y];
    int army_count = 0;

    for( k=0 ; k<8 ; k++ )
    {
        dx = x + DirX[k];
        dy = y + DirY[k];
        if( In_Board(dx,dy) && Now_Board[dx][dy] == army )
        {
            army_count += Check_Straight_Army( x, y, k, update ) ;
        }
    }

    if(army_count >0)
        return TRUE;
    else
        return FALSE;
}

//---------------------------------------------------------------------------

int Check_Straight_Army(int x, int y, int d, int update)
{
    int me = Now_Board[x][y];
    int army = 3 - me;
    int army_count=0;
    int found_flag=FALSE;
    int flag[ Board_Size ][ Board_Size ] = {{0}};

    int i, j;
    int tx, ty;

    tx = x;
    ty = y;

    for(i=0 ; i<Board_Size ; i++)
    {
        tx += DirX[d];
        ty += DirY[d];

        if(In_Board(tx,ty) )
        {
            if( Now_Board[tx][ty] == army )
            {
                army_count ++;
                flag[tx][ty] = TRUE;
            }
            else if( Now_Board[tx][ty] == me)
            {
                found_flag = TRUE;
                break;
            }
            else
                break;
        }
        else
            break;
    }

    if( (found_flag == TRUE) && (army_count > 0) && update)
    {
        for(i=0 ; i<Board_Size ; i++)
            for(j=0; j<Board_Size ; j++)
                if(flag[i][j]==TRUE)
                {
                    if(Now_Board[i][j] != 0)
                        Now_Board[i][j]= 3 - Now_Board[i][j];
                }
    }
    if( (found_flag == TRUE) && (army_count > 0))
        return army_count;
    else return 0;
}

//---------------------------------------------------------------------------

int In_Board(int x, int y)
{
    if( x >= 0 && x < Board_Size && y >= 0 && y < Board_Size )
        return TRUE;
    else
        return FALSE;
}

//---------------------------------------------------------------------------

int Compute_Grades(int flag)
{
    int i,j;
    int B=0, W=0;
    int BW=0, WW=0;

    for(i=0 ; i<Board_Size ; i++)
        for(j=0 ; j<Board_Size ; j++)
        {
            if( Now_Board[i][j]==1 ) // Black
            {
                B++;
                BW = BW + board_weight[i][j] ;
            }
            else if( Now_Board[i][j]==2 ) // White
            {
                W++;
                WW = WW + board_weight[i][j] ;
            }
        }

    if(flag)
    {
        Black_Count = B;
        White_Count = W;
        printf("#%d Grade: Black %d, White %d\n", HandNumber, B, W);
        return (BW - WW);
    }

    int utility = (AI_Color == 1 ? BW - WW : WW - BW);
    int total = B + W;

    if (total > 55) {
        int diff = (AI_Color == 1 ? B - W : W - B);
        utility += diff * 20;
    }

    int mob_ai = Find_Legal_Moves(AI_Color);
    int mob_opp = Find_Legal_Moves(3 - AI_Color);
    int mob_factor = (total < 20 ? 15 : (total < 40 ? 10 : 5));
    utility += (mob_ai - mob_opp) * mob_factor;

    return utility;
}

//---------------------------------------------------------------------------

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

        if (HandNumber%2 == 1)
        {
            fprintf(fp, "Total used time= %d min. %d sec.\n", Total_Time/60000, (Total_Time%60000)/1000 );
            fprintf(fp, "Black used time= %d min. %d sec.\n", Think_Time/60000, (Think_Time%60000)/1000 );
        }
        else
        {
            fprintf(fp, "Total used time= %d min. %d sec.\n", Total_Time/60000, (Total_Time%60000)/1000 );
            fprintf(fp, "White used time= %d min. %d sec.\n", Think_Time/60000, (Think_Time%60000)/1000 );
        }

        if(Black_Count > White_Count)
        {
            printf("Black(F) Win!\n");
            fprintf(fp, "wB%d\n", Black_Count - White_Count);
            if(Winner == 0)
                Winner = 1;
        }
        else if(Black_Count < White_Count)
        {
            printf("White(S) Win!\n");
            fprintf(fp, "wW%d\n", White_Count - Black_Count);
            if(Winner == 0)
                Winner = 2;
        }
        else
        {
            printf("Draw\n");
            fprintf(fp, "wZ%d\n", White_Count - Black_Count);
            Winner = 0;
        }
        fclose(fp);

        Show_Board_and_Set_Legal_Moves();
        printf("Game is over");
        return TRUE;
    }

    return FALSE;
}

//---------------------------------------------------------------------------
// 修正 AI 最後落子，保證合法或 PASS

int Fix_AI_Move_For_Current_Turn(int *x, int *y)
{
    int i, j;
    int has_any = 0;

    Find_Legal_Moves(Stones[Turn]);

    if (*x >= 0 && *x < Board_Size &&
        *y >= 0 && *y < Board_Size &&
        Legal_Moves[*x][*y] == TRUE)
    {
        return 1;
    }

    for (i = 0; i < Board_Size; i++)
    {
        for (j = 0; j < Board_Size; j++)
        {
            if (Legal_Moves[i][j] == TRUE)
            {
                *x = i;
                *y = j;
                has_any = 1;
                break;
            }
        }
        if (has_any) break;
    }

    if (!has_any)
    {
        *x = -1;
        *y = -1;
        return 0;
    }
    return 1;
}

//---------------------------------------------------------------------------
// --- 修正後的 MCTS 實作 (基於 UCT 的單層搜尋，更穩定) ---

// 注意：請確保這個函式有被宣告或定義在 Computer_Think 之前
// int simulate_random_game_from_move(int move_x, int move_y, int turn_index); 

void Computer_Think( int *x, int *y )
{
    clock_t clockBegin, clockEnd;
    clock_t tinterval;

    clockBegin = clock();

    AI_Turn = Turn;
    Search_Counter = 0;

    int moves_x[64], moves_y[64];
    int num_moves = 0;
    int i, j;

    // 1. 找出當前所有合法步
    int c = Find_Legal_Moves(Stones[Turn]);
    if (c <= 0)
    {
        *x = *y = -1; // PASS
        return;
    }

    // 將合法步存入陣列
    for (i = 0; i < Board_Size; i++)
        for (j = 0; j < Board_Size; j++)
            if (Legal_Moves[i][j] == TRUE)
            {
                moves_x[num_moves] = i;
                moves_y[num_moves] = j;
                num_moves++;
            }

    // 2. 初始化統計數據
    int visits[64] = {0};
    double total_reward[64] = {0.0};

    // 3. 設定模擬總次數 (可依據電腦效能調整，例如 5000 或 10000)
    // 這裡設為 4000 次，兼顧速度與強度
    const int TOTAL_SIMS = 4000; 
    const double C = 1.414;  // UCT 探索常數 (sqrt(2))

    for (int s = 0; s < TOTAL_SIMS; s++)
    {
        // --- 選擇 (Selection) ---
        // 使用 UCT 公式選擇要模擬哪一步
        double best_uct = -1e9;
        int selected = -1;
        int total_visits_so_far = s + 1;

        for (int m = 0; m < num_moves; m++)
        {
            if (visits[m] == 0)
            {
                // 如果這一步還沒被模擬過，優先選擇 (無限大的 UCT)
                selected = m;
                best_uct = 1e9; // Ensure we pick this
                break;
            }
            else
            {
                // UCT 公式: WinRate + C * sqrt(log(Total) / Visits)
                double avg_reward = total_reward[m] / visits[m];
                double uct = avg_reward + C * sqrt(log((double)total_visits_so_far) / visits[m]);
                if (uct > best_uct)
                {
                    best_uct = uct;
                    selected = m;
                }
            }
        }

        if (selected == -1) selected = 0; // 防呆

        // --- 模擬 (Simulation) ---
        // 呼叫原本程式碼底部的 simulate_random_game_from_move
        int res = simulate_random_game_from_move(moves_x[selected], moves_y[selected], Turn);

        // --- 更新 (Backpropagation) ---
        // res = 1 (AI贏), -1 (AI輸), 0 (平手)
        // 將其轉換為獎勵值: 贏=1.0, 平=0.5, 輸=0.0
        double reward = (res > 0) ? 1.0 : ((res == 0) ? 0.5 : 0.0);

        visits[selected]++;
        total_reward[selected] += reward;
    }

    // 4. 決策 (Decision)
    // 選擇平均勝率最高的一步 (或者訪問次數最多的一步)
    double best_win_rate = -1.0;
    int best_idx = 0;
    
    // 簡單的 Log 輸出，方便除錯
    // printf("\nMoves Evaluation:\n");
    for (int m = 0; m < num_moves; m++)
    {
        if (visits[m] == 0) continue;
        double rate = total_reward[m] / visits[m];
        
        // printf("Move %c%d: Visited %d, WinRate %.2f%%\n", moves_x[m]+97, moves_y[m]+1, visits[m], rate*100);

        if (rate > best_win_rate)
        {
            best_win_rate = rate;
            best_idx = m;
        }
        else if (rate == best_win_rate)
        {
            // 同勝率時，選模擬次數較多的 (較可信)
            if (visits[m] > visits[best_idx])
                best_idx = m;
        }
    }

    *x = moves_x[best_idx];
    *y = moves_y[best_idx];

    // 最後保險：確認這一步真的合法
    Fix_AI_Move_For_Current_Turn(x, y);

    // 時間處理
    clockEnd = clock();
    tinterval = clockEnd - clockBegin;
    Think_Time += (int)tinterval;

    if ( tinterval < 200 )
        Delay( (unsigned int)(200 - tinterval) );

    printf("MCTS stats: %d sims, Best Move %c%d (WinRate: %.1f%%)\n", 
           TOTAL_SIMS, *x+97, *y+1, best_win_rate*100);
           
    printf("used thinking time= %d min. %d.%d sec.\n",
           Think_Time/60000,
           (Think_Time%60000)/1000,
           (Think_Time%60000)%1000);
}


//---------------------------------------------------------------------------
// 以下是「只用在模擬」的版本，不動到 Now_Board / Turn

int sim_Check_Straight_Army(int board[Board_Size][Board_Size], int x, int y, int d, int color, int do_flip)
{
    int army = 3 - color;
    int army_count = 0;
    int found_flag = FALSE;
    int flag[Board_Size][Board_Size] = {{0}};

    int tx = x;
    int ty = y;

    for (int i = 0; i < Board_Size; i++)
    {
        tx += DirX[d];
        ty += DirY[d];

        if (!In_Board(tx, ty)) break;

        if (board[tx][ty] == army)
        {
            army_count++;
            flag[tx][ty] = TRUE;
        }
        else if (board[tx][ty] == color)
        {
            found_flag = TRUE;
            break;
        }
        else
            break;
    }

    if (found_flag && army_count > 0 && do_flip)
    {
        for (int i = 0; i < Board_Size; i++)
            for (int j = 0; j < Board_Size; j++)
                if (flag[i][j] == TRUE && board[i][j] != 0)
                    board[i][j] = 3 - board[i][j];
    }

    if (found_flag && army_count > 0) return army_count;
    else return 0;
}

int sim_Check_Cross(int board[Board_Size][Board_Size], int x, int y, int color, int do_flip)
{
    if (!In_Board(x,y) || board[x][y] != color)
        return FALSE;

    int army_count = 0;
    for (int k = 0; k < 8; k++)
    {
        int dx = x + DirX[k];
        int dy = y + DirY[k];
        if (In_Board(dx,dy) && board[dx][dy] == 3 - color)
        {
            army_count += sim_Check_Straight_Army(board, x, y, k, color, do_flip);
        }
    }
    return (army_count > 0) ? TRUE : FALSE;
}

int sim_Find_Legal_Moves(int board[Board_Size][Board_Size], int color, int moves_x[], int moves_y[])
{
    int count = 0;
    for (int i = 0; i < Board_Size; i++)
        for (int j = 0; j < Board_Size; j++)
            if (board[i][j] == 0)
            {
                board[i][j] = color;
                if (sim_Check_Cross(board, i, j, color, 0))
                {
                    moves_x[count] = i;
                    moves_y[count] = j;
                    count++;
                }
                board[i][j] = 0;
            }
    return count;
}

// 對某一步 (move_x, move_y) 做一次隨機對局，回傳：AI 贏=1，輸=-1，平手=0
int simulate_random_game_from_move(int move_x, int move_y, int turn_index)
{
    int board[Board_Size][Board_Size];
    memcpy(board, Now_Board, sizeof(board));

    int color = Stones[turn_index];        // 現在要下的顏色（真實對局中這一步是 AI 的顏色或人類的顏色）
    int current_index = turn_index;

    // 先在模擬盤上下這一步
    board[move_x][move_y] = color;
    sim_Check_Cross(board, move_x, move_y, color, 1);

    // 換對手
    current_index = 1 - current_index;

    int passes = 0;

    while (passes < 2)
    {
        int moves_x[64], moves_y[64];
        int legal = sim_Find_Legal_Moves(board, Stones[current_index], moves_x, moves_y);

        if (legal == 0)
        {
            passes++;
            current_index = 1 - current_index;
            continue;
        }

        passes = 0;

        int r = rand() % legal;
        int mx = moves_x[r];
        int my = moves_y[r];

        board[mx][my] = Stones[current_index];
        sim_Check_Cross(board, mx, my, Stones[current_index], 1);

        current_index = 1 - current_index;
    }

    int ai = AI_Color;
    int opp = 3 - AI_Color;
    int ai_count = 0, opp_count = 0;

    for (int i = 0; i < Board_Size; i++)
        for (int j = 0; j < Board_Size; j++)
        {
            if (board[i][j] == ai) ai_count++;
            else if (board[i][j] == opp) opp_count++;
        }

    if (ai_count > opp_count) return 1;
    else if (ai_count < opp_count) return -1;
    else return 0;
}