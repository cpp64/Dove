#define _UNICODE
#define UNICODE
#define getvol GetVolumeNameForVolumeMountPointW
#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <shobjidl.h>
#include <direct.h>
#include <shlwapi.h>
#include <stdint.h>
#include <aclapi.h>
#include "..\\set_work_dir.hpp"
WCHAR export_folder[MAX_PATH],disk_guid[MAX_PATH];
BOOL set_drive_letter_by_guid()
{
    WCHAR temp_guid[MAX_PATH];
    WCHAR Drive[] = L"C:\\";
    for (WCHAR I = L'C'; I <= L'Z'; I++)
    {
        Drive[0] = I;
        if (!getvol(Drive,temp_guid,MAX_PATH))
            continue;
        if(wcscmp(temp_guid,disk_guid) != 0)
            continue;
        export_folder[0] = I;
        return TRUE;
    }
    return FALSE;
}
BOOL rawread(FILE* f,WCHAR* ws)
{
    //read size of str, then str itself
    int32_t sz;
    if(fread(&sz,sizeof(sz),1,f) != 1)
        return FALSE;
    if(sz == 0)
        return TRUE;
    if(fread(ws,sz,1,f) != 1)
        return FALSE;
    return TRUE;
}
BOOL load_disk()
{
    // СМЕНА ДИСКА ТОЛЬКО С ПЕРЕЗАГРУЗКОЙ, ВОЗМОЖНО ЛУЧШЕ В disk.exe
    // СДЕЛАТЬ СООБЩЕНИЕ О ПЕРЕЗАГРУЗКЕ, ПОТОМУ ЧТО БЕЗ ЭТОГО ДИСК
    // НЕ СМЕНИТСЯ
    FILE* disk = fopen("disk.dat","rb");
    if(disk == NULL)
        return FALSE;
    if(!rawread(disk,export_folder) || !rawread(disk,disk_guid))
        return FALSE;
    if(fclose(disk) != 0)
        return FALSE;
    if(disk_guid[0] != L'\0' && !set_drive_letter_by_guid())
        return FALSE;
    return TRUE;
}
// from,to are folders
BOOL MoveItem1(PCWSTR from,PCWSTR to,IFileOperation* pfo)
{
    BOOL res = FALSE;
    IShellItem* psiFrom = NULL;
    if(SUCCEEDED(SHCreateItemFromParsingName(from,NULL,IID_PPV_ARGS(&psiFrom))))
    {
        IShellItem* psiTo = NULL;
        if(SUCCEEDED(SHCreateItemFromParsingName(to,NULL,IID_PPV_ARGS(&psiTo))))
        {
            if(SUCCEEDED(pfo->MoveItem(psiFrom,psiTo,NULL,NULL)))
                if(SUCCEEDED(pfo->PerformOperations()))
                    res = TRUE;
            psiTo->Release();
        }
        psiFrom->Release();
    }
    return res;
}
BOOL MoveItem(PCWSTR from,PCWSTR to)
{
    BOOL res = FALSE;
    if(SUCCEEDED(CoInitializeEx(NULL,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE)))
    {
        IFileOperation* pfo = NULL;
        if(SUCCEEDED(CoCreateInstance(CLSID_FileOperation,NULL,CLSCTX_ALL,IID_PPV_ARGS(&pfo))))
        {
            res = MoveItem1(from,to,pfo);
            pfo->Release();
        }
        CoUninitialize();
    }
    return res;
}
HANDLE hMapFile;
LPWSTR pBuf;
BOOL openMapping2(PSECURITY_DESCRIPTOR pSD)
{
    BOOL ret = FALSE;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;
    hMapFile = CreateFileMapping(
                   INVALID_HANDLE_VALUE,            // use paging file
                   &sa,                             // security
                   PAGE_READWRITE,                  // read/write access
                   0,                               // maximum object size (high-order DWORD)
                   sizeof(WCHAR)*MAX_PATH,          // maximum object size (low-order DWORD)
                   _T("Global\\Dove_mem_0x0052C")); // name of mapping object
    if(hMapFile)
    {
        pBuf = (LPTSTR)MapViewOfFile(hMapFile,FILE_MAP_WRITE|FILE_MAP_READ,0,0,sizeof(WCHAR)*MAX_PATH);
        if (pBuf)
            ret = TRUE;
        else
            CloseHandle(hMapFile);
    }
    return ret;
}
BOOL openMapping1(PACL pACL)
{
    BOOL ret = FALSE;
    // что-то непонятное
    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
    if(pSD && InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION))
    {
        // совсем не понятно
        if (SetSecurityDescriptorDacl(pSD,
                                      TRUE,   // bDaclPresent flag
                                      pACL,
                                      FALSE)) // not a default DACL
            ret = openMapping2(pSD);
        LocalFree(pSD); // не понятно, нужно ли чистить или может потребоваться
    }
    return ret;
}
BOOL openMapping()
{
    // почти ничего не понятно как это работает
    BOOL ret = FALSE;
    PSID pEveryoneSID = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    if(AllocateAndInitializeSid(&SIDAuthWorld,1,SECURITY_WORLD_RID,0,0,0,0,0,0,0,&pEveryoneSID))
    {
        EXPLICIT_ACCESS ea;
        ZeroMemory(&ea,sizeof(EXPLICIT_ACCESS));
        ea.grfAccessPermissions = FILE_MAP_WRITE|FILE_MAP_READ;
        ea.grfAccessMode = SET_ACCESS;
        ea.grfInheritance= NO_INHERITANCE;
        ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea.Trustee.ptstrName  = (LPTSTR)pEveryoneSID;
        PACL pACL = NULL;
        if (SetEntriesInAcl(1,&ea,NULL,&pACL) == ERROR_SUCCESS)
        {
            ret = openMapping1(pACL);
            LocalFree(pACL); // не понятно, нужно ли чистить или может потребоваться
        }
        FreeSid(pEveryoneSID); // не понятно, нужно ли чистить или может потребоваться
    }
    return ret;
}
int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nCmdShow)
{
    if(!set_work_dir())
        return -1;
    if(freopen("log.txt","w",stdout) == NULL)
        return -2;
    if(!load_disk())
    {
        puts("err in load_disk");
        fflush(stdout);
        return -3;
    }
    else
    {
        puts("successful disk and folder path load");
        fflush(stdout);
    }
    if(!openMapping())
    {
        puts("err in openMapping");
        fflush(stdout);
        return -4;
    }
    else
    {
        puts("successful openMapping");
        fflush(stdout);
    }
    pBuf[0] = pBuf[1] = L'\0'; // установить в начальное состояние
    MSG messages;
    for(;;)
    {
        if(PeekMessage(&messages,NULL,0,0,PM_REMOVE))
        {
            if(messages.message == WM_QUIT)
                break;
            if(messages.message == WM_DESTROY)
                break;
            if(messages.message == WM_CLOSE)
                break;
        }
        Sleep(10*1000);
        // Возможные ситуации pBuf[1], если pBuf[0] == L'\0':
        //    A. Сервер только что принял путь с папкой и папка была перемещена
        //    B. Сервер только что принял путь с папкой, но такая папка уже приходила
        //    C. Сервер только что принял путь с папкой, но переместить не удалось
        if (pBuf[0] == L'\0')
            continue;
        // dest - ...\папка для экспорта\название перемещаемой папки
        WCHAR dest[MAX_PATH];
        wcscpy(dest,export_folder);
        int i = wcslen(pBuf)-1;
        while(i > 0 && pBuf[i] != L'\\')
            --i;
        wcscat(dest,pBuf+i);
        if(PathFileExistsW(dest))
            pBuf[1] = L'B';
        else if(MoveItem(pBuf,export_folder))
            pBuf[1] = L'A';
        else
            pBuf[1] = L'C';
        pBuf[0] = L'\0';
    }
    // The program return-value is 0 - The value that PostQuitMessage() gave
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    return messages.wParam;
}
