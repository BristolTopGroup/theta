.text
.p2align 4
.globl log2_dot
.type log2_dot, @function
log2_dot:
    fldz
	testl	%edx, %edx
	je .exit
	subl	$1, %edx
	xorl	%eax, %eax
	leaq	8(,%rdx,8), %rdx
	.p2align 4
.loopstart:
    fldl	(%rsi,%rax)
	fldl	(%rdi,%rax)
    fyl2x
	addq	$8, %rax
	cmpq	%rdx, %rax
	faddp	%st, %st(1)
	jne	.loopstart
.exit:
	fstpl	-8(%rsp)
	movsd	-8(%rsp), %xmm0
	ret
