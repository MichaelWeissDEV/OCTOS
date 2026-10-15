	.file	"corpus.cpp"
	.text
	.globl	simple_add                      // -- Begin function simple_add
	.p2align	2
	.type	simple_add,@function
simple_add:                             // @simple_add
.Lfunc_begin0:
	.file	0 "/Users/michaelweiss/git/github/OCTOS/tests/data/asm" "corpus.cpp" md5 0xba4d7a0b733ef1532df489d07fae416a
	.loc	0 2 0                           // corpus.cpp:2:0
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16
	.cfi_def_cfa_offset 16
	str	w0, [sp, #12]
	str	w1, [sp, #8]
.Ltmp1:
	.loc	0 2 39 prologue_end             // corpus.cpp:2:39
	ldr	w8, [sp, #12]
	.loc	0 2 43 is_stmt 0                // corpus.cpp:2:43
	ldr	w9, [sp, #8]
	.loc	0 2 41                          // corpus.cpp:2:41
	add	w0, w8, w9
	.loc	0 2 32 epilogue_begin           // corpus.cpp:2:32
	add	sp, sp, #16
	ret
.Ltmp2:
.Lfunc_end0:
	.size	simple_add, .Lfunc_end0-simple_add
	.cfi_endproc
                                        // -- End function
	.globl	if_else                         // -- Begin function if_else
	.p2align	2
	.type	if_else,@function
if_else:                                // @if_else
.Lfunc_begin1:
	.loc	0 4 0 is_stmt 1                 // corpus.cpp:4:0
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16
	.cfi_def_cfa_offset 16
	str	w0, [sp, #8]
.Ltmp4:
	.loc	0 5 9 prologue_end              // corpus.cpp:5:9
	ldr	w8, [sp, #8]
	.loc	0 5 11 is_stmt 0                // corpus.cpp:5:11
	subs	w8, w8, #10
	b.le	.LBB1_2
	b	.LBB1_1
.LBB1_1:
	.loc	0 5 24                          // corpus.cpp:5:24
	ldr	w8, [sp, #8]
	.loc	0 5 26                          // corpus.cpp:5:26
	lsl	w8, w8, #1
	.loc	0 5 17                          // corpus.cpp:5:17
	str	w8, [sp, #12]
	b	.LBB1_6
.LBB1_2:
.Ltmp5:
	.loc	0 6 14 is_stmt 1                // corpus.cpp:6:14
	ldr	w8, [sp, #8]
	.loc	0 6 16 is_stmt 0                // corpus.cpp:6:16
	adds	w8, w8, #10
	b.ge	.LBB1_4
	b	.LBB1_3
.LBB1_3:
	.loc	0 6 31                          // corpus.cpp:6:31
	ldr	w9, [sp, #8]
	.loc	0 6 30                          // corpus.cpp:6:30
	mov	w8, wzr
	subs	w8, w8, w9
	.loc	0 6 23                          // corpus.cpp:6:23
	str	w8, [sp, #12]
	b	.LBB1_6
.Ltmp6:
.LBB1_4:
	.loc	0 0 23                          // corpus.cpp:0:23
	b	.LBB1_5
.LBB1_5:
	.loc	0 7 5 is_stmt 1                 // corpus.cpp:7:5
	str	wzr, [sp, #12]
	b	.LBB1_6
.LBB1_6:
	.loc	0 8 1                           // corpus.cpp:8:1
	ldr	w0, [sp, #12]
	.loc	0 8 1 epilogue_begin is_stmt 0  // corpus.cpp:8:1
	add	sp, sp, #16
	ret
.Ltmp7:
.Lfunc_end1:
	.size	if_else, .Lfunc_end1-if_else
	.cfi_endproc
                                        // -- End function
	.globl	loop_sum                        // -- Begin function loop_sum
	.p2align	2
	.type	loop_sum,@function
loop_sum:                               // @loop_sum
.Lfunc_begin2:
	.loc	0 10 0 is_stmt 1                // corpus.cpp:10:0
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	w1, [sp, #20]
.Ltmp9:
	.loc	0 11 9 prologue_end             // corpus.cpp:11:9
	str	wzr, [sp, #16]
.Ltmp10:
	.loc	0 12 14                         // corpus.cpp:12:14
	str	wzr, [sp, #12]
	.loc	0 12 10 is_stmt 0               // corpus.cpp:12:10
	b	.LBB2_1
.LBB2_1:                                // =>This Inner Loop Header: Depth=1
.Ltmp11:
	.loc	0 12 21                         // corpus.cpp:12:21
	ldr	w8, [sp, #12]
	.loc	0 12 25                         // corpus.cpp:12:25
	ldr	w9, [sp, #20]
.Ltmp12:
	.loc	0 12 5                          // corpus.cpp:12:5
	subs	w8, w8, w9
	b.ge	.LBB2_4
	b	.LBB2_2
.LBB2_2:                                //   in Loop: Header=BB2_1 Depth=1
.Ltmp13:
	.loc	0 12 38                         // corpus.cpp:12:38
	ldr	x8, [sp, #24]
	.loc	0 12 40                         // corpus.cpp:12:40
	ldrsw	x9, [sp, #12]
	.loc	0 12 38                         // corpus.cpp:12:38
	ldr	w9, [x8, x9, lsl #2]
	.loc	0 12 35                         // corpus.cpp:12:35
	ldr	w8, [sp, #16]
	add	w8, w8, w9
	str	w8, [sp, #16]
	.loc	0 12 33                         // corpus.cpp:12:33
	b	.LBB2_3
.LBB2_3:                                //   in Loop: Header=BB2_1 Depth=1
	.loc	0 12 28                         // corpus.cpp:12:28
	ldr	w8, [sp, #12]
	add	w8, w8, #1
	str	w8, [sp, #12]
	.loc	0 12 5                          // corpus.cpp:12:5
	b	.LBB2_1
.Ltmp14:
.LBB2_4:
	.loc	0 13 12 is_stmt 1               // corpus.cpp:13:12
	ldr	w0, [sp, #16]
	.loc	0 13 5 epilogue_begin is_stmt 0 // corpus.cpp:13:5
	add	sp, sp, #32
	ret
.Ltmp15:
.Lfunc_end2:
	.size	loop_sum, .Lfunc_end2-loop_sum
	.cfi_endproc
                                        // -- End function
	.globl	sw                              // -- Begin function sw
	.p2align	2
	.type	sw,@function
sw:                                     // @sw
.Lfunc_begin3:
	.loc	0 16 0 is_stmt 1                // corpus.cpp:16:0
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16
	.cfi_def_cfa_offset 16
	str	w0, [sp, #8]
.Ltmp17:
	.loc	0 17 13 prologue_end            // corpus.cpp:17:13
	ldr	w8, [sp, #8]
	str	w8, [sp, #4]                    // 4-byte Folded Spill
	.loc	0 17 5 is_stmt 0                // corpus.cpp:17:5
	cbz	w8, .LBB3_6
	b	.LBB3_1
.LBB3_1:
	.loc	0 0 5                           // corpus.cpp:0:5
	ldr	w8, [sp, #4]                    // 4-byte Folded Reload
	.loc	0 17 5                          // corpus.cpp:17:5
	subs	w8, w8, #1
	b.eq	.LBB3_7
	b	.LBB3_2
.LBB3_2:
	.loc	0 0 5                           // corpus.cpp:0:5
	ldr	w8, [sp, #4]                    // 4-byte Folded Reload
	.loc	0 17 5                          // corpus.cpp:17:5
	subs	w8, w8, #2
	b.eq	.LBB3_8
	b	.LBB3_3
.LBB3_3:
	.loc	0 0 5                           // corpus.cpp:0:5
	ldr	w8, [sp, #4]                    // 4-byte Folded Reload
	.loc	0 17 5                          // corpus.cpp:17:5
	subs	w8, w8, #3
	b.eq	.LBB3_9
	b	.LBB3_4
.LBB3_4:
	.loc	0 0 5                           // corpus.cpp:0:5
	ldr	w8, [sp, #4]                    // 4-byte Folded Reload
	.loc	0 17 5                          // corpus.cpp:17:5
	subs	w8, w8, #4
	b.eq	.LBB3_10
	b	.LBB3_5
.LBB3_5:
	.loc	0 0 5                           // corpus.cpp:0:5
	ldr	w8, [sp, #4]                    // 4-byte Folded Reload
	.loc	0 17 5                          // corpus.cpp:17:5
	subs	w8, w8, #5
	b.eq	.LBB3_11
	b	.LBB3_12
.LBB3_6:
.Ltmp18:
	.loc	0 18 17 is_stmt 1               // corpus.cpp:18:17
	mov	w8, #11                         // =0xb
	str	w8, [sp, #12]
	b	.LBB3_13
.LBB3_7:
	.loc	0 19 17                         // corpus.cpp:19:17
	mov	w8, #22                         // =0x16
	str	w8, [sp, #12]
	b	.LBB3_13
.LBB3_8:
	.loc	0 20 17                         // corpus.cpp:20:17
	mov	w8, #33                         // =0x21
	str	w8, [sp, #12]
	b	.LBB3_13
.LBB3_9:
	.loc	0 21 17                         // corpus.cpp:21:17
	mov	w8, #44                         // =0x2c
	str	w8, [sp, #12]
	b	.LBB3_13
.LBB3_10:
	.loc	0 22 17                         // corpus.cpp:22:17
	mov	w8, #55                         // =0x37
	str	w8, [sp, #12]
	b	.LBB3_13
.LBB3_11:
	.loc	0 23 17                         // corpus.cpp:23:17
	mov	w8, #66                         // =0x42
	str	w8, [sp, #12]
	b	.LBB3_13
.LBB3_12:
	.loc	0 24 18                         // corpus.cpp:24:18
	mov	w8, #-1                         // =0xffffffff
	str	w8, [sp, #12]
	b	.LBB3_13
.Ltmp19:
.LBB3_13:
	.loc	0 26 1                          // corpus.cpp:26:1
	ldr	w0, [sp, #12]
	.loc	0 26 1 epilogue_begin is_stmt 0 // corpus.cpp:26:1
	add	sp, sp, #16
	ret
.Ltmp20:
.Lfunc_end3:
	.size	sw, .Lfunc_end3-sw
	.cfi_endproc
                                        // -- End function
	.globl	tail_call                       // -- Begin function tail_call
	.p2align	2
	.type	tail_call,@function
tail_call:                              // @tail_call
.Lfunc_begin4:
	.loc	0 29 0 is_stmt 1                // corpus.cpp:29:0
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	w0, [x29, #-4]
.Ltmp21:
	.loc	0 29 38 prologue_end            // corpus.cpp:29:38
	ldur	w8, [x29, #-4]
	.loc	0 29 40 is_stmt 0               // corpus.cpp:29:40
	add	w0, w8, #1
	.loc	0 29 31                         // corpus.cpp:29:31
	bl	helper
	.loc	0 29 24 epilogue_begin          // corpus.cpp:29:24
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32
	ret
.Ltmp22:
.Lfunc_end4:
	.size	tail_call, .Lfunc_end4-tail_call
	.cfi_endproc
                                        // -- End function
	.globl	sw_call                         // -- Begin function sw_call
	.p2align	2
	.type	sw_call,@function
sw_call:                                // @sw_call
.Lfunc_begin5:
	.loc	0 32 0 is_stmt 1                // corpus.cpp:32:0
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	w0, [sp, #8]
	str	w1, [sp, #4]
.Ltmp23:
	.loc	0 33 13 prologue_end            // corpus.cpp:33:13
	ldr	w8, [sp, #8]
	str	w8, [sp]                        // 4-byte Folded Spill
	.loc	0 33 5 is_stmt 0                // corpus.cpp:33:5
	cbz	w8, .LBB5_6
	b	.LBB5_1
.LBB5_1:
	.loc	0 0 5                           // corpus.cpp:0:5
	ldr	w8, [sp]                        // 4-byte Folded Reload
	.loc	0 33 5                          // corpus.cpp:33:5
	subs	w8, w8, #1
	b.eq	.LBB5_7
	b	.LBB5_2
.LBB5_2:
	.loc	0 0 5                           // corpus.cpp:0:5
	ldr	w8, [sp]                        // 4-byte Folded Reload
	.loc	0 33 5                          // corpus.cpp:33:5
	subs	w8, w8, #2
	b.eq	.LBB5_8
	b	.LBB5_3
.LBB5_3:
	.loc	0 0 5                           // corpus.cpp:0:5
	ldr	w8, [sp]                        // 4-byte Folded Reload
	.loc	0 33 5                          // corpus.cpp:33:5
	subs	w8, w8, #3
	b.eq	.LBB5_9
	b	.LBB5_4
.LBB5_4:
	.loc	0 0 5                           // corpus.cpp:0:5
	ldr	w8, [sp]                        // 4-byte Folded Reload
	.loc	0 33 5                          // corpus.cpp:33:5
	subs	w8, w8, #4
	b.eq	.LBB5_10
	b	.LBB5_5
.LBB5_5:
	.loc	0 0 5                           // corpus.cpp:0:5
	ldr	w8, [sp]                        // 4-byte Folded Reload
	.loc	0 33 5                          // corpus.cpp:33:5
	subs	w8, w8, #5
	b.eq	.LBB5_11
	b	.LBB5_12
.LBB5_6:
.Ltmp24:
	.loc	0 34 27 is_stmt 1               // corpus.cpp:34:27
	ldr	w0, [sp, #4]
	.loc	0 34 24 is_stmt 0               // corpus.cpp:34:24
	bl	f0
	.loc	0 34 17                         // corpus.cpp:34:17
	stur	w0, [x29, #-4]
	b	.LBB5_13
.LBB5_7:
	.loc	0 35 27 is_stmt 1               // corpus.cpp:35:27
	ldr	w0, [sp, #4]
	.loc	0 35 24 is_stmt 0               // corpus.cpp:35:24
	bl	f1
	.loc	0 35 17                         // corpus.cpp:35:17
	stur	w0, [x29, #-4]
	b	.LBB5_13
.LBB5_8:
	.loc	0 36 27 is_stmt 1               // corpus.cpp:36:27
	ldr	w0, [sp, #4]
	.loc	0 36 24 is_stmt 0               // corpus.cpp:36:24
	bl	f2
	.loc	0 36 17                         // corpus.cpp:36:17
	stur	w0, [x29, #-4]
	b	.LBB5_13
.LBB5_9:
	.loc	0 37 27 is_stmt 1               // corpus.cpp:37:27
	ldr	w0, [sp, #4]
	.loc	0 37 24 is_stmt 0               // corpus.cpp:37:24
	bl	f3
	.loc	0 37 17                         // corpus.cpp:37:17
	stur	w0, [x29, #-4]
	b	.LBB5_13
.LBB5_10:
	.loc	0 38 27 is_stmt 1               // corpus.cpp:38:27
	ldr	w0, [sp, #4]
	.loc	0 38 24 is_stmt 0               // corpus.cpp:38:24
	bl	f4
	.loc	0 38 17                         // corpus.cpp:38:17
	stur	w0, [x29, #-4]
	b	.LBB5_13
.LBB5_11:
	.loc	0 39 27 is_stmt 1               // corpus.cpp:39:27
	ldr	w0, [sp, #4]
	.loc	0 39 24 is_stmt 0               // corpus.cpp:39:24
	bl	f5
	.loc	0 39 17                         // corpus.cpp:39:17
	stur	w0, [x29, #-4]
	b	.LBB5_13
.LBB5_12:
	.loc	0 40 18 is_stmt 1               // corpus.cpp:40:18
	stur	wzr, [x29, #-4]
	b	.LBB5_13
.Ltmp25:
.LBB5_13:
	.loc	0 42 1                          // corpus.cpp:42:1
	ldur	w0, [x29, #-4]
	.loc	0 42 1 epilogue_begin is_stmt 0 // corpus.cpp:42:1
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32
	ret
.Ltmp26:
.Lfunc_end5:
	.size	sw_call, .Lfunc_end5-sw_call
	.cfi_endproc
                                        // -- End function
	.globl	call_twice                      // -- Begin function call_twice
	.p2align	2
	.type	call_twice,@function
call_twice:                             // @call_twice
.Lfunc_begin6:
	.loc	0 44 0 is_stmt 1                // corpus.cpp:44:0
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	w0, [x29, #-4]
.Ltmp27:
	.loc	0 44 39 prologue_end            // corpus.cpp:44:39
	ldur	w0, [x29, #-4]
	.loc	0 44 32 is_stmt 0               // corpus.cpp:44:32
	bl	helper
	str	w0, [sp, #8]                    // 4-byte Folded Spill
	.loc	0 44 51                         // corpus.cpp:44:51
	ldur	w8, [x29, #-4]
	.loc	0 44 53                         // corpus.cpp:44:53
	add	w0, w8, #1
	.loc	0 44 44                         // corpus.cpp:44:44
	bl	helper
	mov	w8, w0
	ldr	w0, [sp, #8]                    // 4-byte Folded Reload
	.loc	0 44 42                         // corpus.cpp:44:42
	add	w0, w0, w8
	.loc	0 44 25 epilogue_begin          // corpus.cpp:44:25
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32
	ret
.Ltmp28:
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
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	str	w1, [sp, #4]
.Ltmp29:
	.loc	0 46 46 prologue_end            // corpus.cpp:46:46
	ldr	x8, [sp, #8]
	.loc	0 46 49 is_stmt 0               // corpus.cpp:46:49
	ldr	w0, [sp, #4]
	.loc	0 46 46                         // corpus.cpp:46:46
	blr	x8
	.loc	0 46 39 epilogue_begin          // corpus.cpp:46:39
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32
	ret
.Ltmp30:
.Lfunc_end7:
	.size	indirect, .Lfunc_end7-indirect
	.cfi_endproc
                                        // -- End function
	.globl	uses_local                      // -- Begin function uses_local
	.p2align	2
	.type	uses_local,@function
uses_local:                             // @uses_local
.Lfunc_begin8:
	.loc	0 49 0 is_stmt 1                // corpus.cpp:49:0
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	w0, [x29, #-4]
.Ltmp31:
	.loc	0 49 41 prologue_end            // corpus.cpp:49:41
	ldur	w0, [x29, #-4]
	.loc	0 49 32 is_stmt 0               // corpus.cpp:49:32
	bl	_ZL8local_fni
	str	w0, [sp, #8]                    // 4-byte Folded Spill
	.loc	0 49 55                         // corpus.cpp:49:55
	ldur	w8, [x29, #-4]
	.loc	0 49 57                         // corpus.cpp:49:57
	add	w0, w8, #1
	.loc	0 49 46                         // corpus.cpp:49:46
	bl	_ZL8local_fni
	mov	w8, w0
	ldr	w0, [sp, #8]                    // 4-byte Folded Reload
	.loc	0 49 44                         // corpus.cpp:49:44
	add	w0, w0, w8
	.loc	0 49 25 epilogue_begin          // corpus.cpp:49:25
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32
	ret
.Ltmp32:
.Lfunc_end8:
	.size	uses_local, .Lfunc_end8-uses_local
	.cfi_endproc
                                        // -- End function
	.p2align	2                               // -- Begin function _ZL8local_fni
	.type	_ZL8local_fni,@function
_ZL8local_fni:                          // @_ZL8local_fni
.Lfunc_begin9:
	.loc	0 48 0 is_stmt 1                // corpus.cpp:48:0
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16
	.cfi_def_cfa_offset 16
	str	w0, [sp, #12]
.Ltmp34:
	.loc	0 48 37 prologue_end            // corpus.cpp:48:37
	ldr	w8, [sp, #12]
	.loc	0 48 39 is_stmt 0               // corpus.cpp:48:39
	mov	w9, #3                          // =0x3
	mul	w0, w8, w9
	.loc	0 48 30 epilogue_begin          // corpus.cpp:48:30
	add	sp, sp, #16
	ret
.Ltmp35:
.Lfunc_end9:
	.size	_ZL8local_fni, .Lfunc_end9-_ZL8local_fni
	.cfi_endproc
                                        // -- End function
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
	.byte	4                               // Abbreviation Code
	.byte	52                              // DW_TAG_variable
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
	.byte	11                              // DW_TAG_lexical_block
	.byte	1                               // DW_CHILDREN_yes
	.byte	17                              // DW_AT_low_pc
	.byte	27                              // DW_FORM_addrx
	.byte	18                              // DW_AT_high_pc
	.byte	6                               // DW_FORM_data4
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	6                               // Abbreviation Code
	.byte	46                              // DW_TAG_subprogram
	.byte	1                               // DW_CHILDREN_yes
	.byte	17                              // DW_AT_low_pc
	.byte	27                              // DW_FORM_addrx
	.byte	18                              // DW_AT_high_pc
	.byte	6                               // DW_FORM_data4
	.byte	64                              // DW_AT_frame_base
	.byte	24                              // DW_FORM_exprloc
	.byte	110                             // DW_AT_linkage_name
	.byte	37                              // DW_FORM_strx1
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
	.byte	7                               // Abbreviation Code
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
	.byte	8                               // Abbreviation Code
	.byte	15                              // DW_TAG_pointer_type
	.byte	0                               // DW_CHILDREN_no
	.byte	73                              // DW_AT_type
	.byte	19                              // DW_FORM_ref4
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	9                               // Abbreviation Code
	.byte	38                              // DW_TAG_const_type
	.byte	0                               // DW_CHILDREN_no
	.byte	73                              // DW_AT_type
	.byte	19                              // DW_FORM_ref4
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	10                              // Abbreviation Code
	.byte	21                              // DW_TAG_subroutine_type
	.byte	1                               // DW_CHILDREN_yes
	.byte	73                              // DW_AT_type
	.byte	19                              // DW_FORM_ref4
	.byte	0                               // EOM(1)
	.byte	0                               // EOM(2)
	.byte	11                              // Abbreviation Code
	.byte	5                               // DW_TAG_formal_parameter
	.byte	0                               // DW_CHILDREN_no
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
	.byte	1                               // Abbrev [1] 0xc:0x18e DW_TAG_compile_unit
	.byte	0                               // DW_AT_producer
	.hword	33                              // DW_AT_language
	.byte	1                               // DW_AT_name
	.word	.Lstr_offsets_base0             // DW_AT_str_offsets_base
	.word	.Lline_table_start0             // DW_AT_stmt_list
	.byte	2                               // DW_AT_comp_dir
	.byte	0                               // DW_AT_low_pc
	.word	.Lfunc_end9-.Lfunc_begin0       // DW_AT_high_pc
	.word	.Laddr_table_base0              // DW_AT_addr_base
	.byte	2                               // Abbrev [2] 0x23:0x26 DW_TAG_subprogram
	.byte	0                               // DW_AT_low_pc
	.word	.Lfunc_end0-.Lfunc_begin0       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
	.byte	3                               // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	2                               // DW_AT_decl_line
	.word	379                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0x32:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	145
	.byte	12
	.byte	15                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	2                               // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	3                               // Abbrev [3] 0x3d:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	145
	.byte	8
	.byte	16                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	2                               // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0x49:0x1b DW_TAG_subprogram
	.byte	1                               // DW_AT_low_pc
	.word	.Lfunc_end1-.Lfunc_begin1       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
	.byte	5                               // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	4                               // DW_AT_decl_line
	.word	379                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0x58:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	145
	.byte	8
	.byte	17                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	4                               // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0x64:0x43 DW_TAG_subprogram
	.byte	2                               // DW_AT_low_pc
	.word	.Lfunc_end2-.Lfunc_begin2       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
	.byte	6                               // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	10                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0x73:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	145
	.byte	24
	.byte	15                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	10                              // DW_AT_decl_line
	.word	383                             // DW_AT_type
	.byte	3                               // Abbrev [3] 0x7e:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	145
	.byte	20
	.byte	18                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	10                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	4                               // Abbrev [4] 0x89:0xb DW_TAG_variable
	.byte	2                               // DW_AT_location
	.byte	145
	.byte	16
	.byte	19                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	11                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	5                               // Abbrev [5] 0x94:0x12 DW_TAG_lexical_block
	.byte	3                               // DW_AT_low_pc
	.word	.Ltmp14-.Ltmp10                 // DW_AT_high_pc
	.byte	4                               // Abbrev [4] 0x9a:0xb DW_TAG_variable
	.byte	2                               // DW_AT_location
	.byte	145
	.byte	12
	.byte	20                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	12                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0xa7:0x1b DW_TAG_subprogram
	.byte	4                               // DW_AT_low_pc
	.word	.Lfunc_end3-.Lfunc_begin3       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
	.byte	7                               // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	16                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0xb6:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	145
	.byte	8
	.byte	21                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	16                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0xc2:0x1b DW_TAG_subprogram
	.byte	5                               // DW_AT_low_pc
	.word	.Lfunc_end4-.Lfunc_begin4       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	109
	.byte	8                               // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	29                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0xd1:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	145
	.byte	124
	.byte	17                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	29                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0xdd:0x26 DW_TAG_subprogram
	.byte	6                               // DW_AT_low_pc
	.word	.Lfunc_end5-.Lfunc_begin5       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	109
	.byte	9                               // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	32                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0xec:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	143
	.byte	8
	.byte	21                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	32                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	3                               // Abbrev [3] 0xf7:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	143
	.byte	4
	.byte	22                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	32                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0x103:0x1b DW_TAG_subprogram
	.byte	7                               // DW_AT_low_pc
	.word	.Lfunc_end6-.Lfunc_begin6       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	109
	.byte	10                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	44                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0x112:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	145
	.byte	124
	.byte	17                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	44                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0x11e:0x26 DW_TAG_subprogram
	.byte	8                               // DW_AT_low_pc
	.word	.Lfunc_end7-.Lfunc_begin7       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	109
	.byte	11                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	46                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0x12d:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	143
	.byte	8
	.byte	23                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	46                              // DW_AT_decl_line
	.word	393                             // DW_AT_type
	.byte	3                               // Abbrev [3] 0x138:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	143
	.byte	4
	.byte	22                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	46                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	2                               // Abbrev [2] 0x144:0x1b DW_TAG_subprogram
	.byte	9                               // DW_AT_low_pc
	.word	.Lfunc_end8-.Lfunc_begin8       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	109
	.byte	12                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	49                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
                                        // DW_AT_external
	.byte	3                               // Abbrev [3] 0x153:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	145
	.byte	124
	.byte	17                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	49                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	6                               // Abbrev [6] 0x15f:0x1c DW_TAG_subprogram
	.byte	10                              // DW_AT_low_pc
	.word	.Lfunc_end9-.Lfunc_begin9       // DW_AT_high_pc
	.byte	1                               // DW_AT_frame_base
	.byte	111
	.byte	13                              // DW_AT_linkage_name
	.byte	14                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	48                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	3                               // Abbrev [3] 0x16f:0xb DW_TAG_formal_parameter
	.byte	2                               // DW_AT_location
	.byte	145
	.byte	12
	.byte	17                              // DW_AT_name
	.byte	0                               // DW_AT_decl_file
	.byte	48                              // DW_AT_decl_line
	.word	379                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	7                               // Abbrev [7] 0x17b:0x4 DW_TAG_base_type
	.byte	4                               // DW_AT_name
	.byte	5                               // DW_AT_encoding
	.byte	4                               // DW_AT_byte_size
	.byte	8                               // Abbrev [8] 0x17f:0x5 DW_TAG_pointer_type
	.word	388                             // DW_AT_type
	.byte	9                               // Abbrev [9] 0x184:0x5 DW_TAG_const_type
	.word	379                             // DW_AT_type
	.byte	8                               // Abbrev [8] 0x189:0x5 DW_TAG_pointer_type
	.word	398                             // DW_AT_type
	.byte	10                              // Abbrev [10] 0x18e:0xb DW_TAG_subroutine_type
	.word	379                             // DW_AT_type
	.byte	11                              // Abbrev [11] 0x193:0x5 DW_TAG_formal_parameter
	.word	379                             // DW_AT_type
	.byte	0                               // End Of Children Mark
	.byte	0                               // End Of Children Mark
.Ldebug_info_end0:
	.section	.debug_str_offsets,"",@progbits
	.word	100                             // Length of String Offsets Set
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
	.asciz	"simple_add"                    // string offset=113
.Linfo_string4:
	.asciz	"int"                           // string offset=124
.Linfo_string5:
	.asciz	"if_else"                       // string offset=128
.Linfo_string6:
	.asciz	"loop_sum"                      // string offset=136
.Linfo_string7:
	.asciz	"sw"                            // string offset=145
.Linfo_string8:
	.asciz	"tail_call"                     // string offset=148
.Linfo_string9:
	.asciz	"sw_call"                       // string offset=158
.Linfo_string10:
	.asciz	"call_twice"                    // string offset=166
.Linfo_string11:
	.asciz	"indirect"                      // string offset=177
.Linfo_string12:
	.asciz	"uses_local"                    // string offset=186
.Linfo_string13:
	.asciz	"_ZL8local_fni"                 // string offset=197
.Linfo_string14:
	.asciz	"local_fn"                      // string offset=211
.Linfo_string15:
	.asciz	"a"                             // string offset=220
.Linfo_string16:
	.asciz	"b"                             // string offset=222
.Linfo_string17:
	.asciz	"x"                             // string offset=224
.Linfo_string18:
	.asciz	"n"                             // string offset=226
.Linfo_string19:
	.asciz	"s"                             // string offset=228
.Linfo_string20:
	.asciz	"i"                             // string offset=230
.Linfo_string21:
	.asciz	"k"                             // string offset=232
.Linfo_string22:
	.asciz	"v"                             // string offset=234
.Linfo_string23:
	.asciz	"fp"                            // string offset=236
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
	.xword	.Ltmp10
	.xword	.Lfunc_begin3
	.xword	.Lfunc_begin4
	.xword	.Lfunc_begin5
	.xword	.Lfunc_begin6
	.xword	.Lfunc_begin7
	.xword	.Lfunc_begin8
	.xword	.Lfunc_begin9
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
