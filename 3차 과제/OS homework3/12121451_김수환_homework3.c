#include <stdio.h>
#include <stdlib.h>

// C������ bool �ڷ����� �������� �����Ƿ� ���� ����
typedef enum { false, true } bool;

int i, j;
// TLB�� demand�� page�� ������ true, ������ false
bool inTLB;
// total demand paging�� Ƚ��
int demandCount;
// pageTable entry : 256
// �� page�� frameNum, validation �� ����
int pageTable[256][2];
// physical memory ������ �迭�� ���� (entry : 128, frame size : 256 Bytes)
unsigned char physicalMemory[128][256];
// victim���� ���õ� page�� frame number
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

// victim�� �����Ͽ� ��ȯ�ϴ� �Լ�
struct entry* selectVictim(struct table*);
// TLB���� page�� ã��, LRU�� TLB�� entry�� �����ϴ� �Լ�
void lookUpTLB(int);
// demandPaging�� page replacement algorithm�� LRU�� �Ѱ����� �۾��� �����ϴ� �Լ�
struct entry* LRU_demandPaging(int);
// demandPaging�� page replacement algorithm�� FIFO�� �Ѱ����� �۾��� �����ϴ� �Լ�
struct entry* FIFO_demandPaging(int);
// Backing Store���� �ش� Page�� �����͸� �ش� frame�� �����ϴ� �Լ�
void upToFrame(int, int);

// victim page�� ��ȯ�ϴ� �Լ�
struct entry* selectVictim(struct table* table) {
	struct entry* p = table->head;
	table->head = p->next;
	table->head->prev = NULL;
	return p;
}

// TLB�� Look up�ϰ�, LRU(Stack implementation)�� �����ϴ� �Լ�
void lookUpTLB(int pNum) {
	// TLB�� ����ִٸ� [MISS]
	// TLB�� entry�� �߰��մϴ�
	if (TLB.size == 0) {
		struct entry* newEntry = (struct entry*)malloc(sizeof(struct entry));
		newEntry->pageNum = pNum;
		// 0��° frame�� �ø���
		newEntry->frameNum = 0;
		newEntry->next = NULL;
		newEntry->prev = NULL;
		TLB.head = newEntry;
		TLB.tail = newEntry;
		TLB.size++;
	}
	// TLB�� ������� �ʴٸ�
	// TLB�� Ž���մϴ�
	else {
		struct entry* p;
		// TLB�� head�� page�� �ִٸ� [HIT]
		if (TLB.head->pageNum == pNum) {
			p = TLB.head;
			// TLB entry�� 1����� �׷��� �Ӵϴ�
			// ������ 1�� �̻��̶�� TLB���� �ش� page�� �̾Ƴ��ϴ� LUR(Stack implementation)
			if (TLB.size != 1) {
				TLB.head = TLB.head->next;
				TLB.head->prev = NULL;
				// TBL�� ������ true
				inTLB = true;
			}
		}
		// TLB�� tail�� page�� �ִٸ� [HIT]
		else if (TLB.tail->pageNum == pNum) {
			p = TLB.tail;
			// TLB entry�� 1����� �׷��� �Ӵϴ�
			// ������ 1�� �̻��̶�� TLB���� �ش� page�� �̾Ƴ��ϴ� LUR(Stack implementation)
			if (TLB.size != 1) {
				TLB.tail = TLB.tail->prev;
				TLB.tail->next = NULL;
				// TBL�� ������ true
				inTLB = true;
			}
		}
		// TLB�� head�� tail ���̿� page�� �ִٸ� [HIT]
		else {
			// p�� head�� ���� ���� ����
			p = TLB.head->next;
			// Ž��
			while (p != NULL) {
				if (p->pageNum == pNum) {
					p->next->prev = p->prev;
					p->prev->next = p->next;
					// TBL�� ������ true
					inTLB = true;
					break;
				}
				p = p->next;
			}
		}
		// TLB�� demand�� page�� �ִٸ� ã�� page�� tail�� �Ű��ݴϴ� (Update)
		if (inTLB == true) {
			// TLB size�� 1�̸� ������ Update�� �ʿ����,
			// TLB size�� 1�� �ƴ� ��쿡�� Update�� �մϴ�
			if (TLB.size != 1) {
				TLB.tail->next = p;
				p->next = NULL;
				p->prev = TLB.tail;
				TLB.tail = p;
			}
			// TLB HIT !!
			TLB.hit++;
		}
		// TLB�� demand�� page�� ���ٸ� [MISS]
		else {
			struct entry* newEntry = (struct entry*)malloc(sizeof(struct entry));
			// TLB�� ����á�ٸ� victim page�� �����մϴ�.
			if (TLB.size == 16) {
				struct entry* victim = selectVictim(&TLB);
				free(victim);
				newEntry->pageNum = pNum;
				// ��� frame���� �ö��� �𸣴� �ϴ� -1�� �ʱ�ȭ�ϰ�,
				// �Ŀ� frame�� �������� ������Ʈ�մϴ�
				newEntry->frameNum = -1;
				// TLB�� tail�� ���ο� entry�� �����մϴ�
				TLB.tail->next = newEntry;
				newEntry->next = NULL;
				newEntry->prev = TLB.tail;
				TLB.tail = newEntry;
			}
			// TLB�� �������� �ʾҴٸ�
			else {
				// ó������ �������� frame�� ���Դϴ�
				newEntry->pageNum = pNum;
				newEntry->frameNum = TLB.size;
				// TLB�� tail�� ���ο� entry�� �����մϴ�
				TLB.tail->next = newEntry;
				newEntry->next = NULL;
				newEntry->prev = TLB.tail;
				TLB.tail = newEntry;
				TLB.size++;
			}
		}
	}
}

// Backing Store���� �ش� Page�� �����͸� �ش� frame�� �����ϴ� �Լ�
void upToFrame(int pNum, int fNum) {
	FILE *backingStore = fopen("BACKING_STORE.bin", "rb");
	fseek(backingStore, pNum * 256, SEEK_SET);
	fread(physicalMemory[fNum], 256, 1, backingStore);
	fclose(backingStore);
}

// demandPaging�� page replacement algorithm�� LRU�� �Ѱ����� �۾��� �����ϴ� �Լ�
struct entry* LRU_demandPaging(int pNum) {
	inTLB = false;
	// demandPaging Ƚ�� ����
	demandCount++;
	// ���� TLB�� Look up
	lookUpTLB(pNum);
	// TLB hit�� ��� �̹� frame�� �����Ƿ�
	// frameTable�� ������Ʈ�մϴ�
	if (inTLB == true) {
		struct entry* p;
		// frameTable�� head�� page�� �ִٸ�
		if (frameTable.head->pageNum == pNum) {
			p = frameTable.head;
			if (frameTable.size != 1) {
				frameTable.head = frameTable.head->next;
				frameTable.head->prev = NULL;
			}
		}
		// frameTable�� tail�� page�� �ִٸ�
		else if (frameTable.tail->pageNum == pNum) {
			p = frameTable.tail;
			if (frameTable.size != 1) {
				p = frameTable.tail;
				frameTable.tail = frameTable.tail->prev;
				frameTable.tail->next = NULL;
			}
		}
		// frameTable�� head�� tail ���̿� page�� �ִٸ�
		// TLB�� page�� �־����Ƿ� frameTable�� �ڿ����� Ž���ϴ� ���� �����ϴ�
		else {
			// p�� tail�� ���� ���� ����
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
		// frameTable�� entrt�� 1���� �ƴ϶�� ã�� page�� frameTable�� tail�� �ű�ϴ�
		if (frameTable.size != 1) {
			frameTable.tail->next = p;
			p->next = NULL;
			p->prev = frameTable.tail;
			frameTable.tail = p;
		}
		return p;
	}
	// TLB Miss�� ��� pageTable�� �����Ͽ� frameTable�� �ش� page�� �ִ��� Ȯ���ؾ��մϴ�
	else {
		// ù��° demand�� ��� pageFault �߻�
		if (demandCount == 1) {
			struct entry* newEntry = (struct entry*)malloc(sizeof(struct entry));
			// frameTable�� ���ο� entry�� �����մϴ�.
			newEntry->pageNum = pNum;
			newEntry->frameNum = 0;
			newEntry->validation = 1;
			newEntry->next = NULL;
			newEntry->prev = NULL;
			frameTable.head = newEntry;
			frameTable.tail = newEntry;
			frameTable.size++;
			pageFaultCount++;
			// 0��° frame�� page�� �ø��ϴ�
			upToFrame(pNum, 0);
			// pageTable ������Ʈ
			pageTable[pNum][0] = 0;
			pageTable[pNum][1] = true;

			return newEntry;
		}
		else {
			// pageTable�� validation���� false�� pageFault �߻�
			if (pageTable[pNum][1] == false) {
				struct entry* newEntry = (struct entry*)malloc(sizeof(struct entry));
				// frameTable�� ����á�ٸ� (��, Physical Memory ������)
				if (frameTable.size == 128) {
					// victim page�� �����մϴ�.
					struct entry* victim = selectVictim(&frameTable);
					emptyFrameNum = victim->frameNum;
					// victim page�� frame�� �ش� page�� �ø��ϴ�
					upToFrame(pNum, emptyFrameNum);
					// pageTable ������Ʈ
					pageTable[pNum][0] = emptyFrameNum;
					pageTable[pNum][1] = true;
					// pageTable���� victim page�� validation�� false�� �ٲߴϴ�
					pageTable[victim->pageNum][1] = false;
					free(victim);

					// frameTable�� tail�� ���ο� entry�� �����մϴ�
					newEntry->pageNum = pNum;
					newEntry->frameNum = emptyFrameNum;
					newEntry->validation = 1;
					frameTable.tail->next = newEntry;
					newEntry->next = NULL;
					newEntry->prev = frameTable.tail;
					frameTable.tail = newEntry;
				}
				// frameTable�� �������� �ʾҴٸ�
				else {
					// �������� frame�� ���Դϴ�
					newEntry->pageNum = pNum;
					newEntry->frameNum = frameTable.size;
					newEntry->validation = 1;
					// frame�� �ش� page�� �ø��ϴ�
					upToFrame(pNum, frameTable.size);
					// pageTable ������Ʈ
					pageTable[pNum][0] = frameTable.size;
					pageTable[pNum][1] = true;
					// frameTable�� tail�� ���ο� entry�� �����մϴ�
					frameTable.tail->next = newEntry;
					newEntry->prev = frameTable.tail;
					newEntry->next = NULL;
					frameTable.tail = newEntry;
					frameTable.size++;
				}
				// �ռ� TLB�� ������ ��Ȳ���� MISS�� ���
				// ���Ӱ� TLB�� �ø� page�� frame�� �����մϴ�
				TLB.tail->frameNum = newEntry->frameNum;
				pageFaultCount++;
				return newEntry;
			}
			//  pageTable�� validation���� true�� [HIT]
			else {
				// hit !!
				frameTable.hit++;
				struct entry* p;
				// frameTable�� head�� page�� �ִٸ�
				if (frameTable.head->pageNum == pNum) {
					p = frameTable.head;
					if (frameTable.size != 1) {
						frameTable.head = frameTable.head->next;
						frameTable.head->prev = NULL;
					}
				}
				// frameTable�� tail�� page�� �ִٸ�
				else if (frameTable.tail->pageNum == pNum) {
					p = frameTable.tail;
					if (frameTable.size != 1) {
						frameTable.tail = frameTable.tail->prev;
						frameTable.tail->next = NULL;
					}
				}
				// frameTable�� head�� tail ���̿� page�� �ִٸ�
				else {
					// p�� head�� ���� ���� ����
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
				// �ռ� TLB�� ������ ��Ȳ���� MISS�� ���
				// ���Ӱ� TLB�� �ø� page�� frame�� �����մϴ�
				TLB.tail->frameNum = p->frameNum;

				// frameTable�� entrt�� 1���� �ƴ϶�� ã�� page�� frameTable�� tail�� �ű�ϴ�
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
	// demandPaging Ƚ�� ����
	demandCount++;
	// ���� TLB�� Look up
	lookUpTLB(pNum);
	// ù��° demand�� ��� pageFault �߻�
	if (demandCount == 1) {
		struct entry* newEntry = (struct entry*)malloc(sizeof(struct entry));
		// frameTable�� ���ο� entry�� �����մϴ�
		newEntry->pageNum = pNum;
		newEntry->frameNum = 0;
		newEntry->validation = 1;
		newEntry->next = NULL;
		newEntry->prev = NULL;
		frameTable.head = newEntry;
		frameTable.tail = newEntry;
		frameTable.size++;
		pageFaultCount++;
		// 0��° frame�� page�� �ø�
		upToFrame(pNum, 0);
		// pageTable ������Ʈ
		pageTable[pNum][0] = 0;
		pageTable[pNum][1] = true;

		return newEntry;
	}
	else {
		// pageTable�� validation���� false�� pageFault �߻�
		if (pageTable[pNum][1] == false) {
			struct entry* newEntry = (struct entry*)malloc(sizeof(struct entry));
			// frameTable�� ����á�ٸ�
			if (frameTable.size == 128) {
				// victim page�� �����մϴ�.
				struct entry* victim = selectVictim(&frameTable);
				emptyFrameNum = victim->frameNum;
				// victim page�� frame�� �ش� page�� �÷��ش�
				upToFrame(pNum, emptyFrameNum);
				// pageTable ������Ʈ
				pageTable[pNum][0] = emptyFrameNum;
				pageTable[pNum][1] = true;
				// pageTable���� victim page�� validation�� false�� �ٲߴϴ�
				pageTable[victim->pageNum][1] = false;
				free(victim);

				// frameTable�� tail�� ���ο� entry�� �����մϴ�
				newEntry->pageNum = pNum;
				newEntry->frameNum = emptyFrameNum;
				newEntry->validation = 1;
				frameTable.tail->next = newEntry;
				newEntry->next = NULL;
				newEntry->prev = frameTable.tail;
				frameTable.tail = newEntry;
			}
			// frameTable�� �������� �ʾҴٸ�
			else {
				// �������� frame�� ���Դϴ�
				newEntry->pageNum = pNum;
				newEntry->frameNum = frameTable.size;
				newEntry->validation = 1;
				// frame�� �ش� page�� �ø��ϴ�
				upToFrame(pNum, frameTable.size);
				// pageTable ������Ʈ
				pageTable[pNum][0] = frameTable.size;
				pageTable[pNum][1] = true;
				// frameTable�� tail�� ���ο� entry�� �����մϴ�
				frameTable.tail->next = newEntry;
				newEntry->prev = frameTable.tail;
				newEntry->next = NULL;
				frameTable.tail = newEntry;
				frameTable.size++;
			}
			// �ռ� TLB�� ������ ��Ȳ���� MISS�� ���
			// ���Ӱ� TLB�� �ø� page�� frame�� �����մϴ�
			TLB.tail->frameNum = newEntry->frameNum;
			pageFaultCount++;
			return newEntry;
		}
		// pageTable�� validation���� true�� [HIT]
		// FIFO algorithm�� ������ frameTable�� ������Ʈ���� �ʽ��ϴ�
		else {
			// TLB���� �̹� hit�� ��� ����
			if (inTLB == false)
				frameTable.hit++;
			// �ռ� TLB�� ������ ��Ȳ���� MISS�� ���
			// ���Ӱ� TLB�� �ø� page�� frame�� �����մϴ�
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
	// input����
	FILE *input;
	// output����
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
		// ���� 16bit�� ���
		virtualAddress = virtualAddress & ((1 << 16) - 1);
		// offset ���
		offset = virtualAddress & ((1 << 8) - 1);
		// Page Number ���
		pNum = virtualAddress >> 8;
		// Demand Paging
		struct entry* result = LRU_demandPaging(pNum);
		// Translation ����� ���Ϸ� ����մϴ�.
		fprintf(LRU_translation, "Virtual address: %d Physical address: %d\n", (pNum << 8) + offset, (result->frameNum << 8) + offset);
	}
	struct entry* p = frameTable.head;
	// frameTable�� �����͸� frame Number ������ �����մϴ�.
	int frameTemp[128][2] = { 0, };
	while (p != NULL) {
		frameTemp[p->frameNum][0] = p->validation;
		frameTemp[p->frameNum][1] = (p->pageNum << 8);
		p = p->next;
	}
	// frame Table�� �����͸� ���Ϸ� ����մϴ�.
	for (i = 0; i < 128; i++) {
		fprintf(LRU_frame, "%d %d %d\n", i + 1, frameTemp[i][0], frameTemp[i][1]);
	}
	// Physical Memory�� �����͸� ���Ϸ� ����մϴ�.
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

	////////////////////// �ʱ�ȭ ///////////////////////////
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
	////////////////////// �ʱ�ȭ ///////////////////////////

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
