alu.or, r0, UNIFORM_READ, #0

alu.or, VPM_LD_ADDR, UNIFORM_READ, #0
alu.or, HOST_INT, r0, r0
alu.or, nop, VPM_LD_WAIT, NOP
li32, VPMVCD_RD_SETUP, #0b 00 000000 0001 00 000000 1 0 10 00000000
alu.nop.never, nop, nop, nop
alu.nop.never, nop, nop, nop
alu.nop.never, nop, nop, nop

alu.or, r1, VPM_READ, #0
alu.nop.never, nop, nop, nop
alu.v8min, r1, r1, #vr15

alu.or.sf, nop, ELEMENT_NUMBER, #0
alu.itof.zs, SFU_RECIP, #5, #5
alu.itof.zs, r2, #9, #9
li32.zs, r3, #32
alu.fmul.zs, r2, r2, r4
alu.itof.zs, r1, r1, #0
alu.fmul.zs, r1, r1, r2
alu.itof.zs, r3, r3, #0
alu.fadd.zs, r1, r1, r3
alu.ftoi.zs, r1, r1, #0

li32, VPMVCD_WR_SETUP, #0b 00 000000000000 000001 1 0 10 00000000

alu.or, VPM_WRITE, r1, r1

li32, VPMVCD_WR_SETUP, #0b 10 0010000 0000001 0 0 00000000000 000
alu.or, VPM_ST_ADDR, UNIFORM_READ, #0
alu.or, NOP, VPM_ST_WAIT, NOP
alu.or, HOST_INT, r0, r0

program_end
alu.nop.never, nop, nop, nop
alu.nop.never, nop, nop, nop
