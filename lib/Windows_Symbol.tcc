#ifndef WINDOWS_SYMBOL_TCC
#define WINDOWS_SYMBOL_TCC

/* Emphasize Windows symbol loading helper template
// Windows_Symbol.tcc
//
// Author: Jan Ringos, http://Tringi.MX-3.cz, jan@ringos.cz
// Version: 1.0
//
// Changelog:
//      03.12.2011 - initial version
*/

template <typename P>
bool Windows::Symbol (HMODULE h, P & pointer, const char * name) {
    if (P p = reinterpret_cast <P> (GetProcAddress (h, name))) {
        pointer = p;
        return true;
    } else
        return false;
};

template <typename P>
bool Windows::Symbol (HMODULE h, P & pointer, unsigned short index) {
    return Windows::Symbol <P> (h, pointer, reinterpret_cast <const char *> (index));
};

template <typename P>
bool Windows::Symbol (const wchar_t * m, P & pointer, const char * name) {
    if (HMODULE h = GetModuleHandleW (m))
        return Windows::Symbol <P> (h, pointer, name);
    else
        return false;
};

template <typename P>
bool Windows::Symbol (const wchar_t * m, P & pointer, unsigned short index) {
    if (HMODULE h = GetModuleHandleW (m))
        return Windows::Symbol <P> (h, pointer, index);
    else
        return false;
};

#endif
