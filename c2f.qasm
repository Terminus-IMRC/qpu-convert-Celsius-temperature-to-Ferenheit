alu.or, r0, uniform_read, #0

alu.or, vpm_ld_addr, uniform_read, #0
alu.or, host_int, r0, r0
alu.or, nop, vpm_ld_wait, nop
li32, vpmvcd_rd_setup, #0b 00 000000 0001 00 000000 1 0 10 00000000
alu.nop.never, nop, nop, nop
alu.nop.never, nop, nop, nop
alu.nop.never, nop, nop, nop

alu.or, r1, vpm_read, #0
alu.nop.never, nop, nop, nop
alu.v8min, r1, r1, #vr15

alu.or.sf, nop, element_number, #0
alu.itof.zs, sfu_recip, -, #5
alu.itof.zs, r2, -, #9
li32.zs, r3, #32
alu.fmul.zs, r2, r2, r4
alu.itof.zs, r1, r1, #0
alu.fmul.zs, r1, r1, r2
alu.itof.zs, r3, r3, #0
alu.fadd.zs, r1, r1, r3
alu.ftoi.zs, r1, r1, #0

li32, vpmvcd_wr_setup, #0b 00 000000000000 000001 1 0 10 00000000

alu.or, vpm_write, r1, r1

li32, vpmvcd_wr_setup, #0b 10 0010000 0000001 0 0 00000000000 000
alu.or, vpm_st_addr, uniform_read, #0
alu.or, nop, nop, vpm_st_wait
alu.or, host_int, r0, r0

alu.program_end.nop.never, nop, nop, nop
alu.nop.never, nop, nop, nop
alu.nop.never, nop, nop, nop
