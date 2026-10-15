	.intel_syntax noprefix
	.file	"corpus.cpp"
	.text
	.globl	simple_add                      # -- Begin function simple_add
	.p2align	4
	.type	simple_add,@function
simple_add:                             # @simple_add
.Lfunc_begin0:
	.file	0 "/Users/michaelweiss/git/github/OCTOS/tests/data/asm" "corpus.cpp" md5 0xba4d7a0b733ef1532df489d07fae416a
	.cfi_startproc
# %bb.0:
	#DEBUG_VALUE: simple_add:a <- $edi
	#DEBUG_VALUE: simple_add:b <- $esi
                                        # kill: def $esi killed $esi def $rsi
                                        # kill: def $edi killed $edi def $rdi
	.loc	0 2 41 prologue_end             # corpus.cpp:2:41
	lea	eax, [rdi + rsi]
	.loc	0 2 32 is_stmt 0                # corpus.cpp:2:32
	ret
.Ltmp0:
.Lfunc_end0:
	.size	simple_add, .Lfunc_end0-simple_add
	.cfi_endproc
                                        # -- End function
	.globl	if_else                         # -- Begin function if_else
	.p2align	4
	.type	if_else,@function
if_else:                                # @if_else
.Lfunc_begin1:
	.cfi_startproc
# %bb.0:
	#DEBUG_VALUE: if_else:x <- $edi
                                        # kill: def $edi killed $edi def $rdi
	.loc	0 5 11 prologue_end is_stmt 1   # corpus.cpp:5:11
	lea	ecx, [rdi + rdi]
	mov	edx, edi
	neg	edx
	xor	eax, eax
	cmp	edi, -10
	cmovl	eax, edx
	cmp	edi, 11
	cmovge	eax, ecx
.Ltmp1:
	.loc	0 8 1                           # corpus.cpp:8:1
	ret
.Ltmp2:
.Lfunc_end1:
	.size	if_else, .Lfunc_end1-if_else
	.cfi_endproc
                                        # -- End function
	.globl	loop_sum                        # -- Begin function loop_sum
	.p2align	4
	.type	loop_sum,@function
loop_sum:                               # @loop_sum
.Lfunc_begin2:
	.cfi_startproc
# %bb.0:
	#DEBUG_VALUE: loop_sum:a <- $rdi
	#DEBUG_VALUE: loop_sum:n <- $esi
	#DEBUG_VALUE: loop_sum:s <- 0
	#DEBUG_VALUE: i <- 0
	.loc	0 12 23 prologue_end            # corpus.cpp:12:23
	test	esi, esi
.Ltmp3:
	.loc	0 12 5 is_stmt 0                # corpus.cpp:12:5
	jle	.LBB2_1
.Ltmp4:
# %bb.2:
	#DEBUG_VALUE: i <- 0
	#DEBUG_VALUE: loop_sum:s <- 0
	#DEBUG_VALUE: loop_sum:n <- $esi
	#DEBUG_VALUE: loop_sum:a <- $rdi
	.loc	0 12 23                         # corpus.cpp:12:23
	mov	ecx, esi
.Ltmp5:
	.loc	0 12 5                          # corpus.cpp:12:5
	cmp	esi, 8
	jae	.LBB2_4
.Ltmp6:
# %bb.3:
	#DEBUG_VALUE: i <- 0
	#DEBUG_VALUE: loop_sum:s <- 0
	#DEBUG_VALUE: loop_sum:n <- $esi
	#DEBUG_VALUE: loop_sum:a <- $rdi
	.loc	0 0 5                           # corpus.cpp:0:5
	xor	edx, edx
	xor	eax, eax
	.loc	0 12 5                          # corpus.cpp:12:5
	jmp	.LBB2_7
.Ltmp7:
.LBB2_1:
	#DEBUG_VALUE: loop_sum:s <- 0
	#DEBUG_VALUE: loop_sum:n <- $esi
	#DEBUG_VALUE: loop_sum:a <- $rdi
	.loc	0 0 5                           # corpus.cpp:0:5
	xor	eax, eax
	.loc	0 13 5 is_stmt 1                # corpus.cpp:13:5
	ret
.Ltmp8:
.LBB2_4:
	#DEBUG_VALUE: i <- 0
	#DEBUG_VALUE: loop_sum:s <- 0
	#DEBUG_VALUE: loop_sum:n <- $esi
	#DEBUG_VALUE: loop_sum:a <- $rdi
	.loc	0 12 5                          # corpus.cpp:12:5
	mov	edx, ecx
	and	edx, 2147483640
	mov	eax, ecx
	shr	eax, 3
	and	eax, 268435455
	shl	rax, 5
	pxor	xmm0, xmm0
	xor	esi, esi
.Ltmp9:
	#DEBUG_VALUE: loop_sum:n <- [DW_OP_LLVM_entry_value 1] $esi
	.loc	0 0 5 is_stmt 0                 # corpus.cpp:0:5
	pxor	xmm1, xmm1
.Ltmp10:
	.p2align	4
.LBB2_5:                                # =>This Inner Loop Header: Depth=1
	#DEBUG_VALUE: loop_sum:n <- [DW_OP_LLVM_entry_value 1] $esi
	#DEBUG_VALUE: i <- 0
	#DEBUG_VALUE: loop_sum:s <- 0
	#DEBUG_VALUE: loop_sum:a <- $rdi
	.loc	0 12 38                         # corpus.cpp:12:38
	movdqu	xmm2, xmmword ptr [rdi + rsi]
	.loc	0 12 35                         # corpus.cpp:12:35
	paddd	xmm0, xmm2
	.loc	0 12 38                         # corpus.cpp:12:38
	movdqu	xmm2, xmmword ptr [rdi + rsi + 16]
	.loc	0 12 35                         # corpus.cpp:12:35
	paddd	xmm1, xmm2
	.loc	0 12 28                         # corpus.cpp:12:28
	add	rsi, 32
	cmp	rax, rsi
	jne	.LBB2_5
.Ltmp11:
# %bb.6:
	#DEBUG_VALUE: loop_sum:n <- [DW_OP_LLVM_entry_value 1] $esi
	#DEBUG_VALUE: i <- 0
	#DEBUG_VALUE: loop_sum:s <- 0
	#DEBUG_VALUE: loop_sum:a <- $rdi
	.loc	0 12 5                          # corpus.cpp:12:5
	paddd	xmm1, xmm0
	pshufd	xmm0, xmm1, 238                 # xmm0 = xmm1[2,3,2,3]
	paddd	xmm0, xmm1
	pshufd	xmm1, xmm0, 85                  # xmm1 = xmm0[1,1,1,1]
	paddd	xmm1, xmm0
	movd	eax, xmm1
	cmp	edx, ecx
	je	.LBB2_8
.Ltmp12:
	.loc	0 0 5                           # :0:5
.Ltmp13:
	.p2align	4
.LBB2_7:                                # =>This Inner Loop Header: Depth=1
	#DEBUG_VALUE: loop_sum:a <- $rdi
	#DEBUG_VALUE: loop_sum:s <- $eax
	#DEBUG_VALUE: i <- $rdx
	.loc	0 12 35                         # corpus.cpp:12:35
	add	eax, dword ptr [rdi + 4*rdx]
.Ltmp14:
	#DEBUG_VALUE: loop_sum:s <- $eax
	.loc	0 12 28                         # corpus.cpp:12:28
	inc	rdx
.Ltmp15:
	#DEBUG_VALUE: i <- $rdx
	.loc	0 12 23                         # corpus.cpp:12:23
	cmp	rcx, rdx
.Ltmp16:
	.loc	0 12 5                          # corpus.cpp:12:5
	jne	.LBB2_7
.Ltmp17:
.LBB2_8:
	#DEBUG_VALUE: loop_sum:a <- $rdi
	.loc	0 13 5 is_stmt 1                # corpus.cpp:13:5
	ret
.Ltmp18:
.Lfunc_end2:
	.size	loop_sum, .Lfunc_end2-loop_sum
	.cfi_endproc
                                        # -- End function
	.globl	sw                              # -- Begin function sw
	.p2align	4
	.type	sw,@function
sw:                                     # @sw
.Lfunc_begin3:
	.cfi_startproc
# %bb.0:
	#DEBUG_VALUE: sw:k <- $edi
                                        # kill: def $edi killed $edi def $rdi
	.loc	0 17 5 prologue_end             # corpus.cpp:17:5
	cmp	edi, 6
	lea	eax, [rdi + 4*rdi]
	lea	ecx, [rdi + 2*rax + 11]
	mov	eax, -1
	cmovb	eax, ecx
	.loc	0 26 1                          # corpus.cpp:26:1
	ret
.Ltmp19:
.Lfunc_end3:
	.size	sw, .Lfunc_end3-sw
	.cfi_endproc
                                        # -- End function
	.globl	tail_call                       # -- Begin function tail_call
	.p2align	4
	.type	tail_call,@function
tail_call:                              # @tail_call
.Lfunc_begin4:
	.cfi_startproc
# %bb.0:
	#DEBUG_VALUE: tail_call:x <- $edi
	.loc	0 29 40 prologue_end            # corpus.cpp:29:40
	inc	edi
.Ltmp20:
	#DEBUG_VALUE: tail_call:x <- [DW_OP_LLVM_entry_value 1] $edi
	.loc	0 29 31 is_stmt 0               # corpus.cpp:29:31
	jmp	helper@PLT                      # TAILCALL
.Ltmp21:
.Lfunc_end4:
	.size	tail_call, .Lfunc_end4-tail_call
	.cfi_endproc
                                        # -- End function
	.globl	sw_call                         # -- Begin function sw_call
	.p2align	4
	.type	sw_call,@function
sw_call:                                # @sw_call
.Lfunc_begin5:
	.cfi_startproc
# %bb.0:
	#DEBUG_VALUE: sw_call:k <- $edi
	#DEBUG_VALUE: sw_call:v <- $esi
	.loc	0 33 5 prologue_end is_stmt 1   # corpus.cpp:33:5
	cmp	edi, 5
	ja	.LBB5_8
.Ltmp22:
# %bb.1:
	#DEBUG_VALUE: sw_call:v <- $esi
	#DEBUG_VALUE: sw_call:k <- $edi
	.loc	0 0 5 is_stmt 0                 # corpus.cpp:0:5
	mov	eax, edi
	lea	rcx, [rip + .LJTI5_0]
	.loc	0 33 5                          # corpus.cpp:33:5
	movsxd	rax, dword ptr [rcx + 4*rax]
	add	rax, rcx
	jmp	rax
.Ltmp23:
.LBB5_2:
	#DEBUG_VALUE: sw_call:v <- $esi
	#DEBUG_VALUE: sw_call:k <- $edi
	.loc	0 34 24 is_stmt 1               # corpus.cpp:34:24
	mov	edi, esi
.Ltmp24:
	#DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $edi
	jmp	f0@PLT                          # TAILCALL
.Ltmp25:
.LBB5_6:
	#DEBUG_VALUE: sw_call:v <- $esi
	#DEBUG_VALUE: sw_call:k <- $edi
	.loc	0 38 24                         # corpus.cpp:38:24
	mov	edi, esi
.Ltmp26:
	#DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $edi
	jmp	f4@PLT                          # TAILCALL
.Ltmp27:
.LBB5_4:
	#DEBUG_VALUE: sw_call:v <- $esi
	#DEBUG_VALUE: sw_call:k <- $edi
	.loc	0 36 24                         # corpus.cpp:36:24
	mov	edi, esi
.Ltmp28:
	#DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $edi
	jmp	f2@PLT                          # TAILCALL
.Ltmp29:
.LBB5_5:
	#DEBUG_VALUE: sw_call:v <- $esi
	#DEBUG_VALUE: sw_call:k <- $edi
	.loc	0 37 24                         # corpus.cpp:37:24
	mov	edi, esi
.Ltmp30:
	#DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $edi
	jmp	f3@PLT                          # TAILCALL
.Ltmp31:
.LBB5_3:
	#DEBUG_VALUE: sw_call:v <- $esi
	#DEBUG_VALUE: sw_call:k <- $edi
	.loc	0 35 24                         # corpus.cpp:35:24
	mov	edi, esi
.Ltmp32:
	#DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $edi
	jmp	f1@PLT                          # TAILCALL
.Ltmp33:
.LBB5_7:
	#DEBUG_VALUE: sw_call:v <- $esi
	#DEBUG_VALUE: sw_call:k <- $edi
	.loc	0 39 24                         # corpus.cpp:39:24
	mov	edi, esi
.Ltmp34:
	#DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $edi
	jmp	f5@PLT                          # TAILCALL
.Ltmp35:
.LBB5_8:
	#DEBUG_VALUE: sw_call:v <- $esi
	#DEBUG_VALUE: sw_call:k <- $edi
	.loc	0 42 1                          # corpus.cpp:42:1
	xor	eax, eax
	ret
.Ltmp36:
.Lfunc_end5:
	.size	sw_call, .Lfunc_end5-sw_call
	.cfi_endproc
	.section	.rodata,"a",@progbits
	.p2align	2, 0x0
.LJTI5_0:
	.long	.LBB5_2-.LJTI5_0
	.long	.LBB5_3-.LJTI5_0
	.long	.LBB5_4-.LJTI5_0
	.long	.LBB5_5-.LJTI5_0
	.long	.LBB5_6-.LJTI5_0
	.long	.LBB5_7-.LJTI5_0
                                        # -- End function
	.text
	.globl	call_twice                      # -- Begin function call_twice
	.p2align	4
	.type	call_twice,@function
call_twice:                             # @call_twice
.Lfunc_begin6:
	.loc	0 44 0                          # corpus.cpp:44:0
	.cfi_startproc
# %bb.0:
	#DEBUG_VALUE: call_twice:x <- $edi
	push	rbp
	.cfi_def_cfa_offset 16
	push	rbx
	.cfi_def_cfa_offset 24
	push	rax
	.cfi_def_cfa_offset 32
	.cfi_offset rbx, -24
	.cfi_offset rbp, -16
	mov	ebx, edi
.Ltmp37:
	#DEBUG_VALUE: call_twice:x <- $ebx
	.loc	0 44 32 prologue_end            # corpus.cpp:44:32
	call	helper@PLT
.Ltmp38:
	mov	ebp, eax
	.loc	0 44 53 is_stmt 0               # corpus.cpp:44:53
	inc	ebx
.Ltmp39:
	#DEBUG_VALUE: call_twice:x <- [DW_OP_LLVM_entry_value 1] $edi
	.loc	0 44 44                         # corpus.cpp:44:44
	mov	edi, ebx
	call	helper@PLT
.Ltmp40:
	.loc	0 44 42                         # corpus.cpp:44:42
	add	eax, ebp
	.loc	0 44 25 epilogue_begin          # corpus.cpp:44:25
	add	rsp, 8
	.cfi_def_cfa_offset 24
	pop	rbx
	.cfi_def_cfa_offset 16
	pop	rbp
	.cfi_def_cfa_offset 8
	ret
.Ltmp41:
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
	#DEBUG_VALUE: indirect:fp <- $rdi
	#DEBUG_VALUE: indirect:v <- $esi
	mov	rax, rdi
.Ltmp42:
	#DEBUG_VALUE: indirect:fp <- $rax
	.loc	0 46 46 prologue_end            # corpus.cpp:46:46
	mov	edi, esi
.Ltmp43:
	#DEBUG_VALUE: indirect:v <- $edi
	jmp	rax                             # TAILCALL
.Ltmp44:
.Lfunc_end7:
	.size	indirect, .Lfunc_end7-indirect
	.cfi_endproc
                                        # -- End function
	.globl	uses_local                      # -- Begin function uses_local
	.p2align	4
	.type	uses_local,@function
uses_local:                             # @uses_local
.Lfunc_begin8:
	.cfi_startproc
# %bb.0:
	#DEBUG_VALUE: uses_local:x <- $edi
                                        # kill: def $edi killed $edi def $rdi
	.loc	0 49 44 prologue_end            # corpus.cpp:49:44
	lea	eax, [rdi + 2*rdi]
	lea	eax, [2*rax + 3]
	.loc	0 49 25 is_stmt 0               # corpus.cpp:49:25
	ret
.Ltmp45:
.Lfunc_end8:
	.size	uses_local, .Lfunc_end8-uses_local
	.cfi_endproc
                                        # -- End function
	.section	.debug_loclists,"",@progbits
	.long	.Ldebug_list_header_end0-.Ldebug_list_header_start0 # Length
.Ldebug_list_header_start0:
	.short	5                               # Version
	.byte	8                               # Address size
	.byte	0                               # Segment selector size
	.long	8                               # Offset entry count
.Lloclists_table_base0:
	.long	.Ldebug_loc0-.Lloclists_table_base0
	.long	.Ldebug_loc1-.Lloclists_table_base0
	.long	.Ldebug_loc2-.Lloclists_table_base0
	.long	.Ldebug_loc3-.Lloclists_table_base0
	.long	.Ldebug_loc4-.Lloclists_table_base0
	.long	.Ldebug_loc5-.Lloclists_table_base0
	.long	.Ldebug_loc6-.Lloclists_table_base0
	.long	.Ldebug_loc7-.Lloclists_table_base0
.Ldebug_loc0:
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Lfunc_begin2-.Lfunc_begin0    #   starting offset
	.uleb128 .Ltmp9-.Lfunc_begin0           #   ending offset
	.byte	1                               # Loc expr size
	.byte	84                              # super-register DW_OP_reg4
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp9-.Lfunc_begin0           #   starting offset
	.uleb128 .Ltmp12-.Lfunc_begin0          #   ending offset
	.byte	4                               # Loc expr size
	.byte	163                             # DW_OP_entry_value
	.byte	1                               # 1
	.byte	84                              # super-register DW_OP_reg4
	.byte	159                             # DW_OP_stack_value
	.byte	0                               # DW_LLE_end_of_list
.Ldebug_loc1:
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Lfunc_begin2-.Lfunc_begin0    #   starting offset
	.uleb128 .Ltmp12-.Lfunc_begin0          #   ending offset
	.byte	3                               # Loc expr size
	.byte	17                              # DW_OP_consts
	.byte	0                               # 0
	.byte	159                             # DW_OP_stack_value
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp12-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp17-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	80                              # super-register DW_OP_reg0
	.byte	0                               # DW_LLE_end_of_list
.Ldebug_loc2:
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Lfunc_begin2-.Lfunc_begin0    #   starting offset
	.uleb128 .Ltmp7-.Lfunc_begin0           #   ending offset
	.byte	3                               # Loc expr size
	.byte	17                              # DW_OP_consts
	.byte	0                               # 0
	.byte	159                             # DW_OP_stack_value
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp8-.Lfunc_begin0           #   starting offset
	.uleb128 .Ltmp12-.Lfunc_begin0          #   ending offset
	.byte	3                               # Loc expr size
	.byte	17                              # DW_OP_consts
	.byte	0                               # 0
	.byte	159                             # DW_OP_stack_value
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp12-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp17-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	81                              # DW_OP_reg1
	.byte	0                               # DW_LLE_end_of_list
.Ldebug_loc3:
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Lfunc_begin4-.Lfunc_begin0    #   starting offset
	.uleb128 .Ltmp20-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	85                              # super-register DW_OP_reg5
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp20-.Lfunc_begin0          #   starting offset
	.uleb128 .Lfunc_end4-.Lfunc_begin0      #   ending offset
	.byte	4                               # Loc expr size
	.byte	163                             # DW_OP_entry_value
	.byte	1                               # 1
	.byte	85                              # super-register DW_OP_reg5
	.byte	159                             # DW_OP_stack_value
	.byte	0                               # DW_LLE_end_of_list
.Ldebug_loc4:
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Lfunc_begin5-.Lfunc_begin0    #   starting offset
	.uleb128 .Ltmp24-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	85                              # super-register DW_OP_reg5
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp24-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp25-.Lfunc_begin0          #   ending offset
	.byte	4                               # Loc expr size
	.byte	163                             # DW_OP_entry_value
	.byte	1                               # 1
	.byte	85                              # super-register DW_OP_reg5
	.byte	159                             # DW_OP_stack_value
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp25-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp26-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	85                              # super-register DW_OP_reg5
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp26-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp27-.Lfunc_begin0          #   ending offset
	.byte	4                               # Loc expr size
	.byte	163                             # DW_OP_entry_value
	.byte	1                               # 1
	.byte	85                              # super-register DW_OP_reg5
	.byte	159                             # DW_OP_stack_value
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp27-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp28-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	85                              # super-register DW_OP_reg5
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp28-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp29-.Lfunc_begin0          #   ending offset
	.byte	4                               # Loc expr size
	.byte	163                             # DW_OP_entry_value
	.byte	1                               # 1
	.byte	85                              # super-register DW_OP_reg5
	.byte	159                             # DW_OP_stack_value
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp29-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp30-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	85                              # super-register DW_OP_reg5
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp30-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp31-.Lfunc_begin0          #   ending offset
	.byte	4                               # Loc expr size
	.byte	163                             # DW_OP_entry_value
	.byte	1                               # 1
	.byte	85                              # super-register DW_OP_reg5
	.byte	159                             # DW_OP_stack_value
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp31-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp32-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	85                              # super-register DW_OP_reg5
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp32-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp33-.Lfunc_begin0          #   ending offset
	.byte	4                               # Loc expr size
	.byte	163                             # DW_OP_entry_value
	.byte	1                               # 1
	.byte	85                              # super-register DW_OP_reg5
	.byte	159                             # DW_OP_stack_value
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp33-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp34-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	85                              # super-register DW_OP_reg5
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp34-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp35-.Lfunc_begin0          #   ending offset
	.byte	4                               # Loc expr size
	.byte	163                             # DW_OP_entry_value
	.byte	1                               # 1
	.byte	85                              # super-register DW_OP_reg5
	.byte	159                             # DW_OP_stack_value
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp35-.Lfunc_begin0          #   starting offset
	.uleb128 .Lfunc_end5-.Lfunc_begin0      #   ending offset
	.byte	1                               # Loc expr size
	.byte	85                              # super-register DW_OP_reg5
	.byte	0                               # DW_LLE_end_of_list
.Ldebug_loc5:
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Lfunc_begin6-.Lfunc_begin0    #   starting offset
	.uleb128 .Ltmp37-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	85                              # super-register DW_OP_reg5
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp37-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp39-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	83                              # super-register DW_OP_reg3
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp39-.Lfunc_begin0          #   starting offset
	.uleb128 .Lfunc_end6-.Lfunc_begin0      #   ending offset
	.byte	4                               # Loc expr size
	.byte	163                             # DW_OP_entry_value
	.byte	1                               # 1
	.byte	85                              # super-register DW_OP_reg5
	.byte	159                             # DW_OP_stack_value
	.byte	0                               # DW_LLE_end_of_list
.Ldebug_loc6:
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Lfunc_begin7-.Lfunc_begin0    #   starting offset
	.uleb128 .Ltmp42-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	85                              # DW_OP_reg5
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp42-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp44-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	80                              # DW_OP_reg0
	.byte	0                               # DW_LLE_end_of_list
.Ldebug_loc7:
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Lfunc_begin7-.Lfunc_begin0    #   starting offset
	.uleb128 .Ltmp43-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	84                              # super-register DW_OP_reg4
	.byte	4                               # DW_LLE_offset_pair
	.uleb128 .Ltmp43-.Lfunc_begin0          #   starting offset
	.uleb128 .Ltmp44-.Lfunc_begin0          #   ending offset
	.byte	1                               # Loc expr size
	.byte	85                              # super-register DW_OP_reg5
	.byte	0                               # DW_LLE_end_of_list
.Ldebug_list_header_end0:
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
	.byte	116                             # DW_AT_rnglists_base
	.byte	23                              # DW_FORM_sec_offset
	.ascii	"\214\001"                      # DW_AT_loclists_base
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
	.byte	122                             # DW_AT_call_all_calls
	.byte	25                              # DW_FORM_flag_present
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
	.byte	5                               # DW_TAG_formal_parameter
	.byte	0                               # DW_CHILDREN_no
	.byte	2                               # DW_AT_location
	.byte	34                              # DW_FORM_loclistx
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
	.byte	52                              # DW_TAG_variable
	.byte	0                               # DW_CHILDREN_no
	.byte	2                               # DW_AT_location
	.byte	34                              # DW_FORM_loclistx
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
	.byte	6                               # Abbreviation Code
	.byte	11                              # DW_TAG_lexical_block
	.byte	1                               # DW_CHILDREN_yes
	.byte	85                              # DW_AT_ranges
	.byte	35                              # DW_FORM_rnglistx
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	7                               # Abbreviation Code
	.byte	72                              # DW_TAG_call_site
	.byte	0                               # DW_CHILDREN_no
	.byte	127                             # DW_AT_call_origin
	.byte	19                              # DW_FORM_ref4
	.ascii	"\202\001"                      # DW_AT_call_tail_call
	.byte	25                              # DW_FORM_flag_present
	.ascii	"\201\001"                      # DW_AT_call_pc
	.byte	27                              # DW_FORM_addrx
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	8                               # Abbreviation Code
	.byte	46                              # DW_TAG_subprogram
	.byte	1                               # DW_CHILDREN_yes
	.byte	3                               # DW_AT_name
	.byte	37                              # DW_FORM_strx1
	.byte	58                              # DW_AT_decl_file
	.byte	11                              # DW_FORM_data1
	.byte	59                              # DW_AT_decl_line
	.byte	11                              # DW_FORM_data1
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	60                              # DW_AT_declaration
	.byte	25                              # DW_FORM_flag_present
	.byte	63                              # DW_AT_external
	.byte	25                              # DW_FORM_flag_present
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	9                               # Abbreviation Code
	.byte	5                               # DW_TAG_formal_parameter
	.byte	0                               # DW_CHILDREN_no
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	10                              # Abbreviation Code
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
	.byte	11                              # Abbreviation Code
	.byte	72                              # DW_TAG_call_site
	.byte	1                               # DW_CHILDREN_yes
	.byte	127                             # DW_AT_call_origin
	.byte	19                              # DW_FORM_ref4
	.byte	125                             # DW_AT_call_return_pc
	.byte	27                              # DW_FORM_addrx
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	12                              # Abbreviation Code
	.byte	73                              # DW_TAG_call_site_parameter
	.byte	0                               # DW_CHILDREN_no
	.byte	2                               # DW_AT_location
	.byte	24                              # DW_FORM_exprloc
	.byte	126                             # DW_AT_call_value
	.byte	24                              # DW_FORM_exprloc
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	13                              # Abbreviation Code
	.byte	72                              # DW_TAG_call_site
	.byte	1                               # DW_CHILDREN_yes
	.ascii	"\203\001"                      # DW_AT_call_target
	.byte	24                              # DW_FORM_exprloc
	.ascii	"\202\001"                      # DW_AT_call_tail_call
	.byte	25                              # DW_FORM_flag_present
	.ascii	"\201\001"                      # DW_AT_call_pc
	.byte	27                              # DW_FORM_addrx
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	14                              # Abbreviation Code
	.byte	15                              # DW_TAG_pointer_type
	.byte	0                               # DW_CHILDREN_no
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	15                              # Abbreviation Code
	.byte	38                              # DW_TAG_const_type
	.byte	0                               # DW_CHILDREN_no
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	16                              # Abbreviation Code
	.byte	21                              # DW_TAG_subroutine_type
	.byte	1                               # DW_CHILDREN_yes
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
	.byte	1                               # Abbrev [1] 0xc:0x212 DW_TAG_compile_unit
	.byte	0                               # DW_AT_producer
	.short	33                              # DW_AT_language
	.byte	1                               # DW_AT_name
	.long	.Lstr_offsets_base0             # DW_AT_str_offsets_base
	.long	.Lline_table_start0             # DW_AT_stmt_list
	.byte	2                               # DW_AT_comp_dir
	.byte	0                               # DW_AT_low_pc
	.long	.Lfunc_end8-.Lfunc_begin0       # DW_AT_high_pc
	.long	.Laddr_table_base0              # DW_AT_addr_base
	.long	.Lrnglists_table_base0          # DW_AT_rnglists_base
	.long	.Lloclists_table_base0          # DW_AT_loclists_base
	.byte	2                               # Abbrev [2] 0x2b:0x24 DW_TAG_subprogram
	.byte	0                               # DW_AT_low_pc
	.long	.Lfunc_end0-.Lfunc_begin0       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	87
                                        # DW_AT_call_all_calls
	.byte	11                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	2                               # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x3a:0xa DW_TAG_formal_parameter
	.byte	1                               # DW_AT_location
	.byte	85
	.byte	20                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	2                               # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0x44:0xa DW_TAG_formal_parameter
	.byte	1                               # DW_AT_location
	.byte	84
	.byte	21                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	2                               # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0x4f:0x1a DW_TAG_subprogram
	.byte	1                               # DW_AT_low_pc
	.long	.Lfunc_end1-.Lfunc_begin1       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	87
                                        # DW_AT_call_all_calls
	.byte	12                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	4                               # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x5e:0xa DW_TAG_formal_parameter
	.byte	1                               # DW_AT_location
	.byte	85
	.byte	22                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	4                               # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0x69:0x38 DW_TAG_subprogram
	.byte	2                               # DW_AT_low_pc
	.long	.Lfunc_end2-.Lfunc_begin2       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	87
                                        # DW_AT_call_all_calls
	.byte	13                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	10                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x78:0xa DW_TAG_formal_parameter
	.byte	1                               # DW_AT_location
	.byte	85
	.byte	20                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	10                              # DW_AT_decl_line
	.long	515                             # DW_AT_type
	.byte	4                               # Abbrev [4] 0x82:0x9 DW_TAG_formal_parameter
	.byte	0                               # DW_AT_location
	.byte	23                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	10                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	5                               # Abbrev [5] 0x8b:0x9 DW_TAG_variable
	.byte	1                               # DW_AT_location
	.byte	24                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	11                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	6                               # Abbrev [6] 0x94:0xc DW_TAG_lexical_block
	.byte	0                               # DW_AT_ranges
	.byte	5                               # Abbrev [5] 0x96:0x9 DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	25                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	12                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0xa1:0x1a DW_TAG_subprogram
	.byte	3                               # DW_AT_low_pc
	.long	.Lfunc_end3-.Lfunc_begin3       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	87
                                        # DW_AT_call_all_calls
	.byte	14                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	16                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0xb0:0xa DW_TAG_formal_parameter
	.byte	1                               # DW_AT_location
	.byte	85
	.byte	26                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	16                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0xbb:0x1f DW_TAG_subprogram
	.byte	4                               # DW_AT_low_pc
	.long	.Lfunc_end4-.Lfunc_begin4       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	87
                                        # DW_AT_call_all_calls
	.byte	15                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	29                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_external
	.byte	4                               # Abbrev [4] 0xca:0x9 DW_TAG_formal_parameter
	.byte	3                               # DW_AT_location
	.byte	22                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	29                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	7                               # Abbrev [7] 0xd3:0x6 DW_TAG_call_site
	.long	218                             # DW_AT_call_origin
                                        # DW_AT_call_tail_call
	.byte	5                               # DW_AT_call_pc
	.byte	0                               # End Of Children Mark
	.byte	8                               # Abbrev [8] 0xda:0xe DW_TAG_subprogram
	.byte	3                               # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	28                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_declaration
                                        # DW_AT_external
	.byte	9                               # Abbrev [9] 0xe2:0x5 DW_TAG_formal_parameter
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	10                              # Abbrev [10] 0xe8:0x4 DW_TAG_base_type
	.byte	4                               # DW_AT_name
	.byte	5                               # DW_AT_encoding
	.byte	4                               # DW_AT_byte_size
	.byte	2                               # Abbrev [2] 0xec:0x47 DW_TAG_subprogram
	.byte	6                               # DW_AT_low_pc
	.long	.Lfunc_end5-.Lfunc_begin5       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	87
                                        # DW_AT_call_all_calls
	.byte	16                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	32                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_external
	.byte	4                               # Abbrev [4] 0xfb:0x9 DW_TAG_formal_parameter
	.byte	4                               # DW_AT_location
	.byte	26                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	32                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0x104:0xa DW_TAG_formal_parameter
	.byte	1                               # DW_AT_location
	.byte	84
	.byte	27                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	32                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	7                               # Abbrev [7] 0x10e:0x6 DW_TAG_call_site
	.long	307                             # DW_AT_call_origin
                                        # DW_AT_call_tail_call
	.byte	7                               # DW_AT_call_pc
	.byte	7                               # Abbrev [7] 0x114:0x6 DW_TAG_call_site
	.long	321                             # DW_AT_call_origin
                                        # DW_AT_call_tail_call
	.byte	8                               # DW_AT_call_pc
	.byte	7                               # Abbrev [7] 0x11a:0x6 DW_TAG_call_site
	.long	335                             # DW_AT_call_origin
                                        # DW_AT_call_tail_call
	.byte	9                               # DW_AT_call_pc
	.byte	7                               # Abbrev [7] 0x120:0x6 DW_TAG_call_site
	.long	349                             # DW_AT_call_origin
                                        # DW_AT_call_tail_call
	.byte	10                              # DW_AT_call_pc
	.byte	7                               # Abbrev [7] 0x126:0x6 DW_TAG_call_site
	.long	363                             # DW_AT_call_origin
                                        # DW_AT_call_tail_call
	.byte	11                              # DW_AT_call_pc
	.byte	7                               # Abbrev [7] 0x12c:0x6 DW_TAG_call_site
	.long	377                             # DW_AT_call_origin
                                        # DW_AT_call_tail_call
	.byte	12                              # DW_AT_call_pc
	.byte	0                               # End Of Children Mark
	.byte	8                               # Abbrev [8] 0x133:0xe DW_TAG_subprogram
	.byte	5                               # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	31                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_declaration
                                        # DW_AT_external
	.byte	9                               # Abbrev [9] 0x13b:0x5 DW_TAG_formal_parameter
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	8                               # Abbrev [8] 0x141:0xe DW_TAG_subprogram
	.byte	6                               # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	31                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_declaration
                                        # DW_AT_external
	.byte	9                               # Abbrev [9] 0x149:0x5 DW_TAG_formal_parameter
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	8                               # Abbrev [8] 0x14f:0xe DW_TAG_subprogram
	.byte	7                               # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	31                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_declaration
                                        # DW_AT_external
	.byte	9                               # Abbrev [9] 0x157:0x5 DW_TAG_formal_parameter
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	8                               # Abbrev [8] 0x15d:0xe DW_TAG_subprogram
	.byte	8                               # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	31                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_declaration
                                        # DW_AT_external
	.byte	9                               # Abbrev [9] 0x165:0x5 DW_TAG_formal_parameter
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	8                               # Abbrev [8] 0x16b:0xe DW_TAG_subprogram
	.byte	9                               # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	31                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_declaration
                                        # DW_AT_external
	.byte	9                               # Abbrev [9] 0x173:0x5 DW_TAG_formal_parameter
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	8                               # Abbrev [8] 0x179:0xe DW_TAG_subprogram
	.byte	10                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	31                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_declaration
                                        # DW_AT_external
	.byte	9                               # Abbrev [9] 0x181:0x5 DW_TAG_formal_parameter
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0x187:0x34 DW_TAG_subprogram
	.byte	13                              # DW_AT_low_pc
	.long	.Lfunc_end6-.Lfunc_begin6       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	87
                                        # DW_AT_call_all_calls
	.byte	17                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	44                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_external
	.byte	4                               # Abbrev [4] 0x196:0x9 DW_TAG_formal_parameter
	.byte	5                               # DW_AT_location
	.byte	22                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	44                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	11                              # Abbrev [11] 0x19f:0xe DW_TAG_call_site
	.long	218                             # DW_AT_call_origin
	.byte	14                              # DW_AT_call_return_pc
	.byte	12                              # Abbrev [12] 0x1a5:0x7 DW_TAG_call_site_parameter
	.byte	1                               # DW_AT_location
	.byte	85
	.byte	3                               # DW_AT_call_value
	.byte	163
	.byte	1
	.byte	85
	.byte	0                               # End Of Children Mark
	.byte	11                              # Abbrev [11] 0x1ad:0xd DW_TAG_call_site
	.long	218                             # DW_AT_call_origin
	.byte	15                              # DW_AT_call_return_pc
	.byte	12                              # Abbrev [12] 0x1b3:0x6 DW_TAG_call_site_parameter
	.byte	1                               # DW_AT_location
	.byte	85
	.byte	2                               # DW_AT_call_value
	.byte	115
	.byte	0
	.byte	0                               # End Of Children Mark
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0x1bb:0x2e DW_TAG_subprogram
	.byte	16                              # DW_AT_low_pc
	.long	.Lfunc_end7-.Lfunc_begin7       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	87
                                        # DW_AT_call_all_calls
	.byte	18                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	46                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_external
	.byte	4                               # Abbrev [4] 0x1ca:0x9 DW_TAG_formal_parameter
	.byte	6                               # DW_AT_location
	.byte	28                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	46                              # DW_AT_decl_line
	.long	525                             # DW_AT_type
	.byte	4                               # Abbrev [4] 0x1d3:0x9 DW_TAG_formal_parameter
	.byte	7                               # DW_AT_location
	.byte	27                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	46                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	13                              # Abbrev [13] 0x1dc:0xc DW_TAG_call_site
	.byte	1                               # DW_AT_call_target
	.byte	80
                                        # DW_AT_call_tail_call
	.byte	17                              # DW_AT_call_pc
	.byte	12                              # Abbrev [12] 0x1e0:0x7 DW_TAG_call_site_parameter
	.byte	1                               # DW_AT_location
	.byte	85
	.byte	3                               # DW_AT_call_value
	.byte	163
	.byte	1
	.byte	84
	.byte	0                               # End Of Children Mark
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0x1e9:0x1a DW_TAG_subprogram
	.byte	18                              # DW_AT_low_pc
	.long	.Lfunc_end8-.Lfunc_begin8       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	87
                                        # DW_AT_call_all_calls
	.byte	19                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	49                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x1f8:0xa DW_TAG_formal_parameter
	.byte	1                               # DW_AT_location
	.byte	85
	.byte	22                              # DW_AT_name
	.byte	0                               # DW_AT_decl_file
	.byte	49                              # DW_AT_decl_line
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	14                              # Abbrev [14] 0x203:0x5 DW_TAG_pointer_type
	.long	520                             # DW_AT_type
	.byte	15                              # Abbrev [15] 0x208:0x5 DW_TAG_const_type
	.long	232                             # DW_AT_type
	.byte	14                              # Abbrev [14] 0x20d:0x5 DW_TAG_pointer_type
	.long	530                             # DW_AT_type
	.byte	16                              # Abbrev [16] 0x212:0xb DW_TAG_subroutine_type
	.long	232                             # DW_AT_type
	.byte	9                               # Abbrev [9] 0x217:0x5 DW_TAG_formal_parameter
	.long	232                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	0                               # End Of Children Mark
.Ldebug_info_end0:
	.section	.debug_rnglists,"",@progbits
	.long	.Ldebug_list_header_end1-.Ldebug_list_header_start1 # Length
.Ldebug_list_header_start1:
	.short	5                               # Version
	.byte	8                               # Address size
	.byte	0                               # Segment selector size
	.long	1                               # Offset entry count
.Lrnglists_table_base0:
	.long	.Ldebug_ranges0-.Lrnglists_table_base0
.Ldebug_ranges0:
	.byte	4                               # DW_RLE_offset_pair
	.uleb128 .Lfunc_begin2-.Lfunc_begin0    #   starting offset
	.uleb128 .Ltmp7-.Lfunc_begin0           #   ending offset
	.byte	4                               # DW_RLE_offset_pair
	.uleb128 .Ltmp8-.Lfunc_begin0           #   starting offset
	.uleb128 .Ltmp17-.Lfunc_begin0          #   ending offset
	.byte	0                               # DW_RLE_end_of_list
.Ldebug_list_header_end1:
	.section	.debug_str_offsets,"",@progbits
	.long	120                             # Length of String Offsets Set
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
	.asciz	"helper"                        # string offset=113
.Linfo_string4:
	.asciz	"int"                           # string offset=120
.Linfo_string5:
	.asciz	"f0"                            # string offset=124
.Linfo_string6:
	.asciz	"f4"                            # string offset=127
.Linfo_string7:
	.asciz	"f2"                            # string offset=130
.Linfo_string8:
	.asciz	"f3"                            # string offset=133
.Linfo_string9:
	.asciz	"f1"                            # string offset=136
.Linfo_string10:
	.asciz	"f5"                            # string offset=139
.Linfo_string11:
	.asciz	"simple_add"                    # string offset=142
.Linfo_string12:
	.asciz	"if_else"                       # string offset=153
.Linfo_string13:
	.asciz	"loop_sum"                      # string offset=161
.Linfo_string14:
	.asciz	"sw"                            # string offset=170
.Linfo_string15:
	.asciz	"tail_call"                     # string offset=173
.Linfo_string16:
	.asciz	"sw_call"                       # string offset=183
.Linfo_string17:
	.asciz	"call_twice"                    # string offset=191
.Linfo_string18:
	.asciz	"indirect"                      # string offset=202
.Linfo_string19:
	.asciz	"uses_local"                    # string offset=211
.Linfo_string20:
	.asciz	"a"                             # string offset=222
.Linfo_string21:
	.asciz	"b"                             # string offset=224
.Linfo_string22:
	.asciz	"x"                             # string offset=226
.Linfo_string23:
	.asciz	"n"                             # string offset=228
.Linfo_string24:
	.asciz	"s"                             # string offset=230
.Linfo_string25:
	.asciz	"i"                             # string offset=232
.Linfo_string26:
	.asciz	"k"                             # string offset=234
.Linfo_string27:
	.asciz	"v"                             # string offset=236
.Linfo_string28:
	.asciz	"fp"                            # string offset=238
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
	.long	.Linfo_string24
	.long	.Linfo_string25
	.long	.Linfo_string26
	.long	.Linfo_string27
	.long	.Linfo_string28
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
	.quad	.Lfunc_begin3
	.quad	.Lfunc_begin4
	.quad	.Ltmp20
	.quad	.Lfunc_begin5
	.quad	.Ltmp24
	.quad	.Ltmp26
	.quad	.Ltmp28
	.quad	.Ltmp30
	.quad	.Ltmp32
	.quad	.Ltmp34
	.quad	.Lfunc_begin6
	.quad	.Ltmp38
	.quad	.Ltmp40
	.quad	.Lfunc_begin7
	.quad	.Ltmp43
	.quad	.Lfunc_begin8
.Ldebug_addr_end0:
	.ident	"Apple clang version 21.0.0 (clang-2100.0.123.102)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.section	.debug_line,"",@progbits
.Lline_table_start0:
