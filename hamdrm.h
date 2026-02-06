/******************************************************************************\
 * Author(s):
 *  Francesco Lanza
 * Description:
 *  DLL spec HAM-DRM for Windows operating system
\******************************************************************************/

#include <windows.h>


#define mode_A 0
#define mode_B 1

#define specocc_23 0
#define specocc_25 1

#define mscprot_norm 0
#define mscprot_low 1

#define qam_4  0
#define qam_16 1
#define qam_64 2

#define interleave_short 0
#define interleave_long 1


//********************************************************************


	// If you get something different than 0, something went the wrong way
	// 1 -> Receiver thread dead on Fatal
	// 2 -> Transmitter thread dead on Fatal
	// 3 -> Receiver thread startup failed
	// 4 -> Transmitter thread startup failed
	__declspec(dllexport) int __cdecl getFatalErr(void);

	// Initialize File Path (max 200 char)
	__declspec(dllexport) void __cdecl SetRXFileSavePath(char * PathToSaveRXFile);
	__declspec(dllexport) void __cdecl SetRXCorruptSavePath(char * PathToCorruptRXFile);
  
	// Parameters for TX
	__declspec(dllexport) void __cdecl SetParams(int mode, 
												 int specocc, 
												 int mscprot, 
												 int qam,
												 int interleave);
	__declspec(dllexport) void __cdecl SetCall(char * CallSign);  // max length 9 char !
	__declspec(dllexport) void __cdecl SetDCFreq(int DcFreq);
	__declspec(dllexport) void __cdecl SetStartDelay(int delay);  // def value 12, long-lead-in 24

	// Parameters for RX (call at 400ms intervals max)
	__declspec(dllexport) boolean __cdecl GetParams(char * mode, 
													char * specocc, 
													char * mscprot, 
													char * qam,
											        char * interleave);
	__declspec(dllexport) boolean __cdecl GetCall(char * CallSign);  // max length 9 char !


	// Get Soundcard Configuration
	__declspec(dllexport) int __cdecl GetAudNumDev();
	__declspec(dllexport) char * __cdecl GetAudDeviceName(int ID);

	// File transfer 
	__declspec(dllexport) boolean __cdecl SetFileTX(char * FileName, char * Dir_and_FileName, int inst);  
		// 1 to 4 allowed for instance parameter
	__declspec(dllexport) boolean __cdecl GetFileRX(char * FileName);  
	__declspec(dllexport) boolean __cdecl GetCorruptFileRX(char * FileName);  
	__declspec(dllexport) boolean __cdecl GetPercentTX(int * piccnt,int * percent); 
	
	// BSR Routines
	// Send BSR Request
	__declspec(dllexport) boolean __cdecl GetBSR(int * numbsrsegments, char * namebsrfile); 
		//   use SetFileTX("bsr.bin","bsr.bin") to send request
		//   numbsrsegments -> number of segments missing, namebsrfile -> name of file (if known) 
	// Answer BSR Request
	__declspec(dllexport) char*  __cdecl readthebsrfile();					
		// reads file bsrreq.bin, returns filename
	__declspec(dllexport) int    __cdecl segnoofbsrfile();					
		// returns number of missing segments in bsr request
	__declspec(dllexport) void   __cdecl writebsrselsegments(int noseg, int inst);	
		// writes selected segments to tx buffer, start tx after this.
		// 1 to 4 allowed for instance parameter
		// auto sends 2 instance if (noseg < 50), 3 if (noseg < 10), 4 if (noseg < 3)

	// Threads. only start once.
	__declspec(dllexport) void __cdecl StartThreadRX(int AudDev);
	__declspec(dllexport) void __cdecl StartThreadTX(int AudDev);
	__declspec(dllexport) void __cdecl StopThreads();  

	// Start/Stop DRM routines
	__declspec(dllexport) boolean __cdecl isTXallowed(void);
	__declspec(dllexport) void	  __cdecl ControlTX(boolean SetON);
	__declspec(dllexport) void	  __cdecl ControlRX(boolean SetON);
	__declspec(dllexport) void    __cdecl ResetRX(void);

	// Get data for display (array length 250 elements, call at 400ms intervals max except Spectrum 100ms)
	__declspec(dllexport) int  __cdecl GetSpectrum(float * DCFreq, float * data);	// 250 bins
	__declspec(dllexport) int  __cdecl GetSpectrumHR(float * data);					// 500 bins
	__declspec(dllexport) int  __cdecl GetSPSD(float * data);
	__declspec(dllexport) int  __cdecl GetTF(float * data, float * gddata);
	__declspec(dllexport) int  __cdecl GetIR(float*lb,float*hb,float*sg,float*eg,float*pb,float*pe,float*data);
	__declspec(dllexport) int  __cdecl GetFAC(float * datax,float * datay);
	__declspec(dllexport) int  __cdecl GetMSC(float * datax,float * datay);

	__declspec(dllexport) int  __cdecl GetSNR();
	__declspec(dllexport) int  __cdecl GetLevel();
	__declspec(dllexport) int  __cdecl GetDCFreq();
	__declspec(dllexport) int  __cdecl GetState(int * states);  
	__declspec(dllexport) void __cdecl GetData(int * totsize,int * actsize,int * actpos);  


//********************************************************************




