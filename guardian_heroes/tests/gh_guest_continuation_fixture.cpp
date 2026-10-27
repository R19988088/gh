DEFINE_REX_FUNC(sub_00002000) {
  REX_FUNC_PROLOGUE();
  uint32_t ea{};
  // mflr r12
  ctx.r12.u64 = ctx.lr;
  // stw r12,-8(r1)
  REX_STORE_U32(ctx.r1.u32 - 8, ctx.r12.u32);
  // stwu r1,-32(r1)
  ea = ctx.r1.u32 - 32;
  REX_STORE_U32(ea, ctx.r1.u32);
  ctx.r1.u32 = ea;
  // stw r3,8(r1)
  REX_STORE_U32(ctx.r1.u32 + 8, ctx.r3.u32);
  // cmpwi cr6,r3,0
  ctx.cr6.compare<int32_t>(ctx.r3.s32, 0, ctx.xer);
  // beq cr6,0x202c
  if (ctx.cr6.eq) goto loc_0000202C;
  // addi r3,r3,-1
  ctx.r3.s64 = ctx.r3.s64 - 1;
  // bl 0x2000
  ctx.lr = 0x2020;
  sub_00002000(ctx, base);
  // lwz r4,8(r1)
  ctx.r4.u64 = REX_LOAD_U32(ctx.r1.u32 + 8);
  // add r3,r3,r4
  ctx.r3.u64 = ctx.r3.u64 + ctx.r4.u64;
  // b 0x2030
  goto loc_00002030;
loc_0000202C:
  // li r3,0
  ctx.r3.s64 = 0;
loc_00002030:
  // addi r1,r1,32
  ctx.r1.u64 = ctx.r1.u64 + 32;
  // lwz r12,-8(r1)
  ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 - 8);
  // mtlr r12
  ctx.lr = ctx.r12.u64;
  // blr
  return;
}

DEFINE_REX_FUNC(sub_00001000) {
  REX_FUNC_PROLOGUE();
  // bl 0x2000
  ctx.lr = 0x1004;
  sub_00002000(ctx, base);
  // li r6,0
  ctx.r6.s64 = 0;
  // li r7,0
  ctx.r7.s64 = 0;
loc_0000100C:
  // cmpw cr6,r7,r5
  ctx.cr6.compare<int32_t>(ctx.r7.s32, ctx.r5.s32, ctx.xer);
  // bge cr6,0x1020
  if (!ctx.cr6.lt) goto loc_00001020;
  // add r6,r6,r7
  ctx.r6.u64 = ctx.r6.u64 + ctx.r7.u64;
  // addi r7,r7,1
  ctx.r7.u64 = ctx.r7.u64 + 1;
  // b 0x100c
  goto loc_0000100C;
loc_00001020:
  // stw r3,0(r0)
  REX_STORE_U32(0, ctx.r3.u32);
  // stw r6,4(r0)
  REX_STORE_U32(4, ctx.r6.u32);
  // blr
  return;
}

DEFINE_REX_FUNC(sub_00004000) {
  REX_FUNC_PROLOGUE();
  // bctrl
  ctx.lr = 0x4004;
  REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
  // blr
  return;
}

DEFINE_REX_FUNC(sub_00005000) {
  REX_FUNC_PROLOGUE();
  uint32_t ea{};
  PPCRegister temp{};
  PPCVRegister vTemp{};
  // li r0,0
  temp.u64 = 0xFEDCBA9876543210ULL;
  ea = 128;
  vTemp.u8[0] = 99;
  // bl 0x2000
  ctx.lr = 0x5008;
  sub_00002000(ctx, base);
  // stw r0,0(r0)
  REX_STORE_U32(ea, temp.u32);
  ctx.r9.u64 = temp.u64;
  ctx.r10.u64 = vTemp.u8[0];
  // blr
  return;
}

DEFINE_REX_FUNC(sub_00006000) {
  REX_FUNC_PROLOGUE();
  // bctr
  switch (ctx.r10.u32) {
  case 0:
    goto loc_00006010;
  case 1:
    goto loc_00006018;
  default:
    __builtin_trap(); // Switch case out of range
  }
loc_00006010:
  // li r3,11
  ctx.r3.u64 = 11;
  // b 0x6020
  goto loc_00006020;
loc_00006018:
  // li r3,22
  ctx.r3.u64 = 22;
loc_00006020:
  // blr
  return;
}

DEFINE_REX_FUNC(sub_00007000) {
  REX_FUNC_PROLOGUE();
  // beqctr
  if (ctx.cr0.eq) {
    REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
    return;
  }
  // li r3,42
  ctx.r3.u64 = 42;
  // blr
  return;
}
