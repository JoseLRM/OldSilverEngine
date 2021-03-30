#include "Windows.h"
#include <iostream>

typedef int(*EntryPoint)(HINSTANCE);

int WINAPI wWinMain(
	HINSTANCE hInstance, 
	HINSTANCE hPrevInstance, 
	PWSTR pCmdLine, 
	int nCmdShow)
{

    HINSTANCE svlib = LoadLibraryA("SilverEngine.dll");
    
    if (svlib) {

	EntryPoint fn = (EntryPoint)GetProcAddress
	    (svlib, "entry_point");

	if (fn) {

	    int res = fn(hInstance);

	    return res;
	}
	else {

	    OutputDebugString("Entry point function not found");
	    MessageBox(0, "Entry point function not found", "Entry point error", MB_OK | MB_ICONERROR);
	}

	FreeLibrary(svlib);
    }
    else {
	
	OutputDebugString("Can't load SilverEngine.dll");
	MessageBox(0, "Can't load SilverEngine.dll", "Entry point error", MB_OK | MB_ICONERROR);
    }

    return 1
	;
}
