
.syntax unified
.cpu cortex-m3
.thumb

.section .text

/**
 * syscallSupercazzola
 */
.global	syscallSupercazzola
.type	syscallSupercazzola, %function
syscallSupercazzola:
    movs r3, #1
    svc  0
    bx   lr
