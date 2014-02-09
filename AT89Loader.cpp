/**   AT89Loader v0.2
 *    Copyright (C) 2014 Joey Shepard
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include "ftdi/ftd2xx.h"

#pragma comment(lib, "ftdi/ftd2xx.lib")

using namespace std;

#define PIN_ORANGE	0x01  //CLOCK
#define PIN_YELLOW	0x02	//TX
#define PIN_GREEN		0x04	//RX
#define PIN_BROWN		0x08	//SS

#define PIN_CLOCK		PIN_ORANGE
#define PIN_TX			PIN_YELLOW
#define PIN_RX			PIN_GREEN
#define PIN_SS			PIN_BROWN

bool progInitialize(FT_HANDLE *handle);
bool progShutdown(FT_HANDLE handle);
void progStart(FT_HANDLE handle);
void progStop(FT_HANDLE handle);
bool progSend(FT_HANDLE handle, unsigned char data);
unsigned char progRead(FT_HANDLE handle);
unsigned char progSendOld(FT_HANDLE handle, unsigned char data);
bool progSendHex(FT_HANDLE handle, char *data);
bool progVerifyHex(FT_HANDLE handle, char *data);

bool cmdEnable(FT_HANDLE handle);
void cmdErase(FT_HANDLE handle);
void cmdPollBusy(FT_HANDLE handle);
void cmdWriteCP(FT_HANDLE handle, unsigned int address, unsigned char *data, int data_len);
void cmdReadCP(FT_HANDLE handle, unsigned int address, unsigned char *data, int data_len);

bool LoudMode=false;
DWORD ByteCount;

unsigned char dbuff[32];

int main(int argc, char **argv)
{
	FT_HANDLE handle=NULL;
	FILE *fp=NULL;
	int i,j;
	char hexbuff[200];
	char filename[200];
	char newfuses[13]="xxxxxxxxxxxx";
	filename[0]=0;
	unsigned char fusebuff;

	bool EraseFlash=false;
	bool SetFuses=false;
	bool SilentMode=false;
	bool VerifyFlash=false;
	bool FileFound=false;
	bool ProgramFlash=false;
	bool ShowHelp=false;

	for (i=1;i<argc;i++)
	{
		if (!strcmp(argv[i],"-?")) ShowHelp=true;
		else if (!strcmp(argv[i],"-e")) EraseFlash=true;
		else if (!strncmp(argv[i],"-f",2))
		{
			SetFuses=true;
			if (strlen(argv[i]+2)>12)
			{
				cout<<"Error: too many fuses. 12 or less fuses expected without spaces. Valid values are 1, 0, or x."<<endl;
				cout<<endl;
				return 1;
			}
			for (j=0;strlen(argv[i]+2)>j;j++)
			{
				if (argv[i][j+2]=='1') newfuses[j]='1';
				else if (argv[i][j+2]=='0') newfuses[j]='0';
				else if ((argv[i][j+2]=='x')||(argv[i][j+2]=='X')) newfuses[j]='x';
				else
				{
					cout<<"Error: invalid fuse value: "<<argv[i][j+2]<<". Valid values are 1, 0, or x."<<endl;
					cout<<endl;
					return 1;
				}
			}
		}	
		else if (!strcmp(argv[i],"-l")) {LoudMode=true;SilentMode=false;}
		else if (!strcmp(argv[i],"-p")) ProgramFlash=true;
		else if (!strcmp(argv[i],"-s")) {SilentMode=true;LoudMode=false;}
		else if (!strcmp(argv[i],"-v")) VerifyFlash=true;
		else
		{
			if (FileFound)
			{
				cout<<"Error: unknown or too many arguments."<<endl;
				cout<<"Argument 1: "<<filename<<endl;
				cout<<"Argument 2: "<<argv[i]<<endl;
				cout<<endl;
				return 1;
			}
			else
			{
				FileFound=true;
				strcpy(filename,argv[i]);
			}
		}
	}

	if (ShowHelp||argc==1)
	{
		cout<<"AT89Loader v0.1"<<endl;
		cout<<"Loads HEX files into AT89LP6440s with an FTDI cable"<<endl;
		cout<<endl;
		cout<<"Usage: AT89Loader [options] [filename]"<<endl;
		cout<<"Options:"<<endl;
		cout<<"   -?              See this help page"<<endl;
		cout<<"   -e              Erase all of the flash"<<endl;
		cout<<"   -fxxxxxxxxxxxx  Set fuses. 1, 0, or x for don't change"<<endl;
		cout<<"   -l              Display information while working"<<endl;
		cout<<"   -p              Program flash with filename"<<endl;
		cout<<"   -s              Only display errors"<<endl;
		cout<<"   -v			     Verify flash against filename"<<endl;
		cout<<"   filename        Path of the ihx or hex file to load"<<endl;
		cout<<endl;
		cout<<"Default pin configuration:"<<endl;
		cout<<"MCU SS   <----> FTDI Brown"<<endl;
		cout<<"MCU MOSI <----> FTDI Yellow"<<endl;
		cout<<"MCU MISO <----> FTDI Green"<<endl;
		cout<<"MCU SCK  <----> FTDI Orange"<<endl;
		cout<<endl;
		return 0;
	}

	if (LoudMode)
	{
		cout<<"AT89Loader v0.1"<<endl;
		cout<<"Loads HEX files into AT89LP6440s with an FTDI cable"<<endl;
		cout<<endl;
	}
	
	if (!SilentMode) cout<<"Initializing...";
	if (!progInitialize(&handle))
	{
		FT_Close(handle);
		if (SilentMode) cout<<"Initialzing failed"<<endl;
		else cout<<"Failed"<<endl;
		if (!SilentMode) cout<<"FTDI cable connected?"<<endl;
		return 1;
	}
	else if (!SilentMode) cout<<"Done\n";

	if (!SilentMode) cout<<"Enabling...";
	if (!cmdEnable(handle))
	{
		FT_Close(handle);
		if (SilentMode) cout<<"Enabling failed"<<endl;
		else cout<<"Failed"<<endl;
		if (!SilentMode) cout<<"Cable connected to chip?"<<endl;
		return 1;
	}
	else if (!SilentMode) cout<<"Done\n";

	if (EraseFlash)
	{
		if (!SilentMode) cout<<"Erasing...";
		cmdErase(handle);
		if (!SilentMode) cout<<"Done\n";
	}

	if (SetFuses)
	{
		if (!SilentMode) cout<<"Writing Fuses...";
		if (LoudMode)
		{
			progStart(handle);
			progSend(handle,0xAA);
			progSend(handle,0x55);
			progSend(handle,0x61);
			progSend(handle,0x00);
			progSend(handle,0x00);
			if (LoudMode) cout<<endl<<"Old fuses: ";
			for (i=0;i<12;i++)
			{
				fusebuff=progRead(handle);
				printf("%02X ",fusebuff);
				if ((fusebuff!=0x00)&&(fusebuff!=0xFF))
				{
					FT_Close(handle);
					cout<<"Error: invlaid fuse value read from chip."<<endl;
					cout<<endl;
					return 1;
				}
			}
			progStop(handle);

			cout<<endl<<"New fuses: ";
			for (i=0;i<12;i++)
			{
				if (newfuses[i]=='1') cout<<"FF ";
				else if (newfuses[i]=='0') cout<<"00 ";
				else cout<<"XX ";
			}
			cout<<endl;
		}
		progStart(handle);
		progSend(handle,0xAA);
		progSend(handle,0x55);
		progSend(handle,0x61);
		progSend(handle,0x00);
		progSend(handle,0x00);
		for (i=0;i<12;i++)
		{
			fusebuff=progRead(handle);
			if ((newfuses[i]=='x')&&(fusebuff==0xFF)) newfuses[i]='1';
			else if ((newfuses[i]=='x')&&(fusebuff==0x00)) newfuses[i]='0';
			else if ((fusebuff!=0x00)&&(fusebuff!=0xFF))
			{
				FT_Close(handle);
				cout<<"Error: invlaid fuse value read from chip."<<endl;
				cout<<endl;
				return 1;
			}
		}
		progStop(handle);

		progStart(handle);
		progSend(handle,0xAA);
		progSend(handle,0x55);
		progSend(handle,0xF1);
		progSend(handle,0x00);
		progSend(handle,0x00);
		for (i=0;i<12;i++)
		{
			if (newfuses[i]=='1') progSend(handle,0xFF);
			else progSend(handle,0x00);
		}
		progStop(handle);
		cmdPollBusy(handle);

		progStart(handle);
		progSend(handle,0xAA);
		progSend(handle,0x55);
		progSend(handle,0x61);
		progSend(handle,0x00);
		progSend(handle,0x00);
		
		if (LoudMode) cout<<"Verified:  ";
		for (i=0;i<12;i++)
		{
			fusebuff=progRead(handle);
			if (LoudMode) printf("%02X ",fusebuff);
			if (((fusebuff==0)&&(newfuses[i]!='0'))||((fusebuff==0xFF)&&(newfuses[i]!='1')))
			{
				if (LoudMode) cout<<endl<<"Fuse verification failed"<<endl;
				FT_Close(handle);
				return 1;
			}
		}
		progStop(handle);
		if (LoudMode) cout<<endl;
		else if (!SilentMode) cout<<"Done\n";
	}

	if (ProgramFlash)
	{
		if (!SilentMode) cout<<"Programming...";
		if (LoudMode) cout<<endl;
		fp=fopen(filename,"r");
		if (fp==NULL)
		{
			FT_Close(handle);
			fclose(fp);
			cout<<"File not found: "<<filename<<endl;
			return 1;
		}

		i=0;
		while (!feof(fp))
		{
			hexbuff[i]=fgetc(fp);
			if (hexbuff[i]==10)
			{
				hexbuff[i]=0;
				progSendHex(handle,hexbuff);
				i=0;
			}
			else i++;
		}
		fclose(fp);
		if (LoudMode) cout<<"Programming Done"<<endl<<endl;
		else if (!SilentMode) cout<<"Done"<<endl;
	}
	
	if (VerifyFlash)
	{
		if (!SilentMode) cout<<"Verifying...";
		if (LoudMode) cout<<endl;
		fp=fopen(filename,"r");
		if (fp==NULL)
		{
			FT_Close(handle);
			cout<<"File not found: "<<filename<<endl;
			return 1;
		}
		i=0;
		while (!feof(fp))
		{
			hexbuff[i]=fgetc(fp);
			if (hexbuff[i]==10)
			{
				hexbuff[i]=0;
				if (!progVerifyHex(handle,hexbuff))
				{
					FT_Close(handle);
					fclose(fp);
					if (SilentMode) cout<<"Verification failed"<<endl;
					else if (LoudMode) cout<<endl<<"Failed"<<endl;
					else cout<<"Failed"<<endl;
					return 1;
				}
				i=0;
			}
			else i++;
		}
		if (LoudMode) cout<<"Verifying Done"<<endl<<endl;
		else if (!SilentMode) cout<<"Done"<<endl;
	}
	
	if (!SilentMode) cout<<"Shutting down...";
	progShutdown(handle);
	if (!SilentMode) cout<<"Done\n";
	
	if (fp) fclose(fp);
	return 0;
}

bool progInitialize(FT_HANDLE *handle)
{
	unsigned char outbuff;
	DWORD bytes;

	if (FT_Open(0, handle)!=FT_OK) return false;
	if (FT_ResetDevice(*handle)!=FT_OK) return false;
	if (FT_SetBitMode(*handle, PIN_CLOCK|PIN_TX|PIN_SS, 0x04)!=FT_OK) return false;
	if (FT_SetUSBParameters(*handle,64,0)!=FT_OK) return false;
	if (FT_SetTimeouts(*handle,1000,1000)!=FT_OK) return false;
	if (FT_SetLatencyTimer(*handle,2)!=FT_OK) return false;
	if (FT_SetBaudRate(*handle,3000000)!=FT_OK) return false;
	Sleep(100);
	FT_Purge(*handle,FT_PURGE_RX);
	Sleep(50);

	outbuff=PIN_SS;//start with SS high
	if (FT_Write(*handle,&outbuff,1,&bytes)!=FT_OK) return false;
	FT_Read(*handle,&outbuff,1,&bytes);
	return true;
}

bool progShutdown(FT_HANDLE handle)
{
	unsigned char outbuff;
	DWORD bytes;

	outbuff=0;
	FT_Write(handle,&outbuff,1,&bytes);
	outbuff=PIN_SS;
	FT_Write(handle,&outbuff,1,&bytes);
	ByteCount+=2;
	
	while (ByteCount)
	{
		FT_Read(handle,&outbuff,1,&bytes);
		ByteCount--;
	}

	FT_SetBitMode(handle, PIN_CLOCK|PIN_SS, 1);//tristate MOSI
	FT_SetBitMode(handle, PIN_SS, 1);//tristate CLOCK
	FT_SetBitMode(handle, 0, 1);//tristate SS
	FT_Close(handle);
	return true;
}

void progStart(FT_HANDLE handle)
{
	unsigned char outbuff;
	DWORD bytes;
	outbuff=PIN_SS;//drive clock low first
	FT_Write(handle,&outbuff,1,&bytes);
	outbuff=0;//drive SS low
	FT_Write(handle,&outbuff,1,&bytes);
	ByteCount=2;
}

void progStop(FT_HANDLE handle)
{
	unsigned char outbuff;
	DWORD bytes;
	outbuff=0;//drive clock low first
	FT_Write(handle,&outbuff,1,&bytes);
	outbuff=PIN_SS;//drive SS high
	FT_Write(handle,&outbuff,1,&bytes);
	ByteCount+=2;
	while (ByteCount)
	{
		FT_Read(handle,&outbuff,1,&bytes);
		ByteCount--;
	}
}

bool progSend(FT_HANDLE handle, unsigned char data)
{
	int i;
	unsigned char outbuff[24];
	DWORD bytes;

	for (i=0;i<8;i++)
	{
		if (data&0x80) outbuff[i*3]=PIN_TX;
		else outbuff[i*3]=0;
		data<<=1;
		outbuff[i*3+1]=outbuff[i*3]|PIN_CLOCK;
		outbuff[i*3+2]=outbuff[i*3];
	}

	FT_Write(handle,outbuff,sizeof(outbuff),&bytes);
	ByteCount+=sizeof(outbuff);

	return true;
}

unsigned char progRead(FT_HANDLE handle)
{
	int i;
	unsigned char retbuff=0, outbuff[32]=
		{PIN_CLOCK,PIN_CLOCK,0,0,PIN_CLOCK,PIN_CLOCK,0,0,PIN_CLOCK,PIN_CLOCK,0,0,PIN_CLOCK,PIN_CLOCK,0,0,PIN_CLOCK,PIN_CLOCK,0,0,PIN_CLOCK,PIN_CLOCK,0,0,PIN_CLOCK,PIN_CLOCK,0,0,PIN_CLOCK,PIN_CLOCK,0,0};
	DWORD bytes;
	FT_STATUS status=0;
	
	//Could be more efficient?
	while (ByteCount)
	{
		FT_Read(handle,&outbuff[0],1,&bytes);
		ByteCount--;
	}
	FT_Write(handle,outbuff,sizeof(outbuff),&bytes);
	FT_Read(handle,outbuff,sizeof(outbuff),&bytes);

	memcpy(dbuff,outbuff,32);

	retbuff=0;
	for (i=0;i<sizeof(outbuff);i+=4)
	{
		retbuff<<=1;
		if (outbuff[i]&PIN_RX) retbuff++;
	}
	return retbuff;
}

bool progSendHex(FT_HANDLE handle, char *data)
{
	static int i=0;
	int bytes, datatype, count, j, k;
	unsigned int address;
	unsigned char upload[64];

	i++;
	if (LoudMode) cout<<hex<<uppercase<<i<<(char*)data<<endl;

	j=1;
	while(data[j])
	{
		data[j]-='0';
		if (data[j]>16) data[j]-=7;
		j++;
	}

	bytes=data[1]*16+data[2];
	address=data[3]*4096+data[4]*256+data[5]*16+data[6];
	datatype=data[7]*16+data[8];

	count=address%64;
	k=0;

	for (j=0;j<bytes;j++)
	{
		upload[k]=data[j*2+9]*16+data[j*2+10];
		k++;
		count++;
		if (count==64)
		{
			cmdWriteCP(handle,address,upload,k);
			k=0;
			count=0;
			address=(address/64+1)*64;
		}
	}
	if (k) cmdWriteCP(handle,address,upload,k);
	return true;
}

bool progVerifyHex(FT_HANDLE handle, char *data)
{
	static int i=0;
	int bytes, datatype, j;
	unsigned int address;
	unsigned char download;

	i++;
	if (LoudMode) cout<<hex<<uppercase<<i<<(char*)data<<endl;

	j=1;
	while(data[j])
	{
		data[j]-='0';
		if (data[j]>16) data[j]-=7;
		j++;
	}

	bytes=data[1]*16+data[2];
	address=data[3]*4096+data[4]*256+data[5]*16+data[6];
	datatype=data[7]*16+data[8];

	if (LoudMode)
	{
		cout<<"Data:     ";
		if (i>0xF) cout<<" ";
		if (i>0xFF) cout<<" ";
		if (i>0xFFF) cout<<" ";
	}
	for (j=0;j<bytes;j++)
	{
		cmdReadCP(handle,address+j,&download,1);
		if (LoudMode) printf("%02X",download);
		if (download!=(data[j*2+9]*16+data[j*2+10])) return false;
	}
	if (LoudMode) cout<<endl;
	return true;
}

bool cmdEnable(FT_HANDLE handle)
{
	unsigned char inbuff;
	progStart(handle);
	progSend(handle,0xAA);
	progSend(handle,0x55);
	progSend(handle,0xAC);
	progSend(handle,0x53);
	inbuff=progRead(handle);
	progStop(handle);
	if (inbuff==0x53) return true;
	else return false;
}

void cmdErase(FT_HANDLE handle)
{
	
	progStart(handle);
	progSend(handle,0xAA);
	progSend(handle,0x55);
	progSend(handle,0x8A);
	progStop(handle);

	cmdPollBusy(handle);
}

void cmdPollBusy(FT_HANDLE handle)
{
	unsigned char inbuff=0;
	progStart(handle);
	progSend(handle,0xAA);
	progSend(handle,0x55);
	progSend(handle,0x60);
	progSend(handle,0x00);
	progSend(handle,0x00);
	while ((inbuff&1)!=1) inbuff=progRead(handle);
	progStop(handle);
}

void cmdWriteCP(FT_HANDLE handle, unsigned int address, unsigned char *data, int data_len)
{
	int i;
	progStart(handle);
	progSend(handle,0xAA);
	progSend(handle,0x55);
	progSend(handle,0x50);
	progSend(handle,address>>8);
	progSend(handle,address&0xFF);
	for (i=0;i<data_len;i++) progSend(handle,data[i]);
	progStop(handle);
	cmdPollBusy(handle);
}

void cmdReadCP(FT_HANDLE handle, unsigned int address, unsigned char *data, int data_len)
{
	int i;
	progStart(handle);
	progSend(handle,0xAA);
	progSend(handle,0x55);
	progSend(handle,0x30);
	progSend(handle,address>>8);
	progSend(handle,address&0xFF);
	for (i=0;i<data_len;i++) data[i]=progRead(handle);
	progStop(handle);
}
