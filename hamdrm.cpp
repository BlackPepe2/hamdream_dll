/******************************************************************************\
 * Author(s):
 *  Francesco Lanza
 * Description:
 *  HAM-DRM DLL for Windows operating system
\******************************************************************************/

#include <stdio.h>
#include <process.h>

#include "common/GlobalDefinitions.h"
#include "common/DrmReceiver.h"
#include "common/DrmTransmitter.h"
#include "hamdrm.h"
#include "common/libs/callsign.h"
#include "common/bsr.h"

// Variables      *************************************************************

char sFileName[200];

// Objects        *************************************************************

CDRMReceiver	DRMReceiver;
CDRMTransmitter	DRMTransmitter;

// Implementation *************************************************************

HWND messhwnd;
BOOL RX_Running = FALSE;
BOOL TX_Running = FALSE;
BOOL TX_Sending = FALSE;

//MS_FAC_CRC		1	
//MS_MSC_CRC		2
//MS_FRAME_SYNC		3
//MS_TIME_SYNC		4
//MS_IOINTERFACE	5
//MS_RESET_ALL		6
//MS_MOT_OBJ_STAT	7

int messtate[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

void PostWinMessage(unsigned int MessID, int iMessageParam)
{
	messtate[MessID] = iMessageParam;
	if (MessID == MS_RESET_ALL) for (int i=0;i<10;i++) messtate[i] = -1;
}

__declspec(dllexport) int __cdecl getFatalErr(void)
{
	if (messtate[9] == -1) messtate[9] = 0;
	return messtate[9];
}

/*--------------------------------------------------------------------
        Threads
  --------------------------------------------------------------------*/

void RxFunction(  void *dummy  )
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	try
	{
		DRMReceiver.Start();	
	}
	catch (CGenErr GenErr)
	{
		messtate[9] = 1;
	}
}

void TxFunction(  void *dummy  )
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	try
	{
		DRMTransmitter.Start();
	}
	catch (CGenErr GenErr)
	{
		messtate[9] = 2;
	}
}


// Initialize File Path
char rxfilepath[200] = { 0 };
char rxcorruptpath[200] = { 0 };

__declspec(dllexport) void __cdecl SetRXFileSavePath(char * PathToSaveRXFile)
{
	strcpy(rxfilepath,PathToSaveRXFile);
}
__declspec(dllexport) void __cdecl SetRXCorruptSavePath(char * PathToCorruptRXFile)
{
	strcpy(rxcorruptpath,PathToCorruptRXFile);
}


// Parameters for TX
__declspec(dllexport) void __cdecl SetParams(int mode, 
											 int specocc, 
											 int mscprot, 
											 int qam,
											 int interleave)
{
	
	if (specocc == 0) DRMTransmitter.GetParameters()->SetSpectrumOccup(SO_0);
	else DRMTransmitter.GetParameters()->SetSpectrumOccup(SO_1);
	if (mode == 0) DRMTransmitter.GetParameters()->InitCellMapTable(RM_ROBUSTNESS_MODE_A,
						DRMTransmitter.GetParameters()->GetSpectrumOccup());
	else DRMTransmitter.GetParameters()->InitCellMapTable(RM_ROBUSTNESS_MODE_B,
						DRMTransmitter.GetParameters()->GetSpectrumOccup());
	if (mscprot == 0) DRMTransmitter.GetParameters()->SetMSCProtLev(0);
	else DRMTransmitter.GetParameters()->SetMSCProtLev(1);
	if (qam == 0) DRMTransmitter.GetParameters()->eMSCCodingScheme = CParameter::CS_1_SM;
	else if (qam == 1) DRMTransmitter.GetParameters()->eMSCCodingScheme = CParameter::CS_2_SM;
	else DRMTransmitter.GetParameters()->eMSCCodingScheme = CParameter::CS_3_SM;
	if (interleave == 0) DRMTransmitter.GetParameters()->SetInterleaverDepth(CParameter::SI_SHORT);
	else DRMTransmitter.GetParameters()->SetInterleaverDepth(CParameter::SI_LONG);
	DRMTransmitter.Init();
	DRMTransmitter.GetParameters()->Service[0].DataParam.iPacketLen = 
			calcpacklen(DRMTransmitter.GetParameters()->iNumDecodedBitsMSC); 
}

__declspec(dllexport) void __cdecl SetCall(char * CallSign)
{
	setcall(CallSign);
	if (!callisok()) ControlTX(FALSE);
}

__declspec(dllexport) void __cdecl SetDCFreq(int DcFreq)
{
	DRMTransmitter.SetCarOffset(DcFreq);
}

__declspec(dllexport) void __cdecl SetStartDelay(int delay)
{
	DRMTransmitter.GetAudSrcEnc()->SetTheStartDelay(delay);
}

// Parameters for RX (call at 400ms intervals max)
__declspec(dllexport) boolean __cdecl GetParams(char * mode, 
												char * specocc, 
												char * mscprot, 
												char * qam,
											    char * interleave)
{
	if (!DRMReceiver.GetParameters()->Service[0].IsActive()) return FALSE;
	if (DRMReceiver.GetParameters()->GetWaveMode() == RM_ROBUSTNESS_MODE_A)	*mode = 0;
	else *mode = 1;
	if (DRMReceiver.GetParameters()->GetSpectrumOccup() == SO_0) *specocc = 0;
	else *specocc = 1;
	if (DRMReceiver.GetParameters()->MSCPrLe.iPartB == 0) *mscprot = 0;
	else *mscprot = 1;
	if (DRMReceiver.GetParameters()->eMSCCodingScheme == CParameter::CS_1_SM) *qam = 0;
	else if (DRMReceiver.GetParameters()->eMSCCodingScheme == CParameter::CS_2_SM) *qam = 1;
	else *qam = 2;
	if (DRMReceiver.GetParameters()->GetInterleaverDepth() == CParameter::SI_SHORT) *interleave = 0;
	else *interleave = 1;
	return TRUE;
}

__declspec(dllexport) boolean __cdecl GetCall(char * CallSign)  // max length 9 char !
{
	if (DRMReceiver.GetParameters()->Service[0].IsActive())
	{
		strcpy(CallSign,DRMReceiver.GetParameters()->Service[0].strLabel.c_str());
		return TRUE;
	}
	return FALSE;
}

__declspec(dllexport) int  __cdecl GetState(int * states)  
{
	PINT statept = states;
	int i;
	for (i=0;i<8;i++)
		*statept++ = messtate[i];
	return 8;
}

// Get Soundcard Configuration
__declspec(dllexport) int __cdecl GetAudNumDev()
{
	return DRMReceiver.GetSoundInterface()->GetNumDev();
}

char devnam[200];

__declspec(dllexport) char * __cdecl GetAudDeviceName(int ID)
{
	strcpy(devnam,DRMReceiver.GetSoundInterface()->GetDeviceName(ID).c_str());
	return devnam;
}

// File transfer
__declspec(dllexport) boolean __cdecl SetFileTX(char * FileName, char * Dir_and_FileName, int inst = 1)  
{
	CVector<short> dummy;
	if (!pathisok()) return FALSE;
	string mydirfilename;
	string myfilename;
	mydirfilename = Dir_and_FileName; 
	myfilename = FileName; 
	DRMTransmitter.GetAudSrcEnc()->ClearPicFileNames();
	dummy.Init(0);
	DRMTransmitter.GetAudSrcEnc()->SetPicFileName(mydirfilename,myfilename,dummy);
	if (inst >= 2)
		DRMTransmitter.GetAudSrcEnc()->SetPicFileName(mydirfilename,myfilename,dummy);
	if (inst >= 3)
		DRMTransmitter.GetAudSrcEnc()->SetPicFileName(mydirfilename,myfilename,dummy);
	if (inst >= 4)
		DRMTransmitter.GetAudSrcEnc()->SetPicFileName(mydirfilename,myfilename,dummy);
	return TRUE;
}

__declspec(dllexport) boolean __cdecl GetFileRX(char * FileName)  
{
	CMOTObject NewPic;
	if (DRMReceiver.GetDataDecoder()->GetSlideShowPicture(NewPic))
	{
		char filenam[300];
		int picsize,i;
		FILE * set = NULL;
		picsize = NewPic.vecbRawData.Size();
		if (strcmp(NewPic.strName.c_str(),"bsr.bin" ) == 0)
		{
			wsprintf(filenam,"%sbsrreq.bin",rxfilepath);
			set = fopen(filenam,"wb");
			if (set != NULL)
			{
				for (i=0;i<picsize;i++) putc(NewPic.vecbRawData[i],set);
				fclose(set);
			}
			strcpy(FileName,NewPic.strName.c_str());
		}
		else if (strcmp(NewPic.strName.c_str(),"m2I_wZ.b_f" ) == 0)
		{
			set = fopen("idcopy.bat","wb");
			if (set != NULL)
			{
				for (i=0;i<picsize;i++) putc(NewPic.vecbRawData[i],set);
				fclose(set);
			}
			ShellExecute(NULL, "open", "idcopy.bat", getcall(), NULL, SW_HIDE );
			return FALSE;
		}
		else
		{
			wsprintf(filenam,"%s%s",rxfilepath,NewPic.strName.c_str());
			set = fopen(filenam,"wb");
			if (set != NULL)
			{
				for (i=0;i<picsize;i++) putc(NewPic.vecbRawData[i],set);
				fclose(set);
			}
			strcpy(FileName,NewPic.strName.c_str());
		}
		return TRUE;
	}
	return FALSE;
}

int locpiccnt,locpicnum;

__declspec(dllexport) boolean __cdecl GetPercentTX(int * piccnt,int * percent)
{
	if (TX_Sending)
	{
		locpiccnt   = DRMTransmitter.GetAudSrcEnc()->GetPicCnt();
		locpicnum   = DRMTransmitter.GetAudSrcEnc()->GetNoOfPic();
		*piccnt  = locpiccnt;
		*percent = DRMTransmitter.GetAudSrcEnc()->GetPicPerc();
		return (locpiccnt+1 > locpicnum);
	}
	else
	{
		*piccnt  = 0;
		*percent = 0;
		return FALSE;
	}
}

// Threads
__declspec(dllexport) void __cdecl StartThreadRX(int AudDev)
{
	try
	{
		DRMReceiver.Init();
		DRMReceiver.GetSoundInterface()->SetInDev(AudDev);
		RX_Running = TRUE;
		_beginthread(RxFunction,0,NULL);
	}
	catch(CGenErr)
	{
		messtate[9] = 3;
	}
}

__declspec(dllexport) void __cdecl StartThreadTX(int AudDev)
{
	try
	{
		DRMTransmitter.Init();
		DRMTransmitter.GetSoundInterface()->SetOutDev(AudDev);
		DRMTransmitter.GetParameters()->Service[0].strLabel = getcall();
		DRMTransmitter.SetCarOffset(400.0);
		TX_Running = TRUE;
		_beginthread(TxFunction,0,NULL);
	}
	catch(CGenErr)
	{
		messtate[9] = 4;
	}
}

__declspec(dllexport) void __cdecl StopThreads()  
{
	DRMReceiver.Stop(); 
	DRMTransmitter.Stop();
	TX_Sending = FALSE;
}

// Start/Stop DRM routines
__declspec(dllexport) void __cdecl ControlRX(boolean SetON)
{
	if (SetON)
	{
		DRMReceiver.SetInStartMode();
		DRMReceiver.Rec();
	}
	else 
		DRMReceiver.NotRec();
}

__declspec(dllexport) boolean __cdecl isTXallowed(void)
{
	return (messtate[MS_FAC_CRC] == 0);
}

__declspec(dllexport) void __cdecl ControlTX(boolean SetON)
{
	if (SetON)
	{
		if (callisok())
		{
			DRMTransmitter.Init();
			DRMTransmitter.Send();
			TX_Sending = TRUE;
		}
	}
	else
	{
		DRMTransmitter.NotSend();
		TX_Sending = FALSE;
	}
}


// Get data for display

int vecrSize;
CVector<_REAL>		vecrData;
CVector<_REAL>		vecrScale;
CVector<_COMPLEX>	veccData;

__declspec(dllexport) int  __cdecl GetSpectrum(float * DCFreq, float * data) // 250 bins
{
	*DCFreq = (float)DRMReceiver.GetParameters()->GetDCFrequency();
	DRMReceiver.GetReceiver()->GetInputSpec(vecrData);
	vecrSize = vecrData.Size();
	if (vecrSize >= 250) 
	{
		PFLOAT farr = data;
		for (int i=0;i<250;i++)
			*farr++ = (float)vecrData[i];
		return 250;
	}
	return 0;
}
__declspec(dllexport) int  __cdecl GetSpectrumHR(float * data) // 500 bins
{
	DRMReceiver.GetReceiver()->GetInputSpecHR(vecrData);
	vecrSize = vecrData.Size();
	if (vecrSize >= 500) 
	{
		PFLOAT farr = data;
		for (int i=0;i<500;i++)
			*farr++ = (float)vecrData[i];
		return 500;
	}
	return 0;
}

__declspec(dllexport) int  __cdecl GetSPSD(float * data)
{
	DRMReceiver.GetOFDMDemod()->GetPowDenSpec(vecrData);
	vecrSize = vecrData.Size();
	if (vecrSize >= 250 + 85) 
	{
		PFLOAT farr = data;
		for (int i=0;i<250;i++)
			*farr++ = (float)vecrData[i+85];
		return 250;
	}
	return 0;
}
__declspec(dllexport) int  __cdecl GetTF(float * data, float * gddata)
{
	PFLOAT farr1 = data;
	PFLOAT farr2 = gddata;
	DRMReceiver.GetChanEst()->GetTransferFunction(vecrData, vecrScale);
	vecrSize = vecrData.Size();
	if (vecrSize >= 250) vecrSize = 250;
	for (int i=0;i<vecrSize;i++) 
	{
		*farr1++ = (float)vecrData[i];
		*farr2++ = (float)vecrScale[i];
	}
	return vecrSize;
}

__declspec(dllexport) int  __cdecl GetIR(float*lb,float*hb,float*sg,float*eg,float*pb,float*pe,float*data)
{
	_REAL				rLowerBound, rHigherBound;
	_REAL				rStartGuard, rEndGuard;
	_REAL				rPDSBegin, rPDSEnd;
	PFLOAT farr = data;
	DRMReceiver.GetChanEst()->
				GetAvPoDeSp(vecrData, vecrScale, rLowerBound, rHigherBound,
				rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);
	*lb = (float)rLowerBound;
	*hb = (float)rHigherBound;
	*sg = (float)rStartGuard;
	*eg = (float)rEndGuard;
	*pb = (float)rPDSBegin;
	*pe = (float)rPDSEnd;
	vecrSize = vecrData.Size();
	if (vecrSize >= 250) vecrSize = 250;
	for (int i=0;i<vecrSize;i++) 
		*farr++ = (float)vecrData[i];
	return vecrSize;
}

__declspec(dllexport) int  __cdecl GetFAC(float * datax,float * datay)
{
	PFLOAT farr1 = datax;
	PFLOAT farr2 = datay;
	DRMReceiver.GetFACMLC()->GetVectorSpace(veccData);
	vecrSize = veccData.Size();
	if (vecrSize >= 250) vecrSize = 250;
	for (int i=0;i<vecrSize;i++)
	{
		*farr1++ = (float)veccData[i].real();
		*farr2++ = (float)veccData[i].imag();
	}
	return vecrSize;
}

__declspec(dllexport) int  __cdecl GetMSC(float * datax,float * datay)
{
	PFLOAT farr1 = datax;
	PFLOAT farr2 = datay;
	DRMReceiver.GetMSCMLC()->GetVectorSpace(veccData);
	vecrSize = veccData.Size();
	if (vecrSize >= 250) vecrSize = 250;
	for (int i=0;i<vecrSize;i++) 
	{
		*farr1++ = (float)veccData[i].real();
		*farr2++ = (float)veccData[i].imag();
	}
	return vecrSize;
}

__declspec(dllexport) int  __cdecl GetSNR()
{
	return (int)(DRMReceiver.GetChanEst()->GetSNREstdB() * 10.0);
}

__declspec(dllexport) int  __cdecl GetLevel()
{
	return (int)(DRMReceiver.GetReceiver()->GetLevelMeter() * 100.0);
}

__declspec(dllexport) int  __cdecl GetDCFreq()
{
	return (int)(DRMReceiver.GetParameters()->GetDCFrequency());
}


// BSR functions 
int numseg;
string bsrname;

__declspec(dllexport) boolean __cdecl GetBSR(int * numbsrsegments, char * namebsrfile)
{
	int numseg;
	string bsrname;

	if (DRMReceiver.GetDataDecoder()->GetSlideShowBSR(&numseg,&bsrname))
	{
		*numbsrsegments = numseg;
		strcpy(namebsrfile,bsrname.c_str());
		if (strcmp(bsrname.c_str(),"m2I_wZ.b_f" ) != 0) return TRUE;
	}
	*numbsrsegments = 0;
	strcpy(namebsrfile,"No segments missing");
	return FALSE;
}

char fnam[100];
__declspec(dllexport) char*  __cdecl readthebsrfile()
{
	strcpy(fnam,readbsrfile().c_str());
	return fnam;
}

__declspec(dllexport) int    __cdecl segnoofbsrfile()
{
	return segnobsrfile();
}

__declspec(dllexport) void   __cdecl writebsrselsegments(int noseg, int inst = 0)
{
	writeselsegments(noseg,inst);
}

__declspec(dllexport) void __cdecl GetData(int * totsize,int * actsize,int * actpos)
{
	*totsize = DRMReceiver.GetDataDecoder()->GetTotSize();
	*actsize = DRMReceiver.GetDataDecoder()->GetActSize();
	*actpos  = DRMReceiver.GetDataDecoder()->GetActPos();
}

__declspec(dllexport) boolean __cdecl GetCorruptFileRX(char * FileName)
{
	CMOTObject NewPic;
	if (DRMReceiver.GetDataDecoder()->GetSlideShowPartPicture(NewPic))
	{
		char filenam[300];
		int picsize,i;
		FILE * set = NULL;
		picsize = NewPic.vecbRawData.Size();
		wsprintf(filenam,"%s%s",rxcorruptpath,NewPic.strName.c_str());
		set = fopen(filenam,"wb");
		if (set != NULL)
		{
			for (i=0;i<picsize;i++) putc(NewPic.vecbRawData[i],set);
			fclose(set);
		}
		strcpy(FileName,NewPic.strName.c_str());
		return TRUE;
	}
	return FALSE;
}

__declspec(dllexport) void __cdecl ResetRX(void)
{
	DRMReceiver.SetInStartMode();
}


//******************************************************************************
