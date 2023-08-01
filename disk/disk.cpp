#define _UNICODE
#define  UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <direct.h>
#include "..\\common.hpp"
#include "..\\set_work_dir.hpp"
/*
Создаваемая задача не работает при работе пк от батареи

ветка реестра планировщика: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Schedule\TaskCache
*/
void rawwrite(FILE* f,WCHAR* ws)
{
    //write size of str, then str itself
    int32_t sz = wcslen(ws)*sizeof(WCHAR);
    fwrite(&sz,sizeof(sz),1,f);
    fwrite(ws,sz,1,f);
}
WCHAR guid[MAX_PATH];
void get_guid_by_drive_letter(WCHAR letter)
{
    WCHAR drive[] = {letter,L':',L'\\',L'\0'};
    GetVolumeNameForVolumeMountPointW(drive,guid,MAX_PATH);
}
WCHAR folder[MAX_PATH];
// НЕ РАБОТАЕТ ЕСЛИ СЕРВЕР ВИСИТ В ОПЕРАТИВКЕ
int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nCmdShow)
{
    //windows defender не даёт запускать с админа
    if(!set_work_dir())
        return -1;
    FILE* tempFile = fopen("disk.tmp","w");
    if(tempFile == NULL)
    {
        MessageBoxW(NULL,L"Выполните запуск от администратора",L"Ошибка",MB_TOPMOST|MB_ICONERROR|MB_OK);
        return -2;
    }
    else
        fclose(tempFile);
    /// choose folder for submissions
    if(!dialog(folder))
    {
        if(wcscmp(folder,L"had a try to show dialogue") != 0)
            MessageBoxW(NULL,L"Не удалось обработать каталог",L"Ошибка",MB_TOPMOST|MB_ICONERROR|MB_OK);
        return -3;
    }
    /// try load guid for disk, containing folder
    get_guid_by_drive_letter(folder[0]);
    /// save guid and folder
    FILE* f = fopen("disk.dat","wb");
    if(f == NULL)
    {
        MessageBoxW(NULL,L"Не удалось сохранить информацию о диске",L"Ошибка",MB_TOPMOST|MB_ICONERROR|MB_OK);
        return -4;
    }
    rawwrite(f,folder);
    rawwrite(f,guid);
    fclose(f);
    return 0;
}
