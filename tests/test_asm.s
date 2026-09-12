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
	testb	$1, %al
	je	.L5
	movl	%eax, %edx
	movl	_async_slots(%rip), %esi
	andl	$254, %edx
	je	.L111
	testb	$2, %al
	jne	.L119
	testb	$4, %al
	je	.L120
	movl	%esi, %ecx
.L12:
	movl	48+_async_slots(%rip), %edi
	movl	%ecx, %eax
	subl	%edi, %eax
	cmpl	$32766, %eax
	cmova	%ecx, %edi
	jmp	.L17
	.p2align 4,,10
	.p2align 3
.L3:
	ret
	.p2align 4,,10
	.p2align 3
.L5:
	movl	24+_async_slots(%rip), %ecx
	testb	$2, %al
	jne	.L10
	testb	$4, %al
	je	.L15
	movl	48+_async_slots(%rip), %edi
	movl	%eax, %edx
.L17:
	movl	%edx, %eax
	andl	$251, %eax
	je	.L112
	testb	$8, %dl
	jne	.L19
	testb	$16, %dl
	jne	.L65
	testb	$32, %dl
	jne	.L121
	andl	$64, %edx
	je	.L100
.L38:
	movl	144+_async_slots(%rip), %ecx
	movl	%edi, %edx
	subl	%ecx, %edx
	cmpl	$32766, %edx
	cmova	%edi, %ecx
.L44:
	andb	$-65, %al
	je	.L47
.L118:
	jns	.L47
.L109:
	movl	168+_async_slots(%rip), %edi
.L46:
	movl	%ecx, %eax
	subl	%edi, %eax
	cmpl	$32766, %eax
	cmovbe	%edi, %ecx
.L47:
	movl	%ecx, _async_next_target(%rip)
	ret
	.p2align 4,,10
	.p2align 3
.L119:
	movl	24+_async_slots(%rip), %ecx
	movl	%esi, %edi
	movl	%edx, %eax
	subl	%ecx, %edi
	cmpl	$32766, %edi
	cmova	%esi, %ecx
.L10:
	movl	%eax, %edx
	andl	$253, %edx
	je	.L47
	testb	$4, %al
	jne	.L12
	testb	$8, %al
	jne	.L122
	testb	$16, %al
	jne	.L123
	testb	$32, %al
	je	.L35
	movl	%edx, %esi
.L32:
	movl	120+_async_slots(%rip), %edi
	movl	%ecx, %eax
	subl	%edi, %eax
	cmpl	$32766, %eax
	cmova	%ecx, %edi
.L36:
	movl	%esi, %eax
	andl	$223, %eax
	je	.L112
	andl	$64, %esi
	jne	.L38
.L100:
	movl	%edi, %ecx
	testb	%al, %al
	jmp	.L118
	.p2align 4,,10
	.p2align 3
.L111:
	movl	%esi, _async_next_target(%rip)
	ret
	.p2align 4,,10
	.p2align 3
.L112:
	movl	%edi, _async_next_target(%rip)
	ret
	.p2align 4,,10
	.p2align 3
.L122:
	movl	%ecx, %edi
	movl	%edx, %eax
.L19:
	movl	72+_async_slots(%rip), %esi
	movl	%edi, %edx
	subl	%esi, %edx
	cmpl	$32766, %edx
	cmova	%edi, %esi
.L22:
	movl	%eax, %edx
	andl	$247, %edx
	je	.L111
	testb	$16, %al
	jne	.L24
	testb	$32, %al
	jne	.L82
	testb	$64, %al
	jne	.L76
	movl	%esi, %ecx
	testb	%dl, %dl
	jmp	.L118
	.p2align 4,,10
	.p2align 3
.L15:
	testb	$8, %al
	je	.L54
	movl	72+_async_slots(%rip), %esi
	jmp	.L22
	.p2align 4,,10
	.p2align 3
.L120:
	testb	$8, %al
	jne	.L124
	testb	$16, %al
	je	.L125
.L24:
	movl	96+_async_slots(%rip), %ecx
	movl	%esi, %eax
	subl	%ecx, %eax
	cmpl	$32766, %eax
	cmova	%esi, %ecx
.L30:
	movl	%edx, %esi
	andl	$239, %esi
	je	.L47
	testb	$32, %dl
	jne	.L32
	andl	$64, %edx
	je	.L126
	movl	%ecx, %edi
	movl	%esi, %eax
	jmp	.L38
	.p2align 4,,10
	.p2align 3
.L65:
	movl	%edi, %esi
	movl	%eax, %edx
	jmp	.L24
	.p2align 4,,10
	.p2align 3
.L82:
	movl	%esi, %ecx
	movl	%edx, %esi
	jmp	.L32
	.p2align 4,,10
	.p2align 3
.L126:
	testb	%sil, %sil
	jmp	.L118
	.p2align 4,,10
	.p2align 3
.L125:
	testb	$32, %al
	jne	.L82
	andl	$64, %eax
	je	.L42
.L76:
	movl	%esi, %edi
	movl	%edx, %eax
	jmp	.L38
.L35:
	testb	$64, %al
	jne	.L127
	testb	%dl, %dl
	js	.L109
	jmp	.L47
	.p2align 4,,10
	.p2align 3
.L54:
	testb	$16, %al
	je	.L27
	movl	96+_async_slots(%rip), %ecx
	movl	%eax, %edx
	jmp	.L30
.L124:
	movl	%esi, %edi
	movl	%edx, %eax
	jmp	.L19
.L27:
	testb	$32, %al
	je	.L128
	movl	120+_async_slots(%rip), %edi
	movl	%eax, %esi
	jmp	.L36
.L123:
	movl	%ecx, %esi
	jmp	.L24
.L121:
	movl	%edi, %ecx
	movl	%eax, %esi
	jmp	.L32
.L128:
	testb	$64, %al
	je	.L70
	movl	144+_async_slots(%rip), %ecx
	jmp	.L44
.L127:
	movl	%ecx, %edi
	movl	%edx, %eax
	jmp	.L38
.L70:
	movl	%eax, %edx
	xorl	%esi, %esi
	movl	$1, %eax
.L42:
	testb	%dl, %dl
	jns	.L74
	movl	168+_async_slots(%rip), %edi
	movl	%edi, %ecx
	testb	%al, %al
	jne	.L47
	movl	%esi, %ecx
	jmp	.L46
.L74:
	movl	%esi, %ecx
	jmp	.L47
	.cfi_endproc
.LFE52:
	.size	_async_recompute_next, .-_async_recompute_next
	.p2align 4
	.type	_async_delay_start_common, @function
_async_delay_start_common:
.LFB53:
	.cfi_startproc
	movl	%edi, %ecx
	movq	%rsi, %r10
	movl	%edx, %r9d
	movl	$-1, %eax
	cmpl	$32768, %edi
	je	.L129
	movzbl	_async_used_mask(%rip), %edx
	leaq	_async_first_free_nibble(%rip), %rsi
	movq	%rdx, %rax
	andl	$15, %eax
	movzbl	(%rsi,%rax), %eax
	cmpb	$4, %al
	jne	.L131
	shrb	$4, %dl
	andl	$15, %edx
	movzbl	(%rsi,%rdx), %eax
	addl	$4, %eax
.L131:
	cmpb	$7, %al
	ja	.L136
	movzbl	%al, %edx
	leaq	_async_slot_bit(%rip), %rsi
	movzbl	SREG(%rip), %r11d
	movzbl	(%rsi,%rdx), %edi
	movl	_async_tick_counter(%rip), %esi
	leaq	(%rdx,%rdx,2), %r8
	leaq	_async_slots(%rip), %rdx
	leaq	(%rdx,%r8,8), %r8
	addl	%ecx, %esi
	cmpb	$1, %r9b
	movl	%ecx, 4(%r8)
	movzbl	_async_active_mask(%rip), %ecx
	sbbl	%edx, %edx
	movl	%esi, (%r8)
	andl	$-4, %edx
	movq	%r10, 8(%r8)
	addl	$5, %edx
	movb	%dl, 16(%r8)
	testb	%cl, %cl
	jne	.L142
.L133:
	movl	%esi, _async_next_target(%rip)
.L134:
	movzbl	_async_used_mask(%rip), %edx
	orl	%edi, %edx
	orl	%ecx, %edi
	movb	%dl, _async_used_mask(%rip)
	movb	%dil, _async_active_mask(%rip)
	movb	%r11b, SREG(%rip)
	ret
	.p2align 4,,10
	.p2align 3
.L136:
	movl	$-1, %eax
.L129:
	ret
	.p2align 4,,10
	.p2align 3
.L142:
	movl	_async_next_target(%rip), %edx
	subl	%esi, %edx
	cmpl	$32766, %edx
	ja	.L134
	jmp	.L133
	.cfi_endproc
.LFE53:
	.size	_async_delay_start_common, .-_async_delay_start_common
	.p2align 4
	.type	async_delay_ticks_until_next, @function
async_delay_ticks_until_next:
.LFB62:
	.cfi_startproc
	movzbl	SREG(%rip), %edx
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L148
	movl	_async_tick_counter(%rip), %ecx
	movl	_async_next_target(%rip), %eax
	movb	%dl, SREG(%rip)
	movl	%ecx, %edx
	subl	%eax, %edx
	subl	%ecx, %eax
	cmpl	$32766, %edx
	jbe	.L145
	ret
.L148:
	movb	%dl, SREG(%rip)
.L145:
	xorl	%eax, %eax
	ret
	.cfi_endproc
.LFE62:
	.size	async_delay_ticks_until_next, .-async_delay_ticks_until_next
	.p2align 4
	.type	async_delay_cancel_all, @function
async_delay_cancel_all:
.LFB63:
	.cfi_startproc
	movzbl	SREG(%rip), %eax
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
	movb	%al, SREG(%rip)
	ret
	.cfi_endproc
.LFE63:
	.size	async_delay_cancel_all, .-async_delay_cancel_all
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
	je	.L151
	testq	%rdx, %rdx
	je	.L152
	movl	4(%rax), %ecx
	movl	%r8d, %edi
	addl	%ecx, (%rax)
	jmp	*%rdx
	.p2align 4,,10
	.p2align 3
.L151:
	testq	%rdx, %rdx
	je	.L152
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
.L152:
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
	je	.L160
	movl	%ebp, %eax
	subl	_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L190
.L160:
	testb	$2, %bl
	je	.L161
	movl	%ebp, %eax
	subl	24+_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L191
.L161:
	testb	$4, %bl
	je	.L162
	movl	%ebp, %eax
	subl	48+_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L192
.L162:
	testb	$8, %bl
	je	.L163
	movl	%ebp, %eax
	subl	72+_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L193
.L163:
	testb	$16, %bl
	je	.L164
	movl	%ebp, %eax
	subl	96+_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L194
.L164:
	testb	$32, %bl
	je	.L165
	movl	%ebp, %eax
	subl	120+_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L195
.L165:
	testb	$64, %bl
	je	.L166
	movl	%ebp, %eax
	subl	144+_async_slots(%rip), %eax
	cmpl	$32766, %eax
	jbe	.L196
.L166:
	testb	%bl, %bl
	js	.L197
.L167:
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
.L197:
	.cfi_restore_state
	subl	168+_async_slots(%rip), %ebp
	cmpl	$32766, %ebp
	ja	.L167
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
.L196:
	.cfi_restore_state
	movl	$191, %esi
	movl	$6, %edi
	call	_async_delay_expire_slot
	jmp	.L166
	.p2align 4,,10
	.p2align 3
.L190:
	movl	$254, %esi
	xorl	%edi, %edi
	call	_async_delay_expire_slot
	jmp	.L160
	.p2align 4,,10
	.p2align 3
.L191:
	movl	$253, %esi
	movl	$1, %edi
	call	_async_delay_expire_slot
	jmp	.L161
	.p2align 4,,10
	.p2align 3
.L192:
	movl	$251, %esi
	movl	$2, %edi
	call	_async_delay_expire_slot
	jmp	.L162
	.p2align 4,,10
	.p2align 3
.L193:
	movl	$247, %esi
	movl	$3, %edi
	call	_async_delay_expire_slot
	jmp	.L163
	.p2align 4,,10
	.p2align 3
.L194:
	movl	$239, %esi
	movl	$4, %edi
	call	_async_delay_expire_slot
	jmp	.L164
	.p2align 4,,10
	.p2align 3
.L195:
	movl	$223, %esi
	movl	$5, %edi
	call	_async_delay_expire_slot
	jmp	.L165
	.cfi_endproc
.LFE65:
	.size	_async_delay_tick_walk, .-_async_delay_tick_walk
	.p2align 4
	.type	test_cb_oneshot, @function
test_cb_oneshot:
.LFB69:
	.cfi_startproc
	endbr64
	movl	cb_count1(%rip), %eax
	addl	$1, %eax
	movl	%eax, cb_count1(%rip)
	movb	%dil, last_cb_slot(%rip)
	ret
	.cfi_endproc
.LFE69:
	.size	test_cb_oneshot, .-test_cb_oneshot
	.p2align 4
	.type	test_cb_periodic, @function
test_cb_periodic:
.LFB70:
	.cfi_startproc
	endbr64
	movl	cb_count2(%rip), %eax
	addl	$1, %eax
	movl	%eax, cb_count2(%rip)
	ret
	.cfi_endproc
.LFE70:
	.size	test_cb_periodic, .-test_cb_periodic
	.p2align 4
	.type	async_delay_restart, @function
async_delay_restart:
.LFB56:
	.cfi_startproc
	cmpb	$7, %dil
	ja	.L205
	cmpl	$32767, %esi
	ja	.L205
	leaq	_async_slot_bit(%rip), %rax
	movzbl	%dil, %edi
	movzbl	(%rax,%rdi), %edx
	movzbl	_async_used_mask(%rip), %eax
	andb	%dl, %al
	je	.L200
	imulq	$24, %rdi, %rdi
	leaq	_async_slots(%rip), %rax
	movzbl	SREG(%rip), %r9d
	movl	_async_tick_counter(%rip), %r8d
	addl	%esi, %r8d
	addq	%rdi, %rax
	movzbl	16(%rax), %ecx
	movl	%esi, 4(%rax)
	movl	%r8d, (%rax)
	andl	$4, %ecx
	cmpb	$1, %cl
	sbbl	%ecx, %ecx
	andl	$-4, %ecx
	addl	$5, %ecx
	movb	%cl, 16(%rax)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L203
	movl	_async_next_target(%rip), %ecx
	subl	%r8d, %ecx
	cmpl	$32766, %ecx
	ja	.L204
.L203:
	movl	%r8d, _async_next_target(%rip)
.L204:
	orl	%eax, %edx
	movl	$1, %eax
	movb	%dl, _async_active_mask(%rip)
	movb	%r9b, SREG(%rip)
	ret
.L205:
	xorl	%eax, %eax
.L200:
	ret
	.cfi_endproc
.LFE56:
	.size	async_delay_restart, .-async_delay_restart
	.p2align 4
	.type	async_delay_elapsed.part.0, @function
async_delay_elapsed.part.0:
.LFB86:
	.cfi_startproc
	leaq	_async_slot_bit(%rip), %rax
	movzbl	%dil, %edi
	movzbl	(%rax,%rdi), %edx
	movzbl	_async_active_mask(%rip), %eax
	andb	%dl, %al
	jne	.L215
	imulq	$24, %rdi, %rdi
	leaq	_async_slots(%rip), %rcx
	addq	%rdi, %rcx
	movzbl	16(%rcx), %esi
	andl	$3, %esi
	cmpb	$2, %sil
	je	.L216
	ret
.L215:
	xorl	%eax, %eax
	ret
.L216:
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
.LFE86:
	.size	async_delay_elapsed.part.0, .-async_delay_elapsed.part.0
	.p2align 4
	.type	async_delay_cancel.part.0, @function
async_delay_cancel.part.0:
.LFB87:
	.cfi_startproc
	movzbl	%dil, %edi
	leaq	_async_slot_bit(%rip), %rax
	leaq	_async_slots(%rip), %rdx
	movzbl	SREG(%rip), %r8d
	movzbl	(%rax,%rdi), %ecx
	leaq	(%rdi,%rdi,2), %rsi
	movzbl	_async_active_mask(%rip), %edi
	leaq	(%rdx,%rsi,8), %rsi
	movzbl	_async_active_mask(%rip), %edx
	movl	%ecx, %eax
	movb	$0, 16(%rsi)
	movl	(%rsi), %r9d
	notl	%eax
	andl	%eax, %edx
	movb	%dl, _async_active_mask(%rip)
	movzbl	_async_used_mask(%rip), %edx
	andl	%edx, %eax
	movb	%al, _async_used_mask(%rip)
	testb	%dil, %cl
	je	.L218
	movl	_async_next_target(%rip), %eax
	cmpl	%eax, %r9d
	je	.L225
.L218:
	movb	%r8b, SREG(%rip)
	ret
.L225:
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L218
	call	_async_recompute_next
	jmp	.L218
	.cfi_endproc
.LFE87:
	.size	async_delay_cancel.part.0, .-async_delay_cancel.part.0
	.p2align 4
	.type	async_delay_is_active.part.0, @function
async_delay_is_active.part.0:
.LFB88:
	.cfi_startproc
	movzbl	%dil, %edi
	leaq	_async_slot_bit(%rip), %rdx
	movzbl	_async_active_mask(%rip), %eax
	andb	(%rdx,%rdi), %al
	setne	%al
	ret
	.cfi_endproc
.LFE88:
	.size	async_delay_is_active.part.0, .-async_delay_is_active.part.0
	.p2align 4
	.type	async_delay_remaining, @function
async_delay_remaining:
.LFB61:
	.cfi_startproc
	cmpb	$7, %dil
	ja	.L230
	movzbl	%dil, %ecx
	movl	%ecx, %edi
	call	async_delay_is_active.part.0
	testb	%al, %al
	je	.L230
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
	jbe	.L230
	subl	%edx, %eax
	ret
	.p2align 4,,10
	.p2align 3
.L230:
	xorl	%eax, %eax
	ret
	.cfi_endproc
.LFE61:
	.size	async_delay_remaining, .-async_delay_remaining
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
	.string	"Running test_hardware_timer_setup..."
	.align 8
.LC75:
	.string	"Timer2 CTC 8MHz prescaler is /64 (0x0C)"
	.section	.rodata.str1.1
.LC76:
	.string	"Timer2 CTC 8MHz OCR2 is 124"
.LC77:
	.string	"Timer2 OCIE2 enabled"
	.section	.rodata.str1.8
	.align 8
.LC78:
	.string	"Timer2 CTC 16MHz prescaler is /64 (0x0C)"
	.section	.rodata.str1.1
.LC79:
	.string	"Timer2 CTC 16MHz OCR2 is 249"
	.section	.rodata.str1.8
	.align 8
.LC80:
	.string	"Timer2 CTC 4MHz prescaler is /32 (0x0B)"
	.section	.rodata.str1.1
.LC81:
	.string	"Timer2 CTC 4MHz OCR2 is 124"
	.section	.rodata.str1.8
	.align 8
.LC82:
	.string	"Timer2 CTC 2MHz prescaler is /8 (0x0A)"
	.section	.rodata.str1.1
.LC83:
	.string	"Timer2 CTC 2MHz OCR2 is 249"
	.section	.rodata.str1.8
	.align 8
.LC84:
	.string	"Timer2 CTC 1MHz prescaler is /8 (0x0A)"
	.section	.rodata.str1.1
.LC85:
	.string	"Timer2 CTC 1MHz OCR2 is 124"
	.section	.rodata.str1.8
	.align 8
.LC86:
	.string	"Timer1 CTC 8MHz prescaler is /8 (0x0A)"
	.align 8
.LC87:
	.string	"Timer1 CTC 8MHz OCR is 999 (0x03E7)"
	.align 8
.LC88:
	.string	"Timer1 hw init 16MHz/1kHz OCR is 1999 (0x07CF)"
	.align 8
.LC89:
	.string	"Timer2 hw init 8MHz/1kHz prescaler is /32 (0x0B)"
	.align 8
.LC90:
	.string	"Timer2 hw init 8MHz/1kHz OCR2 is 249"
	.align 8
.LC91:
	.string	"Running test_duration_boundary_limits..."
	.align 8
.LC92:
	.string	"start succeeds for duration == half_range"
	.align 8
.LC93:
	.string	"restart fails for new_duration > half_range"
	.align 8
.LC94:
	.string	"restart succeeds for valid duration"
	.align 8
.LC95:
	.string	"\n----------------------------------------"
	.align 8
.LC96:
	.string	"Tests Run: %d | Passed: %d | Failed: %d\n"
	.align 8
.LC97:
	.string	"----------------------------------------"
	.section	.rodata.str1.1
.LC98:
	.string	">>> ALL TESTS PASSED! <<<\n"
.LC99:
	.string	">>> SOME TESTS FAILED! <<<\n"
.LC100:
	.string	"elapsed returns 1 at 50 ticks"
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB84:
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
	leaq	_async_popcount_nibble(%rip), %rbp
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
	movzbl	0(%rbp,%rdx), %edx
	addb	0(%rbp,%rcx), %dl
	jne	.L235
	addl	$1, test_passed(%rip)
.L236:
	addl	$1, %eax
	movl	%eax, test_total(%rip)
	call	async_delay_ticks_until_next
	testl	%eax, %eax
	jne	.L237
	addl	$1, test_passed(%rip)
.L238:
	leaq	.LC7(%rip), %rdi
	call	puts@PLT
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$50, %edi
	call	async_delay_init
	call	_async_delay_start_common
	movl	%eax, %ebx
	movl	test_total(%rip), %eax
	leal	1(%rax), %edx
	movl	%edx, test_total(%rip)
	cmpb	$-1, %bl
	je	.L239
	movl	test_passed(%rip), %ecx
	addl	$2, %eax
	movzbl	%bl, %r12d
	leaq	.LC5(%rip), %rsi
	movl	%eax, test_total(%rip)
	leal	1(%rcx), %edx
	movl	%edx, test_passed(%rip)
	cmpb	$7, %bl
	ja	.L242
	movl	%r12d, %edi
	leaq	.LC5(%rip), %rsi
	call	async_delay_is_active.part.0
	subb	$1, %al
	je	.L538
.L242:
	leaq	.LC9(%rip), %rcx
	movl	$104, %edx
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk@PLT
.L243:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	cmpb	$1, %al
	jne	.L244
	addl	$1, test_passed(%rip)
.L245:
	movl	%r12d, %edi
	addl	$1, test_total(%rip)
	call	async_delay_remaining
	cmpl	$50, %eax
	jne	.L246
	addl	$1, test_passed(%rip)
.L247:
	addl	$1, test_total(%rip)
	call	async_delay_ticks_until_next
	cmpl	$50, %eax
	jne	.L248
	addl	$1, test_passed(%rip)
.L249:
	movl	$20, %r13d
	.p2align 4,,10
	.p2align 3
.L251:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L250
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L250
	call	_async_delay_tick_walk
.L250:
	subl	$1, %r13d
	jne	.L251
	movl	%r12d, %edi
	addl	$1, test_total(%rip)
	call	async_delay_remaining
	cmpl	$30, %eax
	jne	.L252
	addl	$1, test_passed(%rip)
.L253:
	addl	$1, test_total(%rip)
	call	async_delay_ticks_until_next
	cmpl	$30, %eax
	jne	.L254
	addl	$1, test_passed(%rip)
.L255:
	addl	$1, test_total(%rip)
	cmpb	$7, %bl
	ja	.L258
	movl	%r12d, %edi
	call	async_delay_elapsed.part.0
	testb	%al, %al
	jne	.L539
.L258:
	addl	$1, test_passed(%rip)
.L257:
	movl	$30, %r13d
	.p2align 4,,10
	.p2align 3
.L260:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L259
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L259
	call	_async_delay_tick_walk
.L259:
	subl	$1, %r13d
	jne	.L260
	movl	test_total(%rip), %eax
	leal	1(%rax), %r8d
	movl	%r8d, test_total(%rip)
	cmpb	$7, %bl
	ja	.L263
	movl	%r12d, %edi
	call	async_delay_is_active.part.0
	testb	%al, %al
	jne	.L540
.L263:
	addl	$1, test_passed(%rip)
.L262:
	addl	$1, %r8d
	movl	%r12d, %edi
	movl	%r8d, test_total(%rip)
	call	async_delay_remaining
	testl	%eax, %eax
	jne	.L264
	addl	$1, test_passed(%rip)
.L265:
	leal	1(%r8), %eax
	movl	%eax, test_total(%rip)
	cmpb	$7, %bl
	ja	.L266
	movl	%r12d, %edi
	call	async_delay_elapsed.part.0
	subb	$1, %al
	je	.L541
	leaq	.LC100(%rip), %rcx
	movl	$117, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %eax
	leal	1(%rax), %r8d
.L448:
	movl	%r12d, %edi
	movl	%r8d, test_total(%rip)
	call	async_delay_elapsed.part.0
	testb	%al, %al
	jne	.L268
.L449:
	addl	$1, test_passed(%rip)
.L269:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %eax
	andl	$15, %edx
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	jne	.L270
	addl	$1, test_passed(%rip)
.L271:
	leaq	.LC20(%rip), %rdi
	call	puts@PLT
	xorl	%edx, %edx
	movl	$40, %edi
	leaq	test_cb_oneshot(%rip), %rsi
	call	async_delay_init
	movl	$0, cb_count1(%rip)
	call	_async_delay_start_common
	addl	$1, test_total(%rip)
	movl	%eax, %ebx
	cmpb	$-1, %al
	je	.L272
	addl	$1, test_passed(%rip)
.L273:
	movl	$39, %r12d
	.p2align 4,,10
	.p2align 3
.L275:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L274
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L274
	call	_async_delay_tick_walk
.L274:
	subl	$1, %r12d
	jne	.L275
	movl	cb_count1(%rip), %eax
	addl	$1, test_total(%rip)
	testl	%eax, %eax
	jne	.L276
	addl	$1, test_passed(%rip)
.L279:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L277
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L277
	call	_async_delay_tick_walk
.L277:
	movl	cb_count1(%rip), %eax
	addl	$1, test_total(%rip)
	cmpl	$1, %eax
	jne	.L280
	addl	$1, test_passed(%rip)
.L281:
	movzbl	last_cb_slot(%rip), %eax
	addl	$1, test_total(%rip)
	cmpb	%al, %bl
	jne	.L282
	addl	$1, test_passed(%rip)
.L283:
	movl	$20, %ebx
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
	subl	$1, %ebx
	jne	.L285
	movl	cb_count1(%rip), %eax
	addl	$1, test_total(%rip)
	cmpl	$1, %eax
	jne	.L286
	addl	$1, test_passed(%rip)
.L287:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %eax
	andl	$15, %edx
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	jne	.L288
	addl	$1, test_passed(%rip)
.L289:
	leaq	.LC27(%rip), %rdi
	call	puts@PLT
	movl	$1, %edx
	movl	$25, %edi
	leaq	test_cb_periodic(%rip), %rsi
	call	async_delay_init
	movl	$0, cb_count2(%rip)
	call	_async_delay_start_common
	addl	$1, test_total(%rip)
	movl	%eax, %ebx
	cmpb	$-1, %al
	je	.L290
	addl	$1, test_passed(%rip)
.L291:
	movl	$24, %r12d
	.p2align 4,,10
	.p2align 3
.L293:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L292
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L292
	call	_async_delay_tick_walk
.L292:
	subl	$1, %r12d
	jne	.L293
	movl	cb_count2(%rip), %eax
	addl	$1, test_total(%rip)
	testl	%eax, %eax
	jne	.L294
	addl	$1, test_passed(%rip)
.L297:
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
	movl	cb_count2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpl	$1, %eax
	jne	.L298
	addl	$1, test_passed(%rip)
.L299:
	movl	$25, %r12d
	.p2align 4,,10
	.p2align 3
.L301:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L300
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L300
	call	_async_delay_tick_walk
.L300:
	subl	$1, %r12d
	jne	.L301
	movl	cb_count2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpl	$2, %eax
	jne	.L302
	addl	$1, test_passed(%rip)
.L303:
	movl	$25, %r12d
	.p2align 4,,10
	.p2align 3
.L305:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L304
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L304
	call	_async_delay_tick_walk
.L304:
	subl	$1, %r12d
	jne	.L305
	movl	cb_count2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpl	$3, %eax
	jne	.L306
	addl	$1, test_passed(%rip)
.L307:
	cmpb	$7, %bl
	ja	.L308
	movzbl	%bl, %edi
	call	async_delay_cancel.part.0
.L308:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %eax
	andl	$15, %edx
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	jne	.L309
	addl	$1, test_passed(%rip)
.L310:
	movl	$50, %ebx
	.p2align 4,,10
	.p2align 3
.L312:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L311
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L311
	call	_async_delay_tick_walk
.L311:
	subl	$1, %ebx
	jne	.L312
	movl	cb_count2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpl	$3, %eax
	jne	.L313
	addl	$1, test_passed(%rip)
.L314:
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
	movl	%eax, %ebx
	movzbl	_async_active_mask(%rip), %eax
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	cmpb	$3, %al
	jne	.L315
	addl	$1, test_passed(%rip)
.L316:
	movzbl	%r13b, %r14d
	cmpb	$7, %r13b
	ja	.L317
	movl	%r14d, %edi
	call	async_delay_cancel.part.0
.L317:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	cmpb	$2, %al
	jne	.L318
	addl	$1, test_passed(%rip)
.L319:
	movl	test_total(%rip), %eax
	leal	1(%rax), %ecx
	movl	%ecx, test_total(%rip)
	cmpb	$7, %r13b
	ja	.L322
	movl	%r14d, %edi
	call	async_delay_is_active.part.0
	testb	%al, %al
	jne	.L542
.L322:
	addl	$1, test_passed(%rip)
.L321:
	addl	$1, %ecx
	movzbl	%r12b, %r14d
	movl	%ecx, test_total(%rip)
	cmpb	$7, %r12b
	ja	.L323
	movl	%r14d, %edi
	call	async_delay_is_active.part.0
	subb	$1, %al
	je	.L543
.L323:
	leaq	.LC39(%rip), %rcx
	movl	$224, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
.L324:
	addl	$1, test_total(%rip)
	movzbl	%bl, %r13d
	cmpb	$7, %bl
	ja	.L325
	movl	%r13d, %edi
	call	async_delay_is_active.part.0
	subb	$1, %al
	je	.L544
.L325:
	leaq	.LC40(%rip), %rcx
	movl	$225, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
.L326:
	call	async_delay_cancel_all
	movzbl	_async_active_mask(%rip), %edx
	movl	test_total(%rip), %eax
	movq	%rdx, %rcx
	shrb	$4, %dl
	addl	$1, %eax
	andl	$15, %edx
	andl	$15, %ecx
	movl	%eax, test_total(%rip)
	movzbl	0(%rbp,%rdx), %edx
	addb	0(%rbp,%rcx), %dl
	jne	.L327
	addl	$1, test_passed(%rip)
.L328:
	leal	1(%rax), %ecx
	movl	%ecx, test_total(%rip)
	cmpb	$7, %r12b
	ja	.L331
	movl	%r14d, %edi
	call	async_delay_is_active.part.0
	testb	%al, %al
	jne	.L545
.L331:
	addl	$1, test_passed(%rip)
.L330:
	leal	1(%rcx), %eax
	movl	%eax, test_total(%rip)
	cmpb	$7, %bl
	ja	.L334
	movl	%r13d, %edi
	call	async_delay_is_active.part.0
	testb	%al, %al
	jne	.L546
.L334:
	addl	$1, test_passed(%rip)
.L333:
	leaq	.LC44(%rip), %rdi
	movl	$40, %r12d
	call	puts@PLT
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	call	async_delay_init
	call	_async_delay_start_common
	movl	%eax, %ebx
	.p2align 4,,10
	.p2align 3
.L336:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L335
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L335
	call	_async_delay_tick_walk
.L335:
	subl	$1, %r12d
	jne	.L336
	movzbl	%bl, %r12d
	addl	$1, test_total(%rip)
	movl	%r12d, %edi
	call	async_delay_remaining
	cmpl	$60, %eax
	jne	.L337
	addl	$1, test_passed(%rip)
.L338:
	movl	$30, %esi
	movl	%r12d, %edi
	call	async_delay_restart
	addl	$1, test_total(%rip)
	cmpb	$1, %al
	jne	.L339
	addl	$1, test_passed(%rip)
.L340:
	movl	%r12d, %edi
	addl	$1, test_total(%rip)
	call	async_delay_remaining
	cmpl	$30, %eax
	jne	.L341
	addl	$1, test_passed(%rip)
.L342:
	movl	$29, %r13d
	.p2align 4,,10
	.p2align 3
.L344:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L343
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L343
	call	_async_delay_tick_walk
.L343:
	subl	$1, %r13d
	jne	.L344
	movl	test_total(%rip), %r8d
	leal	1(%r8), %eax
	movl	%eax, test_total(%rip)
	cmpb	$7, %bl
	ja	.L345
	movl	%r12d, %edi
	call	async_delay_elapsed.part.0
	testb	%al, %al
	jne	.L547
	movl	_async_tick_counter(%rip), %eax
	addl	$1, test_passed(%rip)
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L548
.L351:
	movl	_async_tick_counter(%rip), %edx
	movl	_async_next_target(%rip), %eax
	subl	%eax, %edx
	movl	test_total(%rip), %eax
	cmpl	$32766, %edx
	ja	.L352
	call	_async_delay_tick_walk
	movl	test_total(%rip), %eax
.L352:
	addl	$1, %eax
	movl	%eax, test_total(%rip)
	cmpb	$7, %bl
	ja	.L353
.L447:
	movl	%r12d, %edi
	call	async_delay_elapsed.part.0
	subb	$1, %al
	je	.L549
	.p2align 4,,10
	.p2align 3
.L353:
	leaq	.LC49(%rip), %rcx
	movl	$256, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
.L354:
	leaq	.LC50(%rip), %rdi
	call	puts@PLT
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$20, %edi
	call	async_delay_init
	movl	$65525, _async_tick_counter(%rip)
	call	_async_delay_start_common
	movl	%eax, %ebx
	movl	test_total(%rip), %eax
	addl	$1, %eax
	movl	%eax, test_total(%rip)
	cmpb	$-1, %bl
	je	.L355
	addl	$1, test_passed(%rip)
.L356:
	movzbl	%bl, %r12d
	addl	$1, %eax
	movl	%r12d, %edi
	movl	%eax, test_total(%rip)
	call	async_delay_remaining
	cmpl	$20, %eax
	jne	.L357
	addl	$1, test_passed(%rip)
.L358:
	movl	$19, %r13d
	.p2align 4,,10
	.p2align 3
.L360:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L359
	movl	_async_tick_counter(%rip), %eax
	movl	_async_next_target(%rip), %edx
	subl	%edx, %eax
	cmpl	$32766, %eax
	ja	.L359
	call	_async_delay_tick_walk
.L359:
	subl	$1, %r13d
	jne	.L360
	movl	test_total(%rip), %r8d
	leal	1(%r8), %eax
	movl	%eax, test_total(%rip)
	cmpb	$7, %bl
	ja	.L361
	movl	%r12d, %edi
	call	async_delay_elapsed.part.0
	testb	%al, %al
	jne	.L550
	movl	_async_tick_counter(%rip), %eax
	addl	$1, test_passed(%rip)
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	je	.L551
.L367:
	movl	_async_tick_counter(%rip), %edx
	movl	_async_next_target(%rip), %eax
	subl	%eax, %edx
	movl	test_total(%rip), %eax
	cmpl	$32766, %edx
	ja	.L368
	call	_async_delay_tick_walk
	movl	test_total(%rip), %eax
.L368:
	addl	$1, %eax
	movl	%eax, test_total(%rip)
	cmpb	$7, %bl
	ja	.L369
.L446:
	movl	%r12d, %edi
	call	async_delay_elapsed.part.0
	subb	$1, %al
	je	.L552
	.p2align 4,,10
	.p2align 3
.L369:
	leaq	.LC54(%rip), %rcx
	movl	$287, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
.L370:
	leaq	.LC55(%rip), %rdi
	leaq	30(%rsp), %r14
	call	puts@PLT
	leaq	38(%rsp), %rbx
	leaq	.LC56(%rip), %r13
	call	async_delay_init
	leaq	.LC5(%rip), %r12
	movl	test_total(%rip), %r15d
	jmp	.L373
	.p2align 4,,10
	.p2align 3
.L554:
	addq	$1, %r14
	addl	$1, test_passed(%rip)
	cmpq	%r14, %rbx
	je	.L553
.L373:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	addl	$1, %r15d
	call	_async_delay_start_common
	movl	%r15d, test_total(%rip)
	movb	%al, (%r14)
	cmpb	$-1, %al
	jne	.L554
	movq	%r13, %rcx
	movl	$303, %edx
	movq	%r12, %rsi
	movl	$1, %edi
	xorl	%eax, %eax
	addq	$1, %r14
	call	__printf_chk@PLT
	movl	test_total(%rip), %r15d
	cmpq	%r14, %rbx
	jne	.L373
.L553:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	addl	$1, %r15d
	call	_async_delay_start_common
	movl	%r15d, test_total(%rip)
	cmpb	$-1, %al
	jne	.L374
	addl	$1, test_passed(%rip)
.L375:
	movzbl	30(%rsp), %edi
	cmpb	$7, %dil
	ja	.L376
	call	async_delay_cancel.part.0
.L376:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	call	_async_delay_start_common
	addl	$1, test_total(%rip)
	cmpb	$-1, %al
	je	.L377
	addl	$1, test_passed(%rip)
.L378:
	leaq	.LC59(%rip), %rdi
	xorl	%ebx, %ebx
	leaq	_async_slot_bit(%rip), %r14
	movl	$1, %r12d
	call	async_delay_cancel_all
	leaq	.LC60(%rip), %r15
	leaq	.LC5(%rip), %r13
	call	puts@PLT
	jmp	.L381
	.p2align 4,,10
	.p2align 3
.L556:
	addl	$2, %eax
	addq	$1, %rbx
	addl	$2, test_passed(%rip)
	movl	%eax, test_total(%rip)
	cmpq	$8, %rbx
	je	.L555
.L381:
	movl	test_total(%rip), %eax
	movl	%ebx, %ecx
	leal	1(%rax), %edx
	movl	%edx, test_total(%rip)
	movl	%r12d, %edx
	sall	%cl, %edx
	cmpb	%dl, (%r14,%rbx)
	je	.L556
	movq	%r15, %rcx
	movl	$327, %edx
	movq	%r13, %rsi
	movl	$1, %edi
	xorl	%eax, %eax
	addq	$1, %rbx
	call	__printf_chk@PLT
	movl	$328, %edx
	movq	%r13, %rsi
	xorl	%eax, %eax
	leaq	.LC61(%rip), %rcx
	movl	$1, %edi
	addl	$1, test_total(%rip)
	call	__printf_chk@PLT
	cmpq	$8, %rbx
	jne	.L381
.L555:
	leaq	.LC62(%rip), %rdi
	call	puts@PLT
	call	async_delay_init
	movl	test_total(%rip), %eax
	leal	1(%rax), %ebx
	movzbl	_async_active_mask(%rip), %eax
	movl	%ebx, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %eax
	andl	$15, %edx
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	jne	.L382
	addl	$1, test_passed(%rip)
.L383:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	addl	$1, %ebx
	call	_async_delay_start_common
	movl	%ebx, test_total(%rip)
	testb	%al, %al
	jne	.L384
	addl	$1, test_passed(%rip)
.L385:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, %ebx
	movl	%ebx, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	cmpb	$1, %al
	jne	.L386
	addl	$1, test_passed(%rip)
.L387:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	call	_async_delay_start_common
	addl	$1, test_total(%rip)
	movl	%eax, %ebx
	cmpb	$1, %al
	jne	.L388
	addl	$1, test_passed(%rip)
.L389:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	cmpb	$2, %al
	jne	.L390
	addl	$1, test_passed(%rip)
.L391:
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
	jne	.L392
	cmpb	$3, %al
	jne	.L392
	addl	$1, test_passed(%rip)
.L393:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, %edx
	movl	%edx, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	cmpb	$4, %al
	jne	.L394
	addl	$1, test_passed(%rip)
.L395:
	cmpb	$7, %bl
	ja	.L396
	movzbl	%bl, %edi
	call	async_delay_cancel.part.0
.L396:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	cmpb	$3, %al
	jne	.L397
	addl	$1, test_passed(%rip)
.L398:
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$100, %edi
	call	_async_delay_start_common
	addl	$1, test_total(%rip)
	cmpb	$1, %al
	jne	.L399
	addl	$1, test_passed(%rip)
.L400:
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %edx
	andl	$15, %eax
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	cmpb	$4, %al
	jne	.L401
	addl	$1, test_passed(%rip)
.L402:
	call	async_delay_cancel_all
	movzbl	_async_active_mask(%rip), %eax
	addl	$1, test_total(%rip)
	movq	%rax, %rdx
	shrb	$4, %al
	andl	$15, %eax
	andl	$15, %edx
	movzbl	0(%rbp,%rax), %eax
	addb	0(%rbp,%rdx), %al
	jne	.L403
	addl	$1, test_passed(%rip)
.L404:
	leaq	.LC74(%rip), %rdi
	call	puts@PLT
	movb	$0, TCCR2(%rip)
	movb	$0, OCR2(%rip)
	movb	$0, TIMSK(%rip)
	movb	$12, TCCR2(%rip)
	movb	$0, TCNT2(%rip)
	movb	$124, OCR2(%rip)
	movzbl	TIMSK(%rip), %eax
	addl	$1, test_total(%rip)
	orl	$-128, %eax
	movb	%al, TIMSK(%rip)
	movzbl	TCCR2(%rip), %eax
	cmpb	$12, %al
	jne	.L405
	addl	$1, test_passed(%rip)
.L406:
	movzbl	OCR2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpb	$124, %al
	jne	.L407
	addl	$1, test_passed(%rip)
.L408:
	movzbl	TIMSK(%rip), %eax
	addl	$1, test_total(%rip)
	testb	%al, %al
	jns	.L409
	addl	$1, test_passed(%rip)
.L410:
	movb	$0, TCCR2(%rip)
	movb	$0, OCR2(%rip)
	movb	$0, TIMSK(%rip)
	movb	$12, TCCR2(%rip)
	movb	$0, TCNT2(%rip)
	movb	$-7, OCR2(%rip)
	movzbl	TIMSK(%rip), %eax
	addl	$1, test_total(%rip)
	orl	$-128, %eax
	movb	%al, TIMSK(%rip)
	movzbl	TCCR2(%rip), %eax
	cmpb	$12, %al
	jne	.L411
	addl	$1, test_passed(%rip)
.L412:
	movzbl	OCR2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpb	$-7, %al
	jne	.L413
	addl	$1, test_passed(%rip)
.L414:
	movb	$0, TCCR2(%rip)
	movb	$0, OCR2(%rip)
	movb	$0, TIMSK(%rip)
	movb	$11, TCCR2(%rip)
	movb	$0, TCNT2(%rip)
	movb	$124, OCR2(%rip)
	movzbl	TIMSK(%rip), %eax
	addl	$1, test_total(%rip)
	orl	$-128, %eax
	movb	%al, TIMSK(%rip)
	movzbl	TCCR2(%rip), %eax
	cmpb	$11, %al
	jne	.L415
	addl	$1, test_passed(%rip)
.L416:
	movzbl	OCR2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpb	$124, %al
	jne	.L417
	addl	$1, test_passed(%rip)
.L418:
	movb	$0, TCCR2(%rip)
	movb	$0, OCR2(%rip)
	movb	$0, TIMSK(%rip)
	movb	$10, TCCR2(%rip)
	movb	$0, TCNT2(%rip)
	movb	$-7, OCR2(%rip)
	movzbl	TIMSK(%rip), %eax
	addl	$1, test_total(%rip)
	orl	$-128, %eax
	movb	%al, TIMSK(%rip)
	movzbl	TCCR2(%rip), %eax
	cmpb	$10, %al
	jne	.L419
	addl	$1, test_passed(%rip)
.L420:
	movzbl	OCR2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpb	$-7, %al
	jne	.L421
	addl	$1, test_passed(%rip)
.L422:
	movb	$0, TCCR2(%rip)
	movb	$0, OCR2(%rip)
	movb	$0, TIMSK(%rip)
	movb	$10, TCCR2(%rip)
	movb	$0, TCNT2(%rip)
	movb	$124, OCR2(%rip)
	movzbl	TIMSK(%rip), %eax
	addl	$1, test_total(%rip)
	orl	$-128, %eax
	movb	%al, TIMSK(%rip)
	movzbl	TCCR2(%rip), %eax
	cmpb	$10, %al
	jne	.L423
	addl	$1, test_passed(%rip)
.L424:
	movzbl	OCR2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpb	$124, %al
	jne	.L425
	addl	$1, test_passed(%rip)
.L426:
	movb	$0, TCCR1A(%rip)
	movb	$0, TCCR1B(%rip)
	movb	$0, OCR1AH(%rip)
	movb	$0, OCR1AL(%rip)
	movb	$0, TIMSK(%rip)
	movb	$0, TCCR1A(%rip)
	movb	$10, TCCR1B(%rip)
	movb	$0, TCNT1H(%rip)
	movb	$0, TCNT1L(%rip)
	movb	$3, OCR1AH(%rip)
	movb	$-25, OCR1AL(%rip)
	movzbl	TIMSK(%rip), %eax
	addl	$1, test_total(%rip)
	orl	$16, %eax
	movb	%al, TIMSK(%rip)
	movzbl	TCCR1B(%rip), %eax
	cmpb	$10, %al
	jne	.L427
	addl	$1, test_passed(%rip)
.L428:
	movzbl	OCR1AH(%rip), %eax
	addl	$1, test_total(%rip)
	cmpb	$3, %al
	jne	.L429
	movzbl	OCR1AL(%rip), %eax
	cmpb	$-25, %al
	je	.L557
.L429:
	leaq	.LC87(%rip), %rcx
	movl	$403, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
.L430:
	movb	$0, TCCR1A(%rip)
	movb	$0, TCNT1H(%rip)
	movb	$0, TCNT1L(%rip)
	movb	$7, OCR1AH(%rip)
	movb	$-49, OCR1AL(%rip)
	movb	$10, TCCR1B(%rip)
	movzbl	TIMSK(%rip), %eax
	addl	$1, test_total(%rip)
	orl	$16, %eax
	movb	%al, TIMSK(%rip)
	movzbl	OCR1AH(%rip), %eax
	cmpb	$7, %al
	jne	.L431
	movzbl	OCR1AL(%rip), %eax
	cmpb	$-49, %al
	je	.L558
.L431:
	leaq	.LC88(%rip), %rcx
	movl	$406, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
.L432:
	movb	$0, TCCR2(%rip)
	movb	$0, OCR2(%rip)
	movb	$0, TIMSK(%rip)
	movb	$0, TCNT2(%rip)
	movb	$-7, OCR2(%rip)
	movb	$11, TCCR2(%rip)
	movzbl	TIMSK(%rip), %eax
	addl	$1, test_total(%rip)
	orl	$-128, %eax
	movb	%al, TIMSK(%rip)
	movzbl	TCCR2(%rip), %eax
	cmpb	$11, %al
	jne	.L433
	addl	$1, test_passed(%rip)
.L434:
	movzbl	OCR2(%rip), %eax
	addl	$1, test_total(%rip)
	cmpb	$-7, %al
	jne	.L435
	addl	$1, test_passed(%rip)
.L436:
	leaq	.LC91(%rip), %rdi
	call	puts@PLT
	xorl	%edx, %edx
	xorl	%esi, %esi
	movl	$32767, %edi
	call	async_delay_init
	movl	test_passed(%rip), %r12d
	movl	test_total(%rip), %ebx
	leal	1(%r12), %eax
	movl	%eax, test_passed(%rip)
	call	_async_delay_start_common
	leal	2(%rbx), %r10d
	movl	%r10d, test_total(%rip)
	movl	%eax, %ebp
	cmpb	$-1, %al
	je	.L437
	addl	$2, %r12d
	movl	%r12d, test_passed(%rip)
.L438:
	movzbl	%bpl, %ebx
	addl	$1, %r10d
	movl	$32768, %esi
	movl	%ebx, %edi
	movl	%r10d, test_total(%rip)
	call	async_delay_restart
	testb	%al, %al
	jne	.L439
	addl	$1, test_passed(%rip)
.L440:
	addl	$1, %r10d
	movl	$100, %esi
	movl	%ebx, %edi
	movl	%r10d, test_total(%rip)
	call	async_delay_restart
	cmpb	$1, %al
	jne	.L441
	addl	$1, test_passed(%rip)
.L442:
	cmpb	$7, %bpl
	ja	.L443
	movl	%ebx, %edi
	call	async_delay_cancel.part.0
.L443:
	leaq	.LC95(%rip), %rdi
	call	puts@PLT
	movl	test_total(%rip), %edx
	movl	test_passed(%rip), %ecx
	xorl	%eax, %eax
	leaq	.LC96(%rip), %rsi
	movl	$1, %edi
	movl	%edx, %r8d
	subl	%ecx, %r8d
	call	__printf_chk@PLT
	leaq	.LC97(%rip), %rdi
	call	puts@PLT
	movl	test_total(%rip), %eax
	cmpl	%eax, test_passed(%rip)
	je	.L559
	leaq	.LC99(%rip), %rdi
	call	puts@PLT
	movl	$1, %eax
.L234:
	movq	40(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L560
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
.L541:
	.cfi_restore_state
	addl	$1, test_passed(%rip)
	addl	$2, %r8d
	jmp	.L448
.L538:
	addl	$2, %ecx
	movl	%ecx, test_passed(%rip)
	jmp	.L243
.L540:
	leaq	.LC16(%rip), %rcx
	movl	$115, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %r8d
	jmp	.L262
.L539:
	leaq	.LC15(%rip), %rcx
	movl	$112, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L257
.L441:
	leaq	.LC94(%rip), %rcx
	movl	$430, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L442
.L435:
	leaq	.LC90(%rip), %rcx
	movl	$411, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L436
.L433:
	leaq	.LC89(%rip), %rcx
	movl	$410, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L434
.L427:
	leaq	.LC86(%rip), %rcx
	movl	$402, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L428
.L425:
	leaq	.LC85(%rip), %rcx
	movl	$398, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L426
.L423:
	leaq	.LC84(%rip), %rcx
	movl	$397, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L424
.L421:
	leaq	.LC83(%rip), %rcx
	movl	$393, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L422
.L419:
	leaq	.LC82(%rip), %rcx
	movl	$392, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L420
.L417:
	leaq	.LC81(%rip), %rcx
	movl	$388, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L418
.L415:
	leaq	.LC80(%rip), %rcx
	movl	$387, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L416
.L413:
	leaq	.LC79(%rip), %rcx
	movl	$383, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L414
.L411:
	leaq	.LC78(%rip), %rcx
	movl	$382, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L412
.L407:
	leaq	.LC76(%rip), %rcx
	movl	$377, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L408
.L405:
	leaq	.LC75(%rip), %rcx
	movl	$376, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L406
.L401:
	leaq	.LC72(%rip), %rcx
	movl	$364, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L402
.L399:
	leaq	.LC71(%rip), %rcx
	movl	$363, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L400
.L397:
	leaq	.LC70(%rip), %rcx
	movl	$361, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L398
.L394:
	leaq	.LC69(%rip), %rcx
	movl	$357, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L395
.L390:
	leaq	.LC67(%rip), %rcx
	movl	$350, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L391
.L388:
	leaq	.LC66(%rip), %rcx
	movl	$349, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L389
.L386:
	leaq	.LC65(%rip), %rcx
	movl	$345, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L387
.L374:
	leaq	.LC57(%rip), %rcx
	movl	$307, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L375
.L252:
	leaq	.LC13(%rip), %rcx
	movl	$110, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L253
.L248:
	leaq	.LC12(%rip), %rcx
	movl	$107, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L249
.L246:
	leaq	.LC11(%rip), %rcx
	movl	$106, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L247
.L244:
	leaq	.LC10(%rip), %rcx
	movl	$105, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L245
.L254:
	leaq	.LC14(%rip), %rcx
	movl	$111, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L255
.L341:
	leaq	.LC47(%rip), %rcx
	movl	$250, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L342
.L339:
	leaq	.LC46(%rip), %rcx
	movl	$249, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L340
.L337:
	leaq	.LC45(%rip), %rcx
	movl	$245, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L338
.L357:
	leaq	.LC52(%rip), %rcx
	movl	$281, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L358
.L318:
	leaq	.LC37(%rip), %rcx
	movl	$222, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L319
.L315:
	leaq	.LC36(%rip), %rcx
	movl	$220, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L316
.L313:
	leaq	.LC34(%rip), %rcx
	movl	$202, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L314
.L306:
	leaq	.LC32(%rip), %rcx
	movl	$193, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L307
.L302:
	leaq	.LC31(%rip), %rcx
	movl	$187, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L303
.L298:
	leaq	.LC30(%rip), %rcx
	movl	$181, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L299
.L286:
	leaq	.LC25(%rip), %rcx
	movl	$153, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L287
.L282:
	leaq	.LC24(%rip), %rcx
	movl	$147, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L283
.L280:
	leaq	.LC23(%rip), %rcx
	movl	$146, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L281
.L409:
	leaq	.LC77(%rip), %rcx
	movl	$378, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L410
.L559:
	leaq	.LC98(%rip), %rdi
	call	puts@PLT
	xorl	%eax, %eax
	jmp	.L234
.L266:
	leaq	.LC100(%rip), %rcx
	movl	$117, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	addl	$1, test_total(%rip)
	jmp	.L449
.L361:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, test_passed(%rip)
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	jne	.L367
	addl	$2, %r8d
	movl	%r8d, test_total(%rip)
	jmp	.L369
.L345:
	movl	_async_tick_counter(%rip), %eax
	addl	$1, test_passed(%rip)
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	jne	.L351
	addl	$2, %r8d
	movl	%r8d, test_total(%rip)
	jmp	.L353
.L327:
	leaq	.LC41(%rip), %rcx
	movl	$228, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %eax
	jmp	.L328
.L309:
	leaq	.LC33(%rip), %rcx
	movl	$196, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L310
.L294:
	leaq	.LC29(%rip), %rcx
	movl	$175, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L297
.L439:
	leaq	.LC93(%rip), %rcx
	movl	$429, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %r10d
	jmp	.L440
.L270:
	leaq	.LC19(%rip), %rcx
	movl	$119, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L271
.L237:
	leaq	.LC6(%rip), %rcx
	movl	$90, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L238
.L235:
	leaq	.LC4(%rip), %rcx
	movl	$89, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %eax
	jmp	.L236
.L264:
	leaq	.LC17(%rip), %rcx
	movl	$116, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %r8d
	jmp	.L265
.L276:
	leaq	.LC22(%rip), %rcx
	movl	$140, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L279
.L288:
	leaq	.LC26(%rip), %rcx
	movl	$154, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L289
.L403:
	leaq	.LC73(%rip), %rcx
	movl	$368, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L404
.L392:
	movl	$356, %edx
	leaq	.LC68(%rip), %rcx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %edx
	jmp	.L393
.L384:
	leaq	.LC64(%rip), %rcx
	movl	$344, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %ebx
	jmp	.L385
.L382:
	leaq	.LC63(%rip), %rcx
	movl	$341, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %ebx
	jmp	.L383
.L549:
	addl	$1, test_passed(%rip)
	jmp	.L354
.L552:
	addl	$1, test_passed(%rip)
	jmp	.L370
.L543:
	addl	$1, test_passed(%rip)
	jmp	.L324
.L544:
	addl	$1, test_passed(%rip)
	jmp	.L326
.L546:
	leaq	.LC43(%rip), %rcx
	movl	$230, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L333
.L547:
	leaq	.LC48(%rip), %rcx
	movl	$253, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	jne	.L351
	addl	$1, test_total(%rip)
	jmp	.L447
.L550:
	leaq	.LC53(%rip), %rcx
	movl	$284, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	_async_tick_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, _async_tick_counter(%rip)
	movzbl	_async_active_mask(%rip), %eax
	testb	%al, %al
	jne	.L367
	addl	$1, test_total(%rip)
	jmp	.L446
.L545:
	leaq	.LC42(%rip), %rcx
	movl	$229, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %ecx
	jmp	.L330
.L542:
	leaq	.LC38(%rip), %rcx
	movl	$223, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %ecx
	jmp	.L321
.L268:
	leaq	.LC18(%rip), %rcx
	movl	$118, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L269
.L290:
	leaq	.LC28(%rip), %rcx
	movl	$169, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L291
.L377:
	leaq	.LC58(%rip), %rcx
	movl	$311, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L378
.L355:
	leaq	.LC51(%rip), %rcx
	movl	$280, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %eax
	jmp	.L356
.L239:
	leaq	.LC5(%rip), %rsi
	leaq	.LC8(%rip), %rcx
	movl	$103, %edx
	xorl	%eax, %eax
	movl	$1, %edi
	movq	%rsi, 8(%rsp)
	movl	$255, %r12d
	call	__printf_chk@PLT
	addl	$1, test_total(%rip)
	movq	8(%rsp), %rsi
	jmp	.L242
.L272:
	leaq	.LC21(%rip), %rcx
	movl	$134, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	jmp	.L273
.L437:
	leaq	.LC92(%rip), %rcx
	movl	$426, %edx
	leaq	.LC5(%rip), %rsi
	xorl	%eax, %eax
	movl	$1, %edi
	call	__printf_chk@PLT
	movl	test_total(%rip), %r10d
	jmp	.L438
.L558:
	addl	$1, test_passed(%rip)
	jmp	.L432
.L557:
	addl	$1, test_passed(%rip)
	jmp	.L430
.L551:
	addl	$2, %r8d
	movl	%r8d, test_total(%rip)
	jmp	.L446
.L548:
	addl	$2, %r8d
	movl	%r8d, test_total(%rip)
	jmp	.L447
.L560:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE84:
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
	.local	TIMSK
	.comm	TIMSK,1,1
	.local	OCR2
	.comm	OCR2,1,1
	.local	OCR1AL
	.comm	OCR1AL,1,1
	.local	OCR1AH
	.comm	OCR1AH,1,1
	.local	TCNT2
	.comm	TCNT2,1,1
	.local	TCNT1L
	.comm	TCNT1L,1,1
	.local	TCNT1H
	.comm	TCNT1H,1,1
	.local	TCCR2
	.comm	TCCR2,1,1
	.local	TCCR1B
	.comm	TCCR1B,1,1
	.local	TCCR1A
	.comm	TCCR1A,1,1
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
