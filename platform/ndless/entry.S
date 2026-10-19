.syntax unified
.arm
.section .text.entry,"ax",%progbits
.global _start
.type _start,%function
_start:
    push {r4-r12,lr}
    bl main
    pop {r4-r12,pc}

.section .text,"ax",%progbits
/* Ndless r2022 stable SWI ABI. The ABI may clobber r4 and lr. */
.macro nd_call name,number
.global \name
.type \name,%function
\name:
    push {r4,lr}
    swi \number
    pop {r4,pc}
.endm
nd_call nd_open,0
nd_call nd_read,1
nd_call nd_write,2
nd_call nd_close,3
/* Stable Ndless SDK syscall-list.h: nuc_fseek=33, nuc_ftell=89. */
nd_call nd_seek,33
nd_call nd_tell,89
nd_call nd_unlink,19
nd_call nd_rename,20
nd_call nd_touch_read,75
nd_call nd_touch_write,76
nd_call nd_hwtype,0x200002
nd_call nd_revision,0x200006
nd_call nd_blitter,0x20000D
nd_call nd_lcd_init,0x20000F

/* ARM EABI quotient/remainder convention: quotient r0, remainder r1. */
.global __aeabi_uidiv
.global __aeabi_uidivmod
.type __aeabi_uidiv,%function
.type __aeabi_uidivmod,%function
__aeabi_uidiv:
__aeabi_uidivmod:
    cmp r1,#0
    moveq r0,#0
    bxeq lr
    mov r2,#0
    mov r3,#0
    mov ip,#32
1:
    movs r0,r0,lsl #1
    adcs r3,r3,r3
    cmpcc r3,r1
    subcs r3,r3,r1
    adc r2,r2,r2
    subs ip,ip,#1
    bne 1b
    mov r0,r2
    mov r1,r3
    bx lr
.global __aeabi_idiv
.global __aeabi_idivmod
__aeabi_idiv:
__aeabi_idivmod:
    push {r4-r6,lr}
    mov r4,r0
    eor r5,r0,r1
    cmp r0,#0
    rsbmi r0,r0,#0
    cmp r1,#0
    rsbmi r1,r1,#0
    bl __aeabi_uidivmod
    cmp r5,#0
    rsbmi r0,r0,#0
    cmp r4,#0
    rsbmi r1,r1,#0
    pop {r4-r6,pc}
.global __aeabi_memcpy
.global __aeabi_memcpy4
.global __aeabi_memcpy8
__aeabi_memcpy:
__aeabi_memcpy4:
__aeabi_memcpy8:
    b memcpy
.global __aeabi_memmove
__aeabi_memmove:
    b memmove
.global __aeabi_memset
.global __aeabi_memset4
.global __aeabi_memset8
__aeabi_memset:
__aeabi_memset4:
__aeabi_memset8:
    mov r3,r1
    mov r1,r2
    mov r2,r3
    b memset
.global __aeabi_memclr
.global __aeabi_memclr4
.global __aeabi_memclr8
__aeabi_memclr:
__aeabi_memclr4:
__aeabi_memclr8:
    mov r2,r1
    mov r1,#0
    b memset
.section .note.GNU-stack,"",%progbits
