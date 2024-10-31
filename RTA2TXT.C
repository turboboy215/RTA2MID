/*Realtime Associates (David Warhol) (GB/GBC/GG) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * txt;
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

int sysMode = 1;

char* argv3;

unsigned static char* romData;

const char MagicBytes[8] = { 0x78, 0xE6, 0x3F, 0x87, 0x5F, 0x16, 0x00, 0x21 };
const char MagicBytesGG[6] = { 0x3D, 0x6F, 0x26, 0x00, 0x29, 0x11 };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
void song2txt(int songNum, long ptr);

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

int main(int args, char* argv[])
{
	printf("Realtime Associates (David Warhol) (GB/GBC/GG) to TXT converter\n");
	if (args < 3)
	{
		printf("Usage: RTA2TXT <rom> <bank> <format>\n");
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
							song2txt(songNum, songPtr);
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

void song2txt(int songNum, long ptr)
{
	long seqPos = 0;
	long c1Pos = 0;
	long c2Pos = 0;
	long c3Pos = 0;
	long c4Pos = 0;
	int noteTotal = 0;
	int curNote = 0;
	int curInst = 0;
	int rowTime = 0;
	int noteBank[300][2];
	int seqEnd = 0;
	int k = 0;
	int curTrack = 0;
	unsigned int command[2];
	sprintf(outfile, "song%i.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%i.txt!\n", songNum);
		exit(2);
	}
	else
	{
		seqPos = ptr - bankAmt;
		noteTotal = romData[seqPos];
		fprintf(txt, "Number of unique note values: %01X\n\n", noteTotal);

		seqPos++;
		for (k = 0; k < noteTotal; k++)
		{
			noteBank[k][1] = romData[seqPos] + 1;
			seqPos++;
		}

		for (k = 0; k < noteTotal; k++)
		{
			noteBank[k][0] = romData[seqPos];
			seqPos++;
		}

		/*Convert the sequence data*/
		seqEnd = 0;
		while (seqEnd == 0)
		{
			command[0] = romData[seqPos];
			command[1] = romData[seqPos + 1];

			if (command[0] < noteTotal)
			{
				fprintf(txt, "Play note on channel %i: %01X\n", noteBank[command[0]][1], noteBank[command[0]][0]);
				seqPos++;
			}
			else if (command[0] >= (noteTotal) && command[0] < (noteTotal + 16))
			{
				curTrack = command[0] - noteTotal + 1;
				fprintf(txt, "Rest: Channel %i\n", curTrack);
				seqPos++;
			}
			else if (command[0] >= (noteTotal + 16) && command[0] < (noteTotal + 32))
			{
				curTrack = command[0] - noteTotal - 15;
				curInst = command[1];
				fprintf(txt, "Change channel %i instrument: %01X\n", curTrack, curInst);
				seqPos += 2;
			}
			else if (command[0] >= (noteTotal + 32) && command[0] < 0xFE)
			{
				rowTime = command[0] - (noteTotal + 32);
				fprintf(txt, "Row time: %i\n\n", rowTime);
				seqPos++;
			}
			else if (command[0] == 0xFE)
			{
				fprintf(txt, "End of song with loop\n");
				seqEnd = 1;
			}
			else if (command[0] == 0xFF)
			{
				fprintf(txt, "End of song without loop\n");
				seqEnd = 1;
			}
		}
		fclose(txt);
	}
}