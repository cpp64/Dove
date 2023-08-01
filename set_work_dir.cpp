#include <windows.h>
#include <direct.h>
BOOL set_work_dir()
{
    LPWSTR exepath = (LPWSTR)malloc(MAX_PATH*sizeof(WCHAR));
    if(!GetModuleFileNameW(NULL,exepath,MAX_PATH))
    {
        free(exepath);
        return FALSE;
    }
    for(int i = wcslen(exepath)-1; i >= 0; --i)
    {
        if(L'\\' == exepath[i])
        {
            exepath[i] = L'\0';
            break;
        }
    }
    int res = _wchdir(exepath);
    free(exepath);
    return res == 0;
}
