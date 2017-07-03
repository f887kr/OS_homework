#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// C언어에서는 bool 자료형이 존재하지 않으므로 직접 정의
typedef enum {false, true} bool;
typedef int buffer_item;
#define BUFFER_SIZE 10

buffer_item *buffer;

// produceItem : 생산자가 생산한 Item (producerMonitoring에 사용)
// consumeItem : 소비자가 소비할 Item (consumerMonitoring에 사용)
int front = 0, rear = 0, produceItem = 0, consumeItem = 0;

// producerFlag, consumerFlag : Monitoring의 결과를 저장할 변수
bool producerFlag = 0, consumerFlag = 0;

// 에러를 확인할 변수
int semWaitError;
int semPostError;
int mutexLockError;
int mutexUnlockError;

sem_t empty;
sem_t full;
pthread_mutex_t mutex, conMutex, proMutex, proMoniMutex1, proMoniMutex2, conMoniMutex1, conMoniMutex2;

// 생산자가 생산한 Item을 모니터링합니다.
void *producerMonitoring() {
    while(1) {
        // main에서 proMoniMutex2를 lock 걸었으므로 생산자가 proMoniMutex2의 lock을 풀어주기 전까지
        // 즉, 생산자가 Item을 생산하고 모니터링을 필요로 할 때까지 producerMonitoring은 Block 되어있습니다.
        pthread_mutex_lock(&proMoniMutex2);
        producerFlag = true;
        // 생산자가 생산한 Item이 50보다 크다면 producerFlag를 false로 바꿉니다.
        if (produceItem > 50) {
            printf("producerMonitor filtering : %d\n", produceItem);
            producerFlag = false;
        }
        // 모니터링이 끝나면 proMoniMutex1의 lock을 풀어줍니다. 즉, 생산자의 Block을 해제합니다.
        pthread_mutex_unlock(&proMoniMutex1);
    }
}
// 소비자가 소비할 Item을 모니터링합니다.
void *consumerMonitoring() {
    while(1) {
        // main에서 conMoniMutex2를 lock 걸었으므로 소비자가 conMoniMutex2의 lock을 풀어주기 전까지
        // 즉, 소비자가 소비할 item의 모니터링을 필요로 할 때까지 consumerMonitoring은 Block 되어있습니다.
        pthread_mutex_lock(&conMoniMutex2);
        consumerFlag = true;
        // 소비자가 소비할 Item이 25보다 크다면 consumerFlag를 false로 바꿉니다.
        if(consumeItem > 25) {
            printf("consumerMonitor filtering : %d\n", consumeItem);
            consumerFlag = false;
        }
        // 모니터링이 끝나면 conMoniMutex1의 lock을 풀어줍니다. 즉, 소비자의 Block을 해제합니다.
        pthread_mutex_unlock(&conMoniMutex1);
    }
}

// producerMonitoring에 의해 필터링된 Item을 버퍼에 삽입합니다.
int insert_item(buffer_item *item) {
    semWaitError = sem_wait(&empty);
    mutexLockError = pthread_mutex_lock(&mutex);
    if(semWaitError != 0 || mutexLockError != 0)
        return -1;

    buffer[rear] = *item;
    printf("producer produced : %d\n", buffer[rear]);
    rear = (rear + 1) % BUFFER_SIZE;

    mutexUnlockError = pthread_mutex_unlock(&mutex);
    semPostError = sem_post(&full);
    if(semPostError != 0 || mutexUnlockError != 0)
        return -1;

    return 0;
}

// consumerMonitoring에 의해 정제된 Item을 버퍼로부터 가져옵니다.
int remove_item(buffer_item *item) {
    semWaitError = sem_wait(&full);
    mutexLockError = pthread_mutex_lock(&mutex);
    if(semWaitError != 0 || mutexLockError != 0)
        return -1;

    printf("consumer consumed : %d\n", buffer[front]);
    // 소비한 후 버퍼를 비워줍니다.
    buffer[front] = 0;
    front = (front + 1) % BUFFER_SIZE;

    mutexUnlockError = pthread_mutex_unlock(&mutex);
    semPostError = sem_post(&empty);
    if(semPostError != 0 || mutexUnlockError != 0)
        return -1;

    return 0;
}

void *producer(void *param) {
    buffer_item item;
    while(1){
        // producermonitoring 동안, 생산자가 Block 되도록 하기위해 trylock()을 사용
        // proMoniMutex1이 lock이 걸려있다면 EBUSY를 리턴하고 빠져나옵니다.
        // proMoniMutex1이 lock이 풀려있다면 lock을 걸어준 후, 0을 리턴하고 빠져나옵니다.
        pthread_mutex_trylock(&proMoniMutex1);

        int Time = rand() % 3 + 1; // 1~3초의 sleep time을 얻습니다.
        sleep(Time);

        // produceItem이 전역변수이므로 critical section problem이 발생할 수 있습니다.
        // 따라서 오직 1개의 생산자만 produceItem에 접근할 수 있도록 mutex를 사용하였습니다.
        pthread_mutex_lock(&proMutex);
        item = rand() % 100 + 1; // 1~100사이의 정수를 생성합니다.

        // producerMonitoring에서 모니터링하기 위해 생산한 item을 produceItem(전역변수)에 저장합니다.
        produceItem = item;

        // producerMonitoring의 Block을 해제합니다. 즉, 생산자 모니터링을 수행합니다.
        pthread_mutex_unlock(&proMoniMutex2);

        // pthread_mutex_trylock(&proMoniMutex1)로 proMoniMutex1이 lock 걸려있기 때문에
        // producerMonitoring 동안에 생산자는 Block 됩니다.
        pthread_mutex_lock(&proMoniMutex1);

        // producermonitoring의 결과, 1~50까지의 숫자일 경우에는 버퍼에 삽입합니다.
        // 만약 insert_item()이 정상적으로 이루어지지 않는다면 Error를 출력합니다.
        if(producerFlag == true) {
            if(insert_item(&item))
                printf("report error condition");
        }

        // 다른 생산자가 접근할 수 있도록 lock을 풀어줍니다.
        pthread_mutex_unlock(&proMutex);
    }
}

void *consumer(void *param) {
    buffer_item item;
    while(1){
        // producermonitoring 동안, 소비자가 Block 되도록 하기위해 trylock()을 사용
        // conMoniMutex1이 lock이 걸려있다면 EBUSY를 리턴하고 빠져나옵니다.
        // conMoniMutex1이 lock이 풀려있다면 lock을 걸어준 후, 0을 리턴하고 빠져나옵니다.
        pthread_mutex_trylock(&conMoniMutex1);

        int sleepTime = rand() % 3 + 1; // 1~3초의 sleep time을 얻습니다.
        sleep(sleepTime);

        // 생산자와 같은 이유(critical section problem)로 오직 1개의 소비자만 consumeItem에 접근할 수 있도록 mutex를 사용하였습니다.
        pthread_mutex_lock(&conMutex);

        // consumerMonitoring에서 모니터링하기 위해 소비할 item을 consumeItem(전역변수)에 저장합니다.
        consumeItem = buffer[front];

        // 만약 버이있는 버퍼에 접근할 경우
        if(consumeItem == 0) {
            // lock을 풀어주어 다음 소비자가 접근할 수 있도록 합니다.
            pthread_mutex_unlock(&conMutex);
            continue;
        }

        // consumerMonitoring의 Block을 해제합니다. 즉, 소비자 모니터링을 수행합니다.
        pthread_mutex_unlock(&conMoniMutex2);

        // pthread_mutex_trylock(&conMoniMutex1)로 conMoniMutex1이 lock 걸려있기 때문에
        // consumerMonitoring 동안에 소비자는 Block 됩니다.
        pthread_mutex_lock(&conMoniMutex1);

        // consumermonitoring의 결과, 26~50까지의 숫자일 경우에는 소비할 item을 2로 나눕니다.
        if(consumerFlag == false) {
            buffer[front] = buffer[front] / 2;
        }
        // item을 소비합니다. 만약 remove_item()이 정상적으로 이루어지지 않는다면 Error를 출력합니다.
        if(remove_item(&item))
            printf("report error condition");

        // 다른 소비자가 접근할 수 있도록 lock을 풀어줍니다.
        pthread_mutex_unlock(&conMutex);
    }
}

int main(int argc, char *argv[])
{
    // 1. Get command line arguments argv[1], argv[2], argv[3]

    // arg[1] : How long to sleep before terminating
    int howLongToSleep = atoi(argv[1]);
    // arg[2] : # of producer thread(s)
    int numOfProducerThread = atoi(argv[2]);
    // arg[3] : # of consumer thread(s)
    int numOfConsumerThread = atoi(argv[3]);
    int i = 0;

    // 2. Initialize buffer
    buffer = (buffer_item*)malloc(sizeof(buffer_item) * BUFFER_SIZE);

    // 초기 버퍼는 모두 비어있으므로 empty(semaphore value)를 버퍼의 크기로, full은 0으로 초기화합니다.
    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&full, 0, 0);

    // mutex를 초기화합니다.
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&proMutex, NULL);
    pthread_mutex_init(&conMutex, NULL);
    pthread_mutex_init(&proMoniMutex1, NULL);
    pthread_mutex_init(&proMoniMutex2, NULL);
    pthread_mutex_init(&conMoniMutex1, NULL);
    pthread_mutex_init(&conMoniMutex2, NULL);

    pthread_t proTid[100], conTid[100], proMoniTid, conMoniTid;

    // 3. Create producer thread(s)
    for(i=0; i<numOfProducerThread; i++) {
        pthread_create(&proTid[i],NULL,producer,NULL);
    }

    // 4. Create consumer thread(s)
    for(i=0;i<numOfConsumerThread;i++) {
        pthread_create(&conTid[i],NULL,consumer,NULL);
    }

    // 2개의 Monitor를 Block합니다.
    pthread_mutex_lock(&proMoniMutex2);
    pthread_mutex_lock(&conMoniMutex2);

    // 5. Create monitor threads
    pthread_create(&proMoniTid, NULL,producerMonitoring,NULL);
    pthread_create(&conMoniTid, NULL,consumerMonitoring,NULL);

    // 6. Sleep
    sleep(howLongToSleep);

    // 사용한 semaphore와 mutex의 자원을 해제합니다.
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&proMutex);
    pthread_mutex_destroy(&conMutex);
    pthread_mutex_destroy(&proMoniMutex1);
    pthread_mutex_destroy(&proMoniMutex2);
    pthread_mutex_destroy(&conMoniMutex1);
    pthread_mutex_destroy(&conMoniMutex2);

    // 7. Exit
    return 0;
}