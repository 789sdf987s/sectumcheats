#pragma once
#define WIN32_LEAN_AND_MEAN
#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <string>
#include <psapi.h>
#include <time.h>
#include <process.h>
#include <vector>
#include <map>
#include <ostream>
#include <Shlobj.h>
#include <stdint.h>
#include <string>
#include <string.h>
#include <cmath>
#include <float.h>
#include <codecvt>
#include <iostream>
#include <urlmon.h>
#include <string>
#include <wininet.h>
#include <locale>
#include <windows.h>
#include <sstream> 
#include <atlbase.h>
#include <atlcom.h>
#include <sapi.h>
#include <algorithm>
#include <iterator>
#include <Windows.h>
#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <TlHelp32.h>
#include <WinInet.h>
#include <Wincrypt.h>
#include <time.h>
#pragma comment(lib, "wininet")
#pragma comment(lib,"wininet.lib")
#pragma comment(lib, "urlmon.lib")

namespace Protect
{
	void AntiDumping();
	void Guard();
}
class CLicense
{
private:
	string	GetUrlData(string url);
	string	StringToHex(const string input);
	string	GetHashText(const void * data, const size_t data_size);

	string	GetHwUID();
	DWORD	GetVolumeID();
	string	GetCompUserName(bool User);
	string	GetSerialKey();
	string	GetHashSerialKey();

public:

	string	GetSerial();
	string	GetSerial64();
	bool	CheckLicenseURL(string URL, string GATE, string KEY);
	bool	CheckLicense();
};

extern string base64_encode(char const* bytes_to_encode, unsigned int in_len);