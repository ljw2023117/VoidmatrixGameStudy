#include <graphics.h>

#include <vector>
#include <iostream>

using namespace std;

vector<vector<char>> board(3, vector<char>(3, '-'));

// 绘制棋盘
void DrawBoard()
{
	line(0, 200, 600, 200);
	line(0, 400, 600, 400);
	line(200, 0, 200, 600);
	line(400, 0, 400, 600);
}

// 绘制棋子
void DrawPiece()
{
	for (int i = 0; i < board.size(); i++)
		for (int j = 0; j < board[0].size(); j++)
		{
			if (board[i][j] == 'o')
			{
				circle(200 * j + 100, 200 * i + 100, 100);
			}
			else if (board[i][j] == 'x')
			{
				line(200 * j, 200 * i, 200 * (j + 1), 200 * (i + 1));
				line(200 * (j + 1) , 200 * i, 200 * j, 200 * (i + 1));
			}
		}
}

// 判断游戏是否结束
bool CheckWin(int x, int y, char piece)
{
	if (board[x][0] == piece && board[x][1] == piece && board[x][2] == piece) return true;
	if (board[0][y] == piece && board[1][y] == piece && board[2][y] == piece) return true;
	if (x == y && board[0][0] == piece && board[1][1] == piece && board[2][2] == piece) return true;
	if (x + y == 2 && board[0][2] == piece && board[1][1] == piece && board[2][0] == piece) return true;
	return false;
}

bool CheckDraw()
{
	bool st = true;
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			if (board[i][j] == '-') st = false;
	return st;
}

//文字描述落子信息
void DrawTipText(char curPiece)
{
	static TCHAR str[64];
	_stprintf_s(str, _T("当前棋子类型: %c"), curPiece);
	settextcolor(RGB(225, 175, 45));
	outtextxy(0, 0, str);
}

int main()
{
	DWORD start_time = GetTickCount();
	int pieceNum = 0;
	initgraph(600, 600);

	bool looping = true;

	char curPiece = 'o';
	
	BeginBatchDraw();
	while (looping)
	{
		ExMessage msg;
		int i = 0, j = 0;
		while (peekmessage(&msg))
		{
			if (msg.message == WM_LBUTTONDOWN)
			{
				j = msg.x / 200;
				i = msg.y / 200;

				if (board[i][j] == '-')
				{
					board[i][j] = curPiece;
					if (curPiece == 'o') curPiece = 'x';
					else curPiece = 'o';
				}
			}
		}
		
		DrawBoard();
		DrawPiece();
		DrawTipText(curPiece);
		FlushBatchDraw();

		if (CheckWin(i, j, curPiece == 'o' ? 'x' : 'o'))
		{
			if (curPiece == 'o') MessageBox(GetHWnd(), _T("x 玩家获胜"), _T("游戏结束"), MB_OK);
			else MessageBox(GetHWnd(), _T("o 玩家获胜"), _T("游戏结束"), MB_OK);
			looping = false;
		}
		else if (CheckDraw())
		{
			MessageBox(GetHWnd(), _T("平局"), _T("游戏结束"), MB_OK);
			looping = false;
		}

		// 控制每秒60帧，即循环每秒最多循环60次
		DWORD end_time = GetTickCount();
		DWORD delay_time = end_time - start_time;
		if (delay_time < 1000 / 60)
		{
			Sleep(1000 / 60 - delay_time);
		}
	}
	EndBatchDraw();

	return 0;
}