#include <Windows.h>
#include <cstddef>
#include <charconv>

#include "lib/Windows_Symbol.hpp"

void Print (const wchar_t * text);
void Print (const wchar_t * text, std::size_t length);
void InitPrint ();
WORD GetConsoleColors ();
void SetTextColor (WORD);
void ResetTextColor ();

HKEY hKey = NULL;
HANDLE out = NULL;
bool  file = false;
WORD  colors = 0;
DWORD major = 0;
DWORD minor = 0;
DWORD build = 0;

bool ShowBrandingFromAPI ();
void ShowVersionNumbers ();
bool PrintValueFromRegistry (const wchar_t * value, const wchar_t * prefix = nullptr);

extern "C" void WINAPI RtlGetNtVersionNumbers (LPDWORD, LPDWORD, LPDWORD); // NTDLL

// Entry Point

#ifndef _WIN64
__declspec(naked)
#endif
__declspec (noreturn) void main () {
    InitPrint ();
    RtlGetNtVersionNumbers (&major, &minor, &build);
    RegOpenKeyEx (HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey);

    build &= 0x0FFF'FFFF;
    colors = GetConsoleColors ();

    Print (L"\r\n");

    // Windows 11 Pro Insider Preview
    // Windows 10 Enterprise N 2016 LTSB
    // Windows Server 2022 Datacenter
    // Hyper-V Server
    // Azure Stack HCI

    if (major < 10 || !ShowBrandingFromAPI ()) {
        if (!PrintValueFromRegistry (L"ProductName")) {
            Print (L"Windows");
        }
    }

    // [Version 22H2 Major.Minor.Build.UBR]

    Print (L" [Version "); // TODO: from rsrc
    SetTextColor (15);
    if (PrintValueFromRegistry (L"DisplayVersion") || PrintValueFromRegistry (L"ReleaseId")) {
        Print (L" ");
    }
    ResetTextColor ();
    ShowVersionNumbers ();
    PrintValueFromRegistry (L"CSDVersion", L" "); // TODO: prefer GetVersionEx?
    Print (L"]");

    // TODO: hide further info behing command-line parameters?
    /*Print (L"\r\n\r\nLicensed to:"); // TODO: show if at least one
    PrintValueFromRegistry (L"RegisteredOwner", L"\r\n    ");
    PrintValueFromRegistry (L"RegisteredOrganization", L"\r\n    ");
    // */

    // TODO: hypervisor info?
    // TODO: licensing and expiration status
    // TODO: ?? memory size and OS bitness (3GT)
    // TODO: ?? processors and levels, architectures, SSE,AVX,etc...
    // TODO: ?? supported architectures?

    // RegCloseKey (hKey);
    Print (L"\r\n");
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

void PrintNumber (DWORD number, int base = 10) {
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
    Print (L".");
    PrintNumber (minor);
    Print (L".");
    ResetTextColor ();
    PrintNumber (build);

    DWORD UBR = 0;
    DWORD size = sizeof UBR;
    if (RegQueryValueEx (hKey, L"UBR", NULL, NULL, (LPBYTE) &UBR, &size) == ERROR_SUCCESS) {
        Print (L".");
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
        case FILE_DEVICE_UNKNOWN: // socket/tape
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

WORD GetConsoleColors () {
    if (!file) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (GetConsoleScreenBufferInfo (out, &info)) {
            return info.wAttributes;
        }
    }
    return 0;
}
void SetTextColor (WORD fg) {
    fg &= 0x000F;
    fg |= (colors & 0xFFF0);
    if (fg != colors && !file) {
        SetConsoleTextAttribute (out, fg);
    }
}
void ResetTextColor () {
    if (!file) {
        SetConsoleTextAttribute (out, colors);
    }
}
