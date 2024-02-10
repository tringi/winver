#include <Windows.h>
#include <cstddef>
#include <charconv>
#include <string_view>

#include "lib/Windows_Symbol.hpp"

void Print (wchar_t c);
void Print (const wchar_t * text);
void Print (const wchar_t * text, std::size_t length);
void PrintRsrc (unsigned int);
void PrintNumber (DWORD number, int base, std::size_t digits);
void PrintNumber (DWORD number) { PrintNumber (number, 10, 0); };
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
std::wstring_view args;

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

    Print (L"\r\n");
    
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

    Print (L" [");
    PrintRsrc (2);
    SetTextColor (15);
    if (PrintValueFromRegistry (L"DisplayVersion") || PrintValueFromRegistry (L"ReleaseId")) {
        Print (L' ');
    }
    ResetTextColor ();
    ShowVersionNumbers ();
    PrintValueFromRegistry (L"CSDVersion", L" "); // TODO: prefer GetVersionEx?
    Print (L"]\r\n");

    if (IsOptionPresent (L'b')) {
        SetTextColor (8);
        if (PrintValueFromRegistry (L"BuildLabEx") || PrintValueFromRegistry (L"BuildLab")) {
            Print (L"\r\n");
        }
        ResetTextColor ();
    }

    if (IsOptionPresent (L'o')) {
        PrintUserInformation ();
    }

    if (IsOptionPresent (L'u')) {
        ULONGLONG t = 0;
        ULONGLONG (WINAPI * ptrGetTickCount64) () = NULL;
        if (Windows::Symbol (GetModuleHandle (L"KERNEL32"), ptrGetTickCount64, "GetTickCount64")) {
            t = ptrGetTickCount64 ();
        } else {
            t = GetTickCount ();
        }

        t /= 1000u;
        auto ss = t % 60u;
        t /= 60u;
        auto mm = t % 60u;
        t /= 60u;
        auto hh = t % 24u;
        t /= 24u;
        auto dd = t % 365u;
        t /= 365u;

        PrintRsrc (5);
        if (t) {
            PrintNumber (t);
            PrintRsrc (6); // y
        }
        if (dd) {
            PrintNumber (dd);
            PrintRsrc (7); // d
        }
        PrintNumber (hh, 10, 2);
        Print (L':');
        PrintNumber (mm, 10, 2);
        Print (L':');
        PrintNumber (ss, 10, 2);
        Print (L"\r\n");
    }

    if (IsOptionPresent (L'c')) {
        // CPUID, arch, SSE,AVX,etc...
    }

    if (IsOptionPresent (L'm')) {
        // 32b/3GT/44b/48b/57b
        // 32 GB of Physical Memory
        // 8 GB of Hyper-V Maximum Assigned Memory
    }

    // TODO: hypervisor info?
    // TODO: licensing and expiration status
    // TODO: ?? memory size and OS bitness (3GT)
    // TODO: ?? processors and levels, architectures, SSE,AVX,etc...
    // TODO: ?? supported architectures?
    // TODO: ?? disk size and free space

    // RegCloseKey (hKey);
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
            SetTextColor (11);
            PrintRsrc (3);
            ResetTextColor ();
            if (have_owner) {
                PrintRsrc (4);
                Print (owner, owner_size / sizeof (wchar_t));
                Print (L"\r\n");
            }
            if (have_organization) {
                PrintRsrc (4);
                Print (organization, organization_size / sizeof (wchar_t));
                Print (L"\r\n");
            }
        }
    }
}

void PrintNumber (DWORD number, int base, std::size_t digits) {
    char a [32u];

    if (auto [end, error] = std::to_chars (&a [0], &a [sizeof a], number, base); error == std::errc ()) {
        auto n = end - a;
        auto prefix = (digits > n) ? (digits - n) : 0;
        if (prefix + n > sizeof a) {
            prefix = sizeof a - n;
        }

        wchar_t w [sizeof a];
        for (std::size_t i = 0; i != prefix; ++i) {
            w [i] = L'0';
        }
        for (std::size_t i = 0; i != n; ++i) {
            w [i + prefix] = a [i];
        }
        Print (w, n + prefix);
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
        args = cmdline;
    }
}

bool IsOptionPresent (wchar_t c) {
    return args.contains (c) || args.contains (L'a');
}
