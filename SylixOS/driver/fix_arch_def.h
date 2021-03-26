/*
 * fix_arch_def.h
 *
 *  Created on: Oct 18, 2018
 *      Author: yukangzhi
 */

#ifndef __FIX_ARCH_DEF_H
#define __FIX_ARCH_DEF_H

#ifndef __ASSEMBLER__
#if defined(__GNUC__)

#define read_csr(reg) ({ unsigned long __tmp; \
        asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
        __tmp; })

#define write_csr(reg, val) ({ \
        if (__builtin_constant_p(val) && (unsigned long)(val) < 32) \
            asm volatile ("csrw " #reg ", %0" :: "i"(val)); \
        else \
            asm volatile ("csrw " #reg ", %0" :: "r"(val)); })

#define swap_csr(reg, val) ({ unsigned long __tmp; \
        if (__builtin_constant_p(val) && (unsigned long)(val) < 32) \
            asm volatile ("csrrw %0, " #reg ", %1" : "=r"(__tmp) : "i"(val)); \
        else \
            asm volatile ("csrrw %0, " #reg ", %1" : "=r"(__tmp) : "r"(val)); \
        __tmp; })

#define set_csr(reg, bit) ({ unsigned long __tmp; \
        if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32) \
            asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit)); \
        else \
            asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit)); \
        __tmp; })

#define clear_csr(reg, bit) ({ unsigned long __tmp; \
        if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32) \
            asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit)); \
        else \
            asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit)); \
        __tmp; })

#define read_time()         read_csr(mtime)
#define read_cycle()        read_csr(mcycle)
#define current_coreid()    read_csr(mhartid)

#endif      /* __ASSEMBLER__ */
#endif      /* __GNUC__ */


#endif /* __FIX_ARCH_DEF_H */
