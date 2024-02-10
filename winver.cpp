#include <Windows.h>
#include <cstddef>
#include <charconv>
#include <string_view>

#include "lib/Windows_Symbol.hpp"

void Print (wchar_t c);
void Print (const wchar_t * text);
void Print (const wchar_t * text, std::size_t length);
void PrintRsrc (unsigned int);
void PrintNumber (DWORD number, int base = 10);
void InitPrint ();
void InitArguments ();
UCHAR GetConsoleColors ();
void SetTextColor (unsigned char);
void ResetTextColor ();
bool IsOptionPresent (wchar_t c);

HKEY hKey = NULL;
HANDLE out = NULL;
bool  file = false;
UCHAR colors = 0;
DWORD major = 0;
DWORD minor = 0;
DWORD build = 0;
const wchar_t * args = nullptr;

bool ShowBrandingFromAPI ();
void ShowVersionNumbers ();
bool PrintValueFromRegistry (const wchar_t * value, const wchar_t * prefix = nullptr);
void PrintUserInformation ();

extern "C" IMAGE_DOS_HEADER __ImageBase;
extern "C" void WINAPI RtlGetNtVersionNumbers (LPDWORD, LPDWORD, LPDWORD); // NTDLL

// Entry Point

#ifndef _WIN64
__declspec(naked)
#endif
__declspec (noreturn) void main () {
    InitPrint ();
    InitArguments ();
    RtlGetNtVersionNumbers (&major, &minor, &build);
    RegOpenKeyEx (HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey);

    build &= 0x0FFF'FFFF;
    colors = GetConsoleColors ();

    Print (L'\r');
    Print (L'\n');
    
    // Windows 11 Pro Insider Preview
    // Windows 10 Enterprise N 2016 LTSB
    // Windows Server 2022 Datacenter
    // Hyper-V Server
    // Azure Stack HCI

    if (major < 10 || !ShowBrandingFromAPI ()) {
        if (!PrintValueFromRegistry (L"ProductName")) {
            PrintRsrc (1);
        }
    }

    // [Version 22H2 Major.Minor.Build.UBR]

    Print (L' ');
    Print (L'[');
    PrintRsrc (2);
    SetTextColor (15);
    if (PrintValueFromRegistry (L"DisplayVersion") || PrintValueFromRegistry (L"ReleaseId")) {
        Print (L' ');
    }
    ResetTextColor ();
    ShowVersionNumbers ();
    PrintValueFromRegistry (L"CSDVersion", L" "); // TODO: prefer GetVersionEx?
    Print (L']');

    if (IsOptionPresent (L'a') || IsOptionPresent (L'o')) {
        PrintUserInformation ();
    }

    // TODO: hypervisor info?
    // TODO: licensing and expiration status
    // TODO: ?? memory size and OS bitness (3GT)
    // TODO: ?? processors and levels, architectures, SSE,AVX,etc...
    // TODO: ?? supported architectures?

    // RegCloseKey (hKey);
    Print (L'\r');
    Print (L'\n');
    ExitProcess (0);
}

bool ShowBrandingFromAPI () {
    bool result = false;
    if (auto dll = LoadLibrary (L"WINBRAND")) {
        LPWSTR (WINAPI * ptrBrandingFormatString) (LPCWSTR) = NULL;
        if (Windows::Symbol (dll, ptrBrandingFormatString, "BrandingFormatString")) {
            if (auto text = ptrBrandingFormatString (L"%WINDOWS_LONG%")) {
                Print (text);
                // LocalFree (text);
                result = true;
            }
        }
        // FreeLibrary (dll);
    }
    return result;
}

bool PrintValueFromRegistry (const wchar_t * value, const wchar_t * prefix) {
    if (hKey) {
        wchar_t text [512];
        DWORD size = sizeof text;
        if (RegQueryValueEx (hKey, value, NULL, NULL, (LPBYTE) text, &size) == ERROR_SUCCESS) {
            if (size > sizeof (wchar_t)) {
                if (prefix) {
                    Print (prefix);
                }
                Print (text, size / sizeof (wchar_t) - 1);
            }
            return true;
        }
    }
    return false;
}

void PrintUserInformation () {
    if (hKey) {
        wchar_t owner [512];
        wchar_t organization [512];

        DWORD owner_size = sizeof owner;
        DWORD organization_size = sizeof organization;
        auto have_owner = (RegQueryValueEx (hKey, L"RegisteredOwner", NULL, NULL, (LPBYTE) owner, &owner_size) == ERROR_SUCCESS) && (owner_size > sizeof (wchar_t));
        auto have_organization = (RegQueryValueEx (hKey, L"RegisteredOrganization", NULL, NULL, (LPBYTE) organization, &organization_size) == ERROR_SUCCESS) && (organization_size > sizeof (wchar_t));

        if (have_owner || have_organization) {
            PrintRsrc (3);
            if (have_owner) {
                PrintRsrc (4);
                Print (owner, owner_size / sizeof (wchar_t));
            }
            if (have_organization) {
                PrintRsrc (4);
                Print (organization, organization_size / sizeof (wchar_t));
            }
        }
    }
}

void PrintNumber (DWORD number, int base) {
    char a [24];

    if (auto [end, error] = std::to_chars (&a [0], &a [sizeof a], number, base); error == std::errc ()) {
        auto n = end - a;
        wchar_t w [24];
        for (std::size_t i = 0; i != n; ++i) {
            w [i] = a [i];
        }
        Print (w, n);
    }
}

void ShowVersionNumbers () {
    if (major == 10 && minor == 0) {
        SetTextColor (8);
    }
    PrintNumber (major);
    Print (L'.');
    PrintNumber (minor);
    Print (L'.');
    ResetTextColor ();
    PrintNumber (build);

    DWORD UBR = 0;
    DWORD size = sizeof UBR;
    if (RegQueryValueEx (hKey, L"UBR", NULL, NULL, (LPBYTE) &UBR, &size) == ERROR_SUCCESS) {
        Print (L'.');
        PrintNumber (UBR);
    }
}

void InitPrint () {

    // Output to Console or into File?

    out = GetStdHandle (STD_OUTPUT_HANDLE);
    if (out == NULL) { // nowhere to print
        ExitProcess (ERROR_OUT_OF_PAPER);
    }

    switch (GetFileType (out)) {
        case FILE_TYPE_DISK:
            file = true;
            break;
        case FILE_TYPE_CHAR:
            DWORD dw;
            if (GetConsoleMode (out, &dw)) {
                file = false; // confirmed it's console
                break;
            }

            [[ fallthrough ]]; // else serial/paralel port or printer, continue...
        default:
        case FILE_TYPE_UNKNOWN: // socket/tape
        case FILE_TYPE_PIPE:
            file = true;
    }
}

void Print (const wchar_t * text, std::size_t length) {
    DWORD wtn;
    if (file) {
        char buffer [3072];
        while (length) {
            auto cw = (int) min (length, sizeof buffer);
            auto n = WideCharToMultiByte (CP_ACP, 0, text, cw, buffer, (int) sizeof buffer, NULL, NULL);
            if (n > 0) {
                WriteFile (out, buffer, n, &wtn, NULL);
                text += cw;
                length -= cw;
            } else
                break;
        }
    } else {
        WriteConsole (out, text, (DWORD) length, &wtn, NULL);
    }
}

void Print (const wchar_t * text) {
    Print (text, wcslen (text));
}
void Print (wchar_t c) {
    DWORD wtn;
    if (file) {
        WriteFile (out, &c, 1, &wtn, NULL);
    } else {
        WriteConsole (out, &c, 1, &wtn, NULL);
    }
}
void PrintRsrc (unsigned int id) {
    const wchar_t * string = nullptr;
    if (auto length = LoadString (reinterpret_cast <HINSTANCE> (&__ImageBase), id, (LPWSTR) &string, 0)) {
        Print (string, length);
    }
}

UCHAR GetConsoleColors () {
    if (!file) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (GetConsoleScreenBufferInfo (out, &info)) {
            return (UCHAR) info.wAttributes;
        }
    }
    return 0;
}
void SetTextColor (unsigned char fg) {
    if (!file) {
        fg &= 0x0F;
        fg |= (colors & 0xF0);
        if (fg != colors) {
            SetConsoleTextAttribute (out, fg);
        }
    }
}
void ResetTextColor () {
    if (!file) {
        SetConsoleTextAttribute (out, colors);
    }
}

void InitArguments () {
    std::wstring_view cmdline (GetCommandLine ());
    cmdline.remove_prefix (cmdline.find_last_of (L'"') + 1); // skip until last quote, if any
    if (auto argoffset = cmdline.rfind (L" -") + 1) { // any arguments?
        cmdline.remove_prefix (argoffset + 1);
        if (!cmdline.empty ()) {
            args = cmdline.data ();
        }
    }
}

bool IsOptionPresent (wchar_t c) {
    return args != nullptr
        && std::wstring_view (args).contains (c);
}
