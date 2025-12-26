#include <Windows.h>
#include <winevt.h>
#include <slpublic.h>
#include <intrin.h>

#include <charconv>
#include <cstddef>
#include <span>

#include "lib/Windows_Symbol.hpp"

#pragma warning(disable: 4326) // main should return int, not void
#pragma warning(disable: 4996) // GetVersionEx is deprecated
#pragma warning(disable: 28159) // GetVersionEx is deprecated

void Print (wchar_t c);
void Print (const wchar_t * text);
void Print (const wchar_t * text, std::size_t length);
void Print (const char * text);
void PrintNewline ();
void PrintRsrc (unsigned int);
template <typename T>
void PrintNumber (T number, int base, int digits);
template <typename T>
void PrintNumber (T number) { PrintNumber (number, 10, 0); };
void PrintElapse (std::uint64_t value, bool seconds);
void InitPrint ();
bool InitEvtAPI ();
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
HKEY hVmKey = NULL;
HANDLE out = NULL;
HMODULE hKernel32 = NULL;
bool  file = false;
UCHAR colors = 0;
USHORT native = 0;
DWORD major = 0;
DWORD minor = 0;
DWORD build = 0;

wchar_t ** argv;
int argc;

bool ShowBrandingFromAPI ();
void ShowVersionNumbers ();
bool PrintValueFromRegistry (const char * value, char prefix = 0);
bool PrintValueFromRegistry (HKEY hKey, const char * value, char prefix = 0);
bool PrintUserInformation ();
void PrintSupportedLanguages ();
void PrintSupportedArchitectures ();
void PrintOsArchitecture ();
void PrintLicenseStatus ();
void PrintExpiration ();
bool PrintSecureKernel ();
bool PrintSecureBoot ();
void PrintIsolationInfo ();
void PrintHypervisorInfo ();

extern "C" IMAGE_DOS_HEADER __ImageBase;
extern "C" void WINAPI RtlGetNtVersionNumbers (LPDWORD, LPDWORD, LPDWORD); // NTDLL 5.1
extern "C" void __wgetmainargs (int *, wchar_t ***, wchar_t ***, int, int *);

void InitVersionNumbers () {
#ifdef _WIN64
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

    DWORD dwRegFlags = KEY_QUERY_VALUE;
#ifndef _WIN64
    if (major > 5 || (major == 5 && minor >= 2)) {
        dwRegFlags |= KEY_WOW64_64KEY;
    }
#endif
    RegOpenKeyExA (HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, dwRegFlags, &hKey);
    RegOpenKeyExA (HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Virtual Machine\\Guest\\Parameters", 0, dwRegFlags, &hVmKey);

    build &= 0x0FFF'FFFF;
    colors = GetConsoleColors ();
    hKernel32 = GetModuleHandleA ("KERNEL32");

    // winver.com
    // winver.com -a

    if (argc <= 1 || IsOptionPresent ('a')) {
        PrintNewline ();

        // Windows 11 Pro Insider Preview
        // Windows 10 Enterprise N 2016 LTSB
        // Windows Server 2022 Datacenter
        // Hyper-V Server
        // Azure Stack HCI

        if (major < 10 || !ShowBrandingFromAPI ()) {
            if (!PrintValueFromRegistry ("ProductName")) {
                Print ("Windows");

                OSVERSIONINFOEX os;
                os.dwOSVersionInfoSize = sizeof os;
                if (GetVersionEx ((OSVERSIONINFO *) &os)) {
                    switch (os.dwPlatformId) {
                        default:
                        case VER_PLATFORM_WIN32_WINDOWS:
                            // 9x
                            Print (" 9x");
                            break;
                        case VER_PLATFORM_WIN32_NT:
                            // NT
                            Print (" NT");
                            switch (os.wProductType) {
                                case VER_NT_SERVER:
                                case VER_NT_DOMAIN_CONTROLLER:
                                    Print (" Server");
                                    break;
                            }
                            break;
                    }
                }
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
        native = IMAGE_FILE_MACHINE_ARM64;
#else
        Print ("] ");
        PrintOsArchitecture ();
        PrintNewline ();
#endif
    }

    bool extraNL = false;

    // winver.com -b
    //  - 14393.6611.amd64fre.rs1_release.231218-1733

    if (IsOptionPresent (L'b')) {
        SetTextColor (8);
        if (PrintValueFromRegistry ("BuildLabEx") || PrintValueFromRegistry ("BuildLab")) {
            PrintNewline ();
            extraNL = true;
        }
        ResetTextColor ();
    }

    // winver.com -s
    //  - Secure Kernel

    if (IsOptionPresent (L's')) {
        if (PrintSecureKernel ()) {
            extraNL = true;
        }
    }

    if (extraNL) {
        extraNL = false;
        if (IsOptionPresent (L'a')) {
            PrintNewline ();
        }
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
    //  - License: Activated
    //  - Remaining reasm count: 1001

    if (IsOptionPresent (L'l')) {
        PrintExpiration ();
        PrintLicenseStatus ();
        PrintNewline ();
    }

    // winver.com -o
    //  - Licensed to: \n User name \n Company name

    if (IsOptionPresent (L'o')) {
        if (PrintUserInformation ()) {
            PrintNewline ();
        }
    }

    // winver.com -n
    //  - Supported languages: cs-CZ, en-US

    if (IsOptionPresent (L'n')) {
        PrintSupportedLanguages ();
    }

    // winver.com -i
    //  - Architectures supported: AArch64, AArch32, x86-64, x86-32

    if (IsOptionPresent (L'i')) {
        PrintSupportedArchitectures ();
    }

    // winver.com -s
    //  - Secure Boot

    if (IsOptionPresent (L's')) {
        if (!PrintSecureBoot () && (major >= 6)) {
            PrintRsrc (0x52);
            SetTextColor (14);
            PrintRsrc (0x53);
            ResetTextColor ();
            PrintNewline ();
        }
    }

    // winver.com -h
    //  - No Hyper-V virtualization
    //  - Hyper-V Primary partition
    //  - Virtualized. Parent Hyper-V partition: 

    if (IsOptionPresent (L'h')) {
        PrintIsolationInfo ();
        PrintHypervisorInfo ();
    }

    if (IsOptionPresent (L'c')) {
        // CPUID, arch, SSE,AVX,etc...
    }

    if (IsOptionPresent (L'm')) {
        // 32b/3GT/44b/48b/57b
        // 32 GB of Physical Memory
        // 8 GB of Hyper-V Maximum Assigned Memory
    }

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

        DWORD size = (DWORD) length * sizeof (wchar_t);
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

bool PrintValueFromRegistry (HKEY h, const char * avalue, char prefix) {
    if (h) {
        wchar_t value [256];
        Convert (avalue, value);

        wchar_t text [512];
        DWORD size = sizeof text;
        DWORD type = 0;
        if (RegQueryValueEx (h, value, NULL, &type, (LPBYTE) text, &size) == ERROR_SUCCESS) {
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

bool PrintValueFromRegistry (const char * avalue, char prefix) {
    return PrintValueFromRegistry (hKey, avalue, prefix);
}

bool PrintUserInformation () {
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
                PrintNewline ();
            }
            if (norganization) {
                PrintRsrc (4);
                Print (organization, norganization);
                PrintNewline ();
            }
            return true;
        }
    }
    return false;
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

BOOL (APIENTRY * ptrGetFileVersionInfoA) (_In_ LPCSTR, _Reserved_ DWORD, _In_ DWORD, _Out_writes_bytes_ (dwLen) LPVOID) = NULL;
BOOL (APIENTRY * ptrVerQueryValueA) (_In_ LPCVOID, _In_ LPCSTR, LPVOID *, _Out_ PUINT) = NULL;

bool InitVersionAPI () {
    if (ptrVerQueryValueA)
        return true;

    if (auto hVersionDLL = LoadLibraryA ("VERSION");
           Windows::Symbol (hVersionDLL, ptrGetFileVersionInfoA, "GetFileVersionInfoA")
        && Windows::Symbol (hVersionDLL, ptrVerQueryValueA, "VerQueryValueA")) {

        return true;
    } else
        return false;
}

bool GetFileFixedVersionInfo (const char * filename, VS_FIXEDFILEINFO * result) {
    static char data [3072];

    if (InitVersionAPI ()) {
        if (ptrGetFileVersionInfoA (filename, 0, sizeof data, data)) {

            VS_FIXEDFILEINFO * info = NULL;
            UINT infolen = sizeof info;

            if (ptrVerQueryValueA (data, "\\", (LPVOID *) &info, &infolen)) {
                memcpy (result, info, sizeof (VS_FIXEDFILEINFO));
                return true;
            }
        }
    }
    return 0;
}

UINT GetFileBuildNumber (const char * filename) {
    VS_FIXEDFILEINFO info;
    if (GetFileFixedVersionInfo (filename, &info)) {
        return LOWORD (info.dwProductVersionLS);
    } else
        return 0;
}

void ShowVersionNumbersBase (DWORD m, DWORD n, DWORD b) {
    if (m == 10 && n == 0) {
        SetTextColor (8);
    }
    PrintNumber (m);
    Print (L'.');
    PrintNumber (n);
    Print (L'.');
    ResetTextColor ();
    PrintNumber (b);
}

void ShowVersionNumbers () {
    ShowVersionNumbersBase (major, minor, build);
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

ULONGLONG ftBootTime = 0;

EVT_HANDLE (WINAPI * ptrEvtQuery) (_In_opt_ EVT_HANDLE, _In_opt_z_ LPCWSTR, _In_opt_z_ LPCWSTR, DWORD) = NULL;
EVT_HANDLE (WINAPI * ptrEvtCreateRenderContext) (DWORD, LPCWSTR *, DWORD) = NULL;
BOOL (WINAPI * ptrEvtClose) (_In_ _Post_invalid_ EVT_HANDLE) = NULL;
BOOL (WINAPI * ptrEvtNext) (_In_ EVT_HANDLE, DWORD, PEVT_HANDLE, DWORD, DWORD, PDWORD) = NULL;
BOOL (WINAPI * ptrEvtRender) (_In_opt_ EVT_HANDLE, _In_ EVT_HANDLE, DWORD, DWORD, PVOID, _Out_ PDWORD, _Out_ PDWORD) = NULL;

template <typename T>
bool QueryEvtValue (const char * provider, unsigned int event, const char * name, T * out);

bool InitEvtAPI () {
    if (ptrEvtCreateRenderContext)
        return true;

    if (auto hEvtDLL = LoadLibraryA ("WEVTAPI");
           Windows::Symbol (hEvtDLL, ptrEvtQuery, "EvtQuery")
        && Windows::Symbol (hEvtDLL, ptrEvtClose, "EvtClose")
        && Windows::Symbol (hEvtDLL, ptrEvtNext,  "EvtNext")
        && Windows::Symbol (hEvtDLL, ptrEvtRender,"EvtRender")
        && Windows::Symbol (hEvtDLL, ptrEvtCreateRenderContext, "EvtCreateRenderContext")) {

        QueryEvtValue ("Kernel-General", 12, "StartTime", &ftBootTime);
        return true;
    } else
        return false;
}

template <typename T>
bool GetEvtValue (EVT_HANDLE hEvent, const wchar_t * query, T * value) {
    bool result = false;
    if (auto hContext = ptrEvtCreateRenderContext (1, &query, EvtRenderContextValues)) {

        DWORD n = 0;
        DWORD wtn = 0;
        EVT_VARIANT variant;

        if (ptrEvtRender (hContext, hEvent, EvtRenderEventValues, sizeof variant, &variant, &wtn, &n) && (n != 0)) {

            result = true;
            switch (variant.Type) {
                default:
                    result = false;
                    break;

                case EvtVarTypeBoolean: *value = (T) variant.BooleanVal; break;

                case EvtVarTypeSByte: *value = (T) variant.SByteVal; break;
                case EvtVarTypeInt16: *value = (T) variant.Int16Val; break;
                case EvtVarTypeInt32: *value = (T) variant.Int32Val; break;
                case EvtVarTypeInt64: *value = (T) variant.Int64Val; break;

                case EvtVarTypeByte: *value = (T) variant.ByteVal; break;
                case EvtVarTypeUInt16: *value = (T) variant.UInt16Val; break;
                case EvtVarTypeUInt32: *value = (T) variant.UInt32Val; break;
                case EvtVarTypeUInt64: *value = (T) variant.UInt64Val; break;
                
                case EvtVarTypeHexInt32: *value = (T) variant.Int32Val; break;
                case EvtVarTypeHexInt64: *value = (T) variant.Int64Val; break;

                case EvtVarTypeFileTime:
                    if constexpr (std::is_same_v <T, FILETIME> || std::is_same_v <T, ULONGLONG>) {
                        memcpy (value, &variant.FileTimeVal, sizeof (FILETIME));
                    } else {
                        result = false;
                    }
                    break;
            }
        }
        ptrEvtClose (hContext);
    }
    return result;
}

bool BuildEvtQuery (const char * provider, unsigned int event, wchar_t * buffer, std::size_t length) {
    return _snwprintf (buffer, length, L"Event/System[EventID=%u and Provider[@Name=\"Microsoft-Windows-%S\"]]", event, provider) > 0;
}
bool BuildEvtDataNameXPath (const char * name, wchar_t * buffer, std::size_t length) {
    return _snwprintf (buffer, length, L"Event/EventData/Data[@Name=\"%S\"]", name);
}

template <typename T>
bool QueryEvtValue (const char * provider, unsigned int event, const char * name, T * out) {

    bool result = false;
    if (InitEvtAPI ()) {

        wchar_t query [128];
        BuildEvtQuery (provider, event, query, sizeof query / sizeof query [0]);

        if (auto hResult = ptrEvtQuery (NULL, L"System", query, EvtQueryChannelPath | EvtQueryReverseDirection)) {
            DWORD n = 0;
            EVT_HANDLE hEvent;

            if (ptrEvtNext (hResult, 1, &hEvent, INFINITE, 0, &n) && n) {

                ULONGLONG t = 0;
                if (ftBootTime) {
                    GetEvtValue (hEvent, L"Event/System/TimeCreated/@SystemTime", &t);
                }
                if (t >= ftBootTime) {
                    wchar_t path [96];
                    BuildEvtDataNameXPath (name, path, sizeof path / sizeof path [0]);
                    result = GetEvtValue (hEvent, path, out);
                }
                ptrEvtClose (hEvent);
            }
            ptrEvtClose (hResult);
        }
    }
    return result;
}

#ifndef _M_ARM64
void PrintOsArchitecture () {
    BOOL (WINAPI * ptrIsWow64Process2) (HANDLE, USHORT *, USHORT *) = NULL;
    if (Windows::Symbol (hKernel32, ptrIsWow64Process2, "IsWow64Process2")) {
        USHORT process = 0;
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
    native = IMAGE_FILE_MACHINE_AMD64;
#else
    BOOL wow = FALSE;
    BOOL (WINAPI * ptrIsWow64Process) (HANDLE, BOOL *) = NULL;
    if (Windows::Symbol (hKernel32, ptrIsWow64Process, "IsWow64Process")) {
        if (ptrIsWow64Process ((HANDLE) -1, &wow) && wow) {
            Print ("64-bit");
            native = IMAGE_FILE_MACHINE_AMD64;
            return;
        }
    }

    Print ("32-bit");
    native = IMAGE_FILE_MACHINE_I386;
#endif
}
#endif

void PrintSupportedLanguages () {
    struct Args {
        wchar_t user [30];
        wchar_t system [30];
        unsigned i;
        unsigned n;
    } args = {};

#ifndef _M_ARM64
    int (WINAPI * ptrGetUserDefaultLocaleName) (_Out_writes_ (cchLocaleName) LPWSTR lpLocaleName, _In_ int cchLocaleName) = NULL;
    if (Windows::Symbol (hKernel32, ptrGetUserDefaultLocaleName, "GetUserDefaultLocaleName")) {
        ptrGetUserDefaultLocaleName (args.user, sizeof args.user / sizeof args.user [0]);
    }
    int (WINAPI * ptrGetSystemDefaultLocaleName) (_Out_writes_ (cchLocaleName) LPWSTR lpLocaleName, _In_ int cchLocaleName) = NULL;
    if (Windows::Symbol (hKernel32, ptrGetSystemDefaultLocaleName, "GetSystemDefaultLocaleName")) {
        ptrGetSystemDefaultLocaleName (args.system, sizeof args.system / sizeof args.system [0]);
    }
#else
    GetUserDefaultLocaleName (args.user, sizeof args.user / sizeof args.user [0]);
    GetSystemDefaultLocaleName (args.system, sizeof args.system / sizeof args.system [0]);
#endif

#ifndef _WIN64
    BOOL (WINAPI * ptrEnumUILanguagesW) (_In_ UILANGUAGE_ENUMPROCW lpUILanguageEnumProc,
                                         _In_ DWORD dwFlags, _In_ LONG_PTR lParam);
    if (Windows::Symbol (hKernel32, ptrEnumUILanguagesW, "EnumUILanguagesW")) {
#else
        auto ptrEnumUILanguagesW = EnumUILanguagesW;
#endif
        ptrEnumUILanguagesW ([] (LPWSTR name, LONG_PTR lParam) {
                                    ++((Args *) lParam)->n;
                                    return TRUE;
                             }, MUI_LANGUAGE_NAME, (LONG_PTR) &args);

        if (args.n) {
            PrintRsrc (14);
            ptrEnumUILanguagesW ([] (LPWSTR name, LONG_PTR lParam) {
                                        auto & args = *(Args *) lParam;
                                        if (args.i++) {
                                            Print (", ");
                                        }
                                        if (!file) {
                                            if (std::wcscmp (name, args.user) == 0) {
                                                SetTextColor (11);
                                            } else
                                            if (std::wcscmp (name, args.system) == 0 && args.n >= 3) {
                                                SetTextColor (15);
                                            }
                                        }

                                        Print (name);

                                        if (file) {
                                            if (std::wcscmp (name, args.user) == 0) {
                                                PrintRsrc (15);
                                            }
                                            if (std::wcscmp (name, args.system) == 0) {
                                                PrintRsrc (16);
                                            }
                                        }

                                        ResetTextColor ();
                                        return TRUE;
                                 }, MUI_LANGUAGE_NAME, (LONG_PTR) &args);
            PrintNewline ();
        }
#ifndef _WIN64
    }
#endif
}

void PrintSupportedArchitectures () {
    typedef enum _SYSTEM_INFORMATION_CLASS {
        SystemSupportedProcessorArchitectures = 181,
        SystemSupportedProcessorArchitectures2 = 230, // Not 14393, yes 22543
    } SYSTEM_INFORMATION_CLASS;

    NTSTATUS (NTAPI * ptrNtQuerySystemInformationEx) (_In_ SYSTEM_INFORMATION_CLASS,
                                                      _In_reads_bytes_ (InputBufferLength) PVOID, _In_ ULONG InputBufferLength,
                                                      _Out_writes_bytes_opt_ (SystemInformationLength) PVOID, _In_ ULONG SystemInformationLength,
                                                      _Out_opt_ PULONG) = NULL;
    if (Windows::Symbol (GetModuleHandleA ("NTDLL"), ptrNtQuerySystemInformationEx, "NtQuerySystemInformationEx")) {
        
        typedef struct _SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION {
            DWORD Machine : 16;
            DWORD KernelMode : 1;
            DWORD UserMode : 1;
            DWORD Native : 1;
            DWORD Process : 1;
            DWORD WoW64Container : 1;
            DWORD ReservedZero0 : 11;
        } SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION;

        HANDLE process = (HANDLE) -1;
        SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines [24] = {};

        bool first = true;
        bool oldAPI = false;

        ULONG result = 0;
        if (ptrNtQuerySystemInformationEx (SystemSupportedProcessorArchitectures2, &process, sizeof (process), machines, sizeof (machines), &result) != ERROR_SUCCESS) {
            if (ptrNtQuerySystemInformationEx (SystemSupportedProcessorArchitectures, &process, sizeof (process), machines, sizeof (machines), &result) == ERROR_SUCCESS) {
                oldAPI = true;
            } else {
                result = 0;
            }
        }

        if (result) {
            const auto n = result / sizeof (SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION);
            std::qsort (machines, n, 4,
                        [] (const void * a, const void * b) -> int {
                            return ((const SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION *) a)->Machine
                                 < ((const SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION *) b)->Machine;
                        });

            for (auto machine : std::span <SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION> (machines, n)) {
                if (machine.Machine) {

                    if (first) {
                        PrintRsrc (13);
                    } else {
                        Print (", ");
                    }
                    first = false;

                    if (machine.Native) {
                        SetTextColor (11);
                    }
                    switch (machine.Machine) {
                        case IMAGE_FILE_MACHINE_I386: Print ("x86-32"); break;
                        case IMAGE_FILE_MACHINE_ARMNT: Print ("AArch32"); break;
                        case IMAGE_FILE_MACHINE_AMD64: Print ("x86-64"); break;
                        case IMAGE_FILE_MACHINE_ARM64: Print ("AArch64"); break;
                        case IMAGE_FILE_MACHINE_IA64: Print ("IA-64"); break;
                    }
                    if (machine.Native) {
                        ResetTextColor ();
                    }

                    bool wow64 = !!machine.WoW64Container;
                    bool emulated = !machine.Native;

                    if (oldAPI) {
                        // if (machine.Machine == IMAGE_FILE_MACHINE_IA64
                        wow64 = emulated;
                        emulated = false;
                    } else {
                        // do not consider 32b on 64b of the same architecture emulated
                        switch (machine.Machine) {
                            case IMAGE_FILE_MACHINE_I386:
                                if (native == IMAGE_FILE_MACHINE_AMD64) {
                                    emulated = false;
                                }
                                break;
                            case IMAGE_FILE_MACHINE_ARMNT:
                                if (native == IMAGE_FILE_MACHINE_ARM64) {
                                    emulated = false;
                                }
                                break;
                        }
                    }

                    if (wow64 || emulated) {
                        Print (" (");
                        if (wow64 && emulated) {
                            Print ("WoW64");
                            Print (", ");
                            Print ("emulated");
                        } else {
                            if (wow64) {
                                Print ("WoW64");
                            }
                            if (emulated) {
                                Print ("emulated");
                            }
                        }
                        Print (')');
                    }
                }
            }
            PrintNewline ();
            return;
        }
    }

#ifndef _M_ARM64
    PrintRsrc (13);

#ifdef _WIN64
    Print ("x86-64");
    Print (", ");
    Print ("x86-32");
    Print (" (");
    Print ("WoW64");
    Print (')');
#else
    BOOL wow = FALSE;
    BOOL (WINAPI * ptrIsWow64Process) (HANDLE, BOOL *) = NULL;
    if (Windows::Symbol (hKernel32, ptrIsWow64Process, "IsWow64Process")) {
        if (ptrIsWow64Process ((HANDLE) -1, &wow) && wow) {
            Print ("x86-64");
            Print (", ");
            Print ("x86-32");
            Print (" (");
            Print ("WoW64");
            Print (')');
        } else {
            Print ("x86-32");
        }
    } else {
        Print ("x86-32");
    }
#endif
    PrintNewline ();
#endif
}

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
                    PrintNewline ();
                }
            }
        }
    }
}

void PrintLicenseStatus () {
    static const GUID WindowsGUID = { 0x55c92734, 0xd682, 0x4d71, 0x98, 0x3e, 0xd6, 0xec, 0x3f, 0x16, 0x05, 0x9f };

    HRESULT (WINAPI * ptrSLOpen) (_Out_ HSLC *) = NULL;
    HRESULT (WINAPI * ptrSLGetSLIDList) (_In_ HSLC, _In_ SLIDTYPE, _In_opt_ CONST SLID *, _In_ SLIDTYPE,
                                         _Out_ UINT * pnReturnIds, _Outptr_result_buffer_ (*pnReturnIds) SLID **) = NULL;
    HRESULT (WINAPI * ptrSLGenerateOfflineInstallationId) (_In_ HSLC, _In_ CONST SLID *, _Outptr_ PWSTR *) = NULL;
    HRESULT (WINAPI * ptrSLGetProductSkuInformation) (_In_ HSLC, _In_ CONST SLID *, _In_ PCWSTR, _Out_opt_ SLDATATYPE *,
                                                      _Out_ UINT * pcbValue, _Outptr_result_bytebuffer_ (*pcbValue) PBYTE *) = NULL;
    HRESULT (WINAPI * ptrSLGetApplicationInformation) (_In_ HSLC, _In_ const SLID *, _In_ PCWSTR, _Out_opt_ SLDATATYPE *,
                                                       _Out_ UINT * pcbValue, _Outptr_result_bytebuffer_ (*pcbValue) PBYTE *) = NULL;
    HRESULT (WINAPI * ptrSLGetLicensingStatusInformation) (_In_ HSLC, _In_opt_ CONST SLID *, _In_opt_ CONST SLID *, _In_opt_ PCWSTR,
                                                           _Out_ UINT * pnStatusCount, _Outptr_result_buffer_ (*pnStatusCount) SL_LICENSING_STATUS **) = NULL;

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

                                                bool display_grace_info = true;

                                                if (Windows::Symbol (hSlcDLL, ptrSLGetLicensingStatusInformation, "SLGetLicensingStatusInformation")) {

                                                    UINT nLicensingStatus;
                                                    SL_LICENSING_STATUS * pLicensingStatus;

                                                    if (SUCCEEDED (ptrSLGetLicensingStatusInformation (hSlc, pAppId, pSkuId, NULL, &nLicensingStatus, &pLicensingStatus))) {

                                                        for (UINT iLicensingStatus = 0; iLicensingStatus != nLicensingStatus; ++iLicensingStatus) {
                                                            SL_LICENSING_STATUS * thisLicensingStatus = &pLicensingStatus [iLicensingStatus];

                                                            PrintRsrc (9);
                                                            switch (thisLicensingStatus->eStatus) {
                                                                case SL_LICENSING_STATUS_UNLICENSED: SetTextColor (12); break;
                                                                case SL_LICENSING_STATUS_LICENSED: SetTextColor (10); break;
                                                                case SL_LICENSING_STATUS_NOTIFICATION: SetTextColor (14); break;
                                                            }
                                                            switch (thisLicensingStatus->eStatus) {
                                                                case SL_LICENSING_STATUS_LICENSED:
                                                                    display_grace_info = false;
                                                                    [[ fallthrough ]];
                                                                case SL_LICENSING_STATUS_UNLICENSED:
                                                                case SL_LICENSING_STATUS_IN_GRACE_PERIOD:
                                                                case SL_LICENSING_STATUS_NOTIFICATION:
                                                                case SL_LICENSING_STATUS_LAST:
                                                                    PrintRsrc (0x20 + (UINT) thisLicensingStatus->eStatus);
                                                                    break;
                                                                default:
                                                                    PrintNumber ((UINT) thisLicensingStatus->eStatus);
                                                            }
                                                            ResetTextColor ();
                                                            PrintNewline ();

                                                            if (thisLicensingStatus->dwTotalGraceDays) {
                                                                PrintRsrc (10);
                                                                PrintNumber (thisLicensingStatus->dwTotalGraceDays);
                                                                PrintNewline ();
                                                            }
                                                            if (thisLicensingStatus->dwGraceTime) {
                                                                PrintRsrc (11);
                                                                PrintElapse (thisLicensingStatus->dwGraceTime, false);
                                                            }
                                                        }
                                                    }
                                                }

                                                if (display_grace_info) {
                                                    if (Windows::Symbol (hSlcDLL, ptrSLGetApplicationInformation, "SLGetApplicationInformation")) {
                                                        if (SUCCEEDED (ptrSLGetApplicationInformation (hSlc, pAppId, L"RemainingRearmCount", &type, &size, &data))) {
                                                            if (type == SL_DATA_DWORD) {
                                                                PrintRsrc (12);
                                                                PrintNumber (*(DWORD *) data);
                                                                PrintNewline ();
                                                            }
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
                    case SL_GEN_STATE_INVALID_LICENSE: SetTextColor (12); break;
                    case SL_GEN_STATE_IS_GENUINE: SetTextColor (10); break;
                }
                switch (state) {
                    case SL_GEN_STATE_IS_GENUINE:
                    case SL_GEN_STATE_INVALID_LICENSE:
                    case SL_GEN_STATE_TAMPERED:
                    case SL_GEN_STATE_OFFLINE:
                        PrintRsrc (0x2A + (UINT) state);
                        break;
                    default:
                        PrintNumber ((UINT) state);
                }
                ResetTextColor ();
                PrintNewline ();
            }
        }
    }
#endif
}

#ifndef _M_ARM64
struct HV_VENDOR_AND_MAX_FUNCTION {
    UINT MaxFunction;
    UCHAR VendorName [12];
};
struct HV_HYPERVISOR_VERSION_INFO {
    UINT BuildNumber;
    UINT MinorVersion : 16;
    UINT MajorVersion : 16;
    UINT ServicePack;
    UINT ServiceNumber : 24;
    UINT ServiceBranch : 8;
};

UINT IsHypervisorPresent (char * vendor, HV_HYPERVISOR_VERSION_INFO * info) {
    int registers [4];

    __cpuid (registers, 0);
    const auto maximum_eax = registers [0];
    if (maximum_eax >= 1u) {

        __cpuid (registers, 1);
        if (registers [2] & (1 << 31)) { // RAZ (Hyper-V)

            __cpuid (registers, 0x40000000);
            const auto maximum_hv = registers [0];

            // extract hypervisor name

            if (vendor) {
                const auto p = reinterpret_cast <HV_VENDOR_AND_MAX_FUNCTION *> (registers)->VendorName;
                for (auto i = 0u; i != sizeof HV_VENDOR_AND_MAX_FUNCTION::VendorName; ++i) {
                    if (i >= 32 && i < 127) {
                        *vendor++ = p [i];
                    }
                }
                *vendor = '\0';
            }

            // version

            if (maximum_hv >= 0x40000002) {
                __cpuid (registers, 0x40000002);
                *info = *reinterpret_cast <HV_HYPERVISOR_VERSION_INFO *> (registers);
            }

            // flags

            if (maximum_hv >= 0x40000003) {
                __cpuid (registers, 0x40000003);

                // privileges are 3fff or above on primary partition, 2e7f on limited VM (1607)

                if ((registers [0] & 0x3FFF) == 0x3FFF) {
                    return 1; // primary
                } else {
                    return 2;
                }
            }
        }
    }

    return 0;
}
#endif

void PrintHypervisorInfo () {
    const bool pfvirt = IsProcessorFeaturePresent (PF_VIRT_FIRMWARE_ENABLED);
    const bool pfslat = IsProcessorFeaturePresent (PF_SECOND_LEVEL_ADDRESS_TRANSLATION);

#ifndef _M_ARM64
    HV_HYPERVISOR_VERSION_INFO info;
    const UINT raz = IsHypervisorPresent (nullptr, &info);
#else
    const bool raz = false;
#endif

    PrintRsrc (0x30);

    if (raz || hVmKey != NULL || pfvirt || pfslat) {

        if (pfvirt || pfslat || raz) {
            if (hVmKey != NULL) {
                PrintRsrc (0x33);
            }
            if (pfvirt) {
                PrintRsrc (0x32);
                if (pfslat || raz) {
                    Print (", ");
                }
            }
            if (raz) {
                if (raz == 2) {
                    PrintRsrc (0x3B);
                } else {
                    PrintRsrc (0x3A);
                }
                if (pfslat) {
                    Print (", ");
                }
            }
            if (pfslat) {
                Print ("SLAT");
            }
        }

        PrintNewline ();

        if (hVmKey != NULL) {
            PrintRsrc (0x36);
            PrintValueFromRegistry (hVmKey, "PhysicalHostName"); // "PhysicalHostNameFullyQualified"
            PrintNewline ();
            PrintRsrc (0x34);
            PrintValueFromRegistry (hVmKey, "VirtualMachineName");
            PrintNewline ();
            PrintRsrc (0x35);
            PrintValueFromRegistry (hVmKey, "VirtualMachineId");
            PrintNewline ();

            if (!raz) {
                PrintRsrc (4);
                PrintRsrc (0x37);
                PrintValueFromRegistry (hVmKey, "HypervisorMajorVersion");
                PrintValueFromRegistry (hVmKey, "HypervisorMinorVersion", '.');
                PrintValueFromRegistry (hVmKey, "HypervisorBuildNumber", '.');
                PrintValueFromRegistry (hVmKey, "HypervisorServiceNumber", '.');
                PrintRsrc (0x38);
                PrintValueFromRegistry (hVmKey, "HypervisorServicePack");
                PrintRsrc (0x39);
                PrintValueFromRegistry (hVmKey, "HypervisorServiceBranch");
                PrintNewline ();
            }
        }

        DWORD type = ~0;
        if (QueryEvtValue ("Hyper-V-Hypervisor", 2, "SchedulerType", &type)) {
            if (hVmKey != NULL) {
                PrintRsrc (4);
            }
            PrintRsrc (0x40);
            if (type >= 1 && type <= 4) {
                PrintRsrc (0x40 + type);
            } else {
                PrintNumber (type);
            }
            PrintNewline ();
        }

#ifndef _M_ARM64
        if (raz) {
            if (hVmKey != NULL) {
                PrintRsrc (4);
            }
            PrintRsrc (0x37);
            PrintNumber (info.MajorVersion);
            Print ('.');
            PrintNumber (info.MinorVersion);
            Print ('.');
            PrintNumber (info.BuildNumber);
            Print ('.');
            PrintNumber (info.ServiceNumber);
            PrintRsrc (0x38);
            PrintNumber (info.ServicePack);
            PrintRsrc (0x39);
            PrintNumber (info.ServiceBranch);
            PrintNewline ();
        }
#endif
    } else {
        PrintRsrc (0x31);
    }
}

bool PrintSecureKernel () {
    char path [MAX_PATH + 18];
    if (GetSystemDirectoryA (path, MAX_PATH)) {
        strcat (path, "\\SecureKernel.exe");

        VS_FIXEDFILEINFO info;
        if (GetFileFixedVersionInfo (path, &info)) {

            DWORD status = ~0;
            QueryEvtValue ("IsolatedUserMode", 3, "Status", &status);

            if (status == 0) {
                SetTextColor (10);
            }
            PrintRsrc (0x50); // TODO: read description from EXE instead
            ShowVersionNumbersBase (HIWORD (info.dwProductVersionMS), LOWORD (info.dwProductVersionMS), HIWORD (info.dwProductVersionLS));
            Print (L'.');
            PrintNumber (LOWORD (info.dwProductVersionLS));

            if (status != 0) {
                SetTextColor (14);
                PrintRsrc (0x51);
                ResetTextColor ();
            }

            PrintNewline ();
            return true;
        }
    }
    return false;
}

bool PrintSecureBoot () {
    HKEY hSecureBootKey = NULL;
    DWORD dwRegFlags = KEY_QUERY_VALUE;
#ifndef _WIN64
    if (major > 5 || (major == 5 && minor >= 2)) {
        dwRegFlags |= KEY_WOW64_64KEY;
    }
#endif
    if (RegOpenKeyExA (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State", 0, dwRegFlags, &hSecureBootKey) == ERROR_SUCCESS) {
        DWORD value = 0;
        DWORD size = sizeof value;
        if (RegQueryValueExA (hSecureBootKey, "UEFISecureBootEnabled", NULL, NULL, (LPBYTE) &value, &size) == ERROR_SUCCESS) {
            PrintRsrc (0x52);
            SetTextColor (value ? 10 : 14);
            PrintRsrc (value ? 0x55 : 0x54);
            ResetTextColor ();
            PrintNewline ();
            return true;
        }
    }

    // TODO: QueryEvtValue ("Kernel-Boot", 153, "EnableDisableReason", ...);
    // TODO: QueryEvtValue ("Kernel-Boot", 153, "VsmPolicy", ...);

    return false;
}

void PrintIsolationInfo () {
    if (auto hKernelBase = GetModuleHandleA ("KERNELBASE")) {
        bool isolated = false;
        bool container = false;

        BOOLEAN (WINAPI * ptrWTSIsServerContainer) ();
        if (Windows::Symbol (hKernelBase, ptrWTSIsServerContainer, "WTSIsServerContainer")) {
            if (ptrWTSIsServerContainer ()) {
                container = true;
            }
        }

        DWORD (WINAPI * ptrWTSGetServiceSessionId) ();
        if (Windows::Symbol (hKernelBase, ptrWTSGetServiceSessionId, "WTSGetServiceSessionId")) {
            if (ptrWTSGetServiceSessionId () != 0) {
                isolated = true;
            }
        }

        if (isolated || container) {
            PrintRsrc (0x3E);
            if (container) {
                PrintRsrc (0x3F);
            }
            PrintNewline ();
        }
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

void Print (const char * text) {
    wchar_t buffer [3072];
    Convert (text, buffer); // TODO: length
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
void PrintNewline () {
    Print (L"\r\n", 2);
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
#ifdef _WIN64
    // Windows 2000+
    const wchar_t * string = nullptr;
    if (auto length = LoadString (reinterpret_cast <HINSTANCE> (&__ImageBase), id, (LPWSTR) &string, 0)) {
        Print (string, length);
    }
#else
    // This is needed on NT4
    wchar_t buffer [256];
    auto length = LoadString (reinterpret_cast <HINSTANCE> (&__ImageBase), id, buffer, sizeof buffer / sizeof buffer [0]);
    Print (buffer, length);
#endif
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
    PrintNewline ();
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
    int startupinfo;
    wchar_t ** wenviron;
    __wgetmainargs (&argc, &argv, &wenviron, 0, &startupinfo);

    for (auto i = 1; i < argc; ++i) {
        while (*argv [i] == L'-' || *argv [i] == L'/') { // skip dashes and slashes
            ++argv [i];
        }
    }
}

bool IsOptionPresent (wchar_t c) {
    for (auto i = 1; i < argc; ++i) {
        for (auto a = argv [i]; *a; ++a) {
            if (*a == 'a' || *a == c)
                return true;
        }
    }
    return false;
}
