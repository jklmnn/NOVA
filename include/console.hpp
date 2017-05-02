/*
 * Generic Console
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#pragma once

#include <stdarg.h>
#include "initprio.hpp"
#include "spinlock.hpp"

class Console
{
    private:
        enum
        {
            MODE_FLAGS      = 0,
            MODE_WIDTH      = 1,
            MODE_PRECS      = 2,
            FLAG_SIGNED     = 1UL << 0,
            FLAG_ALT_FORM   = 1UL << 1,
            FLAG_ZERO_PAD   = 1UL << 2,
        };

        Console *next;

        static Console *list;
        static Spinlock lock;

        virtual void putc (int) = 0;
        void print_num (uint64, unsigned, unsigned, unsigned);
        void print_str (char const *, unsigned, unsigned);


        FORMAT (2,0)
        void vprintf (char const *, va_list);

    protected:
        NOINLINE
        void enable()
        {
            Console **ptr; for (ptr = &list; *ptr; ptr = &(*ptr)->next) ; *ptr = this;
        }

    public:
        FORMAT (1,2)
        static void print (char const *, ...);

        FORMAT (1,2) NORETURN
        static void panic (char const *, ...);

        template <typename T>
        static inline  void dump_bytes(char dst[], size_t num_bytes, T const * data)
        {    
            const uint8 *bytes = reinterpret_cast<const uint8*>(data);
            size_t i = 0;
            for(; i < num_bytes; i++){
                uint8 upper = (bytes[i] & 0xf0) >> 4;
                uint8 lower = bytes[i] & 0x0f;
                dst[i*3] = static_cast<char>((upper < 10) ? upper + 0x30 : upper + 0x57);
                dst[i*3 + 1] = static_cast<char>((lower < 10) ? lower + 0x30 : lower + 0x57);
                dst[i*3 + 2] = 0x20;
            }
            dst[(i-1) * 3 + 2] = '\0';
        }
};
