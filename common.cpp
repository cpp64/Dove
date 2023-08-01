#include <windows.h>
#include <shobjidl.h>
BOOL dialog1(IFileOpenDialog *pFileOpen,LPWSTR folder)
{
    BOOL ret = FALSE;
    wcscpy(folder,L"had a try to show dialogue");
    if(SUCCEEDED(pFileOpen->Show(NULL)))
    {
        IShellItem *pItem;
        if(SUCCEEDED(pFileOpen->GetResult(&pItem)))
        {
            PWSTR pszFilePath;
            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH,&pszFilePath)))
            {
                ///полезная работа
                wcscpy(folder,pszFilePath);
                ret = TRUE;
                ///конец полезной работы
                CoTaskMemFree(pszFilePath);
            }
            pItem->Release();
        }
    }
    pFileOpen->Release();
    return ret;
}
BOOL dialog(LPWSTR folder)
{
    BOOL ret = FALSE;
    if(SUCCEEDED(CoInitializeEx(NULL,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE)))
    {
        IFileOpenDialog *pFileOpen;
        if(SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog,NULL,CLSCTX_ALL,
                                      IID_IFileOpenDialog,reinterpret_cast<void**>(&pFileOpen))))
        {
            DWORD dwOptions;
            if(SUCCEEDED(pFileOpen->GetOptions(&dwOptions)))
                if(SUCCEEDED(pFileOpen->SetOptions(dwOptions|FOS_PICKFOLDERS)))
                    ret = dialog1(pFileOpen,folder);
        }
        CoUninitialize();
    }
    return ret;
}
