/*Realtime Associates (David Warhol) (GB/GBC/GG) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * mid;
long bank;
long offset;
long tablePtrLoc;
long tableOffset;
int i, j;
char outfile[1000000];
int songNum;
long seqPtrs[4];
long songPtr;
int chanMask;
long bankAmt;
int foundTable = 0;
long firstPtr = 0;
int curInst[16];
char* argv3;

int sysMode = 1;

unsigned static char* romData;
unsigned static char* midData[16];
unsigned static char* ctrlMidData;

long midLength;

const char MagicBytes[8] = { 0x78, 0xE6, 0x3F, 0x87, 0x5F, 0x16, 0x00, 0x21 };
const char MagicBytesGG[6] = { 0x3D, 0x6F, 0x26, 0x00, 0x29, 0x11 };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
unsigned int WriteNoteEvent(unsigned static char* buffer, unsigned int pos, unsigned int note, int length, int delay, int firstNote, int curChan, int inst);
int WriteDeltaTime(unsigned static char* buffer, unsigned int pos, unsigned int value);
void song2mid(int songNum, long ptr);

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

static void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

static void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

static void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

static void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}

unsigned int WriteNoteEvent(unsigned static char* buffer, unsigned int pos, unsigned int note, int length, int delay, int firstNote, int curChan, int inst)
{
	int deltaValue;
	deltaValue = WriteDeltaTime(buffer, pos, delay);
	pos += deltaValue;

	if (firstNote == 1)
	{
		Write8B(&buffer[pos], 0xC0 | curChan);

		Write8B(&buffer[pos + 1], inst);
		Write8B(&buffer[pos + 2], 0);

		if (curChan == 10)
		{
			Write8B(&buffer[pos + 3], 0x90 | 16);
		}
		else if (curChan == 16)
		{
			Write8B(&buffer[pos + 3], 0x90 | 10);
		}
		else
		{
			Write8B(&buffer[pos + 3], 0x90 | curChan);
		}



		pos += 4;
	}

	Write8B(&buffer[pos], note);
	pos++;
	Write8B(&buffer[pos], 100);
	pos++;

	deltaValue = WriteDeltaTime(buffer, pos, length);
	pos += deltaValue;

	Write8B(&buffer[pos], note);
	pos++;
	Write8B(&buffer[pos], 0);
	pos++;

	return pos;

}

int WriteDeltaTime(unsigned static char* buffer, unsigned int pos, unsigned int value)
{
	unsigned char valSize;
	unsigned char* valData;
	unsigned int tempLen;
	unsigned int curPos;

	valSize = 0;
	tempLen = value;

	while (tempLen != 0)
	{
		tempLen >>= 7;
		valSize++;
	}

	valData = &buffer[pos];
	curPos = valSize;
	tempLen = value;

	while (tempLen != 0)
	{
		curPos--;
		valData[curPos] = 128 | (tempLen & 127);
		tempLen >>= 7;
	}

	valData[valSize - 1] &= 127;

	pos += valSize;

	if (value == 0)
	{
		valSize = 1;
	}
	return valSize;
}

int main(int args, char* argv[])
{
	printf("Realtime Associates (David Warhol) (GB/GBC/GG) to MIDI converter\n");
	if (args < 3)
	{
		printf("Usage: RTA2MID <rom> <bank> <format>\n");
		return -1;
	}
	else
	{
		if (args == 3)
		{
			sysMode = 1;
		}
		else if (args == 4)
		{
			argv3 = argv[3];
			if (strcmp(argv3, "gb") == 0 || strcmp(argv3, "GB") == 0)
			{
				sysMode = 1;
				printf("Selected format: Game Boy\n");
			}
			else if (strcmp(argv3, "gg") == 0 || strcmp(argv3, "GG") == 0)
			{
				sysMode = 2;
				printf("Selected format: Game Gear\n");
			}
			else
			{
				printf("ERROR: Invalid mode switch!\n");
				exit(1);
			}
		}
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			bank = strtol(argv[2], NULL, 16);
			if (bank != 1)
			{
				if (sysMode == 1)
				{
					bankAmt = bankSize;
				}
				else if (sysMode == 2)
				{
					bankAmt = bankSize * 2;
				}

			}
			else
			{
				bankAmt = 0;
			}
			fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
			romData = (unsigned char*)malloc(bankSize);
			fread(romData, 1, bankSize, rom);
			fclose(rom);

			/*Try to search the bank for song table loader*/
			if (sysMode == 1)
			{
				for (i = 0; i < bankSize; i++)
				{
					if ((!memcmp(&romData[i], MagicBytes, 8)) && foundTable != 1)
					{
						tablePtrLoc = bankAmt + i + 8;
						printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
						tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);
						printf("Song table starts at 0x%04x...\n", tableOffset);
						foundTable = 1;
					}
				}
			}
			else if (sysMode == 2)
			{
				for (i = 0; i < bankSize; i++)
				{
					if ((!memcmp(&romData[i], MagicBytesGG, 6)) && foundTable != 1)
					{
						tablePtrLoc = bankAmt + i + 6;
						printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
						tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);
						printf("Song table starts at 0x%04x...\n", tableOffset);
						foundTable = 1;
					}
				}
			}


			if (foundTable == 1)
			{
				i = tableOffset - bankAmt;
				if (sysMode == 2)
				{
					i += 2;
				}
				songPtr = ReadLE16(&romData[i]);
				if (romData[songPtr - bankAmt] <= 3)
				{
					i += 2;
					songPtr = ReadLE16(&romData[i]);

					/*For Mysterium*/
					if (songPtr == 0x7935)
					{
						i = 0x09D7;
						songPtr = ReadLE16(&romData[i]);
					}
					/*For Skate or Die*/
					if (songPtr == 0x4A1F)
					{
						i = 0x09D1;
						songPtr = ReadLE16(&romData[i]);
					}
					/*For Barbie (bank 2)*/
					if (songPtr == 0x56E8 && bank == 7)
					{
						i = 0x9F6;
						songPtr = ReadLE16(&romData[i]);
					}
				}
				songNum = 1;
				while (romData[songPtr - bankAmt] > 3 && ReadLE16(&romData[i]) > bankAmt)
				{
					songPtr = ReadLE16(&romData[i]);
					if (romData[songPtr - bankAmt] > 3)
					{
						printf("Song %i: 0x%04X\n", songNum, songPtr);
						if (songPtr != 0)
						{
							song2mid(songNum, songPtr);
						}
						i += 2;

						/*Workaround for Barbie empty track*/
						if (songPtr == 0x6D9E)
						{
							i += 2;
						}
						songNum++;
					}

				}
			}
			else
			{
				printf("ERROR: Magic bytes not found!\n");
				exit(-1);
			}

			printf("The operation was completed successfully!\n");
			exit(0);
		}
	}
}

void song2mid(int songNum, long ptr)
{
	long seqPos = 0;
	int curDelay[16];
	unsigned int ctrlMidPos = 0;
	long midTrackBase = 0;
	long ctrlMidTrackBase = 0;
	int noteTotal = 0;
	int curNote[16];
	int curNoteLen[16];
	int rowTime = 0;
	int noteBank[300][2];
	int seqEnd = 0;
	int k = 0;
	int curTrack = 0;
	unsigned int command[2];
	int firstNote[16];
	int midPos[16];
	int trackCnt = 16;
	int ticks = 120;
	int tempo = 180;
	long tempPos = 0;
	int trackSize[16];
	int ctrlTrackSize = 0;
	int onOff[16];
	int activeChan[16];

	midLength = 0x10000;
	for (j = 0; j < 16; j++)
	{
		midData[j] = (unsigned char*)malloc(midLength);
	}

	ctrlMidData = (unsigned char*)malloc(midLength);

	for (j = 0; j < midLength; j++)
	{
		for (k = 0; k < 16; k++)
		{
			midData[k][j] = 0;
		}

		ctrlMidData[j] = 0;
	}

	for (k = 0; k < 16; k++)
	{
		onOff[k] = 0;
	}

	sprintf(outfile, "song%d.mid", songNum);
	if ((mid = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.mid!\n", songNum);
		exit(2);
	}
	else
	{
		sprintf(outfile, "song%d.mid", songNum);
		if ((mid = fopen(outfile, "wb")) == NULL)
		{
			printf("ERROR: Unable to write to file song%d.mid!\n", songNum);
			exit(2);
		}
		else
		{
			/*Write MIDI header with "MThd"*/
			WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D546864);
			WriteBE32(&ctrlMidData[ctrlMidPos + 4], 0x00000006);
			ctrlMidPos += 8;

			WriteBE16(&ctrlMidData[ctrlMidPos], 0x0001);
			WriteBE16(&ctrlMidData[ctrlMidPos + 2], trackCnt + 1);
			WriteBE16(&ctrlMidData[ctrlMidPos + 4], ticks);
			ctrlMidPos += 6;
			/*Write initial MIDI information for "control" track*/
			WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D54726B);
			ctrlMidPos += 8;
			ctrlMidTrackBase = ctrlMidPos;

			/*Set channel name (blank)*/
			WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
			ctrlMidPos++;
			WriteBE16(&ctrlMidData[ctrlMidPos], 0xFF03);
			Write8B(&ctrlMidData[ctrlMidPos + 2], 0);
			ctrlMidPos += 2;

			/*Set initial tempo*/
			WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
			ctrlMidPos++;
			WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF5103);
			ctrlMidPos += 4;

			WriteBE24(&ctrlMidData[ctrlMidPos], 60000000 / tempo);
			ctrlMidPos += 3;

			/*Set time signature*/
			WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
			ctrlMidPos++;
			WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5804);
			ctrlMidPos += 3;
			WriteBE32(&ctrlMidData[ctrlMidPos], 0x04021808);
			ctrlMidPos += 4;

			/*Set key signature*/
			WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
			ctrlMidPos++;
			WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5902);
			ctrlMidPos += 4;

			/*Get the note frequency values set*/
			seqPos = ptr - bankAmt;
			noteTotal = romData[seqPos];
			seqPos++;

			/*Get the mapping for each channel*/
			for (k = 0; k < noteTotal; k++)
			{
				noteBank[k][1] = romData[seqPos];
				activeChan[romData[seqPos]] = 1;
				seqPos++;
			}

			/*Get the note values themselves*/
			for (k = 0; k < noteTotal; k++)
			{
				noteBank[k][0] = romData[seqPos];
				seqPos++;
			}

			for (k = 0; k < 16; k++)
			{
				curInst[k] = 0;
			}

			for (curTrack = 0; curTrack < trackCnt; curTrack++)
			{
				midPos[curTrack] = 0;
				firstNote[curTrack] = 1;
				/*Write MIDI chunk header with "MTrk"*/
				WriteBE32(&midData[curTrack][midPos[curTrack]], 0x4D54726B);
				midPos[curTrack] += 8;
				curNoteLen[curTrack] = 0;
				curDelay[curTrack] = 0;
				seqEnd = 0;
				midTrackBase = midPos[curTrack];

				/*Calculate MIDI channel size*/
				trackSize[curTrack] = midPos[curTrack] - midTrackBase;
				WriteBE16(&midData[curTrack][midTrackBase - 2], trackSize[curTrack]);
			}

			/*Convert the sequence data*/
			while (seqEnd == 0)
			{
				command[0] = romData[seqPos];
				command[1] = romData[seqPos + 1];

				/*Play note*/
				if (command[0] < noteTotal)
				{
					curTrack = noteBank[command[0]][1];
					if (onOff[curTrack] == 1)
					{
						tempPos = WriteNoteEvent(midData[curTrack], midPos[curTrack], curNote[curTrack], curNoteLen[curTrack], curDelay[curTrack], firstNote[curTrack], curTrack, curInst[curTrack]);
						firstNote[curTrack] = 0;
						midPos[curTrack] = tempPos;
						curDelay[curTrack] = 0;
						onOff[curTrack] = 0;
					}
					else
					{
						curNote[curTrack] = noteBank[command[0]][0] + 21;
						onOff[curTrack] = 1;
						curNoteLen[curTrack] = 0;
						seqPos++;
					}

				}

				/*Note off/rest*/
				else if (command[0] >= (noteTotal) && command[0] < (noteTotal + 16))
				{
					curTrack = command[0] - noteTotal;
					if (onOff[curTrack] == 1)
					{
						tempPos = WriteNoteEvent(midData[curTrack], midPos[curTrack], curNote[curTrack], curNoteLen[curTrack], curDelay[curTrack], firstNote[curTrack], curTrack, curInst[curTrack]);
						firstNote[curTrack] = 0;
						midPos[curTrack] = tempPos;
						curDelay[curTrack] = 0;
						onOff[curTrack] = 0;
					}
					seqPos++;
				}

				/*Change instrument*/
				else if (command[0] >= (noteTotal + 16) && command[0] < (noteTotal + 32))
				{
					
					curTrack = command[0] - noteTotal - 16;
					if (curInst[curTrack] != command[1])
					{
						curInst[curTrack] = command[1];
						firstNote[curTrack] = 1;

					}

					seqPos += 2;
				}

				/*Row time*/
				else if (command[0] >= (noteTotal + 32) && command[0] < 0xFE)
				{
					rowTime = command[0] - (noteTotal + 32);
					for (curTrack = 0; curTrack < trackCnt; curTrack++)
					{
						if (activeChan[curTrack] == 1)
						{
							if (onOff[curTrack] == 1)
							{
								curNoteLen[curTrack] += rowTime * 6;
							}
							else
							{
								curDelay[curTrack] += rowTime * 6;
							}
						}
					}
					seqPos++;
				}
				else if (command[0] == 0xFE)
				{
					for (curTrack == 0; curTrack < trackCnt; curTrack++)
					{
						if (onOff[curTrack] == 1)
						{
							tempPos = WriteNoteEvent(midData[curTrack], midPos[curTrack], curNote[curTrack], curNoteLen[curTrack], curDelay[curTrack], firstNote[curTrack], curTrack, curInst[curTrack]);
							firstNote[curTrack] = 0;
							midPos[curTrack] = tempPos;
							curDelay[curTrack] = 0;
							onOff[curTrack] = 0;
						}
					}
					seqEnd = 1;
				}
				else if (command[0] == 0xFF)
				{
					for (curTrack == 0; curTrack < trackCnt; curTrack++)
					{
						if (onOff[curTrack] == 1)
						{
							tempPos = WriteNoteEvent(midData[curTrack], midPos[curTrack], curNote[curTrack], curNoteLen[curTrack], curDelay[curTrack], firstNote[curTrack], curTrack, curInst[curTrack]);
							firstNote[curTrack] = 0;
							midPos[curTrack] = tempPos;
							curDelay[curTrack] = 0;
							onOff[curTrack] = 0;
						}
					}
					seqEnd = 1;
				}
			}

			/*End of track*/
			for (curTrack = 0; curTrack < trackCnt; curTrack++)
			{
				/*WriteDeltaTime(midData[curTrack], midPos[curTrack], 0);
				midPos[curTrack]++;*/
				WriteBE32(&midData[curTrack][midPos[curTrack]], 0xFF2F00);
				midPos[curTrack] += 4;
				firstNote[curTrack] = 0;

				/*Calculate MIDI channel size*/
				trackSize[curTrack] = midPos[curTrack] - midTrackBase;
				WriteBE16(&midData[curTrack][midTrackBase - 2], trackSize[curTrack]);
			}

			/*End of control track*/
			ctrlMidPos++;
			WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF2F00);
			ctrlMidPos += 4;

			/*Calculate MIDI channel size*/
			ctrlTrackSize = ctrlMidPos - ctrlMidTrackBase;
			WriteBE16(&ctrlMidData[ctrlMidTrackBase - 2], ctrlTrackSize);

			sprintf(outfile, "song%d.mid", songNum);
			fwrite(ctrlMidData, ctrlMidPos, 1, mid);
			for (curTrack = 0; curTrack < trackCnt; curTrack++)
			{
				fwrite(midData[curTrack], midPos[curTrack], 1, mid);
			}
			fclose(mid);
		}
	}
}