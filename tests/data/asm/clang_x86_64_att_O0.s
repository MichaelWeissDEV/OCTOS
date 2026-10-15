	.file	"corpus.cpp"
	.text
	.globl	simple_add                      # -- Begin function simple_add
	.p2align	4
	.type	simple_add,@function
simple_add:                             # @simple_add
.Lfunc_begin0:
	.file	0 "/Users/michaelweiss/git/github/OCTOS/tests/data/asm" "corpus.cpp" md5 0xba4d7a0b733ef1532df489d07fae416a
	.loc	0 2 0                           # corpus.cpp:2:0
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
.Ltmp0:
	.loc	0 2 39 prologue_end             # corpus.cpp:2:39
	movl	-4(%rbp), %eax
	.loc	0 2 41 is_stmt 0                # corpus.cpp:2:41
	addl	-8(%rbp), %eax
	.loc	0 2 32 epilogue_begin           # corpus.cpp:2:32
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Ltmp1:
.Lfunc_end0:
	.size	simple_add, .Lfunc_end0-simple_add
	.cfi_endproc
                                        # -- End function
	.globl	if_else                         # -- Begin function if_else
	.p2align	4
	.type	if_else,@function
if_else:                                # @if_else
.Lfunc_begin1:
	.loc	0 4 0 is_stmt 1                 # corpus.cpp:4:0
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edi, -8(%rbp)
.Ltmp2:
	.loc	0 5 11 prologue_end             # corpus.cpp:5:11
	cmpl	$10, -8(%rbp)
	jle	.LBB1_2
# %bb.1:
	.loc	0 5 24 is_stmt 0                # corpus.cpp:5:24
	movl	-8(%rbp), %eax
	.loc	0 5 26                          # corpus.cpp:5:26
	shll	%eax
	.loc	0 5 17                          # corpus.cpp:5:17
	movl	%eax, -4(%rbp)
	jmp	.LBB1_6
.LBB1_2:
.Ltmp3:
	.loc	0 6 16 is_stmt 1                # corpus.cpp:6:16
	cmpl	$-10, -8(%rbp)
	jge	.LBB1_4
# %bb.3:
	.loc	0 6 30 is_stmt 0                # corpus.cpp:6:30
	xorl	%eax, %eax
	subl	-8(%rbp), %eax
	.loc	0 6 23                          # corpus.cpp:6:23
	movl	%eax, -4(%rbp)
	jmp	.LBB1_6
.Ltmp4:
.LBB1_4:
	.loc	0 0 23                          # corpus.cpp:0:23
	jmp	.LBB1_5
.LBB1_5:
	.loc	0 7 5 is_stmt 1                 # corpus.cpp:7:5
	movl	$0, -4(%rbp)
.LBB1_6:
	.loc	0 8 1                           # corpus.cpp:8:1
	movl	-4(%rbp), %eax
	.loc	0 8 1 epilogue_begin is_stmt 0  # corpus.cpp:8:1
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Ltmp5:
.Lfunc_end1:
	.size	if_else, .Lfunc_end1-if_else
	.cfi_endproc
                                        # -- End function
	.globl	loop_sum                        # -- Begin function loop_sum
	.p2align	4
	.type	loop_sum,@function
loop_sum:                               # @loop_sum
.Lfunc_begin2:
	.loc	0 10 0 is_stmt 1                # corpus.cpp:10:0
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
.Ltmp6:
	.loc	0 11 9 prologue_end             # corpus.cpp:11:9
	movl	$0, -16(%rbp)
.Ltmp7:
	.loc	0 12 14                         # corpus.cpp:12:14
	movl	$0, -20(%rbp)
.LBB2_1:                                # =>This Inner Loop Header: Depth=1
.Ltmp8:
	.loc	0 12 21 is_stmt 0               # corpus.cpp:12:21
	movl	-20(%rbp), %eax
	.loc	0 12 23                         # corpus.cpp:12:23
	cmpl	-12(%rbp), %eax
.Ltmp9:
	.loc	0 12 5                          # corpus.cpp:12:5
	jge	.LBB2_4
# %bb.2:                                #   in Loop: Header=BB2_1 Depth=1
.Ltmp10:
	.loc	0 12 38                         # corpus.cpp:12:38
	movq	-8(%rbp), %rax
	movslq	-20(%rbp), %rcx
	movl	(%rax,%rcx,4), %eax
	.loc	0 12 35                         # corpus.cpp:12:35
	addl	-16(%rbp), %eax
	movl	%eax, -16(%rbp)
# %bb.3:                                #   in Loop: Header=BB2_1 Depth=1
	.loc	0 12 28                         # corpus.cpp:12:28
	movl	-20(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -20(%rbp)
	.loc	0 12 5                          # corpus.cpp:12:5
	jmp	.LBB2_1
.Ltmp11:
.LBB2_4:
	.loc	0 13 12 is_stmt 1               # corpus.cpp:13:12
	movl	-16(%rbp), %eax
	.loc	0 13 5 epilogue_begin is_stmt 0 # corpus.cpp:13:5
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Ltmp12:
.Lfunc_end2:
	.size	loop_sum, .Lfunc_end2-loop_sum
	.cfi_endproc
                                        # -- End function
	.globl	sw                              # -- Begin function sw
	.p2align	4
	.type	sw,@function
sw:                                     # @sw
.Lfunc_begin3:
	.loc	0 16 0 is_stmt 1                # corpus.cpp:16:0
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edi, -8(%rbp)
.Ltmp13:
	.loc	0 17 13 prologue_end            # corpus.cpp:17:13
	movl	-8(%rbp), %eax
                                        # kill: def $rax killed $eax
	movq	%rax, -16(%rbp)                 # 8-byte Spill
	subq	$5, %rax
	.loc	0 17 5 is_stmt 0                # corpus.cpp:17:5
	ja	.LBB3_7
# %bb.9:
	.loc	0 0 5                           # corpus.cpp:0:5
	movq	-16(%rbp), %rcx                 # 8-byte Reload
	leaq	.LJTI3_0(%rip), %rax
	.loc	0 17 5                          # corpus.cpp:17:5
	movslq	(%rax,%rcx,4), %rcx
	addq	%rcx, %rax
	jmpq	*%rax
.LBB3_1:
.Ltmp14:
	.loc	0 18 17 is_stmt 1               # corpus.cpp:18:17
	movl	$11, -4(%rbp)
	jmp	.LBB3_8
.LBB3_2:
	.loc	0 19 17                         # corpus.cpp:19:17
	movl	$22, -4(%rbp)
	jmp	.LBB3_8
.LBB3_3:
	.loc	0 20 17                         # corpus.cpp:20:17
	movl	$33, -4(%rbp)
	jmp	.LBB3_8
.LBB3_4:
	.loc	0 21 17                         # corpus.cpp:21:17
	movl	$44, -4(%rbp)
	jmp	.LBB3_8
.LBB3_5:
	.loc	0 22 17                         # corpus.cpp:22:17
	movl	$55, -4(%rbp)
	jmp	.LBB3_8
.LBB3_6:
	.loc	0 23 17                         # corpus.cpp:23:17
	movl	$66, -4(%rbp)
	jmp	.LBB3_8
.LBB3_7:
	.loc	0 24 18                         # corpus.cpp:24:18
	movl	$-1, -4(%rbp)
.Ltmp15:
.LBB3_8:
	.loc	0 26 1                          # corpus.cpp:26:1
	movl	-4(%rbp), %eax
	.loc	0 26 1 epilogue_begin is_stmt 0 # corpus.cpp:26:1
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Ltmp16:
.Lfunc_end3:
	.size	sw, .Lfunc_end3-sw
	.cfi_endproc
	.section	.rodata,"a",@progbits
	.p2align	2, 0x0
.LJTI3_0:
	.long	.LBB3_1-.LJTI3_0
	.long	.LBB3_2-.LJTI3_0
	.long	.LBB3_3-.LJTI3_0
	.long	.LBB3_4-.LJTI3_0
	.long	.LBB3_5-.LJTI3_0
	.long	.LBB3_6-.LJTI3_0
                                        # -- End function
	.text
	.globl	tail_call                       # -- Begin function tail_call
	.p2align	4
	.type	tail_call,@function
tail_call:                              # @tail_call
.Lfunc_begin4:
	.loc	0 29 0 is_stmt 1                # corpus.cpp:29:0
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
.Ltmp17:
	.loc	0 29 38 prologue_end            # corpus.cpp:29:38
	movl	-4(%rbp), %edi
	.loc	0 29 40 is_stmt 0               # corpus.cpp:29:40
	addl	$1, %edi
	.loc	0 29 31                         # corpus.cpp:29:31
	callq	helper@PLT
	.loc	0 29 24 epilogue_begin          # corpus.cpp:29:24
	addq	$16, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Ltmp18:
.Lfunc_end4:
	.size	tail_call, .Lfunc_end4-tail_call
	.cfi_endproc
                                        # -- End function
	.globl	sw_call                         # -- Begin function sw_call
	.p2align	4
	.type	sw_call,@function
sw_call:                                # @sw_call
.Lfunc_begin5:
	.loc	0 32 0 is_stmt 1                # corpus.cpp:32:0
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movl	%edi, -8(%rbp)
	movl	%esi, -12(%rbp)
.Ltmp19:
	.loc	0 33 13 prologue_end            # corpus.cpp:33:13
	movl	-8(%rbp), %eax
                                        # kill: def $rax killed $eax
	movq	%rax, -24(%rbp)                 # 8-byte Spill
	subq	$5, %rax
	.loc	0 33 5 is_stmt 0                # corpus.cpp:33:5
	ja	.LBB5_7
# %bb.9:
	.loc	0 0 5                           # corpus.cpp:0:5
	movq	-24(%rbp), %rcx                 # 8-byte Reload
	leaq	.LJTI5_0(%rip), %rax
	.loc	0 33 5                          # corpus.cpp:33:5
	movslq	(%rax,%rcx,4), %rcx
	addq	%rcx, %rax
	jmpq	*%rax
.LBB5_1:
.Ltmp20:
	.loc	0 34 27 is_stmt 1               # corpus.cpp:34:27
	movl	-12(%rbp), %edi
	.loc	0 34 24 is_stmt 0               # corpus.cpp:34:24
	callq	f0@PLT
	.loc	0 34 17                         # corpus.cpp:34:17
	movl	%eax, -4(%rbp)
	jmp	.LBB5_8
.LBB5_2:
	.loc	0 35 27 is_stmt 1               # corpus.cpp:35:27
	movl	-12(%rbp), %edi
	.loc	0 35 24 is_stmt 0               # corpus.cpp:35:24
	callq	f1@PLT
	.loc	0 35 17                         # corpus.cpp:35:17
	movl	%eax, -4(%rbp)
	jmp	.LBB5_8
.LBB5_3:
	.loc	0 36 27 is_stmt 1               # corpus.cpp:36:27
	movl	-12(%rbp), %edi
	.loc	0 36 24 is_stmt 0               # corpus.cpp:36:24
	callq	f2@PLT
	.loc	0 36 17                         # corpus.cpp:36:17
	movl	%eax, -4(%rbp)
	jmp	.LBB5_8
.LBB5_4:
	.loc	0 37 27 is_stmt 1               # corpus.cpp:37:27
	movl	-12(%rbp), %edi
	.loc	0 37 24 is_stmt 0               # corpus.cpp:37:24
	callq	f3@PLT
	.loc	0 37 17                         # corpus.cpp:37:17
	movl	%eax, -4(%rbp)
	jmp	.LBB5_8
.LBB5_5:
	.loc	0 38 27 is_stmt 1               # corpus.cpp:38:27
	movl	-12(%rbp), %edi
	.loc	0 38 24 is_stmt 0               # corpus.cpp:38:24
	callq	f4@PLT
	.loc	0 38 17                         # corpus.cpp:38:17
	movl	%eax, -4(%rbp)
	jmp	.LBB5_8
.LBB5_6:
	.loc	0 39 27 is_stmt 1               # corpus.cpp:39:27
	movl	-12(%rbp), %edi
	.loc	0 39 24 is_stmt 0               # corpus.cpp:39:24
	callq	f5@PLT
	.loc	0 39 17                         # corpus.cpp:39:17
	movl	%eax, -4(%rbp)
	jmp	.LBB5_8
.LBB5_7:
	.loc	0 40 18 is_stmt 1               # corpus.cpp:40:18
	movl	$0, -4(%rbp)
.Ltmp21:
.LBB5_8:
	.loc	0 42 1                          # corpus.cpp:42:1
	movl	-4(%rbp), %eax
	.loc	0 42 1 epilogue_begin is_stmt 0 # corpus.cpp:42:1
	addq	$32, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Ltmp22:
.Lfunc_end5:
	.size	sw_call, .Lfunc_end5-sw_call
	.cfi_endproc
	.section	.rodata,"a",@progbits
	.p2align	2, 0x0
.LJTI5_0:
	.long	.LBB5_1-.LJTI5_0
	.long	.LBB5_2-.LJTI5_0
	.long	.LBB5_3-.LJTI5_0
	.long	.LBB5_4-.LJTI5_0
	.long	.LBB5_5-.LJTI5_0
	.long	.LBB5_6-.LJTI5_0
                                        # -- End function
	.text
	.globl	call_twice                      # -- Begin function call_twice
	.p2align	4
	.type	call_twice,@function
call_twice:                             # @call_twice
.Lfunc_begin6:
	.loc	0 44 0 is_stmt 1                # corpus.cpp:44:0
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
.Ltmp23:
	.loc	0 44 39 prologue_end            # corpus.cpp:44:39
	movl	-4(%rbp), %edi
	.loc	0 44 32 is_stmt 0               # corpus.cpp:44:32
	callq	helper@PLT
	movl	%eax, -8(%rbp)                  # 4-byte Spill
	.loc	0 44 51                         # corpus.cpp:44:51
	movl	-4(%rbp), %edi
	.loc	0 44 53                         # corpus.cpp:44:53
	addl	$1, %edi
	.loc	0 44 44                         # corpus.cpp:44:44
	callq	helper@PLT
	movl	%eax, %ecx
	movl	-8(%rbp), %eax                  # 4-byte Reload
	.loc	0 44 42                         # corpus.cpp:44:42
	addl	%ecx, %eax
	.loc	0 44 25 epilogue_begin          # corpus.cpp:44:25
	addq	$16, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Ltmp24:
.Lfunc_end6:
	.size	call_twice, .Lfunc_end6-call_twice
	.cfi_endproc
                                        # -- End function
	.globl	indirect                        # -- Begin function indirect
	.p2align	4
	.type	indirect,@function
indirect:                               # @indirect
.Lfunc_begin7:
	.loc	0 46 0 is_stmt 1                # corpus.cpp:46:0
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
.Ltmp25:
	.loc	0 46 46 prologue_end            # corpus.cpp:46:46
	movq	-8(%rbp), %rax
	.loc	0 46 49 is_stmt 0               # corpus.cpp:46:49
	movl	-12(%rbp), %edi
	.loc	0 46 46                         # corpus.cpp:46:46
	callq	*%rax
	.loc	0 46 39 epilogue_begin          # corpus.cpp:46:39
	addq	$16, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Ltmp26:
.Lfunc_end7:
	.size	indirect, .Lfunc_end7-indirect
	.cfi_endproc
                                        # -- End function
	.globl	uses_local                      # -- Begin function uses_local
	.p2align	4
	.type	uses_local,@function
uses_local:                             # @uses_local
.Lfunc_begin8:
	.loc	0 49 0 is_stmt 1                # corpus.cpp:49:0
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
.Ltmp27:
	.loc	0 49 41 prologue_end            # corpus.cpp:49:41
	movl	-4(%rbp), %edi
	.loc	0 49 32 is_stmt 0               # corpus.cpp:49:32
	callq	_ZL8local_fni
	movl	%eax, -8(%rbp)                  # 4-byte Spill
	.loc	0 49 55                         # corpus.cpp:49:55
	movl	-4(%rbp), %edi
	.loc	0 49 57                         # corpus.cpp:49:57
	addl	$1, %edi
	.loc	0 49 46                         # corpus.cpp:49:46
	callq	_ZL8local_fni
	movl	%eax, %ecx
	movl	-8(%rbp), %eax                  # 4-byte Reload
	.loc	0 49 44                         # corpus.cpp:49:44
	addl	%ecx, %eax
	.loc	0 49 25 epilogue_begin          # corpus.cpp:49:25
	addq	$16, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Ltmp28:
.Lfunc_end8:
	.size	uses_local, .Lfunc_end8-uses_local
	.cfi_endproc
                                        # -- End function
	.p2align	4                               # -- Begin function _ZL8local_fni
	.type	_ZL8local_fni,@function
_ZL8local_fni:                          # @_ZL8local_fni
.Lfunc_begin9:
	.loc	0 48 0 is_stmt 1                # corpus.cpp:48:0
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edi, -4(%rbp)
.Ltmp29:
	.loc	0 48 39 prologue_end            # corpus.cpp:48:39
	imull	$3, -4(%rbp), %eax
	.loc	0 48 30 epilogue_begin is_stmt 0 # corpus.cpp:48:30
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Ltmp30:
.Lfunc_end9:
	.size	_ZL8local_fni, .Lfunc_end9-_ZL8local_fni
	.cfi_endproc
                                        # -- End function
	.section	.debug_abbrev,"",@progbits
	.byte	1                               # Abbreviation Code
	.byte	17                              # DW_TAG_compile_unit
	.byte	1                               # DW_CHILDREN_yes
	.byte	37                              # DW_AT_producer
	.byte	37                              # DW_FORM_strx1
	.byte	19                              # DW_AT_language
	.byte	5                               # DW_FORM_data2
	.byte	3                               # DW_AT_name
	.byte	37                              # DW_FORM_strx1
	.byte	114                             # DW_AT_str_offsets_base
	.byte	23                              # DW_FORM_sec_offset
	.byte	16                              # DW_AT_stmt_list
	.byte	23                              # DW_FORM_sec_offset
	.byte	27                              # DW_AT_comp_dir
	.byte	37                              # DW_FORM_strx1
	.byte	17                              # DW_AT_low_pc
	.byte	27                              # DW_FORM_addrx
	.byte	18                              # DW_AT_high_pc
	.byte	6                               # DW_FORM_data4
	.byte	115                             # DW_AT_addr_base
	.byte	23                              # DW_FORM_sec_offset
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	2                               # Abbreviation Code
	.byte	46                              # DW_TAG_subprogram
	.byte	1                               # DW_CHILDREN_yes
	.byte	17                              # DW_AT_low_pc
	.byte	27                              # DW_FORM_addrx
	.byte	18                              # DW_AT_high_pc
	.byte	6                               # DW_FORM_data4
	.byte	64                              # DW_AT_frame_base
	.byte	24                              # DW_FORM_exprloc
	.byte	3                               # DW_AT_name
	.byte	37                              # DW_FORM_strx1
	.byte	58                              # DW_AT_decl_file
	.byte	11                              # DW_FORM_data1
	.byte	59                              # DW_AT_decl_line
	.byte	11                              # DW_FORM_data1
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	63                              # DW_AT_external
	.byte	25                              # DW_FORM_flag_present
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	3                               # Abbreviation Code
	.byte	5                               # DW_TAG_formal_parameter
	.byte	0                               # DW_CHILDREN_no
	.byte	2                               # DW_AT_location
	.byte	24                              # DW_FORM_exprloc
	.byte	3                               # DW_AT_name
	.byte	37                              # DW_FORM_strx1
	.byte	58                              # DW_AT_decl_file
	.byte	11                              # DW_FORM_data1
	.byte	59                              # DW_AT_decl_line
	.byte	11                              # DW_FORM_data1
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	4                               # Abbreviation Code
	.byte	52                              # DW_TAG_variable
	.byte	0                               # DW_CHILDREN_no
	.byte	2                               # DW_AT_location
	.byte	24                              # DW_FORM_exprloc
	.byte	3                               # DW_AT_name
	.byte	37                              # DW_FORM_strx1
	.byte	58                              # DW_AT_decl_file
	.byte	11                              # DW_FORM_data1
	.byte	59                              # DW_AT_decl_line
	.byte	11                              # DW_FORM_data1
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	5                               # Abbreviation Code
	.byte	11                              # DW_TAG_lexical_block
	.byte	1                               # DW_CHILDREN_yes
	.byte	17                              # DW_AT_low_pc
	.byte	27                              # DW_FORM_addrx
	.byte	18                              # DW_AT_high_pc
	.byte	6                               # DW_FORM_data4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	6                               # Abbreviation Code
	.byte	46                              # DW_TAG_subprogram
	.byte	1                               # DW_CHILDREN_yes
	.byte	17                              # DW_AT_low_pc
	.byte	27                              # DW_FORM_addrx
	.byte	18                              # DW_AT_high_pc
	.byte	6                               # DW_FORM_data4
	.byte	64                              # DW_AT_frame_base
	.byte	24                              # DW_FORM_exprloc
	.byte	110                             # DW_AT_linkage_name
	.byte	37                              # DW_FORM_strx1
	.byte	3                               # DW_AT_name
	.byte	37                              # DW_FORM_strx1
	.byte	58                              # DW_AT_decl_file
	.byte	11                              # DW_FORM_data1
	.byte	59                              # DW_AT_decl_line
	.byte	11                              # DW_FORM_data1
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	7                               # Abbreviation Code
	.byte	36                              # DW_TAG_base_type
	.byte	0                               # DW_CHILDREN_no
	.byte	3                               # DW_AT_name
	.byte	37                              # DW_FORM_strx1
	.byte	62                              # DW_AT_encoding
	.byte	11                              # DW_FORM_data1
	.byte	11                              # DW_AT_byte_size
	.byte	11                              # DW_FORM_data1
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	8                               # Abbreviation Code
	.byte	15                              # DW_TAG_pointer_type
	.byte	0                               # DW_CHILDREN_no
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	9                               # Abbreviation Code
	.byte	38                              # DW_TAG_const_type
	.byte	0                               # DW_CHILDREN_no
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	10                              # Abbreviation Code
	.byte	21                              # DW_TAG_subroutine_type
	.byte	1                               # DW_CHILDREN_yes
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	11                              # Abbreviation Code
	.byte	5                               # DW_TAG_formal_parameter
	.byte	0                               # DW_CHILDREN_no
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	0                               # EOM(3)
	.section	.debug_info,"",@progbits
.Lcu_begin0:
	.long	.Ldebug_info_end0-.Ldebug_info_start0 # Length of Unit
.Ldebug_info_start0:
	.short	5                               # DWARF version number
	.byte	1                               # DWARF Unit Type
	.byte	8                               # Address Size (in bytes)
	.long	.debug_abbrev                   # Offset Into Abbrev. Section
	.byte	1                               # Abbrev [1] 0xc:0x18e DW_TAG_compile_unit
	.byte	0                               # DW_AT_producer
	.short	33                              # DW_AT_language
	.byte	1                               # DW_AT_name
	.long	.Lstr_offsets_base0             # DW_AT_str_offsets_base
	.long	.Lline_table_start0             # DW_AT_stmt_list
	.byte	2                               # DW_AT_comp_dir
	.byte	0                               # DW_AT_low_pc
	.long	.Lfunc_end9-.Lfunc_begin0       # DW_AT_high_pc
	.long	.Laddr_table_base0              # DW_AT_addr_base
	.byte	2                               # Abbrev [2] 0x23:0x26 DW_TAG_subprogram
	.byte	0                               # DW_AT_low_pc
	.long	.Lfunc_end0-.Lfunc_begin0       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	86
	.byte	3                               # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	2                               # DW_AT_decl_line
	.long	379                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x32:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	124
	.byte	15                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	2                               # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0x3d:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	120
	.byte	16                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	2                               # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0x49:0x1b DW_TAG_subprogram
	.byte	1                               # DW_AT_low_pc
	.long	.Lfunc_end1-.Lfunc_begin1       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	86
	.byte	5                               # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	4                               # DW_AT_decl_line
	.long	379                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x58:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	120
	.byte	17                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	4                               # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0x64:0x43 DW_TAG_subprogram
	.byte	2                               # DW_AT_low_pc
	.long	.Lfunc_end2-.Lfunc_begin2       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	86
	.byte	6                               # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	10                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x73:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	120
	.byte	15                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	10                              # DW_AT_decl_line
	.long	383                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0x7e:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	116
	.byte	18                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	10                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	4                               # Abbrev [4] 0x89:0xb DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	112
	.byte	19                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	11                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	5                               # Abbrev [5] 0x94:0x12 DW_TAG_lexical_block
	.byte	3                               # DW_AT_low_pc
	.long	.Ltmp11-.Ltmp7                  # DW_AT_high_pc
	.byte	4                               # Abbrev [4] 0x9a:0xb DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	108
	.byte	20                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	12                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0xa7:0x1b DW_TAG_subprogram
	.byte	4                               # DW_AT_low_pc
	.long	.Lfunc_end3-.Lfunc_begin3       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	86
	.byte	7                               # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	16                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0xb6:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	120
	.byte	21                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	16                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0xc2:0x1b DW_TAG_subprogram
	.byte	5                               # DW_AT_low_pc
	.long	.Lfunc_end4-.Lfunc_begin4       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	86
	.byte	8                               # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	29                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0xd1:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	124
	.byte	17                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	29                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0xdd:0x26 DW_TAG_subprogram
	.byte	6                               # DW_AT_low_pc
	.long	.Lfunc_end5-.Lfunc_begin5       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	86
	.byte	9                               # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	32                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0xec:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	120
	.byte	21                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	32                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0xf7:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	116
	.byte	22                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	32                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0x103:0x1b DW_TAG_subprogram
	.byte	7                               # DW_AT_low_pc
	.long	.Lfunc_end6-.Lfunc_begin6       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	86
	.byte	10                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	44                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x112:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	124
	.byte	17                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	44                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0x11e:0x26 DW_TAG_subprogram
	.byte	8                               # DW_AT_low_pc
	.long	.Lfunc_end7-.Lfunc_begin7       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	86
	.byte	11                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	46                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x12d:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	120
	.byte	23                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	46                              # DW_AT_decl_line
	.long	393                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0x138:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	116
	.byte	22                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	46                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0x144:0x1b DW_TAG_subprogram
	.byte	9                               # DW_AT_low_pc
	.long	.Lfunc_end8-.Lfunc_begin8       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	86
	.byte	12                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	49                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x153:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	124
	.byte	17                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	49                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	6                               # Abbrev [6] 0x15f:0x1c DW_TAG_subprogram
	.byte	10                              # DW_AT_low_pc
	.long	.Lfunc_end9-.Lfunc_begin9       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	86
	.byte	13                              # DW_AT_linkage_name
	.byte	14                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	48                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0x16f:0xb DW_TAG_formal_parameter
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	124
	.byte	17                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	48                              # DW_AT_decl_line
	.long	379                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	7                               # Abbrev [7] 0x17b:0x4 DW_TAG_base_type
	.byte	4                               # DW_AT_name
	.byte	5                               # DW_AT_encoding
	.byte	4                               # DW_AT_byte_size
	.byte	8                               # Abbrev [8] 0x17f:0x5 DW_TAG_pointer_type
	.long	388                             # DW_AT_type
	.byte	9                               # Abbrev [9] 0x184:0x5 DW_TAG_const_type
	.long	379                             # DW_AT_type
	.byte	8                               # Abbrev [8] 0x189:0x5 DW_TAG_pointer_type
	.long	398                             # DW_AT_type
	.byte	10                              # Abbrev [10] 0x18e:0xb DW_TAG_subroutine_type
	.long	379                             # DW_AT_type
	.byte	11                              # Abbrev [11] 0x193:0x5 DW_TAG_formal_parameter
	.long	379                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	0                               # End Of Children Mark
.Ldebug_info_end0:
	.section	.debug_str_offsets,"",@progbits
	.long	100                             # Length of String Offsets Set
	.short	5
	.short	0
.Lstr_offsets_base0:
	.section	.debug_str,"MS",@progbits,1
.Linfo_string0:
	.asciz	"Apple clang version 21.0.0 (clang-2100.0.123.102)" # string offset=0
.Linfo_string1:
	.asciz	"corpus.cpp"                    # string offset=50
.Linfo_string2:
	.asciz	"/Users/michaelweiss/git/github/OCTOS/tests/data/asm" # string offset=61
.Linfo_string3:
	.asciz	"simple_add"                    # string offset=113
.Linfo_string4:
	.asciz	"int"                           # string offset=124
.Linfo_string5:
	.asciz	"if_else"                       # string offset=128
.Linfo_string6:
	.asciz	"loop_sum"                      # string offset=136
.Linfo_string7:
	.asciz	"sw"                            # string offset=145
.Linfo_string8:
	.asciz	"tail_call"                     # string offset=148
.Linfo_string9:
	.asciz	"sw_call"                       # string offset=158
.Linfo_string10:
	.asciz	"call_twice"                    # string offset=166
.Linfo_string11:
	.asciz	"indirect"                      # string offset=177
.Linfo_string12:
	.asciz	"uses_local"                    # string offset=186
.Linfo_string13:
	.asciz	"_ZL8local_fni"                 # string offset=197
.Linfo_string14:
	.asciz	"local_fn"                      # string offset=211
.Linfo_string15:
	.asciz	"a"                             # string offset=220
.Linfo_string16:
	.asciz	"b"                             # string offset=222
.Linfo_string17:
	.asciz	"x"                             # string offset=224
.Linfo_string18:
	.asciz	"n"                             # string offset=226
.Linfo_string19:
	.asciz	"s"                             # string offset=228
.Linfo_string20:
	.asciz	"i"                             # string offset=230
.Linfo_string21:
	.asciz	"k"                             # string offset=232
.Linfo_string22:
	.asciz	"v"                             # string offset=234
.Linfo_string23:
	.asciz	"fp"                            # string offset=236
	.section	.debug_str_offsets,"",@progbits
	.long	.Linfo_string0
	.long	.Linfo_string1
	.long	.Linfo_string2
	.long	.Linfo_string3
	.long	.Linfo_string4
	.long	.Linfo_string5
	.long	.Linfo_string6
	.long	.Linfo_string7
	.long	.Linfo_string8
	.long	.Linfo_string9
	.long	.Linfo_string10
	.long	.Linfo_string11
	.long	.Linfo_string12
	.long	.Linfo_string13
	.long	.Linfo_string14
	.long	.Linfo_string15
	.long	.Linfo_string16
	.long	.Linfo_string17
	.long	.Linfo_string18
	.long	.Linfo_string19
	.long	.Linfo_string20
	.long	.Linfo_string21
	.long	.Linfo_string22
	.long	.Linfo_string23
	.section	.debug_addr,"",@progbits
	.long	.Ldebug_addr_end0-.Ldebug_addr_start0 # Length of contribution
.Ldebug_addr_start0:
	.short	5                               # DWARF version number
	.byte	8                               # Address size
	.byte	0                               # Segment selector size
.Laddr_table_base0:
	.quad	.Lfunc_begin0
	.quad	.Lfunc_begin1
	.quad	.Lfunc_begin2
	.quad	.Ltmp7
	.quad	.Lfunc_begin3
	.quad	.Lfunc_begin4
	.quad	.Lfunc_begin5
	.quad	.Lfunc_begin6
	.quad	.Lfunc_begin7
	.quad	.Lfunc_begin8
	.quad	.Lfunc_begin9
.Ldebug_addr_end0:
	.ident	"Apple clang version 21.0.0 (clang-2100.0.123.102)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym helper
	.addrsig_sym f0
	.addrsig_sym f1
	.addrsig_sym f2
	.addrsig_sym f3
	.addrsig_sym f4
	.addrsig_sym f5
	.addrsig_sym _ZL8local_fni
	.section	.debug_line,"",@progbits
.Lline_table_start0:
