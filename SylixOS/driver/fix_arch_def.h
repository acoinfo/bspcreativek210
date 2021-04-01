#ifndef __FIX_ARCH_DEF_H
#define __FIX_ARCH_DEF_H

#ifndef __ASSEMBLER__
#if defined(__GNUC__)

#ifdef  read_csr
#undef  read_csr
#endif
#define read_csr(reg) ({ unsigned long __tmp; \
        asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
        __tmp; })

#ifdef  write_csr
#undef  write_csr
#endif
#define write_csr(reg, val) ({ \
        if (__builtin_constant_p(val) && (unsigned long)(val) < 32) \
            asm volatile ("csrw " #reg ", %0" :: "i"(val)); \
        else \
            asm volatile ("csrw " #reg ", %0" :: "r"(val)); })

#ifdef  swap_csr
#undef  swap_csr
#endif
#define swap_csr(reg, val) ({ unsigned long __tmp; \
        if (__builtin_constant_p(val) && (unsigned long)(val) < 32) \
            asm volatile ("csrrw %0, " #reg ", %1" : "=r"(__tmp) : "i"(val)); \
        else \
            asm volatile ("csrrw %0, " #reg ", %1" : "=r"(__tmp) : "r"(val)); \
        __tmp; })

#ifdef  set_csr
#undef  set_csr
#endif
#define set_csr(reg, bit) ({ unsigned long __tmp; \
        if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32) \
            asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit)); \
        else \
            asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit)); \
        __tmp; })

#ifdef  clear_csr
#undef  clear_csr
#endif
#define clear_csr(reg, bit) ({ unsigned long __tmp; \
        if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32) \
            asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit)); \
        else \
            asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit)); \
        __tmp; })

#define read_time()         read_csr(mtime)
#define read_cycle()        read_csr(mcycle)
#define current_coreid()    read_csr(mhartid)

#endif /* __ASSEMBLER__ */
#endif /* __GNUC__ */


#endif /* __FIX_ARCH_DEF_H */
