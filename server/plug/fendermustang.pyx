# distutils: sources = mustang.cpp
# cython: language_level=3

from libcpp cimport bool

cdef extern from "data_structs.h":
    cdef struct amp_settings:
        unsigned char amp_num
        unsigned char gain
        unsigned char volume
        unsigned char treble
        unsigned char middle
        unsigned char bass
        unsigned char cabinet
        unsigned char noise_gate
        unsigned char master_vol
        unsigned char gain2
        unsigned char presence
        unsigned char threshold
        unsigned char depth
        unsigned char bias
        unsigned char sag
        bool brightness
        unsigned char usb_gain

    cdef struct fx_pedal_settings:
        unsigned char fx_slot
        unsigned char effect_num
        unsigned char knob1
        unsigned char knob2
        unsigned char knob3
        unsigned char knob4
        unsigned char knob5
        unsigned char knob6
        bool put_post_amp

cdef extern from "mustang.h":
    cdef cppclass Mustang:
        Mustang() except +
        int start_amp(char list[][32], char *name, amp_settings *amp_set, fx_pedal_settings *effects_set)
        int stop_amp()
        int set_effect(fx_pedal_settings)
        int set_amplifier(amp_settings)
        int save_on_amp(char *name, int slot)
        int load_memory_bank(int, char *name, amp_settings *amp_set, fx_pedal_settings *effects_set)


cdef class FenderMustang:
    cdef Mustang *thisptr
    def __cinit__(self):
        self.thisptr = new Mustang()
    def __dealloc__(self):
        del self.thisptr

    def start(self):
        cdef char amp_list[200][32]
        cdef char name[32]
        cdef amp_settings amp_set
        cdef fx_pedal_settings effects_set[4]
        self.thisptr.start_amp(amp_list, name, &amp_set, effects_set)
        return amp_list, name, amp_set, effects_set

    def stop(self):
        return self.thisptr.stop_amp()

    def set_effect(self, fx_pedal_settings settings):
        return self.thisptr.set_effect(settings)

    def set_amplifier(self, amp_settings settings):
        return self.thisptr.set_amplifier(settings)

    def save_on_amp(self, name, slot):
        return self.thisptr.save_on_amp(name, slot)

    def load_memory_bank(self, slot):
        cdef char name[32]
        cdef amp_settings amp_set
        cdef fx_pedal_settings effects_set[4]
        self.thisptr.load_memory_bank(slot, name, &amp_set, effects_set)
        return name, amp_set, effects_set
