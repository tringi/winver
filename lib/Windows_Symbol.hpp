#ifndef WINDOWS_SYMBOL_HPP
#define WINDOWS_SYMBOL_HPP

/* Emphasize Windows symbol loading helper template
// Windows_Symbol.hpp
//
// Author: Jan Ringos, http://Tringi.MX-3.cz, jan@ringos.cz
// Version: 1.0
// Description: Abstracts GetProcAddress calls
//
// Changelog:
//      03.12.2011 - initial version
*/

#include <windows.h>

namespace Windows {

    // Symbol
    //  - loads symbol named 'name' from 'HMODULE' and stores it in 'pointer'
    //  - returns: true if 'pointer' was updated with loaded symbol pointer
    //             false otherwise, 'pointer' is left untouched
    //  - parameters: P - function pointer or pointer type
    //                HMODULE - module handle where the symbol should reside
    //                 - module - or module name (GetModuleHandleW is then used)
    //                pointer - variable that receives the symbol pointer
    //                name - symbol ASCII name
    //                index - symbol index

    template <typename P>
    bool Symbol (HMODULE, P & pointer, const char * name);

    template <typename P>
    bool Symbol (HMODULE, P & pointer, unsigned short index);

    template <typename P>
    bool Symbol (const wchar_t * m, P & pointer, const char * name);

    template <typename P>
    bool Symbol (const wchar_t * m, P & pointer, unsigned short index);
};

#include "Windows_Symbol.tcc"
#endif
