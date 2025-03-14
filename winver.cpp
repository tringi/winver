#include <Windows.h>
#include <slpublic.h>

#include <cstddef>
#include <charconv>
#include <string_view>

#include "lib/Windows_Symbol.hpp"

#pragma warning(disable: 28159)

void Print (wchar_t c);
void Print (const wchar_t * text);
void Print (const wchar_t * text, std::size_t length);
void Print (const char * text);
void PrintRsrc (unsigned int);
template <typename T>
void PrintNumber (T number, int base, int digits);
template <typename T>
void PrintNumber (T number) { PrintNumber (number, 10, 0); };
void PrintElapse (std::uint64_t value, bool seconds);
void InitPrint ();
void InitArguments ();
UCHAR GetConsoleColors ();
void SetTextColor (unsigned char);
void ResetTextColor ();
bool IsOptionPresent (wchar_t c);

template <std::size_t N>
std::size_t Convert (const char * in, _Out_writes_z_ (N) wchar_t (&out) [N]) {
    return MultiByteToWideChar (CP_ACP, 0, in, -1, out, N);
}

HKEY hKey = NULL;
HANDLE out = NULL;
HMODULE hKernel32 = NULL;
bool  file = false;
UCHAR colors = 0;
DWORD major = 0;
DWORD minor = 0;
DWORD build = 0;
std::wstring_view args;

bool ShowBrandingFromAPI ();
void ShowVersionNumbers ();
bool PrintValueFromRegistry (const char * value, char prefix = 0);
void PrintUserInformation ();
void PrintOsArchitecture ();
void PrintLicenseStatus ();
void PrintExpiration ();

extern "C" IMAGE_DOS_HEADER __ImageBase;
extern "C" void WINAPI RtlGetNtVersionNumbers (LPDWORD, LPDWORD, LPDWORD); // NTDLL 5.1

void InitVersionNumbers () {
#ifndef _WIN64
    RtlGetNtVersionNumbers (&major, &minor, &build);
#else
    void (WINAPI * ptrRtlGetNtVersionNumbers) (LPDWORD, LPDWORD, LPDWORD) = NULL;
    if (Windows::Symbol (GetModuleHandleA ("NTDLL"), ptrRtlGetNtVersionNumbers, "RtlGetNtVersionNumbers")) {
        ptrRtlGetNtVersionNumbers (&major, &minor, &build);
    } else {
        OSVERSIONINFO os;
        os.dwOSVersionInfoSize = sizeof os;
        if (GetVersionEx (&os)) {
            major = os.dwMajorVersion;
            minor = os.dwMinorVersion;
            build = os.dwBuildNumber;
        }
    }
#endif
    // NT 10+
    // auto NtBuildNumber = *(const ULONG *) 0x7ffe0260;
}

// Entry Point

__declspec (noreturn) void main () {
    InitPrint ();
    InitArguments ();
    InitVersionNumbers ();
    RegOpenKeyExA (HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey);

    build &= 0x0FFF'FFFF;
    colors = GetConsoleColors ();
    hKernel32 = GetModuleHandleA ("KERNEL32");

    // winver.com
    // winver.com -a

    if (args.empty () || args.contains (L'a')) {
        Print ("\r\n");

        // Windows 11 Pro Insider Preview
        // Windows 10 Enterprise N 2016 LTSB
        // Windows Server 2022 Datacenter
        // Hyper-V Server
        // Azure Stack HCI

        if (major < 10 || !ShowBrandingFromAPI ()) {
            if (!PrintValueFromRegistry ("ProductName")) {
                PrintRsrc (1);
            }
        }

        // [Version 22H2 Major.Minor.Build.UBR]

        Print (" [");
        PrintRsrc (2);
        SetTextColor (15);
        if (PrintValueFromRegistry ("DisplayVersion") || PrintValueFromRegistry ("ReleaseId")) {
            Print (L' ');
        }
        ResetTextColor ();
        ShowVersionNumbers ();
        PrintValueFromRegistry ("CSDVersion", ' ');

#ifdef _M_ARM64
        Print ("] ARM-64\r\n");
#else
        Print ("] ");
        PrintOsArchitecture ();
        Print ("\r\n");
#endif
    }

    // winver.com -b
    //  - 14393.6611.amd64fre.rs1_release.231218-1733

    if (IsOptionPresent (L'b')) {
        SetTextColor (8);
        if (PrintValueFromRegistry ("BuildLabEx") || PrintValueFromRegistry ("BuildLab")) {
            Print ("\r\n");
        }
        ResetTextColor ();
    }

    // winver.com -u
    //  - Uptime: 1y 123d 23:02:59

    if (IsOptionPresent (L'u')) {
#ifdef _M_ARM64
        ULONGLONG t = GetTickCount64 ();
#else
        ULONGLONG t = 0;
        ULONGLONG (WINAPI * ptrGetTickCount64) () = NULL;
        if (Windows::Symbol (hKernel32, ptrGetTickCount64, "GetTickCount64")) {
            t = ptrGetTickCount64 ();
        } else {
            t = GetTickCount ();
        }

#endif
        t /= 1000u;
        PrintRsrc (5);
        PrintElapse (t, true);
    }

    // winver.com -l
    //  - Expires 23.12.2025 12:34
    //  - 

    if (IsOptionPresent (L'l')) {
        PrintExpiration ();
        PrintLicenseStatus ();
    }

    // winver.com -o
    //  - Licensed to: \n User name \n Company name

    if (IsOptionPresent (L'o')) {
        PrintUserInformation ();
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

    ExitProcess (0);
}

bool ShowBrandingFromAPI () {
    bool result = false;
    if (auto dll = LoadLibraryA ("WINBRAND")) {
        LPWSTR (WINAPI * ptrBrandingFormatString) (LPCWSTR) = NULL;
        if (Windows::Symbol (dll, ptrBrandingFormatString, "BrandingFormatString")) {
            if (auto text = ptrBrandingFormatString (L"%WINDOWS_LONG%")) {
                if (text [0]) {
                    Print (text);
                    result = true;
                }
            }
        }
    }
    return result;
}

std::size_t GetRegistryString (const char * avalue, wchar_t * buffer, std::size_t length) {
    if (hKey) {
        wchar_t value [256];
        Convert (avalue, value);

        DWORD size = length * sizeof (wchar_t);
        DWORD type = 0;
        if (RegQueryValueEx (hKey, value, NULL, &type, (LPBYTE) buffer, &size) == ERROR_SUCCESS) {
            switch (type) {
                case REG_SZ:
                    if (size > sizeof (wchar_t))
                        return size / sizeof (wchar_t);
            }
        }
    }
    return false;
}

bool PrintValueFromRegistry (const char * avalue, char prefix) {
    if (hKey) {
        wchar_t value [256];
        Convert (avalue, value);

        wchar_t text [512];
        DWORD size = sizeof text;
        DWORD type = 0;
        if (RegQueryValueEx (hKey, value, NULL, &type, (LPBYTE) text, &size) == ERROR_SUCCESS) {
            switch (type) {
                case REG_DWORD:
                case REG_QWORD:
                    if (prefix) {
                        Print (prefix);
                    }
                    PrintNumber (*(DWORD *) text);
                    break;

                case REG_SZ:
                    if (size > sizeof (wchar_t)) {
                        if (prefix) {
                            Print (prefix);
                        }
                        Print (text, size / sizeof (wchar_t) - 1);
                    }
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

        auto nowner = GetRegistryString ("RegisteredOwner", owner, 512);
        auto norganization = GetRegistryString ("RegisteredOrganization", organization, 512);

        if (nowner + norganization) {
            SetTextColor (11);
            PrintRsrc (3);
            ResetTextColor ();
            if (nowner) {
                PrintRsrc (4);
                Print (owner, nowner);
                Print ("\r\n");
            }
            if (norganization) {
                PrintRsrc (4);
                Print (organization, norganization);
                Print ("\r\n");
            }
        }
    }
}

template <typename T>
void PrintNumber (T number, int base, int digits) {
    char a [28u];

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

UINT GetFileBuildNumber (const char * filename) {
    auto cb = GetFileVersionInfoSizeA (filename, NULL);
    if (auto data = LocalAlloc (LMEM_FIXED, cb)) {
        if (GetFileVersionInfoA (filename, 0, cb, data)) {

            VS_FIXEDFILEINFO * info = NULL;
            UINT infolen = sizeof info;
            if (VerQueryValueA (data, "\\", (LPVOID *) &info, &infolen)) {
                return LOWORD (info->dwProductVersionLS);
            }
        }
    }
    return 0;
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

    if (!PrintValueFromRegistry ("UBR", '.')) {
#ifndef _M_ARM64
        wchar_t text [128];
        if (GetRegistryString ("BuildLabEx", text, 128)) {
            if (auto pUBR = std::wcschr (text, L'.')) {
                if (auto pUBRend = std::wcschr (pUBR + 1, L'.')) {
                    *pUBRend = L'\0';
                }
                Print (pUBR);
            }
        } else {
            if (!PrintValueFromRegistry ("CSDBuildNumber", '.')) {
#ifndef _WIN64
                auto ubrKernel = GetFileBuildNumber ("NTOSKRNL.EXE");
                auto ubrUser = GetFileBuildNumber ("NTDLL");
                
                if (ubrKernel || ubrUser) {
                    Print (L'.');
                    if (ubrKernel > ubrUser) {
                        PrintNumber (ubrKernel);
                    } else {
                        PrintNumber (ubrUser);
                    }
                }
#endif
            }
        }
#endif
    }
}

#ifndef _M_ARM64
void PrintOsArchitecture () {
    BOOL (WINAPI * ptrIsWow64Process2) (HANDLE, USHORT *, USHORT *) = NULL;
    if (Windows::Symbol (hKernel32, ptrIsWow64Process2, "IsWow64Process2")) {
        USHORT process = 0;
        USHORT native = 0;

        if (ptrIsWow64Process2 ((HANDLE) -1, &process, &native)) {
            switch (native) {
                default:
#ifndef _WIN64                
                case IMAGE_FILE_MACHINE_I386:
                    Print ("32-bit");
                    break;
#endif
                case IMAGE_FILE_MACHINE_AMD64:
                    Print ("64-bit");
                    break;
                case IMAGE_FILE_MACHINE_ARM64:
                    Print ("ARM-64");
                    break;
            }
            return;
        }
    }

#ifdef _WIN64
    Print ("64-bit");
#else
    BOOL wow = FALSE;
    BOOL (WINAPI * ptrIsWow64Process) (HANDLE, BOOL *) = NULL;
    if (Windows::Symbol (hKernel32, ptrIsWow64Process, "IsWow64Process")) {
        if (ptrIsWow64Process ((HANDLE) -1, &wow) && wow) {
            Print ("64-bit");
            return;
        }
    }

    Print ("32-bit");
#endif
}
#endif

void PrintExpiration () {
    SYSTEMTIME st;
    FILETIME ft;
    if (*(const std::uint64_t *) 0x7ffe02c8) {
        if (FileTimeToLocalFileTime ((const FILETIME *) 0x7ffe02c8, &ft)) {
            if (FileTimeToSystemTime (&ft, &st)) {

                wchar_t buffer [48];
                if (auto length = GetDateFormatW (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, buffer, sizeof buffer / sizeof buffer [0])) {
                    PrintRsrc (8);
                    Print (buffer, length);
                    if (length = GetTimeFormatW (LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, buffer, sizeof buffer / sizeof buffer [0])) {
                        Print (L' ');
                        Print (buffer, length);
                    }
                    Print (L"\r\n");
                }
            }
        }
    }
}

void PrintLicenseStatus () {
    static const GUID WindowsGUID = { 0x55c92734, 0xd682, 0x4d71, 0x98, 0x3e, 0xd6, 0xec, 0x3f, 0x16, 0x05, 0x9f };

    HRESULT (WINAPI * ptrSLOpen) (_Out_ HSLC *) = NULL;
    HRESULT (WINAPI * ptrSLGetSLIDList) (_In_ HSLC, _In_ SLIDTYPE, _In_opt_ CONST SLID *, _In_ SLIDTYPE, _Out_ UINT * pnReturnIds, _Outptr_result_buffer_ (*pnReturnIds) SLID **) = NULL;
    HRESULT (WINAPI * ptrSLGenerateOfflineInstallationId) (_In_ HSLC, _In_ CONST SLID *, _Outptr_ PWSTR *) = NULL;
    HRESULT (WINAPI * ptrSLGetProductSkuInformation) (_In_ HSLC, _In_ CONST SLID *, _In_ PCWSTR, _Out_opt_ SLDATATYPE *, _Out_ UINT * pcbValue, _Outptr_result_bytebuffer_ (*pcbValue) PBYTE *) = NULL;
    HRESULT (WINAPI * ptrSLGetApplicationInformation) (_In_ HSLC, _In_ const SLID *, _In_ PCWSTR, _Out_opt_ SLDATATYPE *, _Out_ UINT * pcbValue, _Outptr_result_bytebuffer_ (*pcbValue) PBYTE *) = NULL;
    HRESULT (WINAPI * ptrSLGetLicensingStatusInformation) (_In_ HSLC, _In_opt_ CONST SLID *, _In_opt_ CONST SLID *, _In_opt_ PCWSTR, _Out_ UINT * pnStatusCount, _Outptr_result_buffer_ (*pnStatusCount) SL_LICENSING_STATUS **) = NULL;

    // Win8+

    if (auto hSlcDLL = LoadLibraryA ("SLC")) {
        if (Windows::Symbol (hSlcDLL, ptrSLOpen, "SLOpen")) {

            HSLC hSlc;
            if (SUCCEEDED (ptrSLOpen (&hSlc))) {

                if (Windows::Symbol (hSlcDLL, ptrSLGetSLIDList, "SLGetSLIDList")) {

                    UINT nProductSKUs = 0;
                    SLID * pProductSKUs = NULL;

                    if (SUCCEEDED (ptrSLGetSLIDList (hSlc, SL_ID_PRODUCT_SKU, NULL, SL_ID_PRODUCT_SKU, &nProductSKUs, &pProductSKUs))) {

                        for (UINT iProductSKUs = 0; iProductSKUs != nProductSKUs; ++iProductSKUs) {
                            const auto pSkuId = &pProductSKUs [iProductSKUs];

                            if (Windows::Symbol (hSlcDLL, ptrSLGenerateOfflineInstallationId, "SLGenerateOfflineInstallationId")) {
                                PWSTR pIID = NULL;
                                if (SUCCEEDED (ptrSLGenerateOfflineInstallationId (hSlc, pSkuId, &pIID))) {

                                    UINT nAppIds = 0;
                                    SLID * pAppIds;
                                    if (SUCCEEDED (ptrSLGetSLIDList (hSlc, SL_ID_PRODUCT_SKU, pSkuId, SL_ID_APPLICATION, &nAppIds, &pAppIds))) {

                                        for (UINT iAppIds = 0; iAppIds != nAppIds; ++iAppIds) {
                                            SLID * pAppId = &pAppIds [iAppIds];

                                            if (IsEqualGUID (*pAppId, WindowsGUID)) {
                                                
                                                UINT size;
                                                BYTE * data;
                                                SLDATATYPE type;

                                                if (Windows::Symbol (hSlcDLL, ptrSLGetLicensingStatusInformation, "SLGetLicensingStatusInformation")) {

                                                    UINT nLicensingStatus;
                                                    SL_LICENSING_STATUS * pLicensingStatus;

                                                    if (SUCCEEDED (ptrSLGetLicensingStatusInformation (hSlc, pAppId, pSkuId, NULL, &nLicensingStatus, &pLicensingStatus))) {

                                                        for (UINT iLicensingStatus = 0; iLicensingStatus != nLicensingStatus; ++iLicensingStatus) {
                                                            SL_LICENSING_STATUS * thisLicensingStatus = &pLicensingStatus [iLicensingStatus];

                                                            PrintRsrc (9);
                                                            switch (thisLicensingStatus->eStatus) {
                                                                case SL_LICENSING_STATUS_UNLICENSED:
                                                                case SL_LICENSING_STATUS_LICENSED:
                                                                case SL_LICENSING_STATUS_IN_GRACE_PERIOD:
                                                                case SL_LICENSING_STATUS_NOTIFICATION:
                                                                case SL_LICENSING_STATUS_LAST:
                                                                    PrintRsrc (0x20 + (UINT) thisLicensingStatus->eStatus);
                                                                    break;
                                                                default:
                                                                    PrintNumber ((UINT) thisLicensingStatus->eStatus);
                                                            }
                                                            Print (L"\r\n");

                                                            if (thisLicensingStatus->dwTotalGraceDays) {
                                                                PrintRsrc (10);
                                                                PrintNumber (thisLicensingStatus->dwTotalGraceDays);
                                                                Print (L"\r\n");
                                                            }
                                                            if (thisLicensingStatus->dwGraceTime) {
                                                                PrintRsrc (11);
                                                                PrintElapse (thisLicensingStatus->dwGraceTime, false);
                                                            }
                                                        }
                                                    }
                                                }

                                                if (Windows::Symbol (hSlcDLL, ptrSLGetApplicationInformation, "SLGetApplicationInformation")) {
                                                    if (SUCCEEDED (ptrSLGetApplicationInformation (hSlc, pAppId, L"RemainingRearmCount", &type, &size, &data))) {
                                                        if (type == SL_DATA_DWORD) {
                                                            PrintRsrc (12);
                                                            PrintNumber (*(DWORD *) data);
                                                            Print (L"\r\n");
                                                        }
                                                    }
                                                }

                                                return;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Vista/7

#ifndef _M_ARM64
    if (auto hSlwgaDLL = LoadLibraryA ("SLWGA")) {
        HRESULT (WINAPI * ptrSLIsGenuineLocal) (_In_ CONST SLID *, _Out_ SL_GENUINE_STATE *, _Inout_opt_ SL_NONGENUINE_UI_OPTIONS *) = NULL;

        SL_LICENSING_STATUS status {};
        if (Windows::Symbol (hSlwgaDLL, ptrSLIsGenuineLocal, "SLIsGenuineLocal")) {

            SL_GENUINE_STATE state {};
            if (SUCCEEDED (ptrSLIsGenuineLocal (&WindowsGUID, &state, NULL))) {

                PrintRsrc (9);
                switch (state) {
                    case SL_GEN_STATE_IS_GENUINE:
                    case SL_GEN_STATE_INVALID_LICENSE:
                    case SL_GEN_STATE_TAMPERED:
                    case SL_GEN_STATE_OFFLINE:
                        PrintRsrc (0x10 + (UINT) state);
                        break;
                    default:
                        PrintNumber ((UINT) state);
                }
                Print (L"\r\n");
            }
        }
    }
#endif
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

void Print (const char * text) {
    wchar_t buffer [3072];
    Convert (text, buffer); // TODO: lenghht
    return Print (buffer);
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

void PrintElapse (std::uint64_t t, bool seconds) {
    DWORD ss = 0;
    if (seconds) {
        ss = t % 60u;
        t /= 60u;
    }
    DWORD mm = t % 60u;
    t /= 60u;
    DWORD hh = t % 24u;
    t /= 24u;
    DWORD dd = t % 365u;
    t /= 365u;

    if (t) {
        PrintNumber ((DWORD) t);
        PrintRsrc (6); // y
    }
    if (dd) {
        PrintNumber (dd);
        PrintRsrc (7); // d
    }
    PrintNumber (hh, 10, 2);
    Print (L':');
    PrintNumber (mm, 10, 2);
    if (seconds) {
        Print (L':');
        PrintNumber (ss, 10, 2);
    }
    Print ("\r\n");
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
