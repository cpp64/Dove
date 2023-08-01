#define _UNICODE
#define UNICODE
#include <tchar.h>
#include <windows.h>
#include <stdint.h>
#include "..\\common.hpp"
WCHAR path[MAX_PATH]; // путь к папке, которую выберет пользователь
int32_t folder_size()
{
    // может как-то покрасивее можно обрабатывать ошибки переполнения и проч.
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;

    uint32_t sum = 0;
    size_t len0 = wcslen(path);

    wcscat(path,L"\\*");
    hFind = FindFirstFileW(path,&ffd);
    path[len0] = L'\0';
    //not overflow error, ret -2
    if (hFind == INVALID_HANDLE_VALUE)
        return -2;
    do
    {
        if(wcscmp(ffd.cFileName,L".") == 0)
            continue;
        if(wcscmp(ffd.cFileName,L"..") == 0)
            continue;
        if(ffd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
        {
            wcscat(path,L"\\");
            wcscat(path,ffd.cFileName);
            uint32_t temp = folder_size();
            path[len0] = L'\0';
            if(temp < 0)
            {
                FindClose(hFind);
                return temp;
            }
            sum += temp;
            if(sum > (50<<20))
            {
                FindClose(hFind);
                return -1;
            }
            continue;
        }
        //overflow error, size > 2^32 || size > 50 Mb, ret -1
        if(ffd.nFileSizeHigh > 0 || ffd.nFileSizeLow > (50<<20))
        {
            FindClose(hFind);
            return -1;
        }
        sum += ffd.nFileSizeLow;
        //overflow error, ret -1
        if(sum > (50<<20))
        {
            FindClose(hFind);
            return -1;
        }
    }
    while(FindNextFileW(hFind,&ffd) != 0);
    //not overflow error, ret -2
    if(GetLastError() != ERROR_NO_MORE_FILES)
        sum = -2;
    FindClose(hFind);
    return sum;
}
int counter = 100;
// ABOUT FONT:
// Logical units are device dependent pixels, so this will create a handle
// to a logical font that is 48 pixels in height.
// The width, when set to 0, will cause the font mapper to choose the closest matching value.
// The font face name will be Calibri.
HFONT hFont1 = CreateFont(24,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                          CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Calibri"));
LRESULT CALLBACK WindowProcedure(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    switch(message)
    {
    case WM_DESTROY:
    {
        // не понял до конца когда происходит WM_DESTROY,WM_CLOSE,WM_QUIT
        PostQuitMessage(0);
        break;
    }
    case WM_TIMER:
    {
        InvalidateRect(hwnd,NULL,FALSE);
        --counter;
        break;
    }
    case WM_PAINT:
    {
        WCHAR s[20];
        snwprintf(s,20,L"Ожидайте %dс...",counter);
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd,&ps);
        HFONT hFontOriginal = (HFONT)SelectObject(hdc,hFont1);
        RECT rect;
        GetClientRect(hwnd,&rect);
        FillRect(hdc,&rect,WHITE_BRUSH);
        // DT_VCENTER используется только вместе с DT_SINGLELINE
        DrawText(hdc,s,(int)wcslen(s),&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        SelectObject(hdc,hFontOriginal);
        EndPaint(hwnd,&ps);
        break;
    }
    default:
        return DefWindowProc(hwnd,message,wParam,lParam);
    }
    return 0;
}
//  Make the class name into a global variable
TCHAR szClassName[] = _T("Dove_client");
int WINAPI WinMain(HINSTANCE hThisInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpszArgument,
                   int nCmdShow)
{
    if(!dialog(path))
    {
        if(wcscmp(path,L"had a try to show dialogue") != 0)
            MessageBoxW(NULL,L"Системная ошибка",L"Ошибка",MB_TOPMOST|MB_ICONERROR|MB_OK);
        return -1;
    }
    WCHAR *i = _wgetenv(L"USERPROFILE"), *j = path;
    for(; (*i) && (*j) && (*i) == (*j); ++i,++j);
    if(*i)
    {
        WCHAR s[MAX_PATH+100];
        snwprintf(s,MAX_PATH+100,L"Отправка данных вне домашнего каталога пользователя %ls запрещена",
                  _wgetenv(L"USERPROFILE"));
        MessageBoxW(NULL,s,L"Ошибка",MB_TOPMOST|MB_ICONERROR|MB_OK);
        return -8;
    }
    int32_t _folder_size = folder_size();
    if(_folder_size == -1)
    {
        MessageBoxW(NULL,L"Размер файлов превысил 50 Мб",L"Ошибка",MB_TOPMOST|MB_OK|MB_ICONERROR);
        return -2;
    }
    if(_folder_size == -2)
    {
        MessageBoxW(NULL,L"Ошибка обработки каталога",L"Ошибка",MB_TOPMOST|MB_OK|MB_ICONERROR);
        return -3;
    }
    WCHAR s[MAX_PATH+200];
    snwprintf(s,
              MAX_PATH+200,
              L"Вы действительно собираетесь отправить на проверку папку %ls размером %d байт?",
              path,
              _folder_size);
    if(MessageBoxW(NULL,s,L"Отправка решения",MB_TOPMOST|MB_YESNO|MB_ICONQUESTION) != IDYES)
        return -4;
    /// shmem
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ|FILE_MAP_WRITE,FALSE,_T("Global\\Dove_mem_0x0052C"));
    if(hMapFile == NULL)
    {
        MessageBoxW(NULL,L"Ошибка подключения к серверу",L"Ошибка",MB_TOPMOST|MB_OK|MB_ICONERROR);
        return -5;
    }
    LPWSTR pBuf = (LPWSTR)MapViewOfFile(hMapFile,FILE_MAP_READ|FILE_MAP_WRITE,0,0,sizeof(path));
    if (pBuf == NULL)
    {
        MessageBoxW(NULL,L"Ошибка подключения к серверу",L"Ошибка",MB_TOPMOST|MB_OK|MB_ICONERROR);
        CloseHandle(hMapFile);
        return -6;
    }
    CopyMemory((PVOID)pBuf,path,sizeof(path));
    /// delay
    HWND hwnd;
    WNDCLASSEX wincl;
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;
    wincl.style = CS_DBLCLKS; // не понятное поле
    wincl.cbSize = sizeof(WNDCLASSEX);
    // Use default icon and mouse-pointer
    wincl.hIcon = LoadIcon(hThisInstance,_T("MAINICON"));
    wincl.hIconSm = NULL;
    wincl.hCursor = LoadCursor(NULL,IDC_WAIT);
    wincl.lpszMenuName = NULL;// No menu
    wincl.cbClsExtra = 0;     // No extra bytes after the window class
    wincl.cbWndExtra = 0;     // structure or the window instance
    // Use Windows's default colour as the background of the window
    wincl.hbrBackground = WHITE_BRUSH;
    // Register the window class, and if it fails quit the program
    if (!RegisterClassEx(&wincl))
        return -7;
    // The class is registered, let's create the program
    hwnd = CreateWindowEx(
               NULL,
               szClassName,                    // Classname
               _T("Получение подтверждения"),  // Title
               WS_SYSMENU,                     // только кнопка закрыть
               CW_USEDEFAULT,                  // Windows decides the position
               CW_USEDEFAULT,                  // where the window ends up on the screen
               340,                            // The programs width
               200,                            // and height in pixels
               HWND_DESKTOP,                   // The window is a child-window to desktop
               NULL,                           // No menu
               hThisInstance,                  // Program Instance handler
               NULL                            // No Window Creation data
           );
    ShowWindow(hwnd,nCmdShow);
    SetTimer(hwnd,0,1000,NULL); // без hwnd не получается запустить таймер
    MSG messages;
    for(;;)
    {
        if(counter <= 0)
            break;
        if(pBuf[0] == L'\0')
            break;
        if(!GetMessage(&messages,NULL,0,0))
            break;
        // Translate virtual-key messages into character messages
        TranslateMessage(&messages);
        // Send message to WindowProcedure
        DispatchMessage(&messages);
    }
    if(!KillTimer(hwnd,0))
        MessageBoxW(NULL,L"Системная ошибка: не удалось отключить таймер",L"Ошибка",MB_TOPMOST|MB_OK|MB_ICONERROR);
    if(pBuf[1] == L'B')
        MessageBoxW(NULL,L"Папка с этим именем уже была принята.",L"Ошибка",MB_TOPMOST|MB_ICONERROR|MB_OK);
    if(pBuf[1] == L'C')
        MessageBoxW(NULL,L"Отправка не удалась",L"Ошибка",MB_TOPMOST|MB_OK|MB_ICONERROR);
    pBuf[1] = L'\0';
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    return EXIT_SUCCESS;
}
