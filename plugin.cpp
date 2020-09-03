#include "plugin.h"

enum
{
    MENU_TEST,
};

#define ASLR_BIT 0x40

// Thanks to https://github.com/klks/checksec/blob/master/checksec/plugin.cpp#L25
#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD_PTR)(ptr) + (DWORD_PTR)(addValue))


std::string removeASLR() {

    Module::ModuleInfo mainModule;
    HANDLE hModuleFileHandle;
    HANDLE hMappedFileHandle;
    LPVOID lpFileView;

    PIMAGE_DOS_HEADER dosHeader;
    PIMAGE_NT_HEADERS NTheader;
    PIMAGE_OPTIONAL_HEADER optionalHeader;
    WORD characteristics;
    
    bool moduleOk = Module::GetMainModuleInfo(&mainModule);

    if (!moduleOk) {
        return "Nothing is being debugged !";
    }

    std::string newFileName = mainModule.path;
    newFileName.append("_no_aslr");

    if (!CopyFile(mainModule.path, newFileName.c_str(), FALSE)) {
#ifdef _DEBUG
        _plugin_logprintf("Unable to copy file %s to %s, error code %d\n", mainModule.path, newFileName.c_str(), GetLastError());
#endif

        return "Error on copying file";
    }

    //  Because our file is already "locked" by dbg process, we need open in SHARED_READ
    //
    hModuleFileHandle = CreateFile(newFileName.c_str(), GENERIC_READ | GENERIC_WRITE , NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hModuleFileHandle == INVALID_HANDLE_VALUE) {
        
        CloseHandle(hModuleFileHandle);
#ifdef _DEBUG
        _plugin_logprintf("Error on opening main module, error code => %d\n", GetLastError());
#endif
        return "Main module is not available!";
    }

    // Map file in our memory

    hMappedFileHandle = CreateFileMapping(hModuleFileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);

    if (hMappedFileHandle == INVALID_HANDLE_VALUE) {
        CloseHandle(hModuleFileHandle);
#ifdef _DEBUG
        _plugin_logprintf("Error on mapping main module, error code => %d\n", GetLastError());
#endif
        return "Error on mapping";
    }

    // Get mapped data pointer
    lpFileView = MapViewOfFile(hMappedFileHandle, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);

    if (!lpFileView) {
        CloseHandle(hModuleFileHandle);
        CloseHandle(hMappedFileHandle);
#ifdef _DEBUG
        _plugin_logprintf("Error on get main module view, error code => %d\n", GetLastError());
#endif
        return "Error on mapping";
    }
    
    // PE Parsing
    dosHeader = (PIMAGE_DOS_HEADER)lpFileView;

    if (!dosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
        CloseHandle(hModuleFileHandle);
        CloseHandle(hMappedFileHandle);
        UnmapViewOfFile(lpFileView);
        return "Invalid PE file!";
    }

    //NTheader = (PIMAGE_NT_HEADERS)(dosHeader + dosHeader->e_lfanew);
    NTheader = MakePtr(PIMAGE_NT_HEADERS, dosHeader, dosHeader->e_lfanew);

    // Get OptionalHeader
    optionalHeader = (PIMAGE_OPTIONAL_HEADER) &NTheader->OptionalHeader; // Holds the pointer

    // Patch DllCharacteristics

    optionalHeader->DllCharacteristics &= ~0xFF;
    
    CloseHandle(hModuleFileHandle);
    CloseHandle(hMappedFileHandle);
    UnmapViewOfFile(lpFileView);

    std::string success = "ASLR removed! reload your new file " + newFileName;

    return success;


}

PLUG_EXPORT void CBMENUENTRY(CBTYPE cbType, PLUG_CB_MENUENTRY* info)
{
    bool canContinue;
    switch(info->hEntry)
    {
    case MENU_TEST:
            MessageBoxA(hwndDlg, removeASLR().c_str(), PLUGIN_NAME, MB_ICONINFORMATION);
            break;
    default:
        break;
    }
}

//Initialize your plugin data here.
bool pluginInit(PLUG_INITSTRUCT* initStruct)
{
    return true; //Return false to cancel loading the plugin.
}

//Deinitialize your plugin data here.
void pluginStop()
{
}

//Do GUI/Menu related things here.
void pluginSetup()
{
    _plugin_menuaddentry(hMenu, MENU_TEST, "Remove ASLR");
}
