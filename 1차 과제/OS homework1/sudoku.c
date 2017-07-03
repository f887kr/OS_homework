#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// C언어에서는 bool 자료형이 존재하지 않으므로 직접 정의
typedef enum {false, true} bool;
// 3x3 크기의 블록 구역의 가운데 칸의 위치를 저장할 구조체 변수 선언
typedef struct
{
	int x, y;
} Point;

// 8방향 탐색 배열 : 좌상, 상, 우상, 좌, 우, 좌하, 하, 우하
int dir[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
int sudoku[10][10];

// 스도쿠의 열 단위로 확인하는 함수
void *confirmCol() {
		int col = 0, row = 0;
	  for(col = 0 ; col < 9 ; col++) {
		  	// 9개의 숫자가 모두 존재하는지 확인할 배열 선언
		    bool confirmNumber[10] = {false, };
		  	// 해당 스도쿠 칸에 있는 숫자를 인덱스로하여
		  	// confirmNumber 배열의 값을 true로 바꿔준다
		    for(row = 0 ; row < 9 ; row++) {
		      confirmNumber[sudoku[row][col]] = true;
		    }
		  	// 9개의 숫자가 모두 존재하는지 확인
	  		int i = 0 ;
	    	for(i = 1 ; i < 10 ; i++) {
	      		// 스도쿠의 규칙을 위반한 경우
						// thread를 termiantion하고, pthread_join에 false값을 전달
						if(confirmNumber[i] == false) {
		      	    pthread_exit((void *)false);
		        }
	    	}
	  }
	  // 스도쿠의 규칙을 지킨 경우
	  // thread를 termiantion하고, pthread_join에 true값을 전달
	  pthread_exit((void *)true);
}
// 스도쿠의 행 단위로 확인하는 함수
void *confirmRow() {
		// confirmCol 함수와 같은 알고리즘으로 동작
		int row = 0, col = 0;
    for(row = 0 ; row < 9 ; row++) {
        bool confirmNumber[10] = {false, };
        for(col = 0 ; col < 9 ; col++) {
           confirmNumber[sudoku[row][col]] = true;
        }
    		// 9개의 숫자가 모두 존재하는지 확인
	    	int i = 0 ;
        for(i = 1 ; i < 10 ; i++) {
        		// 스도쿠의 규칙을 위반한 경우
						// thread를 termiantion하고, pthread_join에 false값을 전달
            if(confirmNumber[i] == false) {
              	pthread_exit((void *)false);
            }
        }
    }
    // 스도쿠의 규칙을 지킨 경우
		// thread를 termiantion하고, pthread_join에 true값을 전달
		pthread_exit((void *)true);
}
// 스도쿠의 3x3 크기의 블록 구역을 확인하는 함수
void *comfirmBlock(void *startXY) {
		// (void *)자료형의 매개변수를 (Point *)자료형으로 형변환
    Point *p = (Point *)startXY;
		// 해당 3x3 크기의 블록 구역의 가운데 칸의 위치를 저장
    int startY = p->y;
    int startX = p->x;
		// 9개의 숫자가 모두 존재하는지 확인할 배열 선언
    bool confirmNumber[10] = {false, };
		// 가운데 칸의 숫자 확인
    confirmNumber[sudoku[startY][startX]] = true;
		// 전역변수로 선언한 dir 배열을 이용하여 가운데를 기준으로 8방향 탐색
		int i = 0 ;
    for(i = 0 ; i < 8 ; i++) {
        int nowY = startY + dir[i][0];
        int nowX = startX + dir[i][1];
        confirmNumber[sudoku[nowY][nowX]] = true;
    }
		// 9개의 숫자가 모두 존재하는지 확인
    for(i = 1 ; i < 10 ; i++) {
		// 스도쿠의 규칙을 위반한 경우
		// thread를 termiantion하고, pthread_join에 false값을 전달
        if(confirmNumber[i] == false) {
            pthread_exit((void *)false);
        }
    }
    // 스도쿠의 규칙을 지킨 경우
		// thread를 termiantion하고, pthread_join에 true값을 전달
    pthread_exit((void *)true);
}

int main(int argc,const char **argv)
{
		FILE *f;
		f = fopen("input.txt","r");
    // File로부터 Sudoku puzzle number를 입력받는다
    int i =  0, j = 0 ;
    for(i = 0 ; i < 9 ; i++) {
        for(j = 0 ; j < 9 ; j++) {
            fscanf(f, "%1d", &sudoku[i][j]);
        }
    }
    fclose(f);
		// 11개의 tid를 저장할 배열을 선언
    pthread_t sudoku_thread[11];
    // 스도쿠의 열 단위를 확인하는 thread 생성
    pthread_create(&sudoku_thread[0], NULL, (void *)confirmCol, NULL);
    // 스도쿠의 행 단위를 확인하는 thread 생성
    pthread_create(&sudoku_thread[1], NULL, (void *)confirmRow, NULL);

		// 3x3 구역의 가운데 위치를 저장할 구조체 변수 선언
    Point *ConfirmXY;
		// 나머지 9개의 thread를 생성하기 위해 사용할 변수 선언
    int index = 2;
    for(i = 0 ; i < 3 ; i++) {
        for(j = 0 ; j < 3 ; j++) {
						// 구조체 변수를 동적할당
            ConfirmXY = (Point *)malloc(sizeof(Point));
            // 3x3 구역의 가운데 위치로 초기화
            ConfirmXY->y = 1 + (i * 3);
            ConfirmXY->x = 1 + (j * 3);
            // 스도쿠의 3x3 크기의 블록 구역을 확인하는 thread 생성
            pthread_create(&sudoku_thread[index++], NULL, (void *)comfirmBlock, ConfirmXY);
        }
    }

    // 11개 자식 thread의 리턴값을 저장할 배열 선언
    bool is_valid[11];
    for(i = 0 ; i < 11 ; i++) {
				// 11개의 자식 thread의 종료를 기다림과 동시에 리턴값을 is_valid 배열에 저장
        int receive = pthread_join(sudoku_thread[i], (void **)&is_valid[i]);
    }

		// 11개의 규칙을 모두 지켰는지 확인할 변수 선언
    int sudoku_valid = 0;
    for(i = 0 ; i < 11 ; i++) {
        sudoku_valid += is_valid[i];
    }
		// 모든 자식 thread의 리턴값이 true(1)일 때, 모든 규칙을 지킨 것이므로 유효한 스도쿠
    if(sudoku_valid == 11) {
        printf("Valid result !\n");
    }
		// 자식 thread의 리턴값이 하나라도 false(0)일 때, 규칙을 어긴 것이므로 무효한 스도쿠
    else {
        printf("Invalid result !\n");
    }
		// 동적할당 해제
		free(ConfirmXY);

    return 0;
}
