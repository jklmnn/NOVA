/*
 * Initialization Code
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

#include "acpi.hpp"
#include "compiler.hpp"
#include "console_serial.hpp"
#include "console_vga.hpp"
#include "gsi.hpp"
#include "hip.hpp"
#include "hpt.hpp"
#include "idt.hpp"
#include "keyb.hpp"
#include "pd.hpp"
#include "multiboot.hpp"
#include "multiboot2.hpp"

extern "C" INIT

mword kern_ptab_setup()
{
    Hptp hpt;

    // Allocate and map cpu page
    hpt.update (Pd::kern.quota, CPU_LOCAL_DATA, 0,
                Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Pd::kern.quota, Buddy::FILL_0)),
                Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_W | Hpt::HPT_P);

    // Allocate and map kernel stack
    hpt.update (Pd::kern.quota, CPU_LOCAL_STCK, 0,
                Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Pd::kern.quota, Buddy::FILL_0)),
                Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_W | Hpt::HPT_P);

    // Sync kernel code and data
    hpt.sync_master_range (Pd::kern.quota, LINK_ADDR, CPU_LOCAL);

    return hpt.addr();
}

extern "C" INIT REGPARM (2)
void init (mword magic, mword mbi)
{
    // Setup 0-page and 1-page
    memset (reinterpret_cast<void *>(&PAGE_0),  0,  PAGE_SIZE);
    memset (reinterpret_cast<void *>(&PAGE_1), ~0u, PAGE_SIZE);

    for (void (**func)() = &CTORS_G; func != &CTORS_E; (*func++)()) ;

    Hip *hip = Hip::build (magic, mbi);

    for (void (**func)() = &CTORS_C; func != &CTORS_G; (*func++)()) ;

    // Now we're ready to talk to the world
    Console::print ("\fNOVA Microhypervisor v%d-%07lx (%s): %s %s [%s] [%s]\n", CFG_VER, reinterpret_cast<mword>(&GIT_VER), ARCH, __DATE__, __TIME__, COMPILER_STRING, magic == Multiboot::MAGIC ? "MBI" : (magic==Multiboot2::MAGIC ? "MBI2" : ""));
    
    if(magic==Multiboot2::MAGIC)
    {
        Hip_fb *fb = &hip->fb_desc;
        Console::print("Multiboot2 framebuffer [%u]: %ux%u width depth %u bpp of type %u on %p\n",
                fb->is,
                fb->width,
                fb->height,
                fb->bpp,
                fb->type,
                (void*)fb->addr);
        
        Console::print("Tags: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                Hip::tags2[1],
                Hip::tags2[2],
                Hip::tags2[3],
                Hip::tags2[4],
                Hip::tags2[5],
                Hip::tags2[6],
                Hip::tags2[7],
                Hip::tags2[8],
                Hip::tags2[9],
                Hip::tags2[10],
                Hip::tags2[11],
                Hip::tags2[12],
                Hip::tags2[13],
                Hip::tags2[14],
                Hip::tags2[15],
                Hip::tags2[16],
                Hip::tags2[17],
                Hip::tags2[18],
                Hip::tags2[19]
                );
        Multiboot2::Header const *mbh = reinterpret_cast<const Multiboot2::Header *>(Hip::tags[19]);
    
        char buf[8 * 3];
        Console::dump_bytes(buf, 8, mbh);

        Console::print("Header (%u): %s\n", mbh->type, buf);

        Console::print("Memory map:\nAddr: %llu\nSize: %llu\nType: %u\n", debug.mem_addr, debug.mem_len, debug.mem_type);

        Console::print("Module:\nType: %u\nSize: %u\nAddr: %p\n", debug.type, debug.size, debug.tag);

        Console::print("Cmd line: %s\nType: %u\nSize: %u\nAddr: %p\n", debug.cmd_addr, debug.cmd_type, debug.cmd_size, debug.cmd_addr);
        char cmd_buf[debug.cmd_size];
        Console::dump_bytes(cmd_buf, 10, debug.cmd_addr);
        Console::print("Cmdline: %s\n", cmd_buf);

    }
     

    Idt::build();
    Gsi::setup();
    Acpi::setup();

    Console_vga::con.setup();

    Console_vga::print("test vga\n");

    Keyb::init();
}
