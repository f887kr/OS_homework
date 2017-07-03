#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

// fibonacci 수열을 저장하는 배열
int *fibonacciNum;

// input번째까지 fibonacci 수열을 계산하고 저장하는 함수
void *generateFibonacci(void *input) {
  	// (void *)자료형의 매개변수를 (int *)자료형으로 형변환
    int num = *(int *)input;
    // 1번째까지 fibonacci 수열을 원하는 경우
    if(num == 1) {
        fibonacciNum[0] = 0;
        pthread_exit((void *)NULL);
    }
    // 2 ~ input까지의 fibonacci 수열을 원하는 경우
    else {
        fibonacciNum[0] = 0;
        fibonacciNum[1] = 1;
        // fibonacci 수열을 계산하고 저장
        int i = 0;
        for(i = 2 ; i < num ; i++) {
            fibonacciNum[i] = fibonacciNum[i - 1] + fibonacciNum[i - 2];
        }
    }
    // thread를 termination하고,
    // 다른 결과값을 전달할 필요가 없으므로 pthread_join에 NULL을 전달
    pthread_exit((void *)NULL);
}

int main(void) {
    int n;
    scanf("%d",&n);
    // fibonacci 수열을 저장할 배열 동적할당
    fibonacciNum = (int *)malloc(sizeof(int) * n);

    // tid를 저장할 변수 선언
    pthread_t fibo_thread;
    // 피보나치 수열을 계산하고 저장할 thread 생성
    pthread_create(&fibo_thread, NULL, (void *)generateFibonacci, &n);
  	// 자식 thread의 종료를 기다림
    pthread_join(fibo_thread, NULL);
    // fibonacci 수열을 출력
    int i = 0;
    for(i = 0 ; i < n; i++) {
        if(i != 0)
            printf(",");
        printf("%d",fibonacciNum[i]);
    }
    printf("\n");
    // 동적할당 해제
    free(fibonacciNum);
    return 0;
}
