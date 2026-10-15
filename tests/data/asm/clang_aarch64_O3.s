	.file	"corpus.cpp"
	.text
	.globl	simple_add                      // -- Begin function simple_add
	.p2align	2
	.type	simple_add,@function
simple_add:                             // @simple_add
.Lfunc_begin0:
	.file	0 "/Users/michaelweiss/git/github/OCTOS/tests/data/asm" "corpus.cpp" md5 0xba4d7a0b733ef1532df489d07fae416a
	.cfi_startproc
// %bb.0:
	//DEBUG_VALUE: simple_add:a <- $w0
	//DEBUG_VALUE: simple_add:b <- $w1
	.loc	0 2 41 prologue_end             // corpus.cpp:2:41
	add	w0, w1, w0
.Ltmp0:
	//DEBUG_VALUE: simple_add:a <- [DW_OP_LLVM_entry_value 1] $w0
	.loc	0 2 32 is_stmt 0                // corpus.cpp:2:32
	ret
.Ltmp1:
.Lfunc_end0:
	.size	simple_add, .Lfunc_end0-simple_add
	.cfi_endproc
                                        // -- End function
	.globl	if_else                         // -- Begin function if_else
	.p2align	2
	.type	if_else,@function
if_else:                                // @if_else
.Lfunc_begin1:
	.cfi_startproc
// %bb.0:
	//DEBUG_VALUE: if_else:x <- $w0
	.loc	0 5 11 prologue_end is_stmt 1   // corpus.cpp:5:11
	lsl	w8, w0, #1
	cmn	w0, #10
	csneg	w9, wzr, w0, ge
	cmp	w0, #10
	csel	w0, w8, w9, gt
.Ltmp2:
	//DEBUG_VALUE: if_else:x <- [DW_OP_LLVM_entry_value 1] $w0
	.loc	0 8 1                           // corpus.cpp:8:1
	ret
.Ltmp3:
.Lfunc_end1:
	.size	if_else, .Lfunc_end1-if_else
	.cfi_endproc
                                        // -- End function
	.globl	loop_sum                        // -- Begin function loop_sum
	.p2align	2
	.type	loop_sum,@function
loop_sum:                               // @loop_sum
.Lfunc_begin2:
	.cfi_startproc
// %bb.0:
	//DEBUG_VALUE: loop_sum:a <- $x0
	//DEBUG_VALUE: loop_sum:n <- $w1
	.loc	0 12 5 prologue_end             // corpus.cpp:12:5
	cmp	w1, #1
.Ltmp4:
	//DEBUG_VALUE: loop_sum:s <- 0
	//DEBUG_VALUE: i <- 0
	b.lt	.LBB2_3
.Ltmp5:
// %bb.1:
	//DEBUG_VALUE: i <- 0
	//DEBUG_VALUE: loop_sum:s <- 0
	//DEBUG_VALUE: loop_sum:n <- $w1
	//DEBUG_VALUE: loop_sum:a <- $x0
	cmp	w1, #8
.Ltmp6:
	.loc	0 12 23 is_stmt 0               // corpus.cpp:12:23
	mov	w9, w1
.Ltmp7:
	.loc	0 12 5                          // corpus.cpp:12:5
	b.hs	.LBB2_4
.Ltmp8:
// %bb.2:
	//DEBUG_VALUE: i <- 0
	//DEBUG_VALUE: loop_sum:s <- 0
	//DEBUG_VALUE: loop_sum:n <- $w1
	//DEBUG_VALUE: loop_sum:a <- $x0
	.loc	0 0 5                           // corpus.cpp:0:5
	mov	x10, xzr
	mov	w8, wzr
	.loc	0 12 5                          // corpus.cpp:12:5
	b	.LBB2_7
.Ltmp9:
.LBB2_3:
	//DEBUG_VALUE: loop_sum:s <- 0
	//DEBUG_VALUE: loop_sum:n <- $w1
	//DEBUG_VALUE: loop_sum:a <- $x0
	.loc	0 0 5                           // corpus.cpp:0:5
	mov	w8, wzr
	.loc	0 13 5 is_stmt 1                // corpus.cpp:13:5
	mov	w0, w8
.Ltmp10:
	//DEBUG_VALUE: loop_sum:a <- [DW_OP_LLVM_entry_value 1] $x0
	ret
.Ltmp11:
.LBB2_4:
	//DEBUG_VALUE: i <- 0
	//DEBUG_VALUE: loop_sum:s <- 0
	//DEBUG_VALUE: loop_sum:n <- $w1
	//DEBUG_VALUE: loop_sum:a <- $x0
	.loc	0 0 5 is_stmt 0                 // corpus.cpp:0:5
	movi	v0.2d, #0000000000000000
	movi	v1.2d, #0000000000000000
.Ltmp12:
	.loc	0 12 5 is_stmt 1                // corpus.cpp:12:5
	and	x10, x9, #0x7ffffff8
	add	x8, x0, #16
	mov	x11, x10
.Ltmp13:
.LBB2_5:                                // =>This Inner Loop Header: Depth=1
	//DEBUG_VALUE: i <- 0
	//DEBUG_VALUE: loop_sum:s <- 0
	//DEBUG_VALUE: loop_sum:n <- $w1
	//DEBUG_VALUE: loop_sum:a <- $x0
	.loc	0 12 38 is_stmt 0               // corpus.cpp:12:38
	ldp	q2, q3, [x8, #-16]
	.loc	0 12 28                         // corpus.cpp:12:28
	subs	x11, x11, #8
	add	x8, x8, #32
	.loc	0 12 35                         // corpus.cpp:12:35
	add	v0.4s, v2.4s, v0.4s
	add	v1.4s, v3.4s, v1.4s
	.loc	0 12 28                         // corpus.cpp:12:28
	b.ne	.LBB2_5
.Ltmp14:
// %bb.6:
	//DEBUG_VALUE: i <- 0
	//DEBUG_VALUE: loop_sum:s <- 0
	//DEBUG_VALUE: loop_sum:n <- $w1
	//DEBUG_VALUE: loop_sum:a <- $x0
	.loc	0 12 5                          // corpus.cpp:12:5
	add	v0.4s, v1.4s, v0.4s
	cmp	x10, x9
	addv	s0, v0.4s
	fmov	w8, s0
	b.eq	.LBB2_9
.Ltmp15:
.LBB2_7:
	//DEBUG_VALUE: i <- 0
	//DEBUG_VALUE: loop_sum:s <- 0
	//DEBUG_VALUE: loop_sum:n <- $w1
	//DEBUG_VALUE: loop_sum:a <- $x0
	add	x11, x0, x10, lsl #2
	sub	x9, x9, x10
.Ltmp16:
.LBB2_8:                                // =>This Inner Loop Header: Depth=1
	//DEBUG_VALUE: loop_sum:n <- $w1
	//DEBUG_VALUE: loop_sum:a <- $x0
	//DEBUG_VALUE: loop_sum:s <- $w8
	//DEBUG_VALUE: i <- [DW_OP_LLVM_arg 0, DW_OP_consts 4, DW_OP_LLVM_arg 0, DW_OP_mul, DW_OP_LLVM_arg 0, DW_OP_plus, DW_OP_minus, DW_OP_consts 4, DW_OP_div, DW_OP_LLVM_arg 0, DW_OP_plus, DW_OP_stack_value] undef
	.loc	0 12 38                         // corpus.cpp:12:38
	ldr	w10, [x11], #4
	.loc	0 12 23                         // corpus.cpp:12:23
	subs	x9, x9, #1
	.loc	0 12 35                         // corpus.cpp:12:35
	add	w8, w10, w8
.Ltmp17:
	//DEBUG_VALUE: loop_sum:s <- $w8
	//DEBUG_VALUE: i <- [DW_OP_LLVM_arg 0, DW_OP_consts 4, DW_OP_LLVM_arg 0, DW_OP_mul, DW_OP_LLVM_arg 0, DW_OP_plus, DW_OP_minus, DW_OP_consts 4, DW_OP_div, DW_OP_consts 1, DW_OP_LLVM_arg 0, DW_OP_plus, DW_OP_plus, DW_OP_stack_value] undef
	.loc	0 12 5                          // corpus.cpp:12:5
	b.ne	.LBB2_8
.Ltmp18:
.LBB2_9:
	//DEBUG_VALUE: loop_sum:n <- $w1
	//DEBUG_VALUE: loop_sum:a <- $x0
	.loc	0 13 5 is_stmt 1                // corpus.cpp:13:5
	mov	w0, w8
.Ltmp19:
	//DEBUG_VALUE: loop_sum:a <- [DW_OP_LLVM_entry_value 1] $x0
	ret
.Ltmp20:
.Lfunc_end2:
	.size	loop_sum, .Lfunc_end2-loop_sum
	.cfi_endproc
                                        // -- End function
	.globl	sw                              // -- Begin function sw
	.p2align	2
	.type	sw,@function
sw:                                     // @sw
.Lfunc_begin3:
	.loc	0 16 0                          // corpus.cpp:16:0
	.cfi_startproc
// %bb.0:
	//DEBUG_VALUE: sw:k <- $w0
	mov	w8, #11                         // =0xb
.Ltmp21:
	.loc	0 17 5 prologue_end             // corpus.cpp:17:5
	cmp	w0, #6
	madd	w8, w0, w8, w8
	csinv	w0, w8, wzr, lo
.Ltmp22:
	//DEBUG_VALUE: sw:k <- [DW_OP_LLVM_entry_value 1] $w0
	.loc	0 26 1                          // corpus.cpp:26:1
	ret
.Ltmp23:
.Lfunc_end3:
	.size	sw, .Lfunc_end3-sw
	.cfi_endproc
                                        // -- End function
	.globl	tail_call                       // -- Begin function tail_call
	.p2align	2
	.type	tail_call,@function
tail_call:                              // @tail_call
.Lfunc_begin4:
	.cfi_startproc
// %bb.0:
	//DEBUG_VALUE: tail_call:x <- $w0
	.loc	0 29 40 prologue_end            // corpus.cpp:29:40
	add	w0, w0, #1
.Ltmp24:
	//DEBUG_VALUE: tail_call:x <- [DW_OP_LLVM_entry_value 1] $w0
	.loc	0 29 31 is_stmt 0               // corpus.cpp:29:31
	b	helper
.Ltmp25:
.Lfunc_end4:
	.size	tail_call, .Lfunc_end4-tail_call
	.cfi_endproc
                                        // -- End function
	.globl	sw_call                         // -- Begin function sw_call
	.p2align	2
	.type	sw_call,@function
sw_call:                                // @sw_call
.Lfunc_begin5:
	.cfi_startproc
// %bb.0:
	//DEBUG_VALUE: sw_call:k <- $w0
	//DEBUG_VALUE: sw_call:v <- $w1
	.loc	0 33 5 prologue_end is_stmt 1   // corpus.cpp:33:5
	cmp	w0, #2
	b.gt	.LBB5_5
.Ltmp26:
// %bb.1:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	cbz	w0, .LBB5_9
.Ltmp27:
// %bb.2:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	cmp	w0, #1
	b.eq	.LBB5_12
.Ltmp28:
// %bb.3:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	cmp	w0, #2
	b.ne	.LBB5_11
.Ltmp29:
// %bb.4:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	.loc	0 36 24                         // corpus.cpp:36:24
	mov	w0, w1
.Ltmp30:
	//DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $w0
	b	f2
.Ltmp31:
.LBB5_5:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	.loc	0 33 5                          // corpus.cpp:33:5
	cmp	w0, #3
	b.eq	.LBB5_10
.Ltmp32:
// %bb.6:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	cmp	w0, #4
	b.eq	.LBB5_13
.Ltmp33:
// %bb.7:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	cmp	w0, #5
	b.ne	.LBB5_11
.Ltmp34:
// %bb.8:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	.loc	0 39 24                         // corpus.cpp:39:24
	mov	w0, w1
.Ltmp35:
	//DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $w0
	b	f5
.Ltmp36:
.LBB5_9:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	.loc	0 34 24                         // corpus.cpp:34:24
	mov	w0, w1
.Ltmp37:
	//DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $w0
	b	f0
.Ltmp38:
.LBB5_10:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	.loc	0 37 24                         // corpus.cpp:37:24
	mov	w0, w1
.Ltmp39:
	//DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $w0
	b	f3
.Ltmp40:
.LBB5_11:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	.loc	0 42 1                          // corpus.cpp:42:1
	mov	w0, wzr
.Ltmp41:
	//DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $w0
	ret
.Ltmp42:
.LBB5_12:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	.loc	0 35 24                         // corpus.cpp:35:24
	mov	w0, w1
.Ltmp43:
	//DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $w0
	b	f1
.Ltmp44:
.LBB5_13:
	//DEBUG_VALUE: sw_call:v <- $w1
	//DEBUG_VALUE: sw_call:k <- $w0
	.loc	0 38 24                         // corpus.cpp:38:24
	mov	w0, w1
.Ltmp45:
	//DEBUG_VALUE: sw_call:k <- [DW_OP_LLVM_entry_value 1] $w0
	b	f4
.Ltmp46:
.Lfunc_end5:
	.size	sw_call, .Lfunc_end5-sw_call
	.cfi_endproc
                                        // -- End function
	.globl	call_twice                      // -- Begin function call_twice
	.p2align	2
	.type	call_twice,@function
call_twice:                             // @call_twice
.Lfunc_begin6:
	.loc	0 44 0                          // corpus.cpp:44:0
	.cfi_startproc
// %bb.0:
	//DEBUG_VALUE: call_twice:x <- $w0
	stp	x29, x30, [sp, #-32]!           // 16-byte Folded Spill
	stp	x20, x19, [sp, #16]             // 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 32
	.cfi_offset w19, -8
	.cfi_offset w20, -16
	.cfi_offset w30, -24
	.cfi_offset w29, -32
	mov	w19, w0
.Ltmp47:
	//DEBUG_VALUE: call_twice:x <- $w19
	.loc	0 44 32 prologue_end            // corpus.cpp:44:32
	bl	helper
.Ltmp48:
	mov	w20, w0
	.loc	0 44 53 is_stmt 0               // corpus.cpp:44:53
	add	w0, w19, #1
	.loc	0 44 44                         // corpus.cpp:44:44
	bl	helper
.Ltmp49:
	.loc	0 44 42                         // corpus.cpp:44:42
	add	w0, w0, w20
	.loc	0 44 25 epilogue_begin          // corpus.cpp:44:25
	ldp	x20, x19, [sp, #16]             // 16-byte Folded Reload
.Ltmp50:
	//DEBUG_VALUE: call_twice:x <- [DW_OP_LLVM_entry_value 1] $w0
	ldp	x29, x30, [sp], #32             // 16-byte Folded Reload
	ret
.Ltmp51:
.Lfunc_end6:
	.size	call_twice, .Lfunc_end6-call_twice
	.cfi_endproc
                                        // -- End function
	.globl	indirect                        // -- Begin function indirect
	.p2align	2
	.type	indirect,@function
indirect:                               // @indirect
.Lfunc_begin7:
	.loc	0 46 0 is_stmt 1                // corpus.cpp:46:0
	.cfi_startproc
// %bb.0:
	//DEBUG_VALUE: indirect:fp <- $x0
	//DEBUG_VALUE: indirect:v <- $w1
	mov	x2, x0
.Ltmp52:
	//DEBUG_VALUE: indirect:fp <- $x2
	.loc	0 46 46 prologue_end            // corpus.cpp:46:46
	mov	w0, w1
.Ltmp53:
	//DEBUG_VALUE: indirect:v <- $w0
	br	x2
.Ltmp54:
.Lfunc_end7:
	.size	indirect, .Lfunc_end7-indirect
	.cfi_endproc
                                        // -- End function
	.globl	uses_local                      // -- Begin function uses_local
	.p2align	2
	.type	uses_local,@function
uses_local:                             // @uses_local
.Lfunc_begin8:
	.loc	0 49 0                          // corpus.cpp:49:0
	.cfi_startproc
// %bb.0:
	//DEBUG_VALUE: uses_local:x <- $w0
	mov	w8, #6                          // =0x6
.Ltmp55:
	.loc	0 49 44 prologue_end            // corpus.cpp:49:44
	mov	w9, #3                          // =0x3
	madd	w0, w0, w8, w9
.Ltmp56:
	//DEBUG_VALUE: uses_local:x <- [DW_OP_LLVM_entry_value 1] $w0
	.loc	0 49 25 is_stmt 0               // corpus.cpp:49:25
	ret
.Ltmp57:
.Lfunc_end8:
	.size	uses_local, .Lfunc_end8-uses_local
	.cfi_endproc
                                        // -- End function
	.section	.debug_loclists,"",@progbits
	.word	.Ldebug_list_header_end0-.Ldebug_list_header_start0 // Length
.Ldebug_list_header_start0:
	.hword	5                               // Version
	.byte	8                               // Address size
	.byte	0                               // Segment selector size
	.word	12                              // Offset entry count
.Lloclists_table_base0:
	.word	.Ldebug_loc0-.Lloclists_table_base0
	.word	.Ldebug_loc1-.Lloclists_table_base0
	.word	.Ldebug_loc2-.Lloclists_table_base0
	.word	.Ldebug_loc3-.Lloclists_table_base0
	.word	.Ldebug_loc4-.Lloclists_table_base0
	.word	.Ldebug_loc5-.Lloclists_table_base0
	.word	.Ldebug_loc6-.Lloclists_table_base0
	.word	.Ldebug_loc7-.Lloclists_table_base0
	.word	.Ldebug_loc8-.Lloclists_table_base0
	.word	.Ldebug_loc9-.Lloclists_table_base0
	.word	.Ldebug_loc10-.Lloclists_table_base0
	.word	.Ldebug_loc11-.Lloclists_table_base0
.Ldebug_loc0:
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Lfunc_begin0-.Lfunc_begin0    //   starting offset
	.uleb128 .Ltmp0-.Lfunc_begin0           //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp0-.Lfunc_begin0           //   starting offset
	.uleb128 .Lfunc_end0-.Lfunc_begin0      //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	0                               // DW_LLE_end_of_list
.Ldebug_loc1:
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Lfunc_begin1-.Lfunc_begin0    //   starting offset
	.uleb128 .Ltmp2-.Lfunc_begin0           //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp2-.Lfunc_begin0           //   starting offset
	.uleb128 .Lfunc_end1-.Lfunc_begin0      //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	0                               // DW_LLE_end_of_list
.Ldebug_loc2:
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Lfunc_begin2-.Lfunc_begin0    //   starting offset
	.uleb128 .Ltmp10-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp10-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp11-.Lfunc_begin0          //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp11-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp19-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp19-.Lfunc_begin0          //   starting offset
	.uleb128 .Lfunc_end2-.Lfunc_begin0      //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	0                               // DW_LLE_end_of_list
.Ldebug_loc3:
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp4-.Lfunc_begin0           //   starting offset
	.uleb128 .Ltmp16-.Lfunc_begin0          //   ending offset
	.byte	3                               // Loc expr size
	.byte	17                              // DW_OP_consts
	.byte	0                               // 0
	.byte	159                             // DW_OP_stack_value
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp16-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp18-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	88                              // DW_OP_reg8
	.byte	0                               // DW_LLE_end_of_list
.Ldebug_loc4:
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp4-.Lfunc_begin0           //   starting offset
	.uleb128 .Ltmp9-.Lfunc_begin0           //   ending offset
	.byte	3                               // Loc expr size
	.byte	17                              // DW_OP_consts
	.byte	0                               // 0
	.byte	159                             // DW_OP_stack_value
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp11-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp16-.Lfunc_begin0          //   ending offset
	.byte	3                               // Loc expr size
	.byte	17                              // DW_OP_consts
	.byte	0                               // 0
	.byte	159                             // DW_OP_stack_value
	.byte	0                               // DW_LLE_end_of_list
.Ldebug_loc5:
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Lfunc_begin3-.Lfunc_begin0    //   starting offset
	.uleb128 .Ltmp22-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp22-.Lfunc_begin0          //   starting offset
	.uleb128 .Lfunc_end3-.Lfunc_begin0      //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	0                               // DW_LLE_end_of_list
.Ldebug_loc6:
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Lfunc_begin4-.Lfunc_begin0    //   starting offset
	.uleb128 .Ltmp24-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp24-.Lfunc_begin0          //   starting offset
	.uleb128 .Lfunc_end4-.Lfunc_begin0      //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	0                               // DW_LLE_end_of_list
.Ldebug_loc7:
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Lfunc_begin5-.Lfunc_begin0    //   starting offset
	.uleb128 .Ltmp30-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp30-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp31-.Lfunc_begin0          //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp31-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp35-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp35-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp36-.Lfunc_begin0          //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp36-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp37-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp37-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp38-.Lfunc_begin0          //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp38-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp39-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp39-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp40-.Lfunc_begin0          //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp40-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp41-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp41-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp42-.Lfunc_begin0          //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp42-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp43-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp43-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp44-.Lfunc_begin0          //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp44-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp45-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp45-.Lfunc_begin0          //   starting offset
	.uleb128 .Lfunc_end5-.Lfunc_begin0      //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	0                               // DW_LLE_end_of_list
.Ldebug_loc8:
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Lfunc_begin6-.Lfunc_begin0    //   starting offset
	.uleb128 .Ltmp47-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp47-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp50-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	99                              // DW_OP_reg19
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp50-.Lfunc_begin0          //   starting offset
	.uleb128 .Lfunc_end6-.Lfunc_begin0      //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	0                               // DW_LLE_end_of_list
.Ldebug_loc9:
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Lfunc_begin7-.Lfunc_begin0    //   starting offset
	.uleb128 .Ltmp52-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp52-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp54-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	82                              // DW_OP_reg2
	.byte	0                               // DW_LLE_end_of_list
.Ldebug_loc10:
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Lfunc_begin7-.Lfunc_begin0    //   starting offset
	.uleb128 .Ltmp53-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	81                              // DW_OP_reg1
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp53-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp54-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	0                               // DW_LLE_end_of_list
.Ldebug_loc11:
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Lfunc_begin8-.Lfunc_begin0    //   starting offset
	.uleb128 .Ltmp56-.Lfunc_begin0          //   ending offset
	.byte	1                               // Loc expr size
	.byte	80                              // DW_OP_reg0
	.byte	4                               // DW_LLE_offset_pair
	.uleb128 .Ltmp56-.Lfunc_begin0          //   starting offset
	.uleb128 .Lfunc_end8-.Lfunc_begin0      //   ending offset
	.byte	4                               // Loc expr size
	.byte	163                             // DW_OP_entry_value
	.byte	1                               // 1
	.byte	80                              // DW_OP_reg0
	.byte	159                             // DW_OP_stack_value
	.byte	0                               // DW_LLE_end_of_list
.Ldebug_list_header_end0:
	.section	.debug_abbrev,"",@progbits
	.byte	1                               // Abbreviation Code
	.byte	17                              // DW_TAG_compile_unit
	.byte	1                               // DW_CHILDREN_yes
	.byte	37                              // DW_AT_producer
	.byte	37                              // DW_FORM_strx1
	.byte	19                              // DW_AT_language
	.byte	5                               // DW_FORM_data2
	.byte	3                               // DW_AT_name
	.byte	37                              // DW_FORM_strx1
	.byte	114                             // DW_AT_str_offsets_base
	.byte	23                              // DW_FORM_sec_offset
	.byte	16                              // DW_AT_stmt_list
	.byte	23                              // DW_FORM_sec_offset
	.byte	27                              // DW_AT_comp_dir
	.byte	37                              // DW_FORM_strx1
	.byte	17                              // DW_AT_low_pc
	.byte	27                              // DW_FORM_addrx
	.byte	18                              // DW_AT_high_pc
	.byte	6                               // DW_FORM_data4
	.byte	115                             // DW_AT_addr_base
	.byte	23                              // DW_FORM_sec_offset
	.byte	116                             // DW_AT_rnglists_base
	.byte	23                              // DW_FORM_sec_offset
	.ascii	"\214\001"                      // DW_AT_loclists_base
	.byte	23                              // DW_FORM_sec_offset
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	2                               // Abbreviation Code
	.byte	46                              // DW_TAG_subprogram
	.byte	1                               // DW_CHILDREN_yes
	.byte	17                              // DW_AT_low_pc
	.byte	27                              // DW_FORM_addrx
	.byte	18                              // DW_AT_high_pc
	.byte	6                               // DW_FORM_data4
	.byte	64                              // DW_AT_frame_base
	.byte	24                              // DW_FORM_exprloc
	.byte	122                             // DW_AT_call_all_calls
	.byte	25                              // DW_FORM_flag_present
	.byte	3                               // DW_AT_name
	.byte	37                              // DW_FORM_strx1
	.byte	58                              // DW_AT_decl_file
	.byte	11                              // DW_FORM_data1
	.byte	59                              // DW_AT_decl_line
	.byte	11                              // DW_FORM_data1
	.byte	73                              // DW_AT_type
	.byte	19                              // DW_FORM_ref4
	.byte	63                              // DW_AT_external
	.byte	25                              // DW_FORM_flag_present
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	3                               // Abbreviation Code
	.byte	5                               // DW_TAG_formal_parameter
	.byte	0                               // DW_CHILDREN_no
	.byte	2                               // DW_AT_location
	.byte	34                              // DW_FORM_loclistx
	.byte	3                               // DW_AT_name
	.byte	37                              // DW_FORM_strx1
	.byte	58                              // DW_AT_decl_file
	.byte	11                              // DW_FORM_data1
	.byte	59                              // DW_AT_decl_line
	.byte	11                              // DW_FORM_data1
	.byte	73                              // DW_AT_type
	.byte	19                              // DW_FORM_ref4
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	4                               // Abbreviation Code
	.byte	5                               // DW_TAG_formal_parameter
	.byte	0                               // DW_CHILDREN_no
	.byte	2                               // DW_AT_location
	.byte	24                              // DW_FORM_exprloc
	.byte	3                               // DW_AT_name
	.byte	37                              // DW_FORM_strx1
	.byte	58                              // DW_AT_decl_file
	.byte	11                              // DW_FORM_data1
	.byte	59                              // DW_AT_decl_line
	.byte	11                              // DW_FORM_data1
	.byte	73                              // DW_AT_type
	.byte	19                              // DW_FORM_ref4
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	5                               // Abbreviation Code
	.byte	52                              // DW_TAG_variable
	.byte	0                               // DW_CHILDREN_no
	.byte	2                               // DW_AT_location
	.byte	34                              // DW_FORM_loclistx
	.byte	3                               // DW_AT_name
	.byte	37                              // DW_FORM_strx1
	.byte	58                              // DW_AT_decl_file
	.byte	11                              // DW_FORM_data1
	.byte	59                              // DW_AT_decl_line
	.byte	11                              // DW_FORM_data1
	.byte	73                              // DW_AT_type
	.byte	19                              // DW_FORM_ref4
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	6                               // Abbreviation Code
	.byte	11                              // DW_TAG_lexical_block
	.byte	1                               // DW_CHILDREN_yes
	.byte	85                              // DW_AT_ranges
	.byte	35                              // DW_FORM_rnglistx
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	7                               // Abbreviation Code
	.byte	72                              // DW_TAG_call_site
	.byte	0                               // DW_CHILDREN_no
	.byte	127                             // DW_AT_call_origin
	.byte	19                              // DW_FORM_ref4
	.ascii	"\202\001"                      // DW_AT_call_tail_call
	.byte	25                              // DW_FORM_flag_present
	.ascii	"\201\001"                      // DW_AT_call_pc
	.byte	27                              // DW_FORM_addrx
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	8                               // Abbreviation Code
	.byte	46                              // DW_TAG_subprogram
	.byte	1                               // DW_CHILDREN_yes
	.byte	3                               // DW_AT_name
	.byte	37                              // DW_FORM_strx1
	.byte	58                              // DW_AT_decl_file
	.byte	11                              // DW_FORM_data1
	.byte	59                              // DW_AT_decl_line
	.byte	11                              // DW_FORM_data1
	.byte	73                              // DW_AT_type
	.byte	19                              // DW_FORM_ref4
	.byte	60                              // DW_AT_declaration
	.byte	25                              // DW_FORM_flag_present
	.byte	63                              // DW_AT_external
	.byte	25                              // DW_FORM_flag_present
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	9                               // Abbreviation Code
	.byte	5                               // DW_TAG_formal_parameter
	.byte	0                               // DW_CHILDREN_no
	.byte	73                              // DW_AT_type
	.byte	19                              // DW_FORM_ref4
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	10                              // Abbreviation Code
	.byte	36                              // DW_TAG_base_type
	.byte	0                               // DW_CHILDREN_no
	.byte	3                               // DW_AT_name
	.byte	37                              // DW_FORM_strx1
	.byte	62                              // DW_AT_encoding
	.byte	11                              // DW_FORM_data1
	.byte	11                              // DW_AT_byte_size
	.byte	11                              // DW_FORM_data1
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	11                              // Abbreviation Code
	.byte	72                              // DW_TAG_call_site
	.byte	1                               // DW_CHILDREN_yes
	.byte	127                             // DW_AT_call_origin
	.byte	19                              // DW_FORM_ref4
	.byte	125                             // DW_AT_call_return_pc
	.byte	27                              // DW_FORM_addrx
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	12                              // Abbreviation Code
	.byte	73                              // DW_TAG_call_site_parameter
	.byte	0                               // DW_CHILDREN_no
	.byte	2                               // DW_AT_location
	.byte	24                              // DW_FORM_exprloc
	.byte	126                             // DW_AT_call_value
	.byte	24                              // DW_FORM_exprloc
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	13                              // Abbreviation Code
	.byte	72                              // DW_TAG_call_site
	.byte	1                               // DW_CHILDREN_yes
	.ascii	"\203\001"                      // DW_AT_call_target
	.byte	24                              // DW_FORM_exprloc
	.ascii	"\202\001"                      // DW_AT_call_tail_call
	.byte	25                              // DW_FORM_flag_present
	.ascii	"\201\001"                      // DW_AT_call_pc
	.byte	27                              // DW_FORM_addrx
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	14                              // Abbreviation Code
	.byte	15                              // DW_TAG_pointer_type
	.byte	0                               // DW_CHILDREN_no
	.byte	73                              // DW_AT_type
	.byte	19                              // DW_FORM_ref4
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	15                              // Abbreviation Code
	.byte	38                              // DW_TAG_const_type
	.byte	0                               // DW_CHILDREN_no
	.byte	73                              // DW_AT_type
	.byte	19                              // DW_FORM_ref4
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	16                              // Abbreviation Code
	.byte	21                              // DW_TAG_subroutine_type
	.byte	1                               // DW_CHILDREN_yes
	.byte	73                              // DW_AT_type
	.byte	19                              // DW_FORM_ref4
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	0                               // EOM(3)
	.section	.debug_info,"",@progbits
.Lcu_begin0:
	.word	.Ldebug_info_end0-.Ldebug_info_start0 // Length of Unit
.Ldebug_info_start0:
	.hword	5                               // DWARF version number
	.byte	1                               // DWARF Unit Type
	.byte	8                               // Address Size (in bytes)
	.word	.debug_abbrev                   // Offset Into Abbrev. Section
	.byte	1                               // Abbrev [1] 0xc:0x20e DW_TAG_compile_unit
	.byte	0                               // DW_AT_producer
	.hword	33                              // DW_AT_language
	.byte	1                               // DW_AT_name
	.word	.Lstr_offsets_base0             // DW_AT_str_offsets_base
	.word	.Lline_table_start0             // DW_AT_stmt_list
	.byte	2                               // DW_AT_comp_dir
	.byte	0                               // DW_AT_low_pc
	.word	.Lfunc_end8-.Lfunc_begin0       // DW_AT_high_pc
	.word	.Laddr_table_base0              // DW_AT_addr_base
	.word	.Lrnglists_table_base0          // DW_AT_rnglists_base
	.word	.Lloclists_table_base0          // DW_AT_loclists_base
	.byte	2                               // Abbrev [2] 0x2b:0x23 DW_TAG_subprogram
	.byte	0                               // DW_AT_low_pc
	.word	.Lfunc_end0-.Lfunc_begin0       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
                                        // DW_AT_call_all_calls
	.byte	11                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	2                               // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0x3a:0x9 DW_TAG_formal_parameter
	.byte	0                               // DW_AT_location
	.byte	20                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	2                               // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	4                               // Abbrev [4] 0x43:0xa DW_TAG_formal_parameter
	.byte	1                               // DW_AT_location
	.byte	81
	.byte	21                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	2                               // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0x4e:0x19 DW_TAG_subprogram
	.byte	1                               // DW_AT_low_pc
	.word	.Lfunc_end1-.Lfunc_begin1       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
                                        // DW_AT_call_all_calls
	.byte	12                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	4                               // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0x5d:0x9 DW_TAG_formal_parameter
	.byte	1                               // DW_AT_location
	.byte	22                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	4                               // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0x67:0x38 DW_TAG_subprogram
	.byte	2                               // DW_AT_low_pc
	.word	.Lfunc_end2-.Lfunc_begin2       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
                                        // DW_AT_call_all_calls
	.byte	13                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	10                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0x76:0x9 DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	20                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	10                              // DW_AT_decl_line
	.word	511                             // DW_AT_type
	.byte	4                               // Abbrev [4] 0x7f:0xa DW_TAG_formal_parameter
	.byte	1                               // DW_AT_location
	.byte	81
	.byte	23                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	10                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	5                               // Abbrev [5] 0x89:0x9 DW_TAG_variable
	.byte	3                               // DW_AT_location
	.byte	24                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	11                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	6                               // Abbrev [6] 0x92:0xc DW_TAG_lexical_block
	.byte	0                               // DW_AT_ranges
	.byte	5                               // Abbrev [5] 0x94:0x9 DW_TAG_variable
	.byte	4                               // DW_AT_location
	.byte	25                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	12                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0x9f:0x19 DW_TAG_subprogram
	.byte	3                               // DW_AT_low_pc
	.word	.Lfunc_end3-.Lfunc_begin3       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
                                        // DW_AT_call_all_calls
	.byte	14                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	16                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0xae:0x9 DW_TAG_formal_parameter
	.byte	5                               // DW_AT_location
	.byte	26                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	16                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0xb8:0x1f DW_TAG_subprogram
	.byte	4                               // DW_AT_low_pc
	.word	.Lfunc_end4-.Lfunc_begin4       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
                                        // DW_AT_call_all_calls
	.byte	15                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	29                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0xc7:0x9 DW_TAG_formal_parameter
	.byte	6                               // DW_AT_location
	.byte	22                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	29                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	7                               // Abbrev [7] 0xd0:0x6 DW_TAG_call_site
	.word	215                             // DW_AT_call_origin
                                        // DW_AT_call_tail_call
	.byte	5                               // DW_AT_call_pc
	.byte	0                               // End Of Children Mark
	.byte	8                               // Abbrev [8] 0xd7:0xe DW_TAG_subprogram
	.byte	3                               // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	28                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_declaration
                                        // DW_AT_external
	.byte	9                               // Abbrev [9] 0xdf:0x5 DW_TAG_formal_parameter
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	10                              // Abbrev [10] 0xe5:0x4 DW_TAG_base_type
	.byte	4                               // DW_AT_name
	.byte	5                               // DW_AT_encoding
	.byte	4                               // DW_AT_byte_size
	.byte	2                               // Abbrev [2] 0xe9:0x47 DW_TAG_subprogram
	.byte	6                               // DW_AT_low_pc
	.word	.Lfunc_end5-.Lfunc_begin5       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
                                        // DW_AT_call_all_calls
	.byte	16                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	32                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0xf8:0x9 DW_TAG_formal_parameter
	.byte	7                               // DW_AT_location
	.byte	26                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	32                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	4                               // Abbrev [4] 0x101:0xa DW_TAG_formal_parameter
	.byte	1                               // DW_AT_location
	.byte	81
	.byte	27                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	32                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	7                               // Abbrev [7] 0x10b:0x6 DW_TAG_call_site
	.word	304                             // DW_AT_call_origin
                                        // DW_AT_call_tail_call
	.byte	7                               // DW_AT_call_pc
	.byte	7                               // Abbrev [7] 0x111:0x6 DW_TAG_call_site
	.word	318                             // DW_AT_call_origin
                                        // DW_AT_call_tail_call
	.byte	8                               // DW_AT_call_pc
	.byte	7                               // Abbrev [7] 0x117:0x6 DW_TAG_call_site
	.word	332                             // DW_AT_call_origin
                                        // DW_AT_call_tail_call
	.byte	9                               // DW_AT_call_pc
	.byte	7                               // Abbrev [7] 0x11d:0x6 DW_TAG_call_site
	.word	346                             // DW_AT_call_origin
                                        // DW_AT_call_tail_call
	.byte	10                              // DW_AT_call_pc
	.byte	7                               // Abbrev [7] 0x123:0x6 DW_TAG_call_site
	.word	360                             // DW_AT_call_origin
                                        // DW_AT_call_tail_call
	.byte	11                              // DW_AT_call_pc
	.byte	7                               // Abbrev [7] 0x129:0x6 DW_TAG_call_site
	.word	374                             // DW_AT_call_origin
                                        // DW_AT_call_tail_call
	.byte	12                              // DW_AT_call_pc
	.byte	0                               // End Of Children Mark
	.byte	8                               // Abbrev [8] 0x130:0xe DW_TAG_subprogram
	.byte	5                               // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	31                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_declaration
                                        // DW_AT_external
	.byte	9                               // Abbrev [9] 0x138:0x5 DW_TAG_formal_parameter
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	8                               // Abbrev [8] 0x13e:0xe DW_TAG_subprogram
	.byte	6                               // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	31                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_declaration
                                        // DW_AT_external
	.byte	9                               // Abbrev [9] 0x146:0x5 DW_TAG_formal_parameter
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	8                               // Abbrev [8] 0x14c:0xe DW_TAG_subprogram
	.byte	7                               // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	31                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_declaration
                                        // DW_AT_external
	.byte	9                               // Abbrev [9] 0x154:0x5 DW_TAG_formal_parameter
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	8                               // Abbrev [8] 0x15a:0xe DW_TAG_subprogram
	.byte	8                               // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	31                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_declaration
                                        // DW_AT_external
	.byte	9                               // Abbrev [9] 0x162:0x5 DW_TAG_formal_parameter
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	8                               // Abbrev [8] 0x168:0xe DW_TAG_subprogram
	.byte	9                               // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	31                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_declaration
                                        // DW_AT_external
	.byte	9                               // Abbrev [9] 0x170:0x5 DW_TAG_formal_parameter
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	8                               // Abbrev [8] 0x176:0xe DW_TAG_subprogram
	.byte	10                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	31                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_declaration
                                        // DW_AT_external
	.byte	9                               // Abbrev [9] 0x17e:0x5 DW_TAG_formal_parameter
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0x184:0x34 DW_TAG_subprogram
	.byte	13                              // DW_AT_low_pc
	.word	.Lfunc_end6-.Lfunc_begin6       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	109
                                        // DW_AT_call_all_calls
	.byte	17                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	44                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0x193:0x9 DW_TAG_formal_parameter
	.byte	8                               // DW_AT_location
	.byte	22                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	44                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	11                              // Abbrev [11] 0x19c:0xe DW_TAG_call_site
	.word	215                             // DW_AT_call_origin
	.byte	14                              // DW_AT_call_return_pc
	.byte	12                              // Abbrev [12] 0x1a2:0x7 DW_TAG_call_site_parameter
	.byte	1                               // DW_AT_location
	.byte	80
	.byte	3                               // DW_AT_call_value
	.byte	163
	.byte	1
	.byte	80
	.byte	0                               // End Of Children Mark
	.byte	11                              // Abbrev [11] 0x1aa:0xd DW_TAG_call_site
	.word	215                             // DW_AT_call_origin
	.byte	15                              // DW_AT_call_return_pc
	.byte	12                              // Abbrev [12] 0x1b0:0x6 DW_TAG_call_site_parameter
	.byte	1                               // DW_AT_location
	.byte	80
	.byte	2                               // DW_AT_call_value
	.byte	131
	.byte	1
	.byte	0                               // End Of Children Mark
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0x1b8:0x2e DW_TAG_subprogram
	.byte	16                              // DW_AT_low_pc
	.word	.Lfunc_end7-.Lfunc_begin7       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
                                        // DW_AT_call_all_calls
	.byte	18                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	46                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0x1c7:0x9 DW_TAG_formal_parameter
	.byte	9                               // DW_AT_location
	.byte	28                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	46                              // DW_AT_decl_line
	.word	521                             // DW_AT_type
	.byte	3                               // Abbrev [3] 0x1d0:0x9 DW_TAG_formal_parameter
	.byte	10                              // DW_AT_location
	.byte	27                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	46                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	13                              // Abbrev [13] 0x1d9:0xc DW_TAG_call_site
	.byte	1                               // DW_AT_call_target
	.byte	82
                                        // DW_AT_call_tail_call
	.byte	17                              // DW_AT_call_pc
	.byte	12                              // Abbrev [12] 0x1dd:0x7 DW_TAG_call_site_parameter
	.byte	1                               // DW_AT_location
	.byte	80
	.byte	3                               // DW_AT_call_value
	.byte	163
	.byte	1
	.byte	81
	.byte	0                               // End Of Children Mark
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0x1e6:0x19 DW_TAG_subprogram
	.byte	18                              // DW_AT_low_pc
	.word	.Lfunc_end8-.Lfunc_begin8       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
                                        // DW_AT_call_all_calls
	.byte	19                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	49                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0x1f5:0x9 DW_TAG_formal_parameter
	.byte	11                              // DW_AT_location
	.byte	22                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	49                              // DW_AT_decl_line
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	14                              // Abbrev [14] 0x1ff:0x5 DW_TAG_pointer_type
	.word	516                             // DW_AT_type
	.byte	15                              // Abbrev [15] 0x204:0x5 DW_TAG_const_type
	.word	229                             // DW_AT_type
	.byte	14                              // Abbrev [14] 0x209:0x5 DW_TAG_pointer_type
	.word	526                             // DW_AT_type
	.byte	16                              // Abbrev [16] 0x20e:0xb DW_TAG_subroutine_type
	.word	229                             // DW_AT_type
	.byte	9                               // Abbrev [9] 0x213:0x5 DW_TAG_formal_parameter
	.word	229                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	0                               // End Of Children Mark
.Ldebug_info_end0:
	.section	.debug_rnglists,"",@progbits
	.word	.Ldebug_list_header_end1-.Ldebug_list_header_start1 // Length
.Ldebug_list_header_start1:
	.hword	5                               // Version
	.byte	8                               // Address size
	.byte	0                               // Segment selector size
	.word	1                               // Offset entry count
.Lrnglists_table_base0:
	.word	.Ldebug_ranges0-.Lrnglists_table_base0
.Ldebug_ranges0:
	.byte	4                               // DW_RLE_offset_pair
	.uleb128 .Lfunc_begin2-.Lfunc_begin0    //   starting offset
	.uleb128 .Ltmp9-.Lfunc_begin0           //   ending offset
	.byte	4                               // DW_RLE_offset_pair
	.uleb128 .Ltmp12-.Lfunc_begin0          //   starting offset
	.uleb128 .Ltmp18-.Lfunc_begin0          //   ending offset
	.byte	0                               // DW_RLE_end_of_list
.Ldebug_list_header_end1:
	.section	.debug_str_offsets,"",@progbits
	.word	120                             // Length of String Offsets Set
	.hword	5
	.hword	0
.Lstr_offsets_base0:
	.section	.debug_str,"MS",@progbits,1
.Linfo_string0:
	.asciz	"Apple clang version 21.0.0 (clang-2100.0.123.102)" // string offset=0
.Linfo_string1:
	.asciz	"corpus.cpp"                    // string offset=50
.Linfo_string2:
	.asciz	"/Users/michaelweiss/git/github/OCTOS/tests/data/asm" // string offset=61
.Linfo_string3:
	.asciz	"helper"                        // string offset=113
.Linfo_string4:
	.asciz	"int"                           // string offset=120
.Linfo_string5:
	.asciz	"f2"                            // string offset=124
.Linfo_string6:
	.asciz	"f5"                            // string offset=127
.Linfo_string7:
	.asciz	"f0"                            // string offset=130
.Linfo_string8:
	.asciz	"f3"                            // string offset=133
.Linfo_string9:
	.asciz	"f1"                            // string offset=136
.Linfo_string10:
	.asciz	"f4"                            // string offset=139
.Linfo_string11:
	.asciz	"simple_add"                    // string offset=142
.Linfo_string12:
	.asciz	"if_else"                       // string offset=153
.Linfo_string13:
	.asciz	"loop_sum"                      // string offset=161
.Linfo_string14:
	.asciz	"sw"                            // string offset=170
.Linfo_string15:
	.asciz	"tail_call"                     // string offset=173
.Linfo_string16:
	.asciz	"sw_call"                       // string offset=183
.Linfo_string17:
	.asciz	"call_twice"                    // string offset=191
.Linfo_string18:
	.asciz	"indirect"                      // string offset=202
.Linfo_string19:
	.asciz	"uses_local"                    // string offset=211
.Linfo_string20:
	.asciz	"a"                             // string offset=222
.Linfo_string21:
	.asciz	"b"                             // string offset=224
.Linfo_string22:
	.asciz	"x"                             // string offset=226
.Linfo_string23:
	.asciz	"n"                             // string offset=228
.Linfo_string24:
	.asciz	"s"                             // string offset=230
.Linfo_string25:
	.asciz	"i"                             // string offset=232
.Linfo_string26:
	.asciz	"k"                             // string offset=234
.Linfo_string27:
	.asciz	"v"                             // string offset=236
.Linfo_string28:
	.asciz	"fp"                            // string offset=238
	.section	.debug_str_offsets,"",@progbits
	.word	.Linfo_string0
	.word	.Linfo_string1
	.word	.Linfo_string2
	.word	.Linfo_string3
	.word	.Linfo_string4
	.word	.Linfo_string5
	.word	.Linfo_string6
	.word	.Linfo_string7
	.word	.Linfo_string8
	.word	.Linfo_string9
	.word	.Linfo_string10
	.word	.Linfo_string11
	.word	.Linfo_string12
	.word	.Linfo_string13
	.word	.Linfo_string14
	.word	.Linfo_string15
	.word	.Linfo_string16
	.word	.Linfo_string17
	.word	.Linfo_string18
	.word	.Linfo_string19
	.word	.Linfo_string20
	.word	.Linfo_string21
	.word	.Linfo_string22
	.word	.Linfo_string23
	.word	.Linfo_string24
	.word	.Linfo_string25
	.word	.Linfo_string26
	.word	.Linfo_string27
	.word	.Linfo_string28
	.section	.debug_addr,"",@progbits
	.word	.Ldebug_addr_end0-.Ldebug_addr_start0 // Length of contribution
.Ldebug_addr_start0:
	.hword	5                               // DWARF version number
	.byte	8                               // Address size
	.byte	0                               // Segment selector size
.Laddr_table_base0:
	.xword	.Lfunc_begin0
	.xword	.Lfunc_begin1
	.xword	.Lfunc_begin2
	.xword	.Lfunc_begin3
	.xword	.Lfunc_begin4
	.xword	.Ltmp24
	.xword	.Lfunc_begin5
	.xword	.Ltmp30
	.xword	.Ltmp35
	.xword	.Ltmp37
	.xword	.Ltmp39
	.xword	.Ltmp43
	.xword	.Ltmp45
	.xword	.Lfunc_begin6
	.xword	.Ltmp48
	.xword	.Ltmp49
	.xword	.Lfunc_begin7
	.xword	.Ltmp53
	.xword	.Lfunc_begin8
.Ldebug_addr_end0:
	.ident	"Apple clang version 21.0.0 (clang-2100.0.123.102)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.section	.debug_line,"",@progbits
.Lline_table_start0:
