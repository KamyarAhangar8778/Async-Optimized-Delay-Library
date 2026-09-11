	.file	"test_async_delay.c"
	.text
	.p2align 4
	.type	async_delay_init, @function
async_delay_init:
.LFB51:
	.cfi_startproc
	movl	$0, _async_tick_counter(%rip)
	movb	$0, 16+_async_slots(%rip)
	movb	$0, _async_active_mask(%rip)
	movb	$0, 40+_async_slots(%rip)
	movb	$0, _async_used_mask(%rip)
	movb	$0, 64+_async_slots(%rip)
	movl	$0, _async_next_target(%rip)
	movb	$0, 88+_async_slots(%rip)
	movb	$0, 112+_async_slots(%rip)
	movb	$0, 136+_async_slots(%rip)
	movb	$0, 160+_async_slots(%rip)
	movb	$0, 184+_async_slots(%rip)
	ret
	.cfi_endproc
.LFE51:
	.size	async_delay_init, .-async_delay_init
	.p2align 4
	.type	_async_recompute_next, @function
_async_recompute_next:
.LFB52:
	.cfi_startproc
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L3
	movl	%eax, %ecx
	movl	%eax, %edi
	movl	%eax, %esi
	andl	$2, %ecx
	andl	$8, %edi
	andl	$4, %esi
	testb	$1, %al
	je	.L113
	movl	_async_slots(%rip), %edx
	testb	%cl, %cl
	jne	.L114
	testb	%sil, %sil
	jne	.L10
	testb	%dil, %dil
	jne	.L77
	testb	$16, %al
	jne	.L22
	testb	$32, %al
	jne	.L80
	movl	%eax, %edi
	andb	$64, %dil
	je	.L70
	.p2align 4,,10
	.p2align 3
.L79:
	movl	%edx, %ecx
	jmp	.L36
	.p2align 4,,10
	.p2align 3
.L3:
	ret
	.p2align 4,,10
	.p2align 3
.L113:
	movl	24+_async_slots(%rip), %edx
	testb	%cl, %cl
	jne	.L9
	movl	48+_async_slots(%rip), %ecx
	testb	%sil, %sil
	jne	.L15
	testb	%dil, %dil
	je	.L48
	movl	72+_async_slots(%rip), %edx
.L49:
	testb	$16, %al
	je	.L92
.L22:
	movl	96+_async_slots(%rip), %esi
	movl	%edx, %ecx
	subl	%esi, %ecx
	cmpl	$32766, %ecx
	jbe	.L27
.L92:
	testb	$32, %al
	jne	.L80
.L33:
	testb	$64, %al
	jne	.L79
.L111:
	testb	%al, %al
	jns	.L38
.L104:
	movl	168+_async_slots(%rip), %ecx
.L37:
	movl	%edx, %eax
	subl	%ecx, %eax
	cmpl	$32766, %eax
	cmovbe	%ecx, %edx
	movl	%edx, _async_next_target(%rip)
	ret
	.p2align 4,,10
	.p2align 3
.L114:
	movl	24+_async_slots(%rip), %ecx
	movl	%edx, %r8d
	subl	%ecx, %r8d
	cmpl	$32766, %r8d
	ja	.L9
	movl	%ecx, %edx
.L9:
	testb	%sil, %sil
	jne	.L10
	testb	%dil, %dil
	jne	.L77
	movl	%edx, %esi
	testb	$16, %al
	jne	.L22
	testb	$32, %al
	je	.L115
.L30:
	movl	120+_async_slots(%rip), %ecx
	movl	%esi, %edx
	subl	%ecx, %edx
	cmpl	$32766, %edx
	jbe	.L98
	testb	$64, %al
	jne	.L67
.L118:
	movl	%esi, %edx
	testb	%al, %al
	js	.L104
.L38:
	movl	%edx, _async_next_target(%rip)
	ret
	.p2align 4,,10
	.p2align 3
.L10:
	movl	48+_async_slots(%rip), %ecx
	movl	%edx, %esi
	subl	%ecx, %esi
	cmpl	$32766, %esi
	jbe	.L15
	testb	%dil, %dil
	je	.L116
.L77:
	movl	%edx, %ecx
.L17:
	movl	72+_async_slots(%rip), %edx
	movl	%ecx, %esi
	subl	%edx, %esi
	cmpl	$32766, %esi
	jbe	.L49
	testb	$16, %al
	je	.L117
.L59:
	movl	%ecx, %edx
	jmp	.L22
	.p2align 4,,10
	.p2align 3
.L80:
	movl	%edx, %esi
	jmp	.L30
	.p2align 4,,10
	.p2align 3
.L15:
	testb	%dil, %dil
	jne	.L17
	testb	$16, %al
	jne	.L59
.L25:
	testb	$32, %al
	jne	.L75
.L98:
	testb	$64, %al
	je	.L96
.L36:
	movl	144+_async_slots(%rip), %edx
	movl	%ecx, %esi
	subl	%edx, %esi
	cmpl	$32766, %esi
	jbe	.L111
.L96:
	movl	%ecx, %edx
	jmp	.L111
.L48:
	testb	$16, %al
	je	.L26
	movl	96+_async_slots(%rip), %esi
	.p2align 4,,10
	.p2align 3
.L27:
	testb	$32, %al
	jne	.L30
	testb	$64, %al
	je	.L118
.L67:
	movl	%esi, %ecx
	jmp	.L36
	.p2align 4,,10
	.p2align 3
.L117:
	movl	%ecx, %edx
	testb	$32, %al
	je	.L33
.L75:
	movl	%ecx, %esi
	jmp	.L30
	.p2align 4,,10
	.p2align 3
.L116:
	movl	%edx, %ecx
	testb	$16, %al
	je	.L25
	jmp	.L22
.L115:
	testb	$64, %al
	jne	.L79
	movl	168+_async_slots(%rip), %ecx
	testb	%al, %al
	js	.L37
	jmp	.L38
.L70:
	movl	%edx, %esi
.L40:
	testb	%al, %al
	jns	.L73
	movl	168+_async_slots(%rip), %ecx
	movl	%ecx, %edx
	testb	%dil, %dil
	jne	.L38
	movl	%esi, %edx
	jmp	.L37
.L26:
	movl	120+_async_slots(%rip), %ecx
	testb	$32, %al
	jne	.L98
	testb	$64, %al
	je	.L69
	movl	144+_async_slots(%rip), %edx
	jmp	.L111
.L73:
	movl	%esi, %edx
	jmp	.L38
.L69:
	xorl	%esi, %esi
	movl	$1, %edi
	jmp	.L40
	.cfi_endproc
.LFE52:
	.size	_async_recompute_next, .-_async_recompute_next
	.p2align 4
	.type	_async_delay_start_common, @function
_async_delay_start_common:
.LFB53:
	.cfi_startproc
	movl	%edx, %r10d
	movzbl	_async_used_mask(%rip), %edx
	movq	%rsi, %r9
	movl	%edi, %ecx
	leaq	_async_first_free_nibble(%rip), %rsi
	movq	%rdx, %rax
	andl	$15, %eax
	movzbl	(%rsi,%rax), %eax
	cmpb	$4, %al
	jne	.L120
	shrb	$4, %dl
	andl	$15, %edx
	movzbl	(%rsi,%rdx), %eax
	addl	$4, %eax
.L120:
	movzbl	%al, %edx
	cmpb	$7, %al
	ja	.L125
	leaq	_async_slot_bit(%rip), %rsi
	movzbl	SREG(%rip), %r11d
	leaq	(%rdx,%rdx,2), %r8
	movzbl	(%rsi,%rdx), %edi
	movl	_async_tick_counter(%rip), %esi
	leaq	_async_slots(%rip), %rdx
	leaq	(%rdx,%r8,8), %r8
	addl	%ecx, %esi
	cmpb	$1, %r10b
	movl	%ecx, 4(%r8)
	movzbl	_async_active_mask(%rip), %ecx
	sbbl	%edx, %edx
	movl	%esi, (%r8)
	andl	$-4, %edx
	movq	%r9, 8(%r8)
	addl	$5, %edx
	movb	%dl, 16(%r8)
	testb	%cl, %cl
	jne	.L130
.L123:
	movl	%esi, _async_next_target(%rip)
.L124:
	movzbl	_async_used_mask(%rip), %edx
	orl	%edi, %edx
	orl	%ecx, %edi
	movb	%dl, _async_used_mask(%rip)
	movb	%dil, _async_active_mask(%rip)
	movb	%r11b, SREG(%rip)
	ret
	.p2align 4,,10
	.p2align 3
.L130:
	movl	_async_next_target(%rip), %edx
	subl	%esi, %edx
	cmpl	$32766, %edx
	ja	.L124
	jmp	.L123
	.p2align 4,,10
	.p2align 3
.L125:
	movl	$-1, %eax
	ret
	.cfi_endproc
.LFE53:
	.size	_async_delay_start_common, .-_async_delay_start_common
	.p2align 4
	.type	async_delay_ticks_until_next, @function
async_delay_ticks_until_next:
.LFB62:
	.cfi_startproc
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L134
	movzbl	SREG(%rip), %ecx
	movl	_async_tick_counter(%rip), %edx
	movl	_async_next_target(%rip), %eax
	movb	%cl, SREG(%rip)
	movl	%edx, %ecx
	subl	%eax, %ecx
	cmpl	$32766, %ecx
	jbe	.L134
	subl	%edx, %eax
	ret
.L134:
	xorl	%eax, %eax
	ret
	.cfi_endproc
.LFE62:
	.size	async_delay_ticks_until_next, .-async_delay_ticks_until_next
	.p2align 4
	.type	_async_delay_expire_slot, @function
_async_delay_expire_slot:
.LFB64:
	.cfi_startproc
	movzbl	%dil, %r8d
	movzbl	%dil, %edi
	leaq	_async_slots(%rip), %rcx
	leaq	(%rdi,%rdi,2), %rax
	leaq	(%rcx,%rax,8), %rax
	movq	8(%rax), %rdx
	testb	$4, 16(%rax)
	je	.L136
	testq	%rdx, %rdx
	je	.L137
	movl	4(%rax), %ecx
	movl	%r8d, %edi
	addl	%ecx, (%rax)
	jmp	*%rdx
	.p2align 4,,10
	.p2align 3
.L136:
	testq	%rdx, %rdx
	je	.L137
	movzbl	_async_active_mask(%rip), %ecx
	movb	$0, 16(%rax)
	movl	%r8d, %edi
	movq	$0, 8(%rax)
	andl	%esi, %ecx
	movb	%cl, _async_active_mask(%rip)
	movzbl	_async_used_mask(%rip), %ecx
	andl	%esi, %ecx
	movb	%cl, _async_used_mask(%rip)
	jmp	*%rdx
	.p2align 4,,10
	.p2align 3
.L137:
	movzbl	_async_active_mask(%rip), %eax
	andl	%esi, %eax
	movb	%al, _async_active_mask(%rip)
	leaq	(%rdi,%rdi,2), %rax
	movb	$2, 16(%rcx,%rax,8)
	ret
	.cfi_endproc
.LFE64:
	.size	_async_delay_expire_slot, .-_async_delay_expire_slot
	.p2align 4
	.type	_async_delay_tick_walk, @function
_async_delay_tick_walk:
.LFB65:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	subq	$8, %rsp
	.cfi_def_cfa_offset 32
	movl	_async_tick_counter(%rip), %ebp
	movzbl	_async_active_mask(%rip), %ebx
	testb	$1, %bl
	je	.L145
	movl	%ebp, %eax
	subl	_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L175
.L145:
	testb	$2, %bl
	je	.L146
	movl	%ebp, %eax
	subl	24+_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L176
.L146:
	testb	$4, %bl
	je	.L147
	movl	%ebp, %eax
	subl	48+_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L177
.L147:
	testb	$8, %bl
	je	.L148
	movl	%ebp, %eax
	subl	72+_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L178
.L148:
	testb	$16, %bl
	je	.L149
	movl	%ebp, %eax
	subl	96+_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L179
.L149:
	testb	$32, %bl
	je	.L150
	movl	%ebp, %eax
	subl	120+_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L180
.L150:
	testb	$64, %bl
	je	.L151
	movl	%ebp, %eax
	subl	144+_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L181
.L151:
	testb	%bl, %bl
	js	.L182
.L152:
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	jmp	_async_recompute_next
	.p2align 4,,10
	.p2align 3
.L182:
	.cfi_restore_state
	subl	168+_async_slots(%rip), %ebp
	cmpl	$32766, %ebp
	ja	.L152
	movl	$127, %esi
	movl	$7, %edi
	call	_async_delay_expire_slot
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	jmp	_async_recompute_next
	.p2align 4,,10
	.p2align 3
.L181:
	.cfi_restore_state
	movl	$191, %esi
	movl	$6, %edi
	call	_async_delay_expire_slot
	jmp	.L151
	.p2align 4,,10
	.p2align 3
.L175:
	movl	$254, %esi
	xorl	%edi, %edi
	call	_async_delay_expire_slot
	jmp	.L145
	.p2align 4,,10
	.p2align 3
.L176:
	movl	$253, %esi
	movl	$1, %edi
	call	_async_delay_expire_slot
	jmp	.L146
	.p2align 4,,10
	.p2align 3
.L177:
	movl	$251, %esi
	movl	$2, %edi
	call	_async_delay_expire_slot
	jmp	.L147
	.p2align 4,,10
	.p2align 3
.L178:
	movl	$247, %esi
	movl	$3, %edi
	call	_async_delay_expire_slot
	jmp	.L148
	.p2align 4,,10
	.p2align 3
.L179:
	movl	$239, %esi
	movl	$4, %edi
	call	_async_delay_expire_slot
	jmp	.L149
	.p2align 4,,10
	.p2align 3
.L180:
	movl	$223, %esi
	movl	$5, %edi
	call	_async_delay_expire_slot
	jmp	.L150
	.cfi_endproc
.LFE65:
	.size	_async_delay_tick_walk, .-_async_delay_tick_walk
	.p2align 4
	.type	test_cb_oneshot, @function
test_cb_oneshot:
.LFB67:
	.cfi_startproc
	endbr64
	movl	cb_count1(%rip), %eax
	addl	$1, %eax
	movl	%eax, cb_count1(%rip)
	movb	%dil, last_cb_slot(%rip)
	ret
	.cfi_endproc
.LFE67:
	.size	test_cb_oneshot, .-test_cb_oneshot
	.p2align 4
	.type	test_cb_periodic, @function
test_cb_periodic:
.LFB68:
	.cfi_startproc
	endbr64
	movl	cb_count2(%rip), %eax
	addl	$1, %eax
	movl	%eax, cb_count2(%rip)
	ret
	.cfi_endproc
.LFE68:
	.size	test_cb_periodic, .-test_cb_periodic
	.p2align 4
	.type	async_delay_elapsed.part.0, @function
async_delay_elapsed.part.0:
.LFB81:
	.cfi_startproc
	leaq	_async_slot_bit(%rip), %rax
	movzbl	%dil, %edi
	movzbl	(%rax,%rdi), %edx
	movzbl	_async_active_mask(%rip), %eax
	andb	%dl, %al
	jne	.L187
	imulq	$24, %rdi, %rdi
	leaq	_async_slots(%rip), %rcx
	addq	%rdi, %rcx
	movzbl	16(%rcx), %esi
	andl	$3, %esi
	cmpb	$2, %sil
	je	.L188
	ret
.L187:
	xorl	%eax, %eax
	ret
.L188:
	movzbl	SREG(%rip), %eax
	movzbl	_async_used_mask(%rip), %esi
	notl	%edx
	movb	$0, 16(%rcx)
	andl	%esi, %edx
	movb	%dl, _async_used_mask(%rip)
	movb	%al, SREG(%rip)
	movl	$1, %eax
	ret
	.cfi_endproc
.LFE81:
	.size	async_delay_elapsed.part.0, .-async_delay_elapsed.part.0
	.p2align 4
	.type	async_delay_cancel.part.0, @function
async_delay_cancel.part.0:
.LFB82:
	.cfi_startproc
	movzbl	%dil, %edi
	leaq	_async_slot_bit(%rip), %rax
	leaq	_async_slots(%rip), %rdx
	movzbl	SREG(%rip), %r9d
	movzbl	(%rax,%rdi), %ecx
	leaq	(%rdi,%rdi,2), %rsi
	movzbl	_async_active_mask(%rip), %edi
	leaq	(%rdx,%rsi,8), %rsi
	movzbl	_async_active_mask(%rip), %edx
	movl	%ecx, %eax
	movb	$0, 16(%rsi)
	movl	(%rsi), %r8d
	notl	%eax
	andl	%eax, %edx
	movb	%dl, _async_active_mask(%rip)
	movzbl	_async_used_mask(%rip), %edx
	andl	%edx, %eax
	movb	%al, _async_used_mask(%rip)
	testb	%dil, %cl
	je	.L190
	movl	_async_next_target(%rip), %eax
	cmpl	%eax, %r8d
	je	.L197
.L190:
	movb	%r9b, SREG(%rip)
	ret
	.p2align 4,,10
	.p2align 3
.L197:
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L190
	call	_async_recompute_next
	jmp	.L190
	.cfi_endproc
.LFE82:
	.size	async_delay_cancel.part.0, .-async_delay_cancel.part.0
	.p2align 4
	.type	async_delay_is_active.part.0, @function
async_delay_is_active.part.0:
.LFB83:
	.cfi_startproc
	movzbl	%dil, %edi
	leaq	_async_slot_bit(%rip), %rdx
	movzbl	_async_active_mask(%rip), %eax
	andb	(%rdx,%rdi), %al
	setne	%al
	ret
	.cfi_endproc
.LFE83:
	.size	async_delay_is_active.part.0, .-async_delay_is_active.part.0
	.p2align 4
	.type	async_delay_remaining, @function
async_delay_remaining:
.LFB61:
	.cfi_startproc
	cmpb	$7, %dil
	ja	.L202
	movzbl	%dil, %ecx
	movl	%ecx, %edi
	call	async_delay_is_active.part.0
	testb	%al, %al
	je	.L202
	movslq	%ecx, %rdi
	movzbl	SREG(%rip), %esi
	leaq	_async_slots(%rip), %rax
	movl	_async_tick_counter(%rip), %edx
	imulq	$24, %rdi, %rdi
	movl	%edx, %ecx
	movb	%sil, SREG(%rip)
	movl	(%rax,%rdi), %eax
	subl	%eax, %ecx
	cmpl	$32766, %ecx
	jbe	.L202
	subl	%edx, %eax
	ret
	.p2align 4,,10
	.p2align 3
.L202:
	xorl	%eax, %eax
	ret
	.cfi_endproc
.LFE61:
	.size	async_delay_remaining, .-async_delay_remaining
	.p2align 4
	.type	async_delay_cancel_all, @function
async_delay_cancel_all:
.LFB63:
	.cfi_startproc
	leaq	_async_slots(%rip), %r9
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	leaq	_async_slot_bit(%rip), %r10
	leaq	192(%r9), %rbx
	jmp	.L208
	.p2align 4,,10
	.p2align 3
.L207:
	addq	$24, %r9
	movb	%r11b, SREG(%rip)
	addq	$1, %r10
	cmpq	%rbx, %r9
	je	.L217
.L208:
	movzbl	(%r10), %ecx
	movzbl	SREG(%rip), %r11d
	movb	$0, 16(%r9)
	movzbl	_async_active_mask(%rip), %esi
	movzbl	_async_active_mask(%rip), %edx
	movl	%ecx, %eax
	movl	(%r9), %edi
	notl	%eax
	andl	%eax, %edx
	movb	%dl, _async_active_mask(%rip)
	movzbl	_async_used_mask(%rip), %edx
	andl	%edx, %eax
	movb	%al, _async_used_mask(%rip)
	testb	%sil, %cl
	je	.L207
	movl	_async_next_target(%rip), %eax
	cmpl	%eax, %edi
	jne	.L207
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L207
	call	_async_recompute_next
	jmp	.L207
	.p2align 4,,10
	.p2align 3
.L217:
	popq	%rbx
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE63:
	.size	async_delay_cancel_all, .-async_delay_cancel_all
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC0:
	.string	"========================================"
	.align 8
.LC1:
	.string	"  async_delay Host Unit Test Suite"
	.align 8
.LC2:
	.string	"  Config: BITS=%d, SLOTS=%d, DEFERRED=%d\n"
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC3:
	.string	"Running test_init..."
.LC4:
	.string	"active count is 0 after init"
.LC5:
	.string	"  [FAIL] Line %d: %s\n"
	.section	.rodata.str1.8
	.align 8
.LC6:
	.string	"ticks until next is 0 after init"
	.align 8
.LC7:
	.string	"Running test_oneshot_polling..."
	.section	.rodata.str1.1
.LC8:
	.string	"slot allocated successfully"
.LC9:
	.string	"slot is active"
.LC10:
	.string	"active count is 1"
.LC11:
	.string	"remaining ticks is 50"
.LC12:
	.string	"ticks until next is 50"
	.section	.rodata.str1.8
	.align 8
.LC13:
	.string	"remaining ticks is 30 after 20 ticks"
	.align 8
.LC14:
	.string	"ticks until next is 30 after 20 ticks"
	.section	.rodata.str1.1
.LC15:
	.string	"not elapsed at 20 ticks"
	.section	.rodata.str1.8
	.align 8
.LC16:
	.string	"slot is no longer active at 50 ticks"
	.section	.rodata.str1.1
.LC17:
	.string	"remaining ticks is 0"
	.section	.rodata.str1.8
	.align 8
.LC18:
	.string	"second call to elapsed returns 0 (freed)"
	.align 8
.LC19:
	.string	"active count is 0 after polling complete"
	.align 8
.LC20:
	.string	"Running test_oneshot_callback..."
	.section	.rodata.str1.1
.LC21:
	.string	"callback slot allocated"
	.section	.rodata.str1.8
	.align 8
.LC22:
	.string	"callback not fired at 39 ticks"
	.section	.rodata.str1.1
.LC23:
	.string	"callback fired at 40 ticks"
	.section	.rodata.str1.8
	.align 8
.LC24:
	.string	"callback received correct slot id"
	.section	.rodata.str1.1
.LC25:
	.string	"callback did not fire again"
.LC26:
	.string	"slot automatically freed"
	.section	.rodata.str1.8
	.align 8
.LC27:
	.string	"Running test_periodic_callback..."
	.section	.rodata.str1.1
.LC28:
	.string	"periodic slot allocated"
	.section	.rodata.str1.8
	.align 8
.LC29:
	.string	"periodic callback not fired at 24 ticks"
	.section	.rodata.str1.1
.LC30:
	.string	"periodic fired at 25 ticks"
.LC31:
	.string	"periodic fired at 50 ticks"
.LC32:
	.string	"periodic fired at 75 ticks"
.LC33:
	.string	"periodic slot cancelled"
	.section	.rodata.str1.8
	.align 8
.LC34:
	.string	"periodic does not fire after cancel"
	.align 8
.LC35:
	.string	"Running test_cancel_and_cancel_all..."
	.section	.rodata.str1.1
.LC36:
	.string	"3 slots active"
	.section	.rodata.str1.8
	.align 8
.LC37:
	.string	"2 slots active after single cancel"
	.section	.rodata.str1.1
.LC38:
	.string	"id2 is inactive"
.LC39:
	.string	"id1 is still active"
.LC40:
	.string	"id3 is still active"
	.section	.rodata.str1.8
	.align 8
.LC41:
	.string	"0 slots active after cancel_all"
	.section	.rodata.str1.1
.LC42:
	.string	"id1 is inactive"
.LC43:
	.string	"id3 is inactive"
.LC44:
	.string	"Running test_restart..."
	.section	.rodata.str1.8
	.align 8
.LC45:
	.string	"remaining is 60 before restart"
	.section	.rodata.str1.1
.LC46:
	.string	"restart succeeded"
.LC47:
	.string	"remaining is now 30"
	.section	.rodata.str1.8
	.align 8
.LC48:
	.string	"not elapsed at 29 ticks after restart"
	.align 8
.LC49:
	.string	"elapsed at 30 ticks after restart"
	.section	.rodata.str1.1
.LC50:
	.string	"Running test_counter_wrap..."
	.section	.rodata.str1.8
	.align 8
.LC51:
	.string	"allocated across wrap boundary"
	.align 8
.LC52:
	.string	"remaining is 20 across wrap boundary"
	.align 8
.LC53:
	.string	"not elapsed at 19 ticks across wrap"
	.align 8
.LC54:
	.string	"elapsed correctly fires across wrap boundary"
	.align 8
.LC55:
	.string	"Running test_slot_exhaustion..."
	.section	.rodata.str1.1
.LC56:
	.string	"slot allocation under limit"
.LC57:
	.string	"allocation beyond max fails"
	.section	.rodata.str1.8
	.align 8
.LC58:
	.string	"reallocation after free succeeds"
	.section	.rodata.str1.1
.LC59:
	.string	"Running test_lut_bitmask..."
.LC60:
	.string	"LUT slot bit matches (1 << i)"
	.section	.rodata.str1.8
	.align 8
.LC61:
	.string	"LUT slot clr matches ~(1 << i)"
	.align 8
.LC62:
	.string	"Running test_lut_alloc_and_popcount..."
	.section	.rodata.str1.1
.LC63:
	.string	"popcount 0 on empty"
.LC64:
	.string	"first allocated slot is 0"
.LC65:
	.string	"popcount is 1"
.LC66:
	.string	"second allocated slot is 1"
.LC67:
	.string	"popcount is 2"
.LC68:
	.string	"allocated slots 2 and 3"
.LC69:
	.string	"popcount is 4"
	.section	.rodata.str1.8
	.align 8
.LC70:
	.string	"popcount is 3 after cancel slot 1"
	.align 8
.LC71:
	.string	"LUT allocator immediately reused freed slot 1"
	.section	.rodata.str1.1
.LC72:
	.string	"popcount back to 4"
.LC73:
	.string	"popcount 0 after cancel all"
	.section	.rodata.str1.8
	.align 8
.LC74:
	.string	"\n----------------------------------------"
	.align 8
.LC75:
	.string	"Tests Run: %d | Passed: %d | Failed: %d\n"
	.align 8
.LC76:
	.string	"----------------------------------------"
	.section	.rodata.str1.1
.LC77:
	.string	">>> ALL TESTS PASSED! <<<\n"
.LC78:
	.string	">>> SOME TESTS FAILED! <<<\n"
.LC79:
	.string	"elapsed returns 1 at 50 ticks"
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB80:
	.cfi_startproc
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	leaq	.LC0(%rip), %rbx
	movq	%rbx, %rdi
	subq	$56, %rsp
	.cfi_def_cfa_offset 112
	movq	%fs:40, %rax
	movq	%rax, 40(%rsp)
	xorl	%eax, %eax
	call	puts@PLT
	leaq	.LC1(%rip), %rdi
	call	puts@PLT
	xorl	%r8d, %r8d
	movl	$8, %ecx
	xorl	%eax, %eax
	movl	$16, %edx
	leaq	.LC2(%rip), %rsi
	movl	$1, %edi
	call	__printf_chk@PLT
	movq	%rbx, %rdi
	leaq	_async_popcount_nibble(%rip), %rbx
	call	puts@PLT
	leaq	.LC3(%rip), %rdi
	call	puts@PLT
	call	async_delay_init
	movzbl	_async_active_mask(%rip), %edx
	movl	test_total(%rip), %eax
	movq	%rdx, %rcx
	shrb	$4, %dl
	addl	$1, %eax
	andl	$15, %edx
	andl	$15, %ecx
	movl	%eax, test_total(%rip)
	movzbl	(%rbx,%rdx), %edx
	addb	(%rbx,%rcx), %dl
	jne	.L219
	addl	$1, test_passed(%rip)
.L220:
	addl	$1, %eax
	movl	%eax, test_total(%rip)
	call	async_delay_ticks_until_next
	testl	%eax, %eax
	jne	.L221
	addl	$1, test_passed(%rip)
.L222:
	leaq	.LC7(%rip), %rdi
	call	puts@PLT
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$50, %edi
	call	async_delay_init
	call	_async_delay_start_common
	movl	%eax, %ebp
	movl	test_total(%rip), %eax
	leal	1(%rax), %edx
	movl	%edx, test_total(%rip)
	cmpb	$-1, %bpl
	je	.L223
	movl	test_passed(%rip), %ecx
	addl	$2, %eax
	movzbl	%bpl, %r12d
	leaq	.LC5(%rip), %rsi
	movl	%eax, test_total(%rip)
	leal	1(%rcx), %edx
	movl	%edx, test_passed(%rip)
	cmpb	$7, %bpl
	ja	.L226
	movl	%r12d, %edi
	leaq	.LC5(%rip), %rsi
	call	async_delay_is_active.part.0
	subb	$1, %al
	je	.L495
.L226:
	leaq	.LC9(%rip), %rcx
	movl	$90, %edx
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk@PLT
.L227:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	cmpb	$1, %al
	jne	.L228
	addl	$1, test_passed(%rip)
.L229:
	movl	%r12d, %edi
	addl	$1, test_total(%rip)
	call	async_delay_remaining
	cmpl	$50, %eax
	jne	.L230
	addl	$1, test_passed(%rip)
.L231:
	addl	$1, test_total(%rip)
	call	async_delay_ticks_until_next
	cmpl	$50, %eax
	jne	.L232
	addl	$1, test_passed(%rip)
.L233:
	movl	$20, %r13d
	.p2align 4,,10
	.p2align 3
.L235:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L234
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L234
	call	_async_delay_tick_walk
.L234:
	subl	$1, %r13d
	jne	.L235
	movl	%r12d, %edi
	addl	$1, test_total(%rip)
	call	async_delay_remaining
	cmpl	$30, %eax
	jne	.L236
	addl	$1, test_passed(%rip)
.L237:
	addl	$1, test_total(%rip)
	call	async_delay_ticks_until_next
	cmpl	$30, %eax
	jne	.L238
	addl	$1, test_passed(%rip)
.L239:
	addl	$1, test_total(%rip)
	cmpb	$7, %bpl
	ja	.L242
	movl	%r12d, %edi
	call	async_delay_elapsed.part.0
	testb	%al, %al
	jne	.L496
.L242:
	addl	$1, test_passed(%rip)
.L241:
	movl	$30, %r13d
	.p2align 4,,10
	.p2align 3
.L244:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L243
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L243
	call	_async_delay_tick_walk
.L243:
	subl	$1, %r13d
	jne	.L244
	movl	test_total(%rip), %eax
	leal	1(%rax), %r8d
	movl	%r8d, test_total(%rip)
	cmpb	$7, %bpl
	ja	.L247
	movl	%r12d, %edi
	call	async_delay_is_active.part.0
	testb	%al, %al
	jne	.L497
.L247:
	addl	$1, test_passed(%rip)
.L246:
	addl	$1, %r8d
	movl	%r12d, %edi
	movl	%r8d, test_total(%rip)
	call	async_delay_remaining
	testl	%eax, %eax
	jne	.L248
	addl	$1, test_passed(%rip)
.L249:
	leal	1(%r8), %eax
	movl	%eax, test_total(%rip)
	cmpb	$7, %bpl
	ja	.L250
	movl	%r12d, %edi
	call	async_delay_elapsed.part.0
	subb	$1, %al
	je	.L498
	leaq	.LC79(%rip), %rcx
	movl	$103, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %eax
	leal	1(%rax), %r8d
.L396:
	movl	%r12d, %edi
	movl	%r8d, test_total(%rip)
	call	async_delay_elapsed.part.0
	testb	%al, %al
	jne	.L252
.L397:
	addl	$1, test_passed(%rip)
.L253:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %eax
	andl	$15, %edx
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	jne	.L254
	addl	$1, test_passed(%rip)
.L255:
	leaq	.LC20(%rip), %rdi
	call	puts@PLT
	xorl	%edx, %edx
	movl	$40, %edi
	leaq	test_cb_oneshot(%rip), %rsi
	call	async_delay_init
	movl	$0, cb_count1(%rip)
	call	_async_delay_start_common
	addl	$1, test_total(%rip)
	movl	%eax, %ebp
	cmpb	$-1, %al
	je	.L256
	addl	$1, test_passed(%rip)
.L257:
	movl	$39, %r12d
	.p2align 4,,10
	.p2align 3
.L259:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L258
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L258
	call	_async_delay_tick_walk
.L258:
	subl	$1, %r12d
	jne	.L259
	movl	cb_count1(%rip), %eax
	addl	$1, test_total(%rip)
	testl	%eax, %eax
	jne	.L260
	addl	$1, test_passed(%rip)
.L263:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L261
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L261
	call	_async_delay_tick_walk
.L261:
	movl	cb_count1(%rip), %eax
	addl	$1, test_total(%rip)
	cmpl	$1, %eax
	jne	.L264
	addl	$1, test_passed(%rip)
.L265:
	movzbl	last_cb_slot(%rip), %eax
	addl	$1, test_total(%rip)
	cmpb	%al, %bpl
	jne	.L266
	addl	$1, test_passed(%rip)
.L267:
	movl	$20, %ebp
	.p2align 4,,10
	.p2align 3
.L269:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L268
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L268
	call	_async_delay_tick_walk
.L268:
	subl	$1, %ebp
	jne	.L269
	movl	cb_count1(%rip), %eax
	addl	$1, test_total(%rip)
	cmpl	$1, %eax
	jne	.L270
	addl	$1, test_passed(%rip)
.L271:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %eax
	andl	$15, %edx
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	jne	.L272
	addl	$1, test_passed(%rip)
.L273:
	leaq	.LC27(%rip), %rdi
	call	puts@PLT
	movl	$1, %edx
	movl	$25, %edi
	leaq	test_cb_periodic(%rip), %rsi
	call	async_delay_init
	movl	$0, cb_count2(%rip)
	call	_async_delay_start_common
	addl	$1, test_total(%rip)
	movl	%eax, %ebp
	cmpb	$-1, %al
	je	.L274
	addl	$1, test_passed(%rip)
.L275:
	movl	$24, %r12d
	.p2align 4,,10
	.p2align 3
.L277:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L276
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L276
	call	_async_delay_tick_walk
.L276:
	subl	$1, %r12d
	jne	.L277
	movl	cb_count2(%rip), %eax
	addl	$1, test_total(%rip)
	testl	%eax, %eax
	jne	.L278
	addl	$1, test_passed(%rip)
.L281:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L279
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L279
	call	_async_delay_tick_walk
.L279:
	movl	cb_count2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpl	$1, %eax
	jne	.L282
	addl	$1, test_passed(%rip)
.L283:
	movl	$25, %r12d
	.p2align 4,,10
	.p2align 3
.L285:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L284
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L284
	call	_async_delay_tick_walk
.L284:
	subl	$1, %r12d
	jne	.L285
	movl	cb_count2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpl	$2, %eax
	jne	.L286
	addl	$1, test_passed(%rip)
.L287:
	movl	$25, %r12d
	.p2align 4,,10
	.p2align 3
.L289:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L288
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L288
	call	_async_delay_tick_walk
.L288:
	subl	$1, %r12d
	jne	.L289
	movl	cb_count2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpl	$3, %eax
	jne	.L290
	addl	$1, test_passed(%rip)
.L291:
	cmpb	$7, %bpl
	ja	.L292
	movzbl	%bpl, %edi
	call	async_delay_cancel.part.0
.L292:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %eax
	andl	$15, %edx
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	jne	.L293
	addl	$1, test_passed(%rip)
.L294:
	movl	$50, %ebp
	.p2align 4,,10
	.p2align 3
.L296:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L295
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L295
	call	_async_delay_tick_walk
.L295:
	subl	$1, %ebp
	jne	.L296
	movl	cb_count2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpl	$3, %eax
	jne	.L297
	addl	$1, test_passed(%rip)
.L298:
	leaq	.LC35(%rip), %rdi
	call	puts@PLT
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$40, %edi
	call	async_delay_init
	call	_async_delay_start_common
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$60, %edi
	movl	%eax, %r12d
	call	_async_delay_start_common
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$80, %edi
	movl	%eax, %r13d
	call	_async_delay_start_common
	addl	$1, test_total(%rip)
	movl	%eax, %ebp
	movzbl	_async_active_mask(%rip), %eax
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	cmpb	$3, %al
	jne	.L299
	addl	$1, test_passed(%rip)
.L300:
	movzbl	%r13b, %r14d
	cmpb	$7, %r13b
	ja	.L301
	movl	%r14d, %edi
	call	async_delay_cancel.part.0
.L301:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	cmpb	$2, %al
	jne	.L302
	addl	$1, test_passed(%rip)
.L303:
	movl	test_total(%rip), %eax
	leal	1(%rax), %ecx
	movl	%ecx, test_total(%rip)
	cmpb	$7, %r13b
	ja	.L306
	movl	%r14d, %edi
	call	async_delay_is_active.part.0
	testb	%al, %al
	jne	.L499
.L306:
	addl	$1, test_passed(%rip)
.L305:
	addl	$1, %ecx
	movzbl	%r12b, %r14d
	movl	%ecx, test_total(%rip)
	cmpb	$7, %r12b
	ja	.L307
	movl	%r14d, %edi
	call	async_delay_is_active.part.0
	subb	$1, %al
	je	.L500
.L307:
	leaq	.LC39(%rip), %rcx
	movl	$210, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
.L308:
	addl	$1, test_total(%rip)
	movzbl	%bpl, %r13d
	cmpb	$7, %bpl
	ja	.L309
	movl	%r13d, %edi
	call	async_delay_is_active.part.0
	subb	$1, %al
	je	.L501
.L309:
	leaq	.LC40(%rip), %rcx
	movl	$211, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
.L310:
	call	async_delay_cancel_all
	movzbl	_async_active_mask(%rip), %edx
	movl	test_total(%rip), %eax
	movq	%rdx, %rcx
	shrb	$4, %dl
	addl	$1, %eax
	andl	$15, %edx
	andl	$15, %ecx
	movl	%eax, test_total(%rip)
	movzbl	(%rbx,%rdx), %edx
	addb	(%rbx,%rcx), %dl
	jne	.L311
	addl	$1, test_passed(%rip)
.L312:
	leal	1(%rax), %ecx
	movl	%ecx, test_total(%rip)
	cmpb	$7, %r12b
	ja	.L315
	movl	%r14d, %edi
	call	async_delay_is_active.part.0
	testb	%al, %al
	jne	.L502
.L315:
	addl	$1, test_passed(%rip)
.L314:
	leal	1(%rcx), %eax
	movl	%eax, test_total(%rip)
	cmpb	$7, %bpl
	ja	.L318
	movl	%r13d, %edi
	call	async_delay_is_active.part.0
	testb	%al, %al
	jne	.L503
.L318:
	addl	$1, test_passed(%rip)
.L317:
	leaq	.LC44(%rip), %rdi
	movl	$40, %ebp
	call	puts@PLT
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	call	async_delay_init
	call	_async_delay_start_common
	movl	%eax, %r12d
	.p2align 4,,10
	.p2align 3
.L320:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L319
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L319
	call	_async_delay_tick_walk
.L319:
	subl	$1, %ebp
	jne	.L320
	movzbl	%r12b, %r13d
	addl	$1, test_total(%rip)
	movl	%r13d, %edi
	call	async_delay_remaining
	cmpl	$60, %eax
	jne	.L321
	addl	$1, test_passed(%rip)
.L322:
	movl	test_total(%rip), %eax
	leaq	_async_slot_bit(%rip), %rbp
	addl	$1, %eax
	cmpb	$7, %r12b
	ja	.L323
	movslq	%r13d, %rdx
	movzbl	_async_used_mask(%rip), %esi
	movzbl	0(%rbp,%rdx), %ecx
	testb	%sil, %cl
	je	.L323
	imulq	$24, %rdx, %rsi
	leaq	_async_slots(%rip), %rdx
	movzbl	SREG(%rip), %r8d
	movl	_async_tick_counter(%rip), %edi
	addl	$30, %edi
	addq	%rsi, %rdx
	movzbl	16(%rdx), %esi
	movl	$30, 4(%rdx)
	movl	%edi, (%rdx)
	andl	$4, %esi
	cmpb	$1, %sil
	sbbl	%esi, %esi
	andl	$-4, %esi
	addl	$5, %esi
	movb	%sil, 16(%rdx)
	movzbl	_async_active_mask(%rip), %edx
	testb	%dl, %dl
	je	.L325
	movl	_async_next_target(%rip), %esi
	subl	%edi, %esi
	cmpl	$32766, %esi
	jbe	.L325
.L326:
	orl	%edx, %ecx
	addl	$1, test_passed(%rip)
	movb	%cl, _async_active_mask(%rip)
	movl	%eax, test_total(%rip)
	movb	%r8b, SREG(%rip)
.L327:
	movl	%r13d, %edi
	addl	$1, test_total(%rip)
	call	async_delay_remaining
	cmpl	$30, %eax
	jne	.L328
	addl	$1, test_passed(%rip)
.L329:
	movl	$29, %r14d
	.p2align 4,,10
	.p2align 3
.L331:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L330
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L330
	call	_async_delay_tick_walk
.L330:
	subl	$1, %r14d
	jne	.L331
	movl	test_total(%rip), %r8d
	leal	1(%r8), %eax
	movl	%eax, test_total(%rip)
	cmpb	$7, %r12b
	ja	.L332
	movl	%r13d, %edi
	call	async_delay_elapsed.part.0
	testb	%al, %al
	jne	.L504
	movl	_async_tick_counter(%rip), %eax
	addl	$1, test_passed(%rip)
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L505
.L338:
	movl	_async_tick_counter(%rip), %edx
	movl	_async_next_target(%rip), %eax
	subl	%eax, %edx
	movl	test_total(%rip), %eax
	cmpl	$32766, %edx
	ja	.L339
	call	_async_delay_tick_walk
	movl	test_total(%rip), %eax
.L339:
	addl	$1, %eax
	movl	%eax, test_total(%rip)
	cmpb	$7, %r12b
	ja	.L340
.L395:
	movl	%r13d, %edi
	call	async_delay_elapsed.part.0
	subb	$1, %al
	je	.L506
	.p2align 4,,10
	.p2align 3
.L340:
	leaq	.LC49(%rip), %rcx
	movl	$242, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
.L341:
	leaq	.LC50(%rip), %rdi
	call	puts@PLT
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$20, %edi
	call	async_delay_init
	movl	$65525, _async_tick_counter(%rip)
	call	_async_delay_start_common
	movl	%eax, %r12d
	movl	test_total(%rip), %eax
	addl	$1, %eax
	movl	%eax, test_total(%rip)
	cmpb	$-1, %r12b
	je	.L342
	addl	$1, test_passed(%rip)
.L343:
	movzbl	%r12b, %r13d
	addl	$1, %eax
	movl	%r13d, %edi
	movl	%eax, test_total(%rip)
	call	async_delay_remaining
	cmpl	$20, %eax
	jne	.L344
	addl	$1, test_passed(%rip)
.L345:
	movl	$19, %r14d
	.p2align 4,,10
	.p2align 3
.L347:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L346
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L346
	call	_async_delay_tick_walk
.L346:
	subl	$1, %r14d
	jne	.L347
	movl	test_total(%rip), %r8d
	leal	1(%r8), %eax
	movl	%eax, test_total(%rip)
	cmpb	$7, %r12b
	ja	.L348
	movl	%r13d, %edi
	call	async_delay_elapsed.part.0
	testb	%al, %al
	jne	.L507
	movl	_async_tick_counter(%rip), %eax
	addl	$1, test_passed(%rip)
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L508
.L354:
	movl	_async_tick_counter(%rip), %edx
	movl	_async_next_target(%rip), %eax
	subl	%eax, %edx
	movl	test_total(%rip), %eax
	cmpl	$32766, %edx
	ja	.L355
	call	_async_delay_tick_walk
	movl	test_total(%rip), %eax
.L355:
	addl	$1, %eax
	movl	%eax, test_total(%rip)
	cmpb	$7, %r12b
	ja	.L356
.L394:
	movl	%r13d, %edi
	call	async_delay_elapsed.part.0
	subb	$1, %al
	je	.L509
	.p2align 4,,10
	.p2align 3
.L356:
	leaq	.LC54(%rip), %rcx
	movl	$273, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
.L357:
	leaq	.LC55(%rip), %rdi
	leaq	30(%rsp), %r15
	call	puts@PLT
	leaq	38(%rsp), %r12
	leaq	.LC56(%rip), %r14
	call	async_delay_init
	movl	test_total(%rip), %r13d
	jmp	.L360
	.p2align 4,,10
	.p2align 3
.L511:
	addq	$1, %r15
	addl	$1, test_passed(%rip)
	cmpq	%r12, %r15
	je	.L510
.L360:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	addl	$1, %r13d
	call	_async_delay_start_common
	movl	%r13d, test_total(%rip)
	movb	%al, (%r15)
	cmpb	$-1, %al
	jne	.L511
	movq	%r14, %rcx
	movl	$289, %edx
	movl	$1, %edi
	xorl	%eax, %eax
	leaq	.LC5(%rip), %rsi
	addq	$1, %r15
	call	__printf_chk@PLT
	movl	test_total(%rip), %r13d
	cmpq	%r12, %r15
	jne	.L360
.L510:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	call	_async_delay_start_common
	leal	1(%r13), %ecx
	movl	%ecx, test_total(%rip)
	cmpb	$-1, %al
	jne	.L361
	addl	$1, test_passed(%rip)
.L362:
	movzbl	30(%rsp), %edi
	cmpb	$7, %dil
	ja	.L363
	call	async_delay_cancel.part.0
.L363:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	call	_async_delay_start_common
	addl	$1, test_total(%rip)
	cmpb	$-1, %al
	je	.L364
	addl	$1, test_passed(%rip)
.L365:
	call	async_delay_cancel_all
	xorl	%r14d, %r14d
	leaq	.LC59(%rip), %rdi
	movl	$1, %r12d
	call	puts@PLT
	leaq	.LC60(%rip), %r15
	leaq	.LC5(%rip), %r13
	jmp	.L368
	.p2align 4,,10
	.p2align 3
.L513:
	addl	$2, %eax
	addq	$1, %r14
	addl	$2, test_passed(%rip)
	movl	%eax, test_total(%rip)
	cmpq	$8, %r14
	je	.L512
.L368:
	movl	test_total(%rip), %eax
	movl	%r14d, %ecx
	leal	1(%rax), %edx
	movl	%edx, test_total(%rip)
	movl	%r12d, %edx
	sall	%cl, %edx
	cmpb	%dl, 0(%rbp,%r14)
	je	.L513
	movq	%r15, %rcx
	movl	$313, %edx
	movq	%r13, %rsi
	movl	$1, %edi
	xorl	%eax, %eax
	addq	$1, %r14
	call	__printf_chk@PLT
	movl	$314, %edx
	movq	%r13, %rsi
	xorl	%eax, %eax
	leaq	.LC61(%rip), %rcx
	movl	$1, %edi
	addl	$1, test_total(%rip)
	call	__printf_chk@PLT
	cmpq	$8, %r14
	jne	.L368
.L512:
	leaq	.LC62(%rip), %rdi
	call	puts@PLT
	call	async_delay_init
	movl	test_total(%rip), %eax
	leal	1(%rax), %ebp
	movzbl	_async_active_mask(%rip), %eax
	movl	%ebp, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %eax
	andl	$15, %edx
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	jne	.L369
	addl	$1, test_passed(%rip)
.L370:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	addl	$1, %ebp
	call	_async_delay_start_common
	movl	%ebp, test_total(%rip)
	testb	%al, %al
	jne	.L371
	addl	$1, test_passed(%rip)
.L372:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, %ebp
	movl	%ebp, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	cmpb	$1, %al
	jne	.L373
	addl	$1, test_passed(%rip)
.L374:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	call	_async_delay_start_common
	addl	$1, test_total(%rip)
	movl	%eax, %ebp
	cmpb	$1, %al
	jne	.L375
	addl	$1, test_passed(%rip)
.L376:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	cmpb	$2, %al
	jne	.L377
	addl	$1, test_passed(%rip)
.L378:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	call	_async_delay_start_common
	xorl	%edx, %edx
	movl	$100, %edi
	xorl	%esi, %esi
	movl	%eax, %r12d
	call	_async_delay_start_common
	movl	test_total(%rip), %edi
	leal	1(%rdi), %edx
	movl	%edx, test_total(%rip)
	cmpb	$2, %r12b
	jne	.L379
	cmpb	$3, %al
	jne	.L379
	addl	$1, test_passed(%rip)
.L380:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, %edx
	movl	%edx, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	cmpb	$4, %al
	jne	.L381
	addl	$1, test_passed(%rip)
.L382:
	cmpb	$7, %bpl
	ja	.L383
	movzbl	%bpl, %edi
	call	async_delay_cancel.part.0
.L383:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	cmpb	$3, %al
	jne	.L384
	addl	$1, test_passed(%rip)
.L385:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	call	_async_delay_start_common
	addl	$1, test_total(%rip)
	cmpb	$1, %al
	jne	.L386
	addl	$1, test_passed(%rip)
.L387:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	cmpb	$4, %al
	jne	.L388
	addl	$1, test_passed(%rip)
.L389:
	call	async_delay_cancel_all
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %eax
	andl	$15, %edx
	movzbl	(%rbx,%rax), %eax
	addb	(%rbx,%rdx), %al
	jne	.L390
	addl	$1, test_passed(%rip)
.L391:
	leaq	.LC74(%rip), %rdi
	call	puts@PLT
	movl	test_total(%rip), %edx
	movl	test_passed(%rip), %ecx
	xorl	%eax, %eax
	leaq	.LC75(%rip), %rsi
	movl	$1, %edi
	movl	%edx, %r8d
	subl	%ecx, %r8d
	call	__printf_chk@PLT
	leaq	.LC76(%rip), %rdi
	call	puts@PLT
	movl	test_total(%rip), %eax
	cmpl	%eax, test_passed(%rip)
	je	.L514
	leaq	.LC78(%rip), %rdi
	call	puts@PLT
	movl	$1, %eax
.L218:
	movq	40(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L515
	addq	$56, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
.L498:
	.cfi_restore_state
	addl	$1, test_passed(%rip)
	addl	$2, %r8d
	jmp	.L396
.L495:
	addl	$2, %ecx
	movl	%ecx, test_passed(%rip)
	jmp	.L227
.L497:
	leaq	.LC16(%rip), %rcx
	movl	$101, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %r8d
	jmp	.L246
.L496:
	leaq	.LC15(%rip), %rcx
	movl	$98, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L241
.L325:
	movl	%edi, _async_next_target(%rip)
	jmp	.L326
.L381:
	leaq	.LC69(%rip), %rcx
	movl	$343, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L382
.L388:
	leaq	.LC72(%rip), %rcx
	movl	$350, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L389
.L386:
	leaq	.LC71(%rip), %rcx
	movl	$349, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L387
.L384:
	leaq	.LC70(%rip), %rcx
	movl	$347, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L385
.L377:
	leaq	.LC67(%rip), %rcx
	movl	$336, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L378
.L375:
	leaq	.LC66(%rip), %rcx
	movl	$335, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L376
.L373:
	leaq	.LC65(%rip), %rcx
	movl	$331, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L374
.L361:
	leaq	.LC57(%rip), %rcx
	movl	$293, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L362
.L344:
	leaq	.LC52(%rip), %rcx
	movl	$267, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L345
.L236:
	leaq	.LC13(%rip), %rcx
	movl	$96, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L237
.L232:
	leaq	.LC12(%rip), %rcx
	movl	$93, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L233
.L230:
	leaq	.LC11(%rip), %rcx
	movl	$92, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L231
.L228:
	leaq	.LC10(%rip), %rcx
	movl	$91, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L229
.L328:
	leaq	.LC47(%rip), %rcx
	movl	$236, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L329
.L238:
	leaq	.LC14(%rip), %rcx
	movl	$97, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L239
.L323:
	movl	%eax, test_total(%rip)
	leaq	.LC46(%rip), %rcx
	xorl	%eax, %eax
	movl	$235, %edx
	leaq	.LC5(%rip), %rsi
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L327
.L321:
	leaq	.LC45(%rip), %rcx
	movl	$231, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L322
.L302:
	leaq	.LC37(%rip), %rcx
	movl	$208, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L303
.L299:
	leaq	.LC36(%rip), %rcx
	movl	$206, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L300
.L297:
	leaq	.LC34(%rip), %rcx
	movl	$188, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L298
.L290:
	leaq	.LC32(%rip), %rcx
	movl	$179, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L291
.L286:
	leaq	.LC31(%rip), %rcx
	movl	$173, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L287
.L282:
	leaq	.LC30(%rip), %rcx
	movl	$167, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L283
.L270:
	leaq	.LC25(%rip), %rcx
	movl	$139, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L271
.L266:
	leaq	.LC24(%rip), %rcx
	movl	$133, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L267
.L264:
	leaq	.LC23(%rip), %rcx
	movl	$132, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L265
.L514:
	leaq	.LC77(%rip), %rdi
	call	puts@PLT
	xorl	%eax, %eax
	jmp	.L218
.L348:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, test_passed(%rip)
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	jne	.L354
	addl	$2, %r8d
	movl	%r8d, test_total(%rip)
	jmp	.L356
.L332:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, test_passed(%rip)
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	jne	.L338
	addl	$2, %r8d
	movl	%r8d, test_total(%rip)
	jmp	.L340
.L250:
	leaq	.LC79(%rip), %rcx
	movl	$103, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	addl	$1, test_total(%rip)
	jmp	.L397
.L379:
	movl	$342, %edx
	leaq	.LC68(%rip), %rcx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %edx
	jmp	.L380
.L371:
	leaq	.LC64(%rip), %rcx
	movl	$330, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %ebp
	jmp	.L372
.L369:
	leaq	.LC63(%rip), %rcx
	movl	$327, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %ebp
	jmp	.L370
.L260:
	leaq	.LC22(%rip), %rcx
	movl	$126, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L263
.L293:
	leaq	.LC33(%rip), %rcx
	movl	$182, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L294
.L272:
	leaq	.LC26(%rip), %rcx
	movl	$140, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L273
.L278:
	leaq	.LC29(%rip), %rcx
	movl	$161, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L281
.L254:
	leaq	.LC19(%rip), %rcx
	movl	$105, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L255
.L311:
	leaq	.LC41(%rip), %rcx
	movl	$214, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %eax
	jmp	.L312
.L390:
	leaq	.LC73(%rip), %rcx
	movl	$354, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L391
.L221:
	leaq	.LC6(%rip), %rcx
	movl	$76, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L222
.L219:
	leaq	.LC4(%rip), %rcx
	movl	$75, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %eax
	jmp	.L220
.L248:
	leaq	.LC17(%rip), %rcx
	movl	$102, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %r8d
	jmp	.L249
.L500:
	addl	$1, test_passed(%rip)
	jmp	.L308
.L501:
	addl	$1, test_passed(%rip)
	jmp	.L310
.L506:
	addl	$1, test_passed(%rip)
	jmp	.L341
.L509:
	addl	$1, test_passed(%rip)
	jmp	.L357
.L499:
	leaq	.LC38(%rip), %rcx
	movl	$209, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %ecx
	jmp	.L305
.L504:
	leaq	.LC48(%rip), %rcx
	movl	$239, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	jne	.L338
	addl	$1, test_total(%rip)
	jmp	.L395
.L502:
	leaq	.LC42(%rip), %rcx
	movl	$215, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %ecx
	jmp	.L314
.L507:
	leaq	.LC53(%rip), %rcx
	movl	$270, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	jne	.L354
	addl	$1, test_total(%rip)
	jmp	.L394
.L252:
	leaq	.LC18(%rip), %rcx
	movl	$104, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L253
.L503:
	leaq	.LC43(%rip), %rcx
	movl	$216, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L317
.L342:
	leaq	.LC51(%rip), %rcx
	movl	$266, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %eax
	jmp	.L343
.L364:
	leaq	.LC58(%rip), %rcx
	movl	$297, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L365
.L256:
	leaq	.LC21(%rip), %rcx
	movl	$120, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L257
.L223:
	leaq	.LC5(%rip), %rsi
	leaq	.LC8(%rip), %rcx
	movl	$89, %edx
	xorl	%eax, %eax
	movl	$1, %edi
	movq	%rsi, 8(%rsp)
	movl	$255, %r12d
	call	__printf_chk@PLT
	addl	$1, test_total(%rip)
	movq	8(%rsp), %rsi
	jmp	.L226
.L274:
	leaq	.LC28(%rip), %rcx
	movl	$155, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L275
.L508:
	addl	$2, %r8d
	movl	%r8d, test_total(%rip)
	jmp	.L394
.L505:
	addl	$2, %r8d
	movl	%r8d, test_total(%rip)
	jmp	.L395
.L515:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE80:
	.size	main, .-main
	.data
	.type	last_cb_slot, @object
	.size	last_cb_slot, 1
last_cb_slot:
	.byte	-1
	.local	cb_count2
	.comm	cb_count2,4,4
	.local	cb_count1
	.comm	cb_count1,4,4
	.local	test_passed
	.comm	test_passed,4,4
	.local	test_total
	.comm	test_total,4,4
	.local	SREG
	.comm	SREG,1,1
	.local	_async_next_target
	.comm	_async_next_target,4,4
	.local	_async_used_mask
	.comm	_async_used_mask,1,1
	.local	_async_active_mask
	.comm	_async_active_mask,1,1
	.local	_async_tick_counter
	.comm	_async_tick_counter,4,4
	.local	_async_slots
	.comm	_async_slots,192,32
	.section	.rodata
	.align 16
	.type	_async_popcount_nibble, @object
	.size	_async_popcount_nibble, 16
_async_popcount_nibble:
	.string	""
	.ascii	"\001\001\002\001\002\002\003\001\002\002\003\002\003\003\004"
	.align 16
	.type	_async_first_free_nibble, @object
	.size	_async_first_free_nibble, 16
_async_first_free_nibble:
	.string	""
	.string	"\001"
	.string	"\002"
	.string	"\001"
	.string	"\003"
	.string	"\001"
	.string	"\002"
	.string	"\001"
	.ascii	"\004"
	.align 8
	.type	_async_slot_bit, @object
	.size	_async_slot_bit, 8
_async_slot_bit:
	.ascii	"\001\002\004\b\020 @\200"
	.ident	"GCC: (Ubuntu 12.3.0-1ubuntu1~22.04.3) 12.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
