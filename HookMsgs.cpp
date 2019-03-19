#include <windows.h>
#include <time.h>
#include <sys/timeb.h>

#pragma warning(disable:4786)	
#include <map>

using namespace std;

typedef void* (*P_HOOK_PROC) (HWND, int, WPARAM, LPARAM, void *);

HHOOK	g_hMouse	= NULL;
HHOOK	g_hKeyboard	= NULL;
FILE*	g_pFile		= NULL;
BOOL	g_bShowed	= FALSE;
long long g_lastEventTime = 0;

#define MAP		map<UINT, P_HOOK_PROC>
#define PAIR	pair<UINT, P_HOOK_PROC>

#pragma data_seg("MySec")
HWND			g_hWnd		= NULL;

#ifdef  HOOK_PROC_MAP
MAP				g_mosHookMap;
MAP				g_kbdHookMap;
#else
P_HOOK_PROC		g_hookProc	= NULL;
#endif
#pragma data_seg()

//#pragma comment(linker,"/section:MySec,RWS")

#if 0
HINSTANCE g_hInst;

  BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  // handle to the DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved   // reserved
  )
  {
  g_hInst=hinstDLL;
}
#endif

LRESULT CALLBACK MouseProc(
	int nCode,      // hook code
	WPARAM wParam,  // message identifier
	LPARAM lParam   // mouse coordinates
	)
{


#ifdef HOOK_LOG
	if (g_pFile != NULL) {
		fprintf(g_pFile, "[Message] code: %d, wParam: %d, lParam: %d.\n", nCode, wParam, lParam);
		fflush(g_pFile);
	}
#endif

#ifdef HOOK_PROC_MAP	
	MAP::iterator it = g_mosHookMap.begin();
	for (; it != g_mosHookMap.end(); it ++) {
		PAIR p = *it;
		if (p.first == wParam) {
			p.second(g_hWnd, nCode, wParam, lParam, NULL);
			break;
		}
	}
#else 
//	g_hookProc(g_hWnd, nCode, wParam, lParam, NULL);
#endif
	return CallNextHookEx(g_hMouse, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(
	int nCode,       // hook code
	WPARAM wParam,  // virtual-key code
	LPARAM lParam   // keystroke-message information
	)
{
#ifdef HOOK_LOG
	if (g_pFile != NULL) {
		fprintf(g_pFile, "[Message] code: %d, wParam: %d, lParam: %d.\n", nCode, wParam, lParam);
		fflush(g_pFile);
	}
#endif

#ifdef HOOK_PROC_MAP	
	MAP::iterator it = g_kbdHookMap.begin();
	for (; it != g_mosHookMap.end(); it ++) {
		PAIR p = *it;
		if (p.first == wParam) {
			p.second(g_hWnd, nCode, wParam, lParam, NULL);
			break;
		}
	}
#else
//	g_hookProc(g_hWnd, nCode, wParam, lParam, NULL);
#endif

	if (HC_ACTION == nCode) {
		if (WM_KEYDOWN == wParam || WM_SYSKEYDOWN) {
			BOOL bCtrl	= GetKeyState(VK_CONTROL)	& 0x8000;  
			BOOL bShift	= GetKeyState(VK_SHIFT)		& 0x8000;  
			BOOL bAlt	= GetKeyState(VK_MENU)		& 0x8000; 
			if (((char)wParam == 'C' || (char)wParam == 'c') && bShift) {
				if ((lParam & 0x40000000) != 0) { //响应按键按下，按键抬起则略过。
					if (g_bShowed) {
						ShowWindow(g_hWnd, SW_HIDE);
						g_bShowed = FALSE;
					} else {
						::SetWindowPos(g_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); 
						ShowWindow(g_hWnd, SW_SHOW);
						::SetWindowPos(g_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); 
						g_bShowed = TRUE;
					}
				}
				return 1;
			} else if (wParam == VK_DOWN) {
				if ((lParam & 0x40000000) != 0) {
				//	SendMessage(g_hWnd, WM_USER + 101, NULL, NULL);
				}
				return 1;
			} else if (wParam == VK_UP) {
				if ((lParam & 0x40000000) != 0) {
				//	SendMessage(g_hWnd, WM_USER + 102, NULL, NULL);
				}
				return 1;
			} else if (((char)wParam == 'G' || (char)wParam == 'g') && bShift) {
				if ((lParam & 0x40000000) != 0) {
					SendMessage(g_hWnd, WM_USER + 103, NULL, NULL);
				}
				return 1;
			} else if (((char)wParam == 'D' || (char)wParam == 'd') && bShift) {
				if ((lParam & 0x40000000) != 0) {
					SendMessage(g_hWnd, WM_USER + 104, NULL, NULL);
				}
				return 1;
			}
		}
	}
	return CallNextHookEx(g_hKeyboard, nCode, wParam, lParam);
}

void SetHook(HWND hwnd)
{
	g_hWnd=hwnd;
	g_hMouse=SetWindowsHookEx(WH_MOUSE,MouseProc,GetModuleHandle("HookMsgs"),0);
	g_hKeyboard=SetWindowsHookEx(WH_KEYBOARD,KeyboardProc,GetModuleHandle("HookMsgs"),0);

#ifdef HOOK_LOG
	g_pFile = fopen("hook.log", "a+");
	if (g_pFile == NULL) {
		printf("can not open hook log file\n");
	}
#endif

}


void UnHook()
{
	if (g_hKeyboard != NULL) {
		UnhookWindowsHookEx(g_hKeyboard);
	}
	if (g_hMouse != NULL) {
		UnhookWindowsHookEx(g_hMouse);
	}

#ifdef HOOK_LOG
	if (g_pFile != NULL) {
		fclose(g_pFile);
	}
#endif

}

void SetHookConfig(UINT nMsg, UINT nMsgType, P_HOOK_PROC pProc) 
{
#ifdef HOOK_PROC_MAP
	PAIR p(nMsg, pProc);
	if (nMsgType == WH_MOUSE) {		
		g_mosHookMap.insert(p);
	} else if (nMsgType == WH_KEYBOARD) {
		g_mosHookMap.insert(p);
	}
#else
	if (nMsg == WM_NULL) {
		g_hookProc = pProc;
	}
#endif

}

