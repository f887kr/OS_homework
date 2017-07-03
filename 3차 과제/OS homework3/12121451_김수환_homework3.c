#include <stdio.h>
#include <stdlib.h>

// C언어에서는 bool 자료형이 존재하지 않으므로 직접 정의
typedef enum { false, true } bool;

int i, j;
// TLB에 demand한 page가 있으면 true, 없으면 false
bool inTLB;
// total demand paging의 횟수
int demandCount;
// pageTable entry : 256
// 각 page의 frameNum, validation 를 저장
int pageTable[256][2];
// physical memory 구조를 배열로 구현 (entry : 128, frame size : 256 Bytes)
unsigned char physicalMemory[128][256];
// victim으로 선택된 page의 frame number
int emptyFrameNum;
int pageFaultCount;

struct entry {
	int pageNum;
	int frameNum;
	int validation;

	struct entry* next;
	struct entry* prev;
};
struct table {
	// TLB entry : 16, frameTable entry : 128
	int size;
	// TLB.hit (LRU)
	// frameTable.hit (LRU/FIFO)
	int hit;

	struct entry* head;
	struct entry* tail;
}frameTable, TLB;

// victim을 선별하여 반환하는 함수
struct entry* selectVictim(struct table*);
// TLB에서 page를 찾고, LRU로 TLB의 entry를 관리하는 함수
void lookUpTLB(int);
// demandPaging시 page replacement algorithm이 LRU인 총괄적인 작업을 수행하는 함수
struct entry* LRU_demandPaging(int);
// demandPaging시 page replacement algorithm이 FIFO인 총괄적인 작업을 수행하는 함수
struct entry* FIFO_demandPaging(int);
// Backing Store에서 해당 Page의 데이터를 해당 frame에 삽입하는 함수
void upToFrame(int, int);

// victim page를 반환하는 함수
struct entry* selectVictim(struct table* table) {
	struct entry* p = table->head;
	table->head = p->next;
	table->head->prev = NULL;
	return p;
}

// TLB를 Look up하고, LRU(Stack implementation)로 관리하는 함수
void lookUpTLB(int pNum) {
	// TLB가 비어있다면 [MISS]
	// TLB에 entry를 추가합니다
	if (TLB.size == 0) {
		struct entry* newEntry = (struct entry*)malloc(sizeof(struct entry));
		newEntry->pageNum = pNum;
		// 0번째 frame에 올리자
		newEntry->frameNum = 0;
		newEntry->next = NULL;
		newEntry->prev = NULL;
		TLB.head = newEntry;
		TLB.tail = newEntry;
		TLB.size++;
	}
	// TLB가 비어있지 않다면
	// TLB를 탐색합니다
	else {
		struct entry* p;
		// TLB의 head에 page가 있다면 [HIT]
		if (TLB.head->pageNum == pNum) {
			p = TLB.head;
			// TLB entry가 1개라면 그래도 둡니다
			// 하지만 1개 이상이라면 TLB에서 해당 page를 뽑아냅니다 LUR(Stack implementation)
			if (TLB.size != 1) {
				TLB.head = TLB.head->next;
				TLB.head->prev = NULL;
				// TBL에 있으니 true
				inTLB = true;
			}
		}
		// TLB의 tail에 page가 있다면 [HIT]
		else if (TLB.tail->pageNum == pNum) {
			p = TLB.tail;
			// TLB entry가 1개라면 그래도 둡니다
			// 하지만 1개 이상이라면 TLB에서 해당 page를 뽑아냅니다 LUR(Stack implementation)
			if (TLB.size != 1) {
				TLB.tail = TLB.tail->prev;
				TLB.tail->next = NULL;
				// TBL에 있으니 true
				inTLB = true;
			}
		}
		// TLB의 head와 tail 사이에 page가 있다면 [HIT]
		else {
			// p는 head의 다음 노드로 설정
			p = TLB.head->next;
			// 탐색
			while (p != NULL) {
				if (p->pageNum == pNum) {
					p->next->prev = p->prev;
					p->prev->next = p->next;
					// TBL에 있으니 true
					inTLB = true;
					break;
				}
				p = p->next;
			}
		}
		// TLB에 demand한 page가 있다면 찾은 page를 tail로 옮겨줍니다 (Update)
		if (inTLB == true) {
			// TLB size가 1이면 별도의 Update가 필요없고,
			// TLB size가 1이 아닐 경우에만 Update를 합니다
			if (TLB.size != 1) {
				TLB.tail->next = p;
				p->next = NULL;
				p->prev = TLB.tail;
				TLB.tail = p;
			}
			// TLB HIT !!
			TLB.hit++;
		}
		// TLB에 demand한 page가 없다면 [MISS]
		else {
			struct entry* newEntry = (struct entry*)malloc(sizeof(struct entry));
			// TLB가 가득찼다면 victim page를 선택합니다.
			if (TLB.size == 16) {
				struct entry* victim = selectVictim(&TLB);
				free(victim);
				newEntry->pageNum = pNum;
				// 어느 frame으로 올라갈지 모르니 일단 -1로 초기화하고,
				// 후에 frame이 정해지면 업데이트합니다
				newEntry->frameNum = -1;
				// TLB의 tail로 새로운 entry를 삽입합니다
				TLB.tail->next = newEntry;
				newEntry->next = NULL;
				newEntry->prev = TLB.tail;
				TLB.tail = newEntry;
			}
			// TLB가 가득차지 않았다면
			else {
				// 처음에는 차곡차곡 frame에 쌓입니다
				newEntry->pageNum = pNum;
				newEntry->frameNum = TLB.size;
				// TLB의 tail로 새로운 entry를 삽입합니다
				TLB.tail->next = newEntry;
				newEntry->next = NULL;
				newEntry->prev = TLB.tail;
				TLB.tail = newEntry;
				TLB.size++;
			}
		}
	}
}

// Backing Store에서 해당 Page의 데이터를 해당 frame에 삽입하는 함수
void upToFrame(int pNum, int fNum) {
	FILE *backingStore = fopen("BACKING_STORE.bin", "rb");
	fseek(backingStore, pNum * 256, SEEK_SET);
	fread(physicalMemory[fNum], 256, 1, backingStore);
	fclose(backingStore);
}

// demandPaging시 page replacement algorithm이 LRU인 총괄적인 작업을 수행하는 함수
struct entry* LRU_demandPaging(int pNum) {
	inTLB = false;
	// demandPaging 횟수 증가
	demandCount++;
	// 먼저 TLB를 Look up
	lookUpTLB(pNum);
	// TLB hit의 경우 이미 frame에 있으므로
	// frameTable을 업데이트합니다
	if (inTLB == true) {
		struct entry* p;
		// frameTable의 head에 page가 있다면
		if (frameTable.head->pageNum == pNum) {
			p = frameTable.head;
			if (frameTable.size != 1) {
				frameTable.head = frameTable.head->next;
				frameTable.head->prev = NULL;
			}
		}
		// frameTable의 tail에 page가 있다면
		else if (frameTable.tail->pageNum == pNum) {
			p = frameTable.tail;
			if (frameTable.size != 1) {
				p = frameTable.tail;
				frameTable.tail = frameTable.tail->prev;
				frameTable.tail->next = NULL;
			}
		}
		// frameTable의 head와 tail 사이에 page가 있다면
		// TLB에 page가 있었으므로 frameTable의 뒤에부터 탐색하는 것이 빠릅니다
		else {
			// p는 tail의 이전 노드로 설정
			p = frameTable.tail->prev;
			while (p != NULL) {
				if (p->pageNum == pNum) {
					p->next->prev = p->prev;
					p->prev->next = p->next;
					break;
				}
				p = p->prev;
			}
		}
		// frameTable의 entrt가 1개가 아니라면 찾은 page를 frameTable의 tail로 옮깁니다
		if (frameTable.size != 1) {
			frameTable.tail->next = p;
			p->next = NULL;
			p->prev = frameTable.tail;
			frameTable.tail = p;
		}
		return p;
	}
	// TLB Miss의 경우 pageTable에 접근하여 frameTable에 해당 page가 있는지 확인해야합니다
	else {
		// 첫번째 demand일 경우 pageFault 발생
		if (demandCount == 1) {
			struct entry* newEntry = (struct entry*)malloc(sizeof(struct entry));
			// frameTable에 새로운 entry를 삽입합니다.
			newEntry->pageNum = pNum;
			newEntry->frameNum = 0;
			newEntry->validation = 1;
			newEntry->next = NULL;
			newEntry->prev = NULL;
			frameTable.head = newEntry;
			frameTable.tail = newEntry;
			frameTable.size++;
			pageFaultCount++;
			// 0번째 frame에 page를 올립니다
			upToFrame(pNum, 0);
			// pageTable 업데이트
			pageTable[pNum][0] = 0;
			pageTable[pNum][1] = true;

			return newEntry;
		}
		else {
			// pageTable의 validation값이 false면 pageFault 발생
			if (pageTable[pNum][1] == false) {
				struct entry* newEntry = (struct entry*)malloc(sizeof(struct entry));
				// frameTable이 가득찼다면 (즉, Physical Memory 가득참)
				if (frameTable.size == 128) {
					// victim page를 선택합니다.
					struct entry* victim = selectVictim(&frameTable);
					emptyFrameNum = victim->frameNum;
					// victim page의 frame에 해당 page를 올립니다
					upToFrame(pNum, emptyFrameNum);
					// pageTable 업데이트
					pageTable[pNum][0] = emptyFrameNum;
					pageTable[pNum][1] = true;
					// pageTable에서 victim page의 validation을 false로 바꿉니다
					pageTable[victim->pageNum][1] = false;
					free(victim);

					// frameTable의 tail에 새로운 entry를 삽입합니다
					newEntry->pageNum = pNum;
					newEntry->frameNum = emptyFrameNum;
					newEntry->validation = 1;
					frameTable.tail->next = newEntry;
					newEntry->next = NULL;
					newEntry->prev = frameTable.tail;
					frameTable.tail = newEntry;
				}
				// frameTable이 가득차지 않았다면
				else {
					// 차곡차곡 frame에 쌓입니다
					newEntry->pageNum = pNum;
					newEntry->frameNum = frameTable.size;
					newEntry->validation = 1;
					// frame에 해당 page를 올립니다
					upToFrame(pNum, frameTable.size);
					// pageTable 업데이트
					pageTable[pNum][0] = frameTable.size;
					pageTable[pNum][1] = true;
					// frameTable의 tail로 새로운 entry를 삽입합니다
					frameTable.tail->next = newEntry;
					newEntry->prev = frameTable.tail;
					newEntry->next = NULL;
					frameTable.tail = newEntry;
					frameTable.size++;
				}
				// 앞서 TLB가 가득찬 상황에서 MISS일 경우
				// 새롭게 TLB에 올린 page의 frame을 설정합니다
				TLB.tail->frameNum = newEntry->frameNum;
				pageFaultCount++;
				return newEntry;
			}
			//  pageTable의 validation값이 true면 [HIT]
			else {
				// hit !!
				frameTable.hit++;
				struct entry* p;
				// frameTable의 head에 page가 있다면
				if (frameTable.head->pageNum == pNum) {
					p = frameTable.head;
					if (frameTable.size != 1) {
						frameTable.head = frameTable.head->next;
						frameTable.head->prev = NULL;
					}
				}
				// frameTable의 tail에 page가 있다면
				else if (frameTable.tail->pageNum == pNum) {
					p = frameTable.tail;
					if (frameTable.size != 1) {
						frameTable.tail = frameTable.tail->prev;
						frameTable.tail->next = NULL;
					}
				}
				// frameTable의 head와 tail 사이에 page가 있다면
				else {
					// p는 head의 다음 노드로 설정
					p = frameTable.head->next;
					while (p != NULL) {
						if (p->pageNum == pNum) {
							p->next->prev = p->prev;
							p->prev->next = p->next;
							break;
						}
						p = p->next;
					}
				}
				// 앞서 TLB가 가득찬 상황에서 MISS일 경우
				// 새롭게 TLB에 올린 page의 frame을 설정합니다
				TLB.tail->frameNum = p->frameNum;

				// frameTable의 entrt가 1개가 아니라면 찾은 page를 frameTable의 tail로 옮깁니다
				if (frameTable.size != 1) {
					frameTable.tail->next = p;
					p->next = NULL;
					p->prev = frameTable.tail;
					frameTable.tail = p;
				}
				return p;
			}
		}
	}
}

struct entry* FIFO_demandPaging(int pNum) {
	inTLB = false;
	// demandPaging 횟수 증가
	demandCount++;
	// 먼저 TLB를 Look up
	lookUpTLB(pNum);
	// 첫번째 demand일 경우 pageFault 발생
	if (demandCount == 1) {
		struct entry* newEntry = (struct entry*)malloc(sizeof(struct entry));
		// frameTable에 새로운 entry를 삽입합니다
		newEntry->pageNum = pNum;
		newEntry->frameNum = 0;
		newEntry->validation = 1;
		newEntry->next = NULL;
		newEntry->prev = NULL;
		frameTable.head = newEntry;
		frameTable.tail = newEntry;
		frameTable.size++;
		pageFaultCount++;
		// 0번째 frame에 page를 올림
		upToFrame(pNum, 0);
		// pageTable 업데이트
		pageTable[pNum][0] = 0;
		pageTable[pNum][1] = true;

		return newEntry;
	}
	else {
		// pageTable의 validation값이 false면 pageFault 발생
		if (pageTable[pNum][1] == false) {
			struct entry* newEntry = (struct entry*)malloc(sizeof(struct entry));
			// frameTable이 가득찼다면
			if (frameTable.size == 128) {
				// victim page를 선택합니다.
				struct entry* victim = selectVictim(&frameTable);
				emptyFrameNum = victim->frameNum;
				// victim page의 frame에 해당 page를 올려준다
				upToFrame(pNum, emptyFrameNum);
				// pageTable 업데이트
				pageTable[pNum][0] = emptyFrameNum;
				pageTable[pNum][1] = true;
				// pageTable에서 victim page의 validation을 false로 바꿉니다
				pageTable[victim->pageNum][1] = false;
				free(victim);

				// frameTable의 tail에 새로운 entry를 삽입합니다
				newEntry->pageNum = pNum;
				newEntry->frameNum = emptyFrameNum;
				newEntry->validation = 1;
				frameTable.tail->next = newEntry;
				newEntry->next = NULL;
				newEntry->prev = frameTable.tail;
				frameTable.tail = newEntry;
			}
			// frameTable이 가득차지 않았다면
			else {
				// 차곡차곡 frame에 쌓입니다
				newEntry->pageNum = pNum;
				newEntry->frameNum = frameTable.size;
				newEntry->validation = 1;
				// frame에 해당 page를 올립니다
				upToFrame(pNum, frameTable.size);
				// pageTable 업데이트
				pageTable[pNum][0] = frameTable.size;
				pageTable[pNum][1] = true;
				// frameTable의 tail로 새로운 entry를 삽입합니다
				frameTable.tail->next = newEntry;
				newEntry->prev = frameTable.tail;
				newEntry->next = NULL;
				frameTable.tail = newEntry;
				frameTable.size++;
			}
			// 앞서 TLB가 가득찬 상황에서 MISS일 경우
			// 새롭게 TLB에 올린 page의 frame을 설정합니다
			TLB.tail->frameNum = newEntry->frameNum;
			pageFaultCount++;
			return newEntry;
		}
		// pageTable의 validation값이 true면 [HIT]
		// FIFO algorithm은 별도로 frameTable을 업데이트하지 않습니다
		else {
			// TLB에서 이미 hit난 경우 제외
			if (inTLB == false)
				frameTable.hit++;
			// 앞서 TLB가 가득찬 상황에서 MISS일 경우
			// 새롭게 TLB에 올린 page의 frame을 설정합니다
			TLB.tail->frameNum = pageTable[pNum][0];
			struct entry* p = (struct entry*)malloc(sizeof(struct entry));
			p->frameNum = pageTable[pNum][0];
			return p;
		}
	}
}

int main(int argc, char *argv[])
{
	frameTable.size = 0;
	frameTable.hit = 0;
	TLB.size = 0;
	TLB.hit = 0;
	pageFaultCount = 0;

	char address[10];
	// input파일
	FILE *input;
	// output파일
	FILE *LRU_physical_memory;
	FILE *LRU_frame;
	FILE *LRU_translation;

	input = fopen(argv[1], "r");
	LRU_physical_memory = fopen("LRU_Physical_Memory.bin", "wb");
	LRU_frame = fopen("LRU_Frame_Table.txt", "w");
	LRU_translation = fopen("LRU_Physical.txt", "w");

	int pNum, virtualAddress;
	int offset;
	int index;
	while (fscanf(input, "%s\n", address) != EOF) {
		virtualAddress = atoi(address);
		// 하위 16bit만 사용
		virtualAddress = virtualAddress & ((1 << 16) - 1);
		// offset 계산
		offset = virtualAddress & ((1 << 8) - 1);
		// Page Number 계산
		pNum = virtualAddress >> 8;
		// Demand Paging
		struct entry* result = LRU_demandPaging(pNum);
		// Translation 결과를 파일로 출력합니다.
		fprintf(LRU_translation, "Virtual address: %d Physical address: %d\n", (pNum << 8) + offset, (result->frameNum << 8) + offset);
	}
	struct entry* p = frameTable.head;
	// frameTable의 데이터를 frame Number 순서로 저장합니다.
	int frameTemp[128][2] = { 0, };
	while (p != NULL) {
		frameTemp[p->frameNum][0] = p->validation;
		frameTemp[p->frameNum][1] = (p->pageNum << 8);
		p = p->next;
	}
	// frame Table의 데이터를 파일로 출력합니다.
	for (i = 0; i < 128; i++) {
		fprintf(LRU_frame, "%d %d %d\n", i + 1, frameTemp[i][0], frameTemp[i][1]);
	}
	// Physical Memory의 데이터를 파일로 출력합니다.
	for (i = 0; i < 128; i++) {
		for (j = 0; j < 256; j++) {
			fputc(physicalMemory[i][j], LRU_physical_memory);
			/*
			if (i != 0 || j != 0)
				if (j % 16 == 0)
					fprintf(LRU_physical_memory, "\n");
			fprintf(LRU_physical_memory, "%02X ", physicalMemory[i][j]);
			*/
		}
	}

	fclose(input);
	fclose(LRU_physical_memory);
	fclose(LRU_frame);
	fclose(LRU_translation);

	printf("TLB hit ratio : %d hits out of %d\n", TLB.hit, demandCount);
	printf("LRU hit ratio : %d hits out of %d\n", frameTable.hit + TLB.hit, demandCount);

	/////////////////////////////////////////////////////////
	////////////////////// FIFO /////////////////////////////
	/////////////////////////////////////////////////////////

	////////////////////// 초기화 ///////////////////////////
	frameTable.size = 0;
	frameTable.hit = 0;
	TLB.size = 0;
	TLB.hit = 0;
	demandCount = 0;
	pageFaultCount = 0;
	struct entry* del = frameTable.head, temp;
	while (del != NULL) {
		struct entry* temp = del->next;
		free(del);
		del = temp;
	}
	del = TLB.head;
	while (del != NULL) {
		struct entry* temp = del->next;
		free(del);
		del = temp;
	}
	for (i = 0; i < 128; i++)
		for (j = 0; j < 256; j++)
			physicalMemory[i][j] = 0;
	for (i = 0; i < 256; i++)
		pageTable[i][0] = pageTable[i][1] = 0;
	for (i = 0; i < 128; i++)
		frameTemp[i][0] = frameTemp[i][1] = 0;
	////////////////////// 초기화 ///////////////////////////

	FILE *FIFO_physical_memory;
	FILE *FIFO_frame;
	FILE *FIFO_translation;

	input = fopen(argv[1], "r");
	FIFO_physical_memory = fopen("FIFO_Physical_Memory.bin", "wb");
	FIFO_frame = fopen("FIFO_Frame_Table.txt", "w");
	FIFO_translation = fopen("FIFO_Physical.txt", "w");

	while (fscanf(input, "%s\n", address) != EOF) {
		virtualAddress = atoi(address);
		virtualAddress = virtualAddress & ((1 << 16) - 1);
		offset = virtualAddress & ((1 << 8) - 1);
		pNum = virtualAddress >> 8;
		struct entry* result = FIFO_demandPaging(pNum);
		fprintf(FIFO_translation, "Virtual address: %d Physical address: %d\n", (pNum << 8) + offset, (result->frameNum << 8) + offset);
	}
	p = frameTable.head;
	while (p != NULL) {
		frameTemp[p->frameNum][0] = p->validation;
		frameTemp[p->frameNum][1] = (p->pageNum << 8);
		p = p->next;
	}
	for (i = 0; i < 128; i++) {
		fprintf(FIFO_frame, "%d %d %d\n", i + 1, frameTemp[i][0], frameTemp[i][1]);
	}
	for (i = 0; i < 128; i++) {
		for (j = 0; j < 256; j++) {
			fputc(physicalMemory[i][j], FIFO_physical_memory);
			/*
			if (i != 0 || j != 0)
				if (j % 16 == 0)
					fprintf(FIFO_physical_memory, "\n");
			fprintf(FIFO_physical_memory, "%02X ", physicalMemory[i][j]);
			*/
		}
	}
	fclose(input);
	fclose(FIFO_physical_memory);
	fclose(FIFO_frame);
	fclose(FIFO_translation);

	printf("FIFO hit ratio : %d hits out of %d\n", frameTable.hit + TLB.hit, demandCount);


	return 0;
}
