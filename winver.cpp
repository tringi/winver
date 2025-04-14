#include <Windows.h>
#include <slpublic.h>
#include <intrin.h>

#include <charconv>
#include <cstddef>
#include <span>

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
void PrintUserInformation ();
void PrintSupportedArchitectures ();
void PrintOsArchitecture ();
void PrintLicenseStatus ();
void PrintExpiration ();
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
    dwRegFlags |= KEY_NOTIFY;
    RegOpenKeyExA (HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Virtual Machine\\Guest\\Parameters", 0, dwRegFlags, &hVmKey);

    build &= 0x0FFF'FFFF;
    colors = GetConsoleColors ();
    hKernel32 = GetModuleHandleA ("KERNEL32");

    // winver.com
    // winver.com -a

    if (argc <= 1 || IsOptionPresent ('a')) {
        Print ("\r\n");

        // Windows 11 Pro Insider Preview
        // Windows 10 Enterprise N 2016 LTSB
        // Windows Server 2022 Datacenter
        // Hyper-V Server
        // Azure Stack HCI

        if (major < 10 || !ShowBrandingFromAPI ()) {
            if (!PrintValueFromRegistry ("ProductName")) {

                OSVERSIONINFOEX os;
                os.dwOSVersionInfoSize = sizeof os;
                if (GetVersionEx ((OSVERSIONINFO *) &os)) {
                    switch (os.dwPlatformId) {
                        default:
                        case VER_PLATFORM_WIN32_WINDOWS:
                            // 9x
                            Print ("Windows 9x");
                            break;
                        case VER_PLATFORM_WIN32_NT:
                            // NT
                            switch (os.wProductType) {
                                case VER_NT_SERVER:
                                case VER_NT_DOMAIN_CONTROLLER:
                                    Print ("Windows NT Server");
                                    break;
                                default:
                                case VER_NT_WORKSTATION:
                                    Print ("Windows NT");
                                    break;
                            }
                            break;
                    }
                } else {
                    PrintRsrc (1);
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
    //  - License: Activated
    //  - Remaining reasm count: 1001

    if (IsOptionPresent (L'l')) {
        PrintExpiration ();
        PrintLicenseStatus ();
    }

    // winver.com -o
    //  - Licensed to: \n User name \n Company name

    if (IsOptionPresent (L'o')) {
        PrintUserInformation ();
    }

    // winver.com -i
    //  - Architectures supported: AArch64, AArch32, x86-64, x86-32

    if (IsOptionPresent (L'i')) {
        PrintSupportedArchitectures ();
    }

    // winver.com -h
    //  - No Hyper-V virtualization
    //  - Hyper-V Primary partition
    //  - Virtualized. Parent Hyper-V partition: 

    if (IsOptionPresent (L'h')) {
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
    static char data [3072];
    if (auto hVersionDLL = LoadLibraryA ("VERSION")) {

        BOOL (APIENTRY * ptrGetFileVersionInfoA) (_In_ LPCSTR, _Reserved_ DWORD, _In_ DWORD dwLen, _Out_writes_bytes_ (dwLen) LPVOID) = NULL;
        BOOL (APIENTRY * ptrVerQueryValueA) (_In_ LPCVOID, _In_ LPCSTR, LPVOID * lplpBuffer, _Out_ PUINT puLen) = NULL;

        if (Windows::Symbol (hVersionDLL, ptrGetFileVersionInfoA, "GetFileVersionInfoA")) {
            if (ptrGetFileVersionInfoA (filename, 0, sizeof data, data)) {

                VS_FIXEDFILEINFO * info = NULL;
                UINT infolen = sizeof info;

                if (Windows::Symbol (hVersionDLL, ptrVerQueryValueA, "VerQueryValueA")) {
                    if (ptrVerQueryValueA (data, "\\", (LPVOID *) &info, &infolen)) {
                        return LOWORD (info->dwProductVersionLS);
                    }
                }
            }
        }
    }
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

                    switch (machine.Machine) {
                        case IMAGE_FILE_MACHINE_I386: Print ("x86-32"); break;
                        case IMAGE_FILE_MACHINE_ARMNT: Print ("AArch32"); break;
                        case IMAGE_FILE_MACHINE_AMD64: Print ("x86-64"); break;
                        case IMAGE_FILE_MACHINE_ARM64: Print ("AArch64"); break;
                    }

                    bool wow64 = !!machine.WoW64Container;
                    bool emulated = !machine.Native;

                    if (oldAPI) {
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

            Print ("\r\n");
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
    Print ("\r\n");
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
                    Print ("\r\n");
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
                                                            Print ("\r\n");

                                                            if (thisLicensingStatus->dwTotalGraceDays) {
                                                                PrintRsrc (10);
                                                                PrintNumber (thisLicensingStatus->dwTotalGraceDays);
                                                                Print ("\r\n");
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
                                                                Print ("\r\n");
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
                        PrintRsrc (0x10 + (UINT) state);
                        break;
                    default:
                        PrintNumber ((UINT) state);
                }
                ResetTextColor ();
                Print ("\r\n");
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

bool IsHypervisorInstalled (char * vendor, HV_HYPERVISOR_VERSION_INFO * info, UINT * partition) {
    int registers [4];

    __cpuid (registers, 0);
    const auto maximum_eax = registers [0];
    if (maximum_eax >= 1u) {

        __cpuid (registers, 1);
        if (registers [2] & (1 << 31)) { // RAZ (Hyper-V)

            __cpuid (registers, 0x40000000);
            const auto maximum_hv = registers [0];

            // extract hypervisor name

            const auto p = reinterpret_cast <HV_VENDOR_AND_MAX_FUNCTION *> (registers)->VendorName;
            for (auto i = 0u; i != sizeof HV_VENDOR_AND_MAX_FUNCTION::VendorName; ++i) {
                if (i >= 32 && i < 127) {
                    *vendor++ = p [i];
                }
            }
            *vendor = '\0';

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
                    *partition = 1; // primary
                } else {
                    *partition = 2;
                    //this->currentVmHost.vmId
                }
            }
        }
    }

    return false;
}
#endif


void PrintHypervisorInfo () {
    if (hVmKey) {
        //PrintValueFromRegistry (hVmKey, "VirtualMachineId");
    }

    // TSGetServiceSessionId / WTSIsServerContainer

#ifndef _M_ARM64
    HV_HYPERVISOR_VERSION_INFO info;
    UINT partition = 0;
    char vendor [13];

    if (IsHypervisorInstalled (vendor, &info, &partition)) {

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
