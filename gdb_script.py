# Author: Max Bebök
# License: CC0 (https://creativecommons.org/publicdomain/zero/1.0/)

import gdb

TYPE_U32 = gdb.lookup_type('u32')

# address to object-path map, used to unload entire .o files by an address
obj_address_map = {}

def get_section_address(ovl_name, section_name):
  section_start_name = "_" + ovl_name + "Segment" + section_name + "Start"
  # 'section_start_name' is a 'text variable' according to gdb, this can not be resolved via python directly
  return int(gdb.execute('printf "%x", &' + section_start_name, False, True), 16)

# Unloads an object-file by the loaded address of the first section
def unload_object(ptr):
  if ptr in obj_address_map:
    obj_name = obj_address_map[ptr]
    del obj_address_map[ptr]
    print("Unloading overlay: ", obj_name, "from: ", ptr, " (", len(obj_address_map), " left)")
    # This deletea all sections by using the first section address.
    # However, symbols will still resolve to the VRAM address afterwards (like before loading the .o)
    gdb.execute("remove-symbol-file -a " + ptr, False, False)

class BreakpointOverlayLoad(gdb.Breakpoint):
  def stop (self):
    frame = gdb.selected_frame()
    # func_name = frame.name()

    alloc_address = frame.read_var("allocatedRamAddr").cast(TYPE_U32)

    # get first symbol-name starting from vramStart (usually the first function in the overlay)
    target_func_name = gdb.execute("info symbol vramStart", False, True).partition(' ')[0].rstrip()

    # get section in main ELF (returns: "EnKusa_SetupAction in section ..ovl_En_Kusa of zelda_ocarina_mq_dbg.elf")
    ovl_sec_name = gdb.execute("info sym " + target_func_name, False, True)
    # extract section name (@TODO: check if there is a direct API for this)
    ovl_sec_name = ovl_sec_name.partition('section ..')[2].partition(" ")[0].rstrip()

    ovl_address_text   = get_section_address(ovl_sec_name, "Text")
    ovl_address_data   = get_section_address(ovl_sec_name, "Data")
    ovl_address_rodata = get_section_address(ovl_sec_name, "RoData")
    ovl_address_bss    = get_section_address(ovl_sec_name, "Bss")

    ovl_offset_text   = alloc_address
    ovl_offset_data   = alloc_address + (ovl_address_data   - ovl_address_text)
    ovl_offset_rodata = alloc_address + (ovl_address_rodata - ovl_address_text)
    ovl_offset_bss    = alloc_address + (ovl_address_bss    - ovl_address_text)

    # get full object-file path that contains the first symbol
    target_filename = gdb.lookup_symbol(target_func_name)[0].symtab.filename
    obj_name = "build/" + target_filename[:-1] + "o"

    obj_address_map[hex(alloc_address)] = obj_name
    print("Loading overlay: ", obj_name, "(text:", hex(ovl_offset_text), " data:", hex(ovl_offset_data), " rodata:", hex(ovl_offset_rodata), " bss:", hex(ovl_offset_bss), ")")

    gdb.execute("add-symbol-file -readnow " + obj_name +
      " -o 0xFF000000" +
      " -s .text "   + hex(ovl_offset_text)   +
      " -s .data "   + hex(ovl_offset_data)   +
      " -s .rodata " + hex(ovl_offset_rodata) +
      " -s .bss "    + hex(ovl_offset_bss),
      False, True)

    return False

class BreakpointFree(gdb.Breakpoint):
  def stop (self):
    frame = gdb.selected_frame()
    ptr = hex(frame.read_var("ptr").cast(TYPE_U32))
    unload_object(ptr)
    return False

class BreakpointKaleidoFree(gdb.Breakpoint):
  def stop (self):
    frame = gdb.selected_frame()
    ptr = hex(frame.read_var("ovl")["loadedRamAddr"].cast(TYPE_U32))
    unload_object(ptr)
    return False

# SPDX-License-Identifier: CC0-1.0

import gdb

SR_CU1 = 0x20000000
SR_EXL = 0x00000002

CAUSE_CEMASK = 0x30000000
CAUSE_CESHIFT = 28
CAUSE_EXCMASK = 0x0000007C
EXC_CPU = 11 << 2


def on_stop(ev: gdb.StopEvent):
    # notifying a SIGTRAP signal to the gdb client is just
    # how the ares server chose to pass along the information,
    # it's not from the n64 hardware

    # For some reason ev is not a SignalEvent, doc/implementation mistake?
    if isinstance(ev, gdb.SignalEvent):
        print("SignalEvent", ev.stop_signal)
        if ev.stop_signal == "SIGTRAP":
            ...

    if isinstance(ev, gdb.BreakpointEvent):
        return

    sr = int(gdb.newest_frame().read_register("sr"))
    if sr & SR_EXL:
        print("EXL")
        # Exception
        if not (sr & SR_CU1):
            print("!CU1")
            # Coprocessor 1 (FPU) is currently not usable
            cause = int(gdb.newest_frame().read_register("cause"))
            if (cause & CAUSE_EXCMASK) == EXC_CPU:
                print("exc CPU")
                # The exception is coprocessor unusable
                if ((cause & CAUSE_CEMASK) >> CAUSE_CESHIFT) == 1:
                    # The exception is about cop1
                    print("cop1 unusable exception")
                    print("continuing")
                    gdb.execute("c")


gdb.events.stop.connect(on_stop)

bp_load = BreakpointOverlayLoad("Overlay_Load")
bp_load.silent = True

bl_free = BreakpointFree("ZeldaArena_FreeDebug")
bl_free.silent = True

bl_system_free = BreakpointFree("SystemArena_FreeDebug")
bl_system_free.silent = True

bl_kaleido = BreakpointKaleidoFree("KaleidoManager_ClearOvl")
bl_kaleido.silent = True
