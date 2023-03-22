	.arch armv8-a
	.arch_extension crc
	.arm
	.data
addr_ascii_0:
	.word 0
	.text
	.global __aeabi_idiv
	.global my_getint
	.type my_getint , %function
my_getint:
	push {fp, lr}
	push {r4, r5, r6}
	add fp, sp, #0
	sub sp, sp, #8
.L13:
	mov r4, #0
	str r4, [fp, #-8]
	b .L16
.L16:
	ldr r4, =1
	cmp r4, #0
	bne .L17
	b .L18
.L17:
	bl getch(PLT)
	mov r4, r0
	ldr r5, =addr_ascii_0
	ldr r6, [r5]
	subs r5, r4, r6
	str r5, [fp, #-4]
	ldr r4, [fp, #-4]
	cmp r4, #0
	blt .L20
	b .L22
.L18:
	ldr r4, [fp, #-8]
	mov r0, r4
	add sp, fp, #0
	pop {r4}
	pop {r5}
	pop {r6}
	pop {fp, pc}
.L20:
	b .L16
.L21:
	b .L16
.L22:
	ldr r4, [fp, #-4]
	cmp r4, #9
	bgt .L20
	b .L21
	.global main
	.type main , %function
main:
	push {fp}
	push {r4, r5}
	add fp, sp, #0
	sub sp, sp, #0
.L23:
	mov r4, #48
	ldr r5, =addr_ascii_0
	str r4, [r5]
	mov r0, #0
	add sp, fp, #0
	pop {r4}
	pop {r5}
	pop {fp}
	bx lr
