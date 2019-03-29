#include <windows.h>
#include <time.h>
#include <sys/timeb.h>

#pragma warning(disable:4786)	
#include <map>

using namespace std;
#if 1
#define HOOK_LOG
#endif 
typedef void* (*P_HOOK_PROC) (HWND, int, WPARAM, LPARAM, void *);

typedef enum SysKeyType {
	NONE_KEY = 0,
	CTRL_KEY,
	ALT_KEY,
	SHIFT_KEY
};

typedef enum SysKeyStat {
	KEY_NO_STATE = 0,
	KEY_DOWN,
	KEY_UP
};

typedef struct SysKeyMark {
	BOOL		bAlt;
	BOOL		bShift;
	BOOL		bCtrl;
	SysKeyStat	state;
	long long	keyupTime;
};

#define EVENT_EXPIRE_TIME = 500

HHOOK	g_hMouse		= NULL;
HHOOK	g_hKeyboard		= NULL;
FILE*	g_pFile			= NULL;
BOOL	g_bShowed		= FALSE;

long long g_lastEventTime	= 0;
long long g_eventExpireTime = 2000;

SysKeyMark g_sysKeyMark = {0};

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
		fprintf(g_pFile, "[Message] code: %d, wParam: %d, low: %c, lParam: %d.\n", nCode, wParam, (char)wParam, lParam);
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
	LRESULT result = CallNextHookEx(g_hKeyboard, nCode, wParam, lParam);
	long long curTimestamp = GetTickCount();
	long long interval = curTimestamp - g_lastEventTime;
	g_lastEventTime = curTimestamp;

	if ((WM_KEYDOWN == wParam || WM_SYSKEYDOWN)) {
		if ((lParam & 0x40000000) != 0) {

			g_sysKeyMark.bAlt	= GetKeyState(VK_MENU)	  & 0x8000;
			g_sysKeyMark.bCtrl	= GetKeyState(VK_CONTROL) & 0x8000;
			g_sysKeyMark.bShift = GetKeyState(VK_SHIFT)   & 0x8000;

			if (g_sysKeyMark.bAlt || g_sysKeyMark.bCtrl || g_sysKeyMark.bShift) {
				g_sysKeyMark.state = KEY_DOWN;
				g_sysKeyMark.keyupTime = 0;
			}
			
			if (g_sysKeyMark.state == KEY_UP && interval >= g_eventExpireTime) {
				memset(&g_sysKeyMark, 0x00, sizeof(SysKeyMark));
			}
			interval = curTimestamp - g_sysKeyMark.keyupTime;

			if (g_sysKeyMark.bAlt && (g_sysKeyMark.state == KEY_DOWN || (g_sysKeyMark.state == KEY_UP && interval < g_eventExpireTime)) && ((char)wParam == '1')) {
				if (g_bShowed) {
					ShowWindow(g_hWnd, SW_HIDE);
					g_bShowed = FALSE;

				} else {
					//MessageBox(g_hWnd, "SHOW COMMAND", "", NULL);
					ShowWindow(g_hWnd, SW_SHOW);
					SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); 
					g_bShowed = TRUE;
				}
				return 1;
			} else if (g_sysKeyMark.bAlt && (g_sysKeyMark.state == KEY_DOWN || (g_sysKeyMark.state == KEY_UP && interval < g_eventExpireTime)) && ((char)wParam == '2')) {
				if ((lParam & 0x40000000) != 0) {
					SendMessage(g_hWnd, WM_USER + 103, NULL, NULL);
				}
				return 1;
			} else if (g_sysKeyMark.bAlt && (g_sysKeyMark.state == KEY_DOWN || (g_sysKeyMark.state == KEY_UP && interval < g_eventExpireTime)) && ((char)wParam == '3')) {
				if ((lParam & 0x40000000) != 0) {
					SendMessage(g_hWnd, WM_USER + 104, NULL, NULL);
				}
				return 1;
			}
		} else {
			if (!(GetKeyState(VK_CONTROL) & 0x8000) 
				&& !(GetKeyState(VK_SHIFT) & 0x8000)
				&& !(GetKeyState(VK_MENU) & 0x8000)) {
				if (g_sysKeyMark.bAlt || g_sysKeyMark.bCtrl || g_sysKeyMark.bShift) {
					g_sysKeyMark.state = KEY_UP;
					g_sysKeyMark.keyupTime = curTimestamp;
				}
			}
		}

	} else {

	}

#ifdef HOOK_LOG
	if (g_pFile != NULL) {
		fprintf(g_pFile, "[message] balt=%d,bctrl=%d,bshift=%d,keystat=%d,uptime=%I64d,interval=%I64d\n", g_sysKeyMark.bAlt,g_sysKeyMark.bCtrl,g_sysKeyMark.bShift,g_sysKeyMark.state,g_sysKeyMark.keyupTime,interval);
		fflush(g_pFile);
	}
#endif
	g_lastEventTime = curTimestamp;
	return result;
}

void SetHook(HWND hwnd)
{
	g_hWnd=hwnd;
//	g_hMouse=SetWindowsHookEx(WH_MOUSE,MouseProc,GetModuleHandle("HookMsgs"),0);
	g_hKeyboard=SetWindowsHookExA(WH_KEYBOARD, KeyboardProc, GetModuleHandle("HookMsgs"), 0);

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

