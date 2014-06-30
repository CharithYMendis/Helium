#ifndef _DEFINES_BUILDEX_H
#define _DEFINES_BUILDEX_H

#include <stdlib.h>
#include <stdio.h>


enum {
/*   0 */     OP_INVALID, /**< INVALID opcode */
/*   1 */     OP_UNDECODED, /**< UNDECODED opcode */
/*   2 */     OP_CONTD, /**< CONTD opcode */
/*   3 */     OP_LABEL, /**< LABEL opcode */

/*   4 */     OP_add, /**< add opcode */
/*   5 */     OP_or, /**< or opcode */
/*   6 */     OP_adc, /**< adc opcode */
/*   7 */     OP_sbb, /**< sbb opcode */
/*   8 */     OP_and, /**< and opcode */
/*   9 */     OP_daa, /**< daa opcode */
/*  10 */     OP_sub, /**< sub opcode */
/*  11 */     OP_das, /**< das opcode */
/*  12 */     OP_xor, /**< xor opcode */
/*  13 */     OP_aaa, /**< aaa opcode */
/*  14 */     OP_cmp, /**< cmp opcode */
/*  15 */     OP_aas, /**< aas opcode */
/*  16 */     OP_inc, /**< inc opcode */
/*  17 */     OP_dec, /**< dec opcode */
/*  18 */     OP_push, /**< push opcode */
/*  19 */     OP_push_imm, /**< push_imm opcode */
/*  20 */     OP_pop, /**< pop opcode */
/*  21 */     OP_pusha, /**< pusha opcode */
/*  22 */     OP_popa, /**< popa opcode */
/*  23 */     OP_bound, /**< bound opcode */
/*  24 */     OP_arpl, /**< arpl opcode */
/*  25 */     OP_imul, /**< imul opcode */

/*  26 */     OP_jo_short, /**< jo_short opcode */
/*  27 */     OP_jno_short, /**< jno_short opcode */
/*  28 */     OP_jb_short, /**< jb_short opcode */
/*  29 */     OP_jnb_short, /**< jnb_short opcode */
/*  30 */     OP_jz_short, /**< jz_short opcode */
/*  31 */     OP_jnz_short, /**< jnz_short opcode */
/*  32 */     OP_jbe_short, /**< jbe_short opcode */
/*  33 */     OP_jnbe_short, /**< jnbe_short opcode */
/*  34 */     OP_js_short, /**< js_short opcode */
/*  35 */     OP_jns_short, /**< jns_short opcode */
/*  36 */     OP_jp_short, /**< jp_short opcode */
/*  37 */     OP_jnp_short, /**< jnp_short opcode */
/*  38 */     OP_jl_short, /**< jl_short opcode */
/*  39 */     OP_jnl_short, /**< jnl_short opcode */
/*  40 */     OP_jle_short, /**< jle_short opcode */
/*  41 */     OP_jnle_short, /**< jnle_short opcode */

/*  42 */     OP_call, /**< call opcode */
/*  43 */     OP_call_ind, /**< call_ind opcode */
/*  44 */     OP_call_far, /**< call_far opcode */
/*  45 */     OP_call_far_ind, /**< call_far_ind opcode */
/*  46 */     OP_jmp, /**< jmp opcode */
/*  47 */     OP_jmp_short, /**< jmp_short opcode */
/*  48 */     OP_jmp_ind, /**< jmp_ind opcode */
/*  49 */     OP_jmp_far, /**< jmp_far opcode */
/*  50 */     OP_jmp_far_ind, /**< jmp_far_ind opcode */

/*  51 */     OP_loopne, /**< loopne opcode */
/*  52 */     OP_loope, /**< loope opcode */
/*  53 */     OP_loop, /**< loop opcode */
/*  54 */     OP_jecxz, /**< jecxz opcode */

    /* point ld & st at eAX & al instrs, they save 1 byte (no modrm),
     * hopefully time taken considering them doesn't offset that */
/*  55 */     OP_mov_ld, /**< mov_ld opcode */
/*  56 */     OP_mov_st, /**< mov_st opcode */
    /* PR 250397: store of immed is mov_st not mov_imm, even though can be immed->reg,
     * which we address by sharing part of the mov_st template chain */
/*  57 */     OP_mov_imm, /**< mov_imm opcode */
/*  58 */     OP_mov_seg, /**< mov_seg opcode */
/*  59 */     OP_mov_priv, /**< mov_priv opcode */

/*  60 */     OP_test, /**< test opcode */
/*  61 */     OP_lea, /**< lea opcode */
/*  62 */     OP_xchg, /**< xchg opcode */
/*  63 */     OP_cwde, /**< cwde opcode */
/*  64 */     OP_cdq, /**< cdq opcode */
/*  65 */     OP_fwait, /**< fwait opcode */
/*  66 */     OP_pushf, /**< pushf opcode */
/*  67 */     OP_popf, /**< popf opcode */
/*  68 */     OP_sahf, /**< sahf opcode */
/*  69 */     OP_lahf, /**< lahf opcode */

/*  70 */     OP_ret, /**< ret opcode */
/*  71 */     OP_ret_far, /**< ret_far opcode */

/*  72 */     OP_les, /**< les opcode */
/*  73 */     OP_lds, /**< lds opcode */
/*  74 */     OP_enter, /**< enter opcode */
/*  75 */     OP_leave, /**< leave opcode */
/*  76 */     OP_int3, /**< int3 opcode */
/*  77 */     OP_int, /**< int opcode */
/*  78 */     OP_into, /**< into opcode */
/*  79 */     OP_iret, /**< iret opcode */
/*  80 */     OP_aam, /**< aam opcode */
/*  81 */     OP_aad, /**< aad opcode */
/*  82 */     OP_xlat, /**< xlat opcode */
/*  83 */     OP_in, /**< in opcode */
/*  84 */     OP_out, /**< out opcode */
/*  85 */     OP_hlt, /**< hlt opcode */
/*  86 */     OP_cmc, /**< cmc opcode */
/*  87 */     OP_clc, /**< clc opcode */
/*  88 */     OP_stc, /**< stc opcode */
/*  89 */     OP_cli, /**< cli opcode */
/*  90 */     OP_sti, /**< sti opcode */
/*  91 */     OP_cld, /**< cld opcode */
/*  92 */     OP_std, /**< std opcode */


/*  93 */     OP_lar, /**< lar opcode */
/*  94 */     OP_lsl, /**< lsl opcode */
/*  95 */     OP_syscall, /**< syscall opcode */
/*  96 */     OP_clts, /**< clts opcode */
/*  97 */     OP_sysret, /**< sysret opcode */
/*  98 */     OP_invd, /**< invd opcode */
/*  99 */     OP_wbinvd, /**< wbinvd opcode */
/* 100 */     OP_ud2a, /**< ud2a opcode */
/* 101 */     OP_nop_modrm, /**< nop_modrm opcode */
/* 102 */     OP_movntps, /**< movntps opcode */
/* 103 */     OP_movntpd, /**< movntpd opcode */
/* 104 */     OP_wrmsr, /**< wrmsr opcode */
/* 105 */     OP_rdtsc, /**< rdtsc opcode */
/* 106 */     OP_rdmsr, /**< rdmsr opcode */
/* 107 */     OP_rdpmc, /**< rdpmc opcode */
/* 108 */     OP_sysenter, /**< sysenter opcode */
/* 109 */     OP_sysexit, /**< sysexit opcode */

/* 110 */     OP_cmovo, /**< cmovo opcode */
/* 111 */     OP_cmovno, /**< cmovno opcode */
/* 112 */     OP_cmovb, /**< cmovb opcode */
/* 113 */     OP_cmovnb, /**< cmovnb opcode */
/* 114 */     OP_cmovz, /**< cmovz opcode */
/* 115 */     OP_cmovnz, /**< cmovnz opcode */
/* 116 */     OP_cmovbe, /**< cmovbe opcode */
/* 117 */     OP_cmovnbe, /**< cmovnbe opcode */
/* 118 */     OP_cmovs, /**< cmovs opcode */
/* 119 */     OP_cmovns, /**< cmovns opcode */
/* 120 */     OP_cmovp, /**< cmovp opcode */
/* 121 */     OP_cmovnp, /**< cmovnp opcode */
/* 122 */     OP_cmovl, /**< cmovl opcode */
/* 123 */     OP_cmovnl, /**< cmovnl opcode */
/* 124 */     OP_cmovle, /**< cmovle opcode */
/* 125 */     OP_cmovnle, /**< cmovnle opcode */

/* 126 */     OP_punpcklbw, /**< punpcklbw opcode */
/* 127 */     OP_punpcklwd, /**< punpcklwd opcode */
/* 128 */     OP_punpckldq, /**< punpckldq opcode */
/* 129 */     OP_packsswb, /**< packsswb opcode */
/* 130 */     OP_pcmpgtb, /**< pcmpgtb opcode */
/* 131 */     OP_pcmpgtw, /**< pcmpgtw opcode */
/* 132 */     OP_pcmpgtd, /**< pcmpgtd opcode */
/* 133 */     OP_packuswb, /**< packuswb opcode */
/* 134 */     OP_punpckhbw, /**< punpckhbw opcode */
/* 135 */     OP_punpckhwd, /**< punpckhwd opcode */
/* 136 */     OP_punpckhdq, /**< punpckhdq opcode */
/* 137 */     OP_packssdw, /**< packssdw opcode */
/* 138 */     OP_punpcklqdq, /**< punpcklqdq opcode */
/* 139 */     OP_punpckhqdq, /**< punpckhqdq opcode */
/* 140 */     OP_movd, /**< movd opcode */
/* 141 */     OP_movq, /**< movq opcode */
/* 142 */     OP_movdqu, /**< movdqu opcode */
/* 143 */     OP_movdqa, /**< movdqa opcode */
/* 144 */     OP_pshufw, /**< pshufw opcode */
/* 145 */     OP_pshufd, /**< pshufd opcode */
/* 146 */     OP_pshufhw, /**< pshufhw opcode */
/* 147 */     OP_pshuflw, /**< pshuflw opcode */
/* 148 */     OP_pcmpeqb, /**< pcmpeqb opcode */
/* 149 */     OP_pcmpeqw, /**< pcmpeqw opcode */
/* 150 */     OP_pcmpeqd, /**< pcmpeqd opcode */
/* 151 */     OP_emms, /**< emms opcode */

/* 152 */     OP_jo, /**< jo opcode */
/* 153 */     OP_jno, /**< jno opcode */
/* 154 */     OP_jb, /**< jb opcode */
/* 155 */     OP_jnb, /**< jnb opcode */
/* 156 */     OP_jz, /**< jz opcode */
/* 157 */     OP_jnz, /**< jnz opcode */
/* 158 */     OP_jbe, /**< jbe opcode */
/* 159 */     OP_jnbe, /**< jnbe opcode */
/* 160 */     OP_js, /**< js opcode */
/* 161 */     OP_jns, /**< jns opcode */
/* 162 */     OP_jp, /**< jp opcode */
/* 163 */     OP_jnp, /**< jnp opcode */
/* 164 */     OP_jl, /**< jl opcode */
/* 165 */     OP_jnl, /**< jnl opcode */
/* 166 */     OP_jle, /**< jle opcode */
/* 167 */     OP_jnle, /**< jnle opcode */

/* 168 */     OP_seto, /**< seto opcode */
/* 169 */     OP_setno, /**< setno opcode */
/* 170 */     OP_setb, /**< setb opcode */
/* 171 */     OP_setnb, /**< setnb opcode */
/* 172 */     OP_setz, /**< setz opcode */
/* 173 */     OP_setnz, /**< setnz opcode */
/* 174 */     OP_setbe, /**< setbe opcode */
/* 175 */     OP_setnbe, /**< setnbe opcode */
/* 176 */     OP_sets, /**< sets opcode */
/* 177 */     OP_setns, /**< setns opcode */
/* 178 */     OP_setp, /**< setp opcode */
/* 179 */     OP_setnp, /**< setnp opcode */
/* 180 */     OP_setl, /**< setl opcode */
/* 181 */     OP_setnl, /**< setnl opcode */
/* 182 */     OP_setle, /**< setle opcode */
/* 183 */     OP_setnle, /**< setnle opcode */

/* 184 */     OP_cpuid, /**< cpuid opcode */
/* 185 */     OP_bt, /**< bt opcode */
/* 186 */     OP_shld, /**< shld opcode */
/* 187 */     OP_rsm, /**< rsm opcode */
/* 188 */     OP_bts, /**< bts opcode */
/* 189 */     OP_shrd, /**< shrd opcode */
/* 190 */     OP_cmpxchg, /**< cmpxchg opcode */
/* 191 */     OP_lss, /**< lss opcode */
/* 192 */     OP_btr, /**< btr opcode */
/* 193 */     OP_lfs, /**< lfs opcode */
/* 194 */     OP_lgs, /**< lgs opcode */
/* 195 */     OP_movzx, /**< movzx opcode */
/* 196 */     OP_ud2b, /**< ud2b opcode */
/* 197 */     OP_btc, /**< btc opcode */
/* 198 */     OP_bsf, /**< bsf opcode */
/* 199 */     OP_bsr, /**< bsr opcode */
/* 200 */     OP_movsx, /**< movsx opcode */
/* 201 */     OP_xadd, /**< xadd opcode */
/* 202 */     OP_movnti, /**< movnti opcode */
/* 203 */     OP_pinsrw, /**< pinsrw opcode */
/* 204 */     OP_pextrw, /**< pextrw opcode */
/* 205 */     OP_bswap, /**< bswap opcode */
/* 206 */     OP_psrlw, /**< psrlw opcode */
/* 207 */     OP_psrld, /**< psrld opcode */
/* 208 */     OP_psrlq, /**< psrlq opcode */
/* 209 */     OP_paddq, /**< paddq opcode */
/* 210 */     OP_pmullw, /**< pmullw opcode */
/* 211 */     OP_pmovmskb, /**< pmovmskb opcode */
/* 212 */     OP_psubusb, /**< psubusb opcode */
/* 213 */     OP_psubusw, /**< psubusw opcode */
/* 214 */     OP_pminub, /**< pminub opcode */
/* 215 */     OP_pand, /**< pand opcode */
/* 216 */     OP_paddusb, /**< paddusb opcode */
/* 217 */     OP_paddusw, /**< paddusw opcode */
/* 218 */     OP_pmaxub, /**< pmaxub opcode */
/* 219 */     OP_pandn, /**< pandn opcode */
/* 220 */     OP_pavgb, /**< pavgb opcode */
/* 221 */     OP_psraw, /**< psraw opcode */
/* 222 */     OP_psrad, /**< psrad opcode */
/* 223 */     OP_pavgw, /**< pavgw opcode */
/* 224 */     OP_pmulhuw, /**< pmulhuw opcode */
/* 225 */     OP_pmulhw, /**< pmulhw opcode */
/* 226 */     OP_movntq, /**< movntq opcode */
/* 227 */     OP_movntdq, /**< movntdq opcode */
/* 228 */     OP_psubsb, /**< psubsb opcode */
/* 229 */     OP_psubsw, /**< psubsw opcode */
/* 230 */     OP_pminsw, /**< pminsw opcode */
/* 231 */     OP_por, /**< por opcode */
/* 232 */     OP_paddsb, /**< paddsb opcode */
/* 233 */     OP_paddsw, /**< paddsw opcode */
/* 234 */     OP_pmaxsw, /**< pmaxsw opcode */
/* 235 */     OP_pxor, /**< pxor opcode */
/* 236 */     OP_psllw, /**< psllw opcode */
/* 237 */     OP_pslld, /**< pslld opcode */
/* 238 */     OP_psllq, /**< psllq opcode */
/* 239 */     OP_pmuludq, /**< pmuludq opcode */
/* 240 */     OP_pmaddwd, /**< pmaddwd opcode */
/* 241 */     OP_psadbw, /**< psadbw opcode */
/* 242 */     OP_maskmovq, /**< maskmovq opcode */
/* 243 */     OP_maskmovdqu, /**< maskmovdqu opcode */
/* 244 */     OP_psubb, /**< psubb opcode */
/* 245 */     OP_psubw, /**< psubw opcode */
/* 246 */     OP_psubd, /**< psubd opcode */
/* 247 */     OP_psubq, /**< psubq opcode */
/* 248 */     OP_paddb, /**< paddb opcode */
/* 249 */     OP_paddw, /**< paddw opcode */
/* 250 */     OP_paddd, /**< paddd opcode */
/* 251 */     OP_psrldq, /**< psrldq opcode */
/* 252 */     OP_pslldq, /**< pslldq opcode */


/* 253 */     OP_rol, /**< rol opcode */
/* 254 */     OP_ror, /**< ror opcode */
/* 255 */     OP_rcl, /**< rcl opcode */
/* 256 */     OP_rcr, /**< rcr opcode */
/* 257 */     OP_shl, /**< shl opcode */
/* 258 */     OP_shr, /**< shr opcode */
/* 259 */     OP_sar, /**< sar opcode */
/* 260 */     OP_not, /**< not opcode */
/* 261 */     OP_neg, /**< neg opcode */
/* 262 */     OP_mul, /**< mul opcode */
/* 263 */     OP_div, /**< div opcode */
/* 264 */     OP_idiv, /**< idiv opcode */
/* 265 */     OP_sldt, /**< sldt opcode */
/* 266 */     OP_str, /**< str opcode */
/* 267 */     OP_lldt, /**< lldt opcode */
/* 268 */     OP_ltr, /**< ltr opcode */
/* 269 */     OP_verr, /**< verr opcode */
/* 270 */     OP_verw, /**< verw opcode */
/* 271 */     OP_sgdt, /**< sgdt opcode */
/* 272 */     OP_sidt, /**< sidt opcode */
/* 273 */     OP_lgdt, /**< lgdt opcode */
/* 274 */     OP_lidt, /**< lidt opcode */
/* 275 */     OP_smsw, /**< smsw opcode */
/* 276 */     OP_lmsw, /**< lmsw opcode */
/* 277 */     OP_invlpg, /**< invlpg opcode */
/* 278 */     OP_cmpxchg8b, /**< cmpxchg8b opcode */
/* 279 */     OP_fxsave32, /**< fxsave opcode */
/* 280 */     OP_fxrstor32, /**< fxrstor opcode */
/* 281 */     OP_ldmxcsr, /**< ldmxcsr opcode */
/* 282 */     OP_stmxcsr, /**< stmxcsr opcode */
/* 283 */     OP_lfence, /**< lfence opcode */
/* 284 */     OP_mfence, /**< mfence opcode */
/* 285 */     OP_clflush, /**< clflush opcode */
/* 286 */     OP_sfence, /**< sfence opcode */
/* 287 */     OP_prefetchnta, /**< prefetchnta opcode */
/* 288 */     OP_prefetcht0, /**< prefetcht0 opcode */
/* 289 */     OP_prefetcht1, /**< prefetcht1 opcode */
/* 290 */     OP_prefetcht2, /**< prefetcht2 opcode */
/* 291 */     OP_prefetch, /**< prefetch opcode */
/* 292 */     OP_prefetchw, /**< prefetchw opcode */


/* 293 */     OP_movups, /**< movups opcode */
/* 294 */     OP_movss, /**< movss opcode */
/* 295 */     OP_movupd, /**< movupd opcode */
/* 296 */     OP_movsd, /**< movsd opcode */
/* 297 */     OP_movlps, /**< movlps opcode */
/* 298 */     OP_movlpd, /**< movlpd opcode */
/* 299 */     OP_unpcklps, /**< unpcklps opcode */
/* 300 */     OP_unpcklpd, /**< unpcklpd opcode */
/* 301 */     OP_unpckhps, /**< unpckhps opcode */
/* 302 */     OP_unpckhpd, /**< unpckhpd opcode */
/* 303 */     OP_movhps, /**< movhps opcode */
/* 304 */     OP_movhpd, /**< movhpd opcode */
/* 305 */     OP_movaps, /**< movaps opcode */
/* 306 */     OP_movapd, /**< movapd opcode */
/* 307 */     OP_cvtpi2ps, /**< cvtpi2ps opcode */
/* 308 */     OP_cvtsi2ss, /**< cvtsi2ss opcode */
/* 309 */     OP_cvtpi2pd, /**< cvtpi2pd opcode */
/* 310 */     OP_cvtsi2sd, /**< cvtsi2sd opcode */
/* 311 */     OP_cvttps2pi, /**< cvttps2pi opcode */
/* 312 */     OP_cvttss2si, /**< cvttss2si opcode */
/* 313 */     OP_cvttpd2pi, /**< cvttpd2pi opcode */
/* 314 */     OP_cvttsd2si, /**< cvttsd2si opcode */
/* 315 */     OP_cvtps2pi, /**< cvtps2pi opcode */
/* 316 */     OP_cvtss2si, /**< cvtss2si opcode */
/* 317 */     OP_cvtpd2pi, /**< cvtpd2pi opcode */
/* 318 */     OP_cvtsd2si, /**< cvtsd2si opcode */
/* 319 */     OP_ucomiss, /**< ucomiss opcode */
/* 320 */     OP_ucomisd, /**< ucomisd opcode */
/* 321 */     OP_comiss, /**< comiss opcode */
/* 322 */     OP_comisd, /**< comisd opcode */
/* 323 */     OP_movmskps, /**< movmskps opcode */
/* 324 */     OP_movmskpd, /**< movmskpd opcode */
/* 325 */     OP_sqrtps, /**< sqrtps opcode */
/* 326 */     OP_sqrtss, /**< sqrtss opcode */
/* 327 */     OP_sqrtpd, /**< sqrtpd opcode */
/* 328 */     OP_sqrtsd, /**< sqrtsd opcode */
/* 329 */     OP_rsqrtps, /**< rsqrtps opcode */
/* 330 */     OP_rsqrtss, /**< rsqrtss opcode */
/* 331 */     OP_rcpps, /**< rcpps opcode */
/* 332 */     OP_rcpss, /**< rcpss opcode */
/* 333 */     OP_andps, /**< andps opcode */
/* 334 */     OP_andpd, /**< andpd opcode */
/* 335 */     OP_andnps, /**< andnps opcode */
/* 336 */     OP_andnpd, /**< andnpd opcode */
/* 337 */     OP_orps, /**< orps opcode */
/* 338 */     OP_orpd, /**< orpd opcode */
/* 339 */     OP_xorps, /**< xorps opcode */
/* 340 */     OP_xorpd, /**< xorpd opcode */
/* 341 */     OP_addps, /**< addps opcode */
/* 342 */     OP_addss, /**< addss opcode */
/* 343 */     OP_addpd, /**< addpd opcode */
/* 344 */     OP_addsd, /**< addsd opcode */
/* 345 */     OP_mulps, /**< mulps opcode */
/* 346 */     OP_mulss, /**< mulss opcode */
/* 347 */     OP_mulpd, /**< mulpd opcode */
/* 348 */     OP_mulsd, /**< mulsd opcode */
/* 349 */     OP_cvtps2pd, /**< cvtps2pd opcode */
/* 350 */     OP_cvtss2sd, /**< cvtss2sd opcode */
/* 351 */     OP_cvtpd2ps, /**< cvtpd2ps opcode */
/* 352 */     OP_cvtsd2ss, /**< cvtsd2ss opcode */
/* 353 */     OP_cvtdq2ps, /**< cvtdq2ps opcode */
/* 354 */     OP_cvttps2dq, /**< cvttps2dq opcode */
/* 355 */     OP_cvtps2dq, /**< cvtps2dq opcode */
/* 356 */     OP_subps, /**< subps opcode */
/* 357 */     OP_subss, /**< subss opcode */
/* 358 */     OP_subpd, /**< subpd opcode */
/* 359 */     OP_subsd, /**< subsd opcode */
/* 360 */     OP_minps, /**< minps opcode */
/* 361 */     OP_minss, /**< minss opcode */
/* 362 */     OP_minpd, /**< minpd opcode */
/* 363 */     OP_minsd, /**< minsd opcode */
/* 364 */     OP_divps, /**< divps opcode */
/* 365 */     OP_divss, /**< divss opcode */
/* 366 */     OP_divpd, /**< divpd opcode */
/* 367 */     OP_divsd, /**< divsd opcode */
/* 368 */     OP_maxps, /**< maxps opcode */
/* 369 */     OP_maxss, /**< maxss opcode */
/* 370 */     OP_maxpd, /**< maxpd opcode */
/* 371 */     OP_maxsd, /**< maxsd opcode */
/* 372 */     OP_cmpps, /**< cmpps opcode */
/* 373 */     OP_cmpss, /**< cmpss opcode */
/* 374 */     OP_cmppd, /**< cmppd opcode */
/* 375 */     OP_cmpsd, /**< cmpsd opcode */
/* 376 */     OP_shufps, /**< shufps opcode */
/* 377 */     OP_shufpd, /**< shufpd opcode */
/* 378 */     OP_cvtdq2pd, /**< cvtdq2pd opcode */
/* 379 */     OP_cvttpd2dq, /**< cvttpd2dq opcode */
/* 380 */     OP_cvtpd2dq, /**< cvtpd2dq opcode */
/* 381 */     OP_nop, /**< nop opcode */
/* 382 */     OP_pause, /**< pause opcode */

/* 383 */     OP_ins, /**< ins opcode */
/* 384 */     OP_rep_ins, /**< rep_ins opcode */
/* 385 */     OP_outs, /**< outs opcode */
/* 386 */     OP_rep_outs, /**< rep_outs opcode */
/* 387 */     OP_movs, /**< movs opcode */
/* 388 */     OP_rep_movs, /**< rep_movs opcode */
/* 389 */     OP_stos, /**< stos opcode */
/* 390 */     OP_rep_stos, /**< rep_stos opcode */
/* 391 */     OP_lods, /**< lods opcode */
/* 392 */     OP_rep_lods, /**< rep_lods opcode */
/* 393 */     OP_cmps, /**< cmps opcode */
/* 394 */     OP_rep_cmps, /**< rep_cmps opcode */
/* 395 */     OP_repne_cmps, /**< repne_cmps opcode */
/* 396 */     OP_scas, /**< scas opcode */
/* 397 */     OP_rep_scas, /**< rep_scas opcode */
/* 398 */     OP_repne_scas, /**< repne_scas opcode */


/* 399 */     OP_fadd, /**< fadd opcode */
/* 400 */     OP_fmul, /**< fmul opcode */
/* 401 */     OP_fcom, /**< fcom opcode */
/* 402 */     OP_fcomp, /**< fcomp opcode */
/* 403 */     OP_fsub, /**< fsub opcode */
/* 404 */     OP_fsubr, /**< fsubr opcode */
/* 405 */     OP_fdiv, /**< fdiv opcode */
/* 406 */     OP_fdivr, /**< fdivr opcode */
/* 407 */     OP_fld, /**< fld opcode */
/* 408 */     OP_fst, /**< fst opcode */
/* 409 */     OP_fstp, /**< fstp opcode */
/* 410 */     OP_fldenv, /**< fldenv opcode */
/* 411 */     OP_fldcw, /**< fldcw opcode */
/* 412 */     OP_fnstenv, /**< fnstenv opcode */
/* 413 */     OP_fnstcw, /**< fnstcw opcode */
/* 414 */     OP_fiadd, /**< fiadd opcode */
/* 415 */     OP_fimul, /**< fimul opcode */
/* 416 */     OP_ficom, /**< ficom opcode */
/* 417 */     OP_ficomp, /**< ficomp opcode */
/* 418 */     OP_fisub, /**< fisub opcode */
/* 419 */     OP_fisubr, /**< fisubr opcode */
/* 420 */     OP_fidiv, /**< fidiv opcode */
/* 421 */     OP_fidivr, /**< fidivr opcode */
/* 422 */     OP_fild, /**< fild opcode */
/* 423 */     OP_fist, /**< fist opcode */
/* 424 */     OP_fistp, /**< fistp opcode */
/* 425 */     OP_frstor, /**< frstor opcode */
/* 426 */     OP_fnsave, /**< fnsave opcode */
/* 427 */     OP_fnstsw, /**< fnstsw opcode */

/* 428 */     OP_fbld, /**< fbld opcode */
/* 429 */     OP_fbstp, /**< fbstp opcode */


/* 430 */     OP_fxch, /**< fxch opcode */
/* 431 */     OP_fnop, /**< fnop opcode */
/* 432 */     OP_fchs, /**< fchs opcode */
/* 433 */     OP_fabs, /**< fabs opcode */
/* 434 */     OP_ftst, /**< ftst opcode */
/* 435 */     OP_fxam, /**< fxam opcode */
/* 436 */     OP_fld1, /**< fld1 opcode */
/* 437 */     OP_fldl2t, /**< fldl2t opcode */
/* 438 */     OP_fldl2e, /**< fldl2e opcode */
/* 439 */     OP_fldpi, /**< fldpi opcode */
/* 440 */     OP_fldlg2, /**< fldlg2 opcode */
/* 441 */     OP_fldln2, /**< fldln2 opcode */
/* 442 */     OP_fldz, /**< fldz opcode */
/* 443 */     OP_f2xm1, /**< f2xm1 opcode */
/* 444 */     OP_fyl2x, /**< fyl2x opcode */
/* 445 */     OP_fptan, /**< fptan opcode */
/* 446 */     OP_fpatan, /**< fpatan opcode */
/* 447 */     OP_fxtract, /**< fxtract opcode */
/* 448 */     OP_fprem1, /**< fprem1 opcode */
/* 449 */     OP_fdecstp, /**< fdecstp opcode */
/* 450 */     OP_fincstp, /**< fincstp opcode */
/* 451 */     OP_fprem, /**< fprem opcode */
/* 452 */     OP_fyl2xp1, /**< fyl2xp1 opcode */
/* 453 */     OP_fsqrt, /**< fsqrt opcode */
/* 454 */     OP_fsincos, /**< fsincos opcode */
/* 455 */     OP_frndint, /**< frndint opcode */
/* 456 */     OP_fscale, /**< fscale opcode */
/* 457 */     OP_fsin, /**< fsin opcode */
/* 458 */     OP_fcos, /**< fcos opcode */
/* 459 */     OP_fcmovb, /**< fcmovb opcode */
/* 460 */     OP_fcmove, /**< fcmove opcode */
/* 461 */     OP_fcmovbe, /**< fcmovbe opcode */
/* 462 */     OP_fcmovu, /**< fcmovu opcode */
/* 463 */     OP_fucompp, /**< fucompp opcode */
/* 464 */     OP_fcmovnb, /**< fcmovnb opcode */
/* 465 */     OP_fcmovne, /**< fcmovne opcode */
/* 466 */     OP_fcmovnbe, /**< fcmovnbe opcode */
/* 467 */     OP_fcmovnu, /**< fcmovnu opcode */
/* 468 */     OP_fnclex, /**< fnclex opcode */
/* 469 */     OP_fninit, /**< fninit opcode */
/* 470 */     OP_fucomi, /**< fucomi opcode */
/* 471 */     OP_fcomi, /**< fcomi opcode */
/* 472 */     OP_ffree, /**< ffree opcode */
/* 473 */     OP_fucom, /**< fucom opcode */
/* 474 */     OP_fucomp, /**< fucomp opcode */
/* 475 */     OP_faddp, /**< faddp opcode */
/* 476 */     OP_fmulp, /**< fmulp opcode */
/* 477 */     OP_fcompp, /**< fcompp opcode */
/* 478 */     OP_fsubrp, /**< fsubrp opcode */
/* 479 */     OP_fsubp, /**< fsubp opcode */
/* 480 */     OP_fdivrp, /**< fdivrp opcode */
/* 481 */     OP_fdivp, /**< fdivp opcode */
/* 482 */     OP_fucomip, /**< fucomip opcode */
/* 483 */     OP_fcomip, /**< fcomip opcode */

    /* SSE3 instructions */
/* 484 */     OP_fisttp, /**< fisttp opcode */
/* 485 */     OP_haddpd, /**< haddpd opcode */
/* 486 */     OP_haddps, /**< haddps opcode */
/* 487 */     OP_hsubpd, /**< hsubpd opcode */
/* 488 */     OP_hsubps, /**< hsubps opcode */
/* 489 */     OP_addsubpd, /**< addsubpd opcode */
/* 490 */     OP_addsubps, /**< addsubps opcode */
/* 491 */     OP_lddqu, /**< lddqu opcode */
/* 492 */     OP_monitor, /**< monitor opcode */
/* 493 */     OP_mwait, /**< mwait opcode */
/* 494 */     OP_movsldup, /**< movsldup opcode */
/* 495 */     OP_movshdup, /**< movshdup opcode */
/* 496 */     OP_movddup, /**< movddup opcode */

    /* 3D-Now! instructions */
/* 497 */     OP_femms, /**< femms opcode */
/* 498 */     OP_unknown_3dnow, /**< unknown_3dnow opcode */
/* 499 */     OP_pavgusb, /**< pavgusb opcode */
/* 500 */     OP_pfadd, /**< pfadd opcode */
/* 501 */     OP_pfacc, /**< pfacc opcode */
/* 502 */     OP_pfcmpge, /**< pfcmpge opcode */
/* 503 */     OP_pfcmpgt, /**< pfcmpgt opcode */
/* 504 */     OP_pfcmpeq, /**< pfcmpeq opcode */
/* 505 */     OP_pfmin, /**< pfmin opcode */
/* 506 */     OP_pfmax, /**< pfmax opcode */
/* 507 */     OP_pfmul, /**< pfmul opcode */
/* 508 */     OP_pfrcp, /**< pfrcp opcode */
/* 509 */     OP_pfrcpit1, /**< pfrcpit1 opcode */
/* 510 */     OP_pfrcpit2, /**< pfrcpit2 opcode */
/* 511 */     OP_pfrsqrt, /**< pfrsqrt opcode */
/* 512 */     OP_pfrsqit1, /**< pfrsqit1 opcode */
/* 513 */     OP_pmulhrw, /**< pmulhrw opcode */
/* 514 */     OP_pfsub, /**< pfsub opcode */
/* 515 */     OP_pfsubr, /**< pfsubr opcode */
/* 516 */     OP_pi2fd, /**< pi2fd opcode */
/* 517 */     OP_pf2id, /**< pf2id opcode */
/* 518 */     OP_pi2fw, /**< pi2fw opcode */
/* 519 */     OP_pf2iw, /**< pf2iw opcode */
/* 520 */     OP_pfnacc, /**< pfnacc opcode */
/* 521 */     OP_pfpnacc, /**< pfpnacc opcode */
/* 522 */     OP_pswapd, /**< pswapd opcode */

    /* SSSE3 */
/* 523 */     OP_pshufb, /**< pshufb opcode */
/* 524 */     OP_phaddw, /**< phaddw opcode */
/* 525 */     OP_phaddd, /**< phaddd opcode */
/* 526 */     OP_phaddsw, /**< phaddsw opcode */
/* 527 */     OP_pmaddubsw, /**< pmaddubsw opcode */
/* 528 */     OP_phsubw, /**< phsubw opcode */
/* 529 */     OP_phsubd, /**< phsubd opcode */
/* 530 */     OP_phsubsw, /**< phsubsw opcode */
/* 531 */     OP_psignb, /**< psignb opcode */
/* 532 */     OP_psignw, /**< psignw opcode */
/* 533 */     OP_psignd, /**< psignd opcode */
/* 534 */     OP_pmulhrsw, /**< pmulhrsw opcode */
/* 535 */     OP_pabsb, /**< pabsb opcode */
/* 536 */     OP_pabsw, /**< pabsw opcode */
/* 537 */     OP_pabsd, /**< pabsd opcode */
/* 538 */     OP_palignr, /**< palignr opcode */

    /* SSE4 (incl AMD (SSE4A) and Intel-specific (SSE4.1, SSE4.2) extensions */
/* 539 */     OP_popcnt, /**< popcnt opcode */
/* 540 */     OP_movntss, /**< movntss opcode */
/* 541 */     OP_movntsd, /**< movntsd opcode */
/* 542 */     OP_extrq, /**< extrq opcode */
/* 543 */     OP_insertq, /**< insertq opcode */
/* 544 */     OP_lzcnt, /**< lzcnt opcode */
/* 545 */     OP_pblendvb, /**< pblendvb opcode */
/* 546 */     OP_blendvps, /**< blendvps opcode */
/* 547 */     OP_blendvpd, /**< blendvpd opcode */
/* 548 */     OP_ptest, /**< ptest opcode */
/* 549 */     OP_pmovsxbw, /**< pmovsxbw opcode */
/* 550 */     OP_pmovsxbd, /**< pmovsxbd opcode */
/* 551 */     OP_pmovsxbq, /**< pmovsxbq opcode */
/* 552 */     OP_pmovsxwd, /**< pmovsxwd opcode */
/* 553 */     OP_pmovsxwq, /**< pmovsxwq opcode */
/* 554 */     OP_pmovsxdq, /**< pmovsxdq opcode */
/* 555 */     OP_pmuldq, /**< pmuldq opcode */
/* 556 */     OP_pcmpeqq, /**< pcmpeqq opcode */
/* 557 */     OP_movntdqa, /**< movntdqa opcode */
/* 558 */     OP_packusdw, /**< packusdw opcode */
/* 559 */     OP_pmovzxbw, /**< pmovzxbw opcode */
/* 560 */     OP_pmovzxbd, /**< pmovzxbd opcode */
/* 561 */     OP_pmovzxbq, /**< pmovzxbq opcode */
/* 562 */     OP_pmovzxwd, /**< pmovzxwd opcode */
/* 563 */     OP_pmovzxwq, /**< pmovzxwq opcode */
/* 564 */     OP_pmovzxdq, /**< pmovzxdq opcode */
/* 565 */     OP_pcmpgtq, /**< pcmpgtq opcode */
/* 566 */     OP_pminsb, /**< pminsb opcode */
/* 567 */     OP_pminsd, /**< pminsd opcode */
/* 568 */     OP_pminuw, /**< pminuw opcode */
/* 569 */     OP_pminud, /**< pminud opcode */
/* 570 */     OP_pmaxsb, /**< pmaxsb opcode */
/* 571 */     OP_pmaxsd, /**< pmaxsd opcode */
/* 572 */     OP_pmaxuw, /**< pmaxuw opcode */
/* 573 */     OP_pmaxud, /**< pmaxud opcode */
/* 574 */     OP_pmulld, /**< pmulld opcode */
/* 575 */     OP_phminposuw, /**< phminposuw opcode */
/* 576 */     OP_crc32, /**< crc32 opcode */
/* 577 */     OP_pextrb, /**< pextrb opcode */
/* 578 */     OP_pextrd, /**< pextrd opcode */
/* 579 */     OP_extractps, /**< extractps opcode */
/* 580 */     OP_roundps, /**< roundps opcode */
/* 581 */     OP_roundpd, /**< roundpd opcode */
/* 582 */     OP_roundss, /**< roundss opcode */
/* 583 */     OP_roundsd, /**< roundsd opcode */
/* 584 */     OP_blendps, /**< blendps opcode */
/* 585 */     OP_blendpd, /**< blendpd opcode */
/* 586 */     OP_pblendw, /**< pblendw opcode */
/* 587 */     OP_pinsrb, /**< pinsrb opcode */
/* 588 */     OP_insertps, /**< insertps opcode */
/* 589 */     OP_pinsrd, /**< pinsrd opcode */
/* 590 */     OP_dpps, /**< dpps opcode */
/* 591 */     OP_dppd, /**< dppd opcode */
/* 592 */     OP_mpsadbw, /**< mpsadbw opcode */
/* 593 */     OP_pcmpestrm, /**< pcmpestrm opcode */
/* 594 */     OP_pcmpestri, /**< pcmpestri opcode */
/* 595 */     OP_pcmpistrm, /**< pcmpistrm opcode */
/* 596 */     OP_pcmpistri, /**< pcmpistri opcode */

    /* x64 */
/* 597 */     OP_movsxd, /**< movsxd opcode */
/* 598 */     OP_swapgs, /**< swapgs opcode */

    /* VMX */
/* 599 */     OP_vmcall, /**< vmcall opcode */
/* 600 */     OP_vmlaunch, /**< vmlaunch opcode */
/* 601 */     OP_vmresume, /**< vmresume opcode */
/* 602 */     OP_vmxoff, /**< vmxoff opcode */
/* 603 */     OP_vmptrst, /**< vmptrst opcode */
/* 604 */     OP_vmptrld, /**< vmptrld opcode */
/* 605 */     OP_vmxon, /**< vmxon opcode */
/* 606 */     OP_vmclear, /**< vmclear opcode */
/* 607 */     OP_vmread, /**< vmread opcode */
/* 608 */     OP_vmwrite, /**< vmwrite opcode */

    /* undocumented */
/* 609 */     OP_int1, /**< int1 opcode */
/* 610 */     OP_salc, /**< salc opcode */
/* 611 */     OP_ffreep, /**< ffreep opcode */

    /* AMD SVM */
/* 612 */     OP_vmrun, /**< vmrun opcode */
/* 613 */     OP_vmmcall, /**< vmmcall opcode */
/* 614 */     OP_vmload, /**< vmload opcode */
/* 615 */     OP_vmsave, /**< vmsave opcode */
/* 616 */     OP_stgi, /**< stgi opcode */
/* 617 */     OP_clgi, /**< clgi opcode */
/* 618 */     OP_skinit, /**< skinit opcode */
/* 619 */     OP_invlpga, /**< invlpga opcode */
    /* AMD though not part of SVM */
/* 620 */     OP_rdtscp, /**< rdtscp opcode */

    /* Intel VMX additions */
/* 621 */     OP_invept, /**< invept opcode */
/* 622 */     OP_invvpid, /**< invvpid opcode */

    /* added in Intel Westmere */
/* 623 */     OP_pclmulqdq, /**< pclmulqdq opcode */
/* 624 */     OP_aesimc, /**< aesimc opcode */
/* 625 */     OP_aesenc, /**< aesenc opcode */
/* 626 */     OP_aesenclast, /**< aesenclast opcode */
/* 627 */     OP_aesdec, /**< aesdec opcode */
/* 628 */     OP_aesdeclast, /**< aesdeclast opcode */
/* 629 */     OP_aeskeygenassist, /**< aeskeygenassist opcode */

    /* added in Intel Atom */
/* 630 */     OP_movbe, /**< movbe opcode */

    /* added in Intel Sandy Bridge */
/* 631 */     OP_xgetbv, /**< xgetbv opcode */
/* 632 */     OP_xsetbv, /**< xsetbv opcode */
/* 633 */     OP_xsave32, /**< xsave opcode */
/* 634 */     OP_xrstor32, /**< xrstor opcode */
/* 635 */     OP_xsaveopt32, /**< xsaveopt opcode */

    /* AVX */
/* 636 */     OP_vmovss, /**< vmovss opcode */
/* 637 */     OP_vmovsd, /**< vmovsd opcode */
/* 638 */     OP_vmovups, /**< vmovups opcode */
/* 639 */     OP_vmovupd, /**< vmovupd opcode */
/* 640 */     OP_vmovlps, /**< vmovlps opcode */
/* 641 */     OP_vmovsldup, /**< vmovsldup opcode */
/* 642 */     OP_vmovlpd, /**< vmovlpd opcode */
/* 643 */     OP_vmovddup, /**< vmovddup opcode */
/* 644 */     OP_vunpcklps, /**< vunpcklps opcode */
/* 645 */     OP_vunpcklpd, /**< vunpcklpd opcode */
/* 646 */     OP_vunpckhps, /**< vunpckhps opcode */
/* 647 */     OP_vunpckhpd, /**< vunpckhpd opcode */
/* 648 */     OP_vmovhps, /**< vmovhps opcode */
/* 649 */     OP_vmovshdup, /**< vmovshdup opcode */
/* 650 */     OP_vmovhpd, /**< vmovhpd opcode */
/* 651 */     OP_vmovaps, /**< vmovaps opcode */
/* 652 */     OP_vmovapd, /**< vmovapd opcode */
/* 653 */     OP_vcvtsi2ss, /**< vcvtsi2ss opcode */
/* 654 */     OP_vcvtsi2sd, /**< vcvtsi2sd opcode */
/* 655 */     OP_vmovntps, /**< vmovntps opcode */
/* 656 */     OP_vmovntpd, /**< vmovntpd opcode */
/* 657 */     OP_vcvttss2si, /**< vcvttss2si opcode */
/* 658 */     OP_vcvttsd2si, /**< vcvttsd2si opcode */
/* 659 */     OP_vcvtss2si, /**< vcvtss2si opcode */
/* 660 */     OP_vcvtsd2si, /**< vcvtsd2si opcode */
/* 661 */     OP_vucomiss, /**< vucomiss opcode */
/* 662 */     OP_vucomisd, /**< vucomisd opcode */
/* 663 */     OP_vcomiss, /**< vcomiss opcode */
/* 664 */     OP_vcomisd, /**< vcomisd opcode */
/* 665 */     OP_vmovmskps, /**< vmovmskps opcode */
/* 666 */     OP_vmovmskpd, /**< vmovmskpd opcode */
/* 667 */     OP_vsqrtps, /**< vsqrtps opcode */
/* 668 */     OP_vsqrtss, /**< vsqrtss opcode */
/* 669 */     OP_vsqrtpd, /**< vsqrtpd opcode */
/* 670 */     OP_vsqrtsd, /**< vsqrtsd opcode */
/* 671 */     OP_vrsqrtps, /**< vrsqrtps opcode */
/* 672 */     OP_vrsqrtss, /**< vrsqrtss opcode */
/* 673 */     OP_vrcpps, /**< vrcpps opcode */
/* 674 */     OP_vrcpss, /**< vrcpss opcode */
/* 675 */     OP_vandps, /**< vandps opcode */
/* 676 */     OP_vandpd, /**< vandpd opcode */
/* 677 */     OP_vandnps, /**< vandnps opcode */
/* 678 */     OP_vandnpd, /**< vandnpd opcode */
/* 679 */     OP_vorps, /**< vorps opcode */
/* 680 */     OP_vorpd, /**< vorpd opcode */
/* 681 */     OP_vxorps, /**< vxorps opcode */
/* 682 */     OP_vxorpd, /**< vxorpd opcode */
/* 683 */     OP_vaddps, /**< vaddps opcode */
/* 684 */     OP_vaddss, /**< vaddss opcode */
/* 685 */     OP_vaddpd, /**< vaddpd opcode */
/* 686 */     OP_vaddsd, /**< vaddsd opcode */
/* 687 */     OP_vmulps, /**< vmulps opcode */
/* 688 */     OP_vmulss, /**< vmulss opcode */
/* 689 */     OP_vmulpd, /**< vmulpd opcode */
/* 690 */     OP_vmulsd, /**< vmulsd opcode */
/* 691 */     OP_vcvtps2pd, /**< vcvtps2pd opcode */
/* 692 */     OP_vcvtss2sd, /**< vcvtss2sd opcode */
/* 693 */     OP_vcvtpd2ps, /**< vcvtpd2ps opcode */
/* 694 */     OP_vcvtsd2ss, /**< vcvtsd2ss opcode */
/* 695 */     OP_vcvtdq2ps, /**< vcvtdq2ps opcode */
/* 696 */     OP_vcvttps2dq, /**< vcvttps2dq opcode */
/* 697 */     OP_vcvtps2dq, /**< vcvtps2dq opcode */
/* 698 */     OP_vsubps, /**< vsubps opcode */
/* 699 */     OP_vsubss, /**< vsubss opcode */
/* 700 */     OP_vsubpd, /**< vsubpd opcode */
/* 701 */     OP_vsubsd, /**< vsubsd opcode */
/* 702 */     OP_vminps, /**< vminps opcode */
/* 703 */     OP_vminss, /**< vminss opcode */
/* 704 */     OP_vminpd, /**< vminpd opcode */
/* 705 */     OP_vminsd, /**< vminsd opcode */
/* 706 */     OP_vdivps, /**< vdivps opcode */
/* 707 */     OP_vdivss, /**< vdivss opcode */
/* 708 */     OP_vdivpd, /**< vdivpd opcode */
/* 709 */     OP_vdivsd, /**< vdivsd opcode */
/* 710 */     OP_vmaxps, /**< vmaxps opcode */
/* 711 */     OP_vmaxss, /**< vmaxss opcode */
/* 712 */     OP_vmaxpd, /**< vmaxpd opcode */
/* 713 */     OP_vmaxsd, /**< vmaxsd opcode */
/* 714 */     OP_vpunpcklbw, /**< vpunpcklbw opcode */
/* 715 */     OP_vpunpcklwd, /**< vpunpcklwd opcode */
/* 716 */     OP_vpunpckldq, /**< vpunpckldq opcode */
/* 717 */     OP_vpacksswb, /**< vpacksswb opcode */
/* 718 */     OP_vpcmpgtb, /**< vpcmpgtb opcode */
/* 719 */     OP_vpcmpgtw, /**< vpcmpgtw opcode */
/* 720 */     OP_vpcmpgtd, /**< vpcmpgtd opcode */
/* 721 */     OP_vpackuswb, /**< vpackuswb opcode */
/* 722 */     OP_vpunpckhbw, /**< vpunpckhbw opcode */
/* 723 */     OP_vpunpckhwd, /**< vpunpckhwd opcode */
/* 724 */     OP_vpunpckhdq, /**< vpunpckhdq opcode */
/* 725 */     OP_vpackssdw, /**< vpackssdw opcode */
/* 726 */     OP_vpunpcklqdq, /**< vpunpcklqdq opcode */
/* 727 */     OP_vpunpckhqdq, /**< vpunpckhqdq opcode */
/* 728 */     OP_vmovd, /**< vmovd opcode */
/* 729 */     OP_vpshufhw, /**< vpshufhw opcode */
/* 730 */     OP_vpshufd, /**< vpshufd opcode */
/* 731 */     OP_vpshuflw, /**< vpshuflw opcode */
/* 732 */     OP_vpcmpeqb, /**< vpcmpeqb opcode */
/* 733 */     OP_vpcmpeqw, /**< vpcmpeqw opcode */
/* 734 */     OP_vpcmpeqd, /**< vpcmpeqd opcode */
/* 735 */     OP_vmovq, /**< vmovq opcode */
/* 736 */     OP_vcmpps, /**< vcmpps opcode */
/* 737 */     OP_vcmpss, /**< vcmpss opcode */
/* 738 */     OP_vcmppd, /**< vcmppd opcode */
/* 739 */     OP_vcmpsd, /**< vcmpsd opcode */
/* 740 */     OP_vpinsrw, /**< vpinsrw opcode */
/* 741 */     OP_vpextrw, /**< vpextrw opcode */
/* 742 */     OP_vshufps, /**< vshufps opcode */
/* 743 */     OP_vshufpd, /**< vshufpd opcode */
/* 744 */     OP_vpsrlw, /**< vpsrlw opcode */
/* 745 */     OP_vpsrld, /**< vpsrld opcode */
/* 746 */     OP_vpsrlq, /**< vpsrlq opcode */
/* 747 */     OP_vpaddq, /**< vpaddq opcode */
/* 748 */     OP_vpmullw, /**< vpmullw opcode */
/* 749 */     OP_vpmovmskb, /**< vpmovmskb opcode */
/* 750 */     OP_vpsubusb, /**< vpsubusb opcode */
/* 751 */     OP_vpsubusw, /**< vpsubusw opcode */
/* 752 */     OP_vpminub, /**< vpminub opcode */
/* 753 */     OP_vpand, /**< vpand opcode */
/* 754 */     OP_vpaddusb, /**< vpaddusb opcode */
/* 755 */     OP_vpaddusw, /**< vpaddusw opcode */
/* 756 */     OP_vpmaxub, /**< vpmaxub opcode */
/* 757 */     OP_vpandn, /**< vpandn opcode */
/* 758 */     OP_vpavgb, /**< vpavgb opcode */
/* 759 */     OP_vpsraw, /**< vpsraw opcode */
/* 760 */     OP_vpsrad, /**< vpsrad opcode */
/* 761 */     OP_vpavgw, /**< vpavgw opcode */
/* 762 */     OP_vpmulhuw, /**< vpmulhuw opcode */
/* 763 */     OP_vpmulhw, /**< vpmulhw opcode */
/* 764 */     OP_vcvtdq2pd, /**< vcvtdq2pd opcode */
/* 765 */     OP_vcvttpd2dq, /**< vcvttpd2dq opcode */
/* 766 */     OP_vcvtpd2dq, /**< vcvtpd2dq opcode */
/* 767 */     OP_vmovntdq, /**< vmovntdq opcode */
/* 768 */     OP_vpsubsb, /**< vpsubsb opcode */
/* 769 */     OP_vpsubsw, /**< vpsubsw opcode */
/* 770 */     OP_vpminsw, /**< vpminsw opcode */
/* 771 */     OP_vpor, /**< vpor opcode */
/* 772 */     OP_vpaddsb, /**< vpaddsb opcode */
/* 773 */     OP_vpaddsw, /**< vpaddsw opcode */
/* 774 */     OP_vpmaxsw, /**< vpmaxsw opcode */
/* 775 */     OP_vpxor, /**< vpxor opcode */
/* 776 */     OP_vpsllw, /**< vpsllw opcode */
/* 777 */     OP_vpslld, /**< vpslld opcode */
/* 778 */     OP_vpsllq, /**< vpsllq opcode */
/* 779 */     OP_vpmuludq, /**< vpmuludq opcode */
/* 780 */     OP_vpmaddwd, /**< vpmaddwd opcode */
/* 781 */     OP_vpsadbw, /**< vpsadbw opcode */
/* 782 */     OP_vmaskmovdqu, /**< vmaskmovdqu opcode */
/* 783 */     OP_vpsubb, /**< vpsubb opcode */
/* 784 */     OP_vpsubw, /**< vpsubw opcode */
/* 785 */     OP_vpsubd, /**< vpsubd opcode */
/* 786 */     OP_vpsubq, /**< vpsubq opcode */
/* 787 */     OP_vpaddb, /**< vpaddb opcode */
/* 788 */     OP_vpaddw, /**< vpaddw opcode */
/* 789 */     OP_vpaddd, /**< vpaddd opcode */
/* 790 */     OP_vpsrldq, /**< vpsrldq opcode */
/* 791 */     OP_vpslldq, /**< vpslldq opcode */
/* 792 */     OP_vmovdqu, /**< vmovdqu opcode */
/* 793 */     OP_vmovdqa, /**< vmovdqa opcode */
/* 794 */     OP_vhaddpd, /**< vhaddpd opcode */
/* 795 */     OP_vhaddps, /**< vhaddps opcode */
/* 796 */     OP_vhsubpd, /**< vhsubpd opcode */
/* 797 */     OP_vhsubps, /**< vhsubps opcode */
/* 798 */     OP_vaddsubpd, /**< vaddsubpd opcode */
/* 799 */     OP_vaddsubps, /**< vaddsubps opcode */
/* 800 */     OP_vlddqu, /**< vlddqu opcode */
/* 801 */     OP_vpshufb, /**< vpshufb opcode */
/* 802 */     OP_vphaddw, /**< vphaddw opcode */
/* 803 */     OP_vphaddd, /**< vphaddd opcode */
/* 804 */     OP_vphaddsw, /**< vphaddsw opcode */
/* 805 */     OP_vpmaddubsw, /**< vpmaddubsw opcode */
/* 806 */     OP_vphsubw, /**< vphsubw opcode */
/* 807 */     OP_vphsubd, /**< vphsubd opcode */
/* 808 */     OP_vphsubsw, /**< vphsubsw opcode */
/* 809 */     OP_vpsignb, /**< vpsignb opcode */
/* 810 */     OP_vpsignw, /**< vpsignw opcode */
/* 811 */     OP_vpsignd, /**< vpsignd opcode */
/* 812 */     OP_vpmulhrsw, /**< vpmulhrsw opcode */
/* 813 */     OP_vpabsb, /**< vpabsb opcode */
/* 814 */     OP_vpabsw, /**< vpabsw opcode */
/* 815 */     OP_vpabsd, /**< vpabsd opcode */
/* 816 */     OP_vpalignr, /**< vpalignr opcode */
/* 817 */     OP_vpblendvb, /**< vpblendvb opcode */
/* 818 */     OP_vblendvps, /**< vblendvps opcode */
/* 819 */     OP_vblendvpd, /**< vblendvpd opcode */
/* 820 */     OP_vptest, /**< vptest opcode */
/* 821 */     OP_vpmovsxbw, /**< vpmovsxbw opcode */
/* 822 */     OP_vpmovsxbd, /**< vpmovsxbd opcode */
/* 823 */     OP_vpmovsxbq, /**< vpmovsxbq opcode */
/* 824 */     OP_vpmovsxwd, /**< vpmovsxwd opcode */
/* 825 */     OP_vpmovsxwq, /**< vpmovsxwq opcode */
/* 826 */     OP_vpmovsxdq, /**< vpmovsxdq opcode */
/* 827 */     OP_vpmuldq, /**< vpmuldq opcode */
/* 828 */     OP_vpcmpeqq, /**< vpcmpeqq opcode */
/* 829 */     OP_vmovntdqa, /**< vmovntdqa opcode */
/* 830 */     OP_vpackusdw, /**< vpackusdw opcode */
/* 831 */     OP_vpmovzxbw, /**< vpmovzxbw opcode */
/* 832 */     OP_vpmovzxbd, /**< vpmovzxbd opcode */
/* 833 */     OP_vpmovzxbq, /**< vpmovzxbq opcode */
/* 834 */     OP_vpmovzxwd, /**< vpmovzxwd opcode */
/* 835 */     OP_vpmovzxwq, /**< vpmovzxwq opcode */
/* 836 */     OP_vpmovzxdq, /**< vpmovzxdq opcode */
/* 837 */     OP_vpcmpgtq, /**< vpcmpgtq opcode */
/* 838 */     OP_vpminsb, /**< vpminsb opcode */
/* 839 */     OP_vpminsd, /**< vpminsd opcode */
/* 840 */     OP_vpminuw, /**< vpminuw opcode */
/* 841 */     OP_vpminud, /**< vpminud opcode */
/* 842 */     OP_vpmaxsb, /**< vpmaxsb opcode */
/* 843 */     OP_vpmaxsd, /**< vpmaxsd opcode */
/* 844 */     OP_vpmaxuw, /**< vpmaxuw opcode */
/* 845 */     OP_vpmaxud, /**< vpmaxud opcode */
/* 846 */     OP_vpmulld, /**< vpmulld opcode */
/* 847 */     OP_vphminposuw, /**< vphminposuw opcode */
/* 848 */     OP_vaesimc, /**< vaesimc opcode */
/* 849 */     OP_vaesenc, /**< vaesenc opcode */
/* 850 */     OP_vaesenclast, /**< vaesenclast opcode */
/* 851 */     OP_vaesdec, /**< vaesdec opcode */
/* 852 */     OP_vaesdeclast, /**< vaesdeclast opcode */
/* 853 */     OP_vpextrb, /**< vpextrb opcode */
/* 854 */     OP_vpextrd, /**< vpextrd opcode */
/* 855 */     OP_vextractps, /**< vextractps opcode */
/* 856 */     OP_vroundps, /**< vroundps opcode */
/* 857 */     OP_vroundpd, /**< vroundpd opcode */
/* 858 */     OP_vroundss, /**< vroundss opcode */
/* 859 */     OP_vroundsd, /**< vroundsd opcode */
/* 860 */     OP_vblendps, /**< vblendps opcode */
/* 861 */     OP_vblendpd, /**< vblendpd opcode */
/* 862 */     OP_vpblendw, /**< vpblendw opcode */
/* 863 */     OP_vpinsrb, /**< vpinsrb opcode */
/* 864 */     OP_vinsertps, /**< vinsertps opcode */
/* 865 */     OP_vpinsrd, /**< vpinsrd opcode */
/* 866 */     OP_vdpps, /**< vdpps opcode */
/* 867 */     OP_vdppd, /**< vdppd opcode */
/* 868 */     OP_vmpsadbw, /**< vmpsadbw opcode */
/* 869 */     OP_vpcmpestrm, /**< vpcmpestrm opcode */
/* 870 */     OP_vpcmpestri, /**< vpcmpestri opcode */
/* 871 */     OP_vpcmpistrm, /**< vpcmpistrm opcode */
/* 872 */     OP_vpcmpistri, /**< vpcmpistri opcode */
/* 873 */     OP_vpclmulqdq, /**< vpclmulqdq opcode */
/* 874 */     OP_vaeskeygenassist, /**< vaeskeygenassist opcode */
/* 875 */     OP_vtestps, /**< vtestps opcode */
/* 876 */     OP_vtestpd, /**< vtestpd opcode */
/* 877 */     OP_vzeroupper, /**< vzeroupper opcode */
/* 878 */     OP_vzeroall, /**< vzeroall opcode */
/* 879 */     OP_vldmxcsr, /**< vldmxcsr opcode */
/* 880 */     OP_vstmxcsr, /**< vstmxcsr opcode */
/* 881 */     OP_vbroadcastss, /**< vbroadcastss opcode */
/* 882 */     OP_vbroadcastsd, /**< vbroadcastsd opcode */
/* 883 */     OP_vbroadcastf128, /**< vbroadcastf128 opcode */
/* 884 */     OP_vmaskmovps, /**< vmaskmovps opcode */
/* 885 */     OP_vmaskmovpd, /**< vmaskmovpd opcode */
/* 886 */     OP_vpermilps, /**< vpermilps opcode */
/* 887 */     OP_vpermilpd, /**< vpermilpd opcode */
/* 888 */     OP_vperm2f128, /**< vperm2f128 opcode */
/* 889 */     OP_vinsertf128, /**< vinsertf128 opcode */
/* 890 */     OP_vextractf128, /**< vextractf128 opcode */

    /* added in Ivy Bridge I believe, and covered by F16C cpuid flag */
/* 891 */     OP_vcvtph2ps, /**< vcvtph2ps opcode */
/* 892 */     OP_vcvtps2ph, /**< vcvtps2ph opcode */

    /* FMA */
/* 893 */     OP_vfmadd132ps, /**< vfmadd132ps opcode */
/* 894 */     OP_vfmadd132pd, /**< vfmadd132pd opcode */
/* 895 */     OP_vfmadd213ps, /**< vfmadd213ps opcode */
/* 896 */     OP_vfmadd213pd, /**< vfmadd213pd opcode */
/* 897 */     OP_vfmadd231ps, /**< vfmadd231ps opcode */
/* 898 */     OP_vfmadd231pd, /**< vfmadd231pd opcode */
/* 899 */     OP_vfmadd132ss, /**< vfmadd132ss opcode */
/* 900 */     OP_vfmadd132sd, /**< vfmadd132sd opcode */
/* 901 */     OP_vfmadd213ss, /**< vfmadd213ss opcode */
/* 902 */     OP_vfmadd213sd, /**< vfmadd213sd opcode */
/* 903 */     OP_vfmadd231ss, /**< vfmadd231ss opcode */
/* 904 */     OP_vfmadd231sd, /**< vfmadd231sd opcode */
/* 905 */     OP_vfmaddsub132ps, /**< vfmaddsub132ps opcode */
/* 906 */     OP_vfmaddsub132pd, /**< vfmaddsub132pd opcode */
/* 907 */     OP_vfmaddsub213ps, /**< vfmaddsub213ps opcode */
/* 908 */     OP_vfmaddsub213pd, /**< vfmaddsub213pd opcode */
/* 909 */     OP_vfmaddsub231ps, /**< vfmaddsub231ps opcode */
/* 910 */     OP_vfmaddsub231pd, /**< vfmaddsub231pd opcode */
/* 911 */     OP_vfmsubadd132ps, /**< vfmsubadd132ps opcode */
/* 912 */     OP_vfmsubadd132pd, /**< vfmsubadd132pd opcode */
/* 913 */     OP_vfmsubadd213ps, /**< vfmsubadd213ps opcode */
/* 914 */     OP_vfmsubadd213pd, /**< vfmsubadd213pd opcode */
/* 915 */     OP_vfmsubadd231ps, /**< vfmsubadd231ps opcode */
/* 916 */     OP_vfmsubadd231pd, /**< vfmsubadd231pd opcode */
/* 917 */     OP_vfmsub132ps, /**< vfmsub132ps opcode */
/* 918 */     OP_vfmsub132pd, /**< vfmsub132pd opcode */
/* 919 */     OP_vfmsub213ps, /**< vfmsub213ps opcode */
/* 920 */     OP_vfmsub213pd, /**< vfmsub213pd opcode */
/* 921 */     OP_vfmsub231ps, /**< vfmsub231ps opcode */
/* 922 */     OP_vfmsub231pd, /**< vfmsub231pd opcode */
/* 923 */     OP_vfmsub132ss, /**< vfmsub132ss opcode */
/* 924 */     OP_vfmsub132sd, /**< vfmsub132sd opcode */
/* 925 */     OP_vfmsub213ss, /**< vfmsub213ss opcode */
/* 926 */     OP_vfmsub213sd, /**< vfmsub213sd opcode */
/* 927 */     OP_vfmsub231ss, /**< vfmsub231ss opcode */
/* 928 */     OP_vfmsub231sd, /**< vfmsub231sd opcode */
/* 929 */     OP_vfnmadd132ps, /**< vfnmadd132ps opcode */
/* 930 */     OP_vfnmadd132pd, /**< vfnmadd132pd opcode */
/* 931 */     OP_vfnmadd213ps, /**< vfnmadd213ps opcode */
/* 932 */     OP_vfnmadd213pd, /**< vfnmadd213pd opcode */
/* 933 */     OP_vfnmadd231ps, /**< vfnmadd231ps opcode */
/* 934 */     OP_vfnmadd231pd, /**< vfnmadd231pd opcode */
/* 935 */     OP_vfnmadd132ss, /**< vfnmadd132ss opcode */
/* 936 */     OP_vfnmadd132sd, /**< vfnmadd132sd opcode */
/* 937 */     OP_vfnmadd213ss, /**< vfnmadd213ss opcode */
/* 938 */     OP_vfnmadd213sd, /**< vfnmadd213sd opcode */
/* 939 */     OP_vfnmadd231ss, /**< vfnmadd231ss opcode */
/* 940 */     OP_vfnmadd231sd, /**< vfnmadd231sd opcode */
/* 941 */     OP_vfnmsub132ps, /**< vfnmsub132ps opcode */
/* 942 */     OP_vfnmsub132pd, /**< vfnmsub132pd opcode */
/* 943 */     OP_vfnmsub213ps, /**< vfnmsub213ps opcode */
/* 944 */     OP_vfnmsub213pd, /**< vfnmsub213pd opcode */
/* 945 */     OP_vfnmsub231ps, /**< vfnmsub231ps opcode */
/* 946 */     OP_vfnmsub231pd, /**< vfnmsub231pd opcode */
/* 947 */     OP_vfnmsub132ss, /**< vfnmsub132ss opcode */
/* 948 */     OP_vfnmsub132sd, /**< vfnmsub132sd opcode */
/* 949 */     OP_vfnmsub213ss, /**< vfnmsub213ss opcode */
/* 950 */     OP_vfnmsub213sd, /**< vfnmsub213sd opcode */
/* 951 */     OP_vfnmsub231ss, /**< vfnmsub231ss opcode */
/* 952 */     OP_vfnmsub231sd, /**< vfnmsub231sd opcode */

/* 953 */     OP_movq2dq, /**< movq2dq opcode */
/* 954 */     OP_movdq2q, /**< movdq2q opcode */

/* 955 */     OP_fxsave64, /**< fxsave64 opcode */
/* 956 */     OP_fxrstor64, /**< fxrstor64 opcode */
/* 957 */     OP_xsave64, /**< xsave64 opcode */
/* 958 */     OP_xrstor64, /**< xrstor64 opcode */
/* 959 */     OP_xsaveopt64, /**< xsaveopt64 opcode */

    /* added in Intel Ivy Bridge: RDRAND and FSGSBASE cpuid flags */
/* 960 */     OP_rdrand, /**< rdrand opcode */
/* 961 */     OP_rdfsbase, /**< rdfsbase opcode */
/* 962 */     OP_rdgsbase, /**< rdgsbase opcode */
/* 963 */     OP_wrfsbase, /**< wrfsbase opcode */
/* 964 */     OP_wrgsbase, /**< wrgsbase opcode */

    /* coming in the future but adding now since enough details are known */
/* 965 */     OP_rdseed, /**< rdseed opcode */

    /* AMD FMA4 */
/* 966 */     OP_vfmaddsubps, /**< vfmaddsubps opcode */
/* 967 */     OP_vfmaddsubpd, /**< vfmaddsubpd opcode */
/* 968 */     OP_vfmsubaddps, /**< vfmsubaddps opcode */
/* 969 */     OP_vfmsubaddpd, /**< vfmsubaddpd opcode */
/* 970 */     OP_vfmaddps, /**< vfmaddps opcode */
/* 971 */     OP_vfmaddpd, /**< vfmaddpd opcode */
/* 972 */     OP_vfmaddss, /**< vfmaddss opcode */
/* 973 */     OP_vfmaddsd, /**< vfmaddsd opcode */
/* 974 */     OP_vfmsubps, /**< vfmsubps opcode */
/* 975 */     OP_vfmsubpd, /**< vfmsubpd opcode */
/* 976 */     OP_vfmsubss, /**< vfmsubss opcode */
/* 977 */     OP_vfmsubsd, /**< vfmsubsd opcode */
/* 978 */     OP_vfnmaddps, /**< vfnmaddps opcode */
/* 979 */     OP_vfnmaddpd, /**< vfnmaddpd opcode */
/* 980 */     OP_vfnmaddss, /**< vfnmaddss opcode */
/* 981 */     OP_vfnmaddsd, /**< vfnmaddsd opcode */
/* 982 */     OP_vfnmsubps, /**< vfnmsubps opcode */
/* 983 */     OP_vfnmsubpd, /**< vfnmsubpd opcode */
/* 984 */     OP_vfnmsubss, /**< vfnmsubss opcode */
/* 985 */     OP_vfnmsubsd, /**< vfnmsubsd opcode */

    /* AMD XOP */
/* 986 */     OP_vfrczps, /**< vfrczps opcode */
/* 987 */     OP_vfrczpd, /**< vfrczpd opcode */
/* 988 */     OP_vfrczss, /**< vfrczss opcode */
/* 989 */     OP_vfrczsd, /**< vfrczsd opcode */
/* 990 */     OP_vpcmov, /**< vpcmov opcode */
/* 991 */     OP_vpcomb, /**< vpcomb opcode */
/* 992 */     OP_vpcomw, /**< vpcomw opcode */
/* 993 */     OP_vpcomd, /**< vpcomd opcode */
/* 994 */     OP_vpcomq, /**< vpcomq opcode */
/* 995 */     OP_vpcomub, /**< vpcomub opcode */
/* 996 */     OP_vpcomuw, /**< vpcomuw opcode */
/* 997 */     OP_vpcomud, /**< vpcomud opcode */
/* 998 */     OP_vpcomuq, /**< vpcomuq opcode */
/* 999 */     OP_vpermil2pd, /**< vpermil2pd opcode */
/* 1000 */     OP_vpermil2ps, /**< vpermil2ps opcode */
/* 1001 */     OP_vphaddbw, /**< vphaddbw opcode */
/* 1002 */     OP_vphaddbd, /**< vphaddbd opcode */
/* 1003 */     OP_vphaddbq, /**< vphaddbq opcode */
/* 1004 */     OP_vphaddwd, /**< vphaddwd opcode */
/* 1005 */     OP_vphaddwq, /**< vphaddwq opcode */
/* 1006 */     OP_vphadddq, /**< vphadddq opcode */
/* 1007 */     OP_vphaddubw, /**< vphaddubw opcode */
/* 1008 */     OP_vphaddubd, /**< vphaddubd opcode */
/* 1009 */     OP_vphaddubq, /**< vphaddubq opcode */
/* 1010 */     OP_vphadduwd, /**< vphadduwd opcode */
/* 1011 */     OP_vphadduwq, /**< vphadduwq opcode */
/* 1012 */     OP_vphaddudq, /**< vphaddudq opcode */
/* 1013 */     OP_vphsubbw, /**< vphsubbw opcode */
/* 1014 */     OP_vphsubwd, /**< vphsubwd opcode */
/* 1015 */     OP_vphsubdq, /**< vphsubdq opcode */
/* 1016 */     OP_vpmacssww, /**< vpmacssww opcode */
/* 1017 */     OP_vpmacsswd, /**< vpmacsswd opcode */
/* 1018 */     OP_vpmacssdql, /**< vpmacssdql opcode */
/* 1019 */     OP_vpmacssdd, /**< vpmacssdd opcode */
/* 1020 */     OP_vpmacssdqh, /**< vpmacssdqh opcode */
/* 1021 */     OP_vpmacsww, /**< vpmacsww opcode */
/* 1022 */     OP_vpmacswd, /**< vpmacswd opcode */
/* 1023 */     OP_vpmacsdql, /**< vpmacsdql opcode */
/* 1024 */     OP_vpmacsdd, /**< vpmacsdd opcode */
/* 1025 */     OP_vpmacsdqh, /**< vpmacsdqh opcode */
/* 1026 */     OP_vpmadcsswd, /**< vpmadcsswd opcode */
/* 1027 */     OP_vpmadcswd, /**< vpmadcswd opcode */
/* 1028 */     OP_vpperm, /**< vpperm opcode */
/* 1029 */     OP_vprotb, /**< vprotb opcode */
/* 1030 */     OP_vprotw, /**< vprotw opcode */
/* 1031 */     OP_vprotd, /**< vprotd opcode */
/* 1032 */     OP_vprotq, /**< vprotq opcode */
/* 1033 */     OP_vpshlb, /**< vpshlb opcode */
/* 1034 */     OP_vpshlw, /**< vpshlw opcode */
/* 1035 */     OP_vpshld, /**< vpshld opcode */
/* 1036 */     OP_vpshlq, /**< vpshlq opcode */
/* 1037 */     OP_vpshab, /**< vpshab opcode */
/* 1038 */     OP_vpshaw, /**< vpshaw opcode */
/* 1039 */     OP_vpshad, /**< vpshad opcode */
/* 1040 */     OP_vpshaq, /**< vpshaq opcode */

    /* AMD TBM */
/* 1041 */     OP_bextr, /**< bextr opcode */
/* 1042 */     OP_blcfill, /**< blcfill opcode */
/* 1043 */     OP_blci, /**< blci opcode */
/* 1044 */     OP_blcic, /**< blcic opcode */
/* 1045 */     OP_blcmsk, /**< blcmsk opcode */
/* 1046 */     OP_blcs, /**< blcs opcode */
/* 1047 */     OP_blsfill, /**< blsfill opcode */
/* 1048 */     OP_blsic, /**< blsic opcode */
/* 1049 */     OP_t1mskc, /**< t1mskc opcode */
/* 1050 */     OP_tzmsk, /**< tzmsk opcode */

    /* AMD LWP */
/* 1051 */     OP_llwpcb, /**< llwpcb opcode */
/* 1052 */     OP_slwpcb, /**< slwpcb opcode */
/* 1053 */     OP_lwpins, /**< lwpins opcode */
/* 1054 */     OP_lwpval, /**< lwpval opcode */

    /* Intel BMI1 */
    /* (includes non-immed form of OP_bextr) */
/* 1055 */     OP_andn, /**< andn opcode */
/* 1056 */     OP_blsr, /**< blsr opcode */
/* 1057 */     OP_blsmsk, /**< blsmsk opcode */
/* 1058 */     OP_blsi, /**< blsi opcode */
/* 1059 */     OP_tzcnt, /**< tzcnt opcode */

    /* Intel BMI2 */
/* 1060 */     OP_bzhi, /**< bzhi opcode */
/* 1061 */     OP_pext, /**< pext opcode */
/* 1062 */     OP_pdep, /**< pdep opcode */
/* 1063 */     OP_sarx, /**< sarx opcode */
/* 1064 */     OP_shlx, /**< shlx opcode */
/* 1065 */     OP_shrx, /**< shrx opcode */
/* 1066 */     OP_rorx, /**< rorx opcode */
/* 1067 */     OP_mulx, /**< mulx opcode */

    /* Intel Safer Mode Extensions */
/* 1068 */     OP_getsec, /**< getsec opcode */

    /* Misc Intel additions */
/* 1069 */     OP_vmfunc, /**< vmfunc opcode */
/* 1070 */     OP_invpcid, /**< invpcid opcode */

    /* Intel TSX */
/* 1071 */     OP_xabort, /**< xabort opcode */
/* 1072 */     OP_xbegin, /**< xbegin opcode */
/* 1073 */     OP_xend, /**< xend opcode */
/* 1074 */     OP_xtest, /**< xtest opcode */

    /* AVX2 */
/* 1075 */     OP_vpgatherdd, /**< vpgatherdd opcode */
/* 1076 */     OP_vpgatherdq, /**< vpgatherdq opcode */
/* 1077 */     OP_vpgatherqd, /**< vpgatherqd opcode */
/* 1078 */     OP_vpgatherqq, /**< vpgatherqq opcode */
/* 1079 */     OP_vgatherdps, /**< vgatherdps opcode */
/* 1080 */     OP_vgatherdpd, /**< vgatherdpd opcode */
/* 1081 */     OP_vgatherqps, /**< vgatherqps opcode */
/* 1082 */     OP_vgatherqpd, /**< vgatherqpd opcode */
/* 1083 */     OP_vbroadcasti128, /**< vbroadcasti128 opcode */
/* 1084 */     OP_vinserti128, /**< vinserti128 opcode */
/* 1085 */     OP_vextracti128, /**< vextracti128 opcode */
/* 1086 */     OP_vpmaskmovd, /**< vpmaskmovd opcode */
/* 1087 */     OP_vpmaskmovq, /**< vpmaskmovq opcode */
/* 1088 */     OP_vperm2i128, /**< vperm2i128 opcode */
/* 1089 */     OP_vpermd, /**< vpermd opcode */
/* 1090 */     OP_vpermps, /**< vpermps opcode */
/* 1091 */     OP_vpermq, /**< vpermq opcode */
/* 1092 */     OP_vpermpd, /**< vpermpd opcode */
/* 1093 */     OP_vpblendd, /**< vpblendd opcode */
/* 1094 */     OP_vpsllvd, /**< vpsllvd opcode */
/* 1095 */     OP_vpsllvq, /**< vpsllvq opcode */
/* 1096 */     OP_vpsravd, /**< vpsravd opcode */
/* 1097 */     OP_vpsrlvd, /**< vpsrlvd opcode */
/* 1098 */     OP_vpsrlvq, /**< vpsrlvq opcode */

    /* Keep these at the end so that ifdefs don't change internal enum values */

    OP_AFTER_LAST,
    OP_FIRST = OP_add,            /**< First real opcode. */
    OP_LAST  = OP_AFTER_LAST - 1, /**< Last real opcode. */
};

enum {
    DR_REG_NULL, /**< Sentinel value indicating no register, for address modes. */
    /* 64-bit general purpose */
    DR_REG_RAX, /**< The "rax" register. */
    DR_REG_RCX, /**< The "rcx" register. */
    DR_REG_RDX, /**< The "rdx" register. */
    DR_REG_RBX, /**< The "rbx" register. */
    
    DR_REG_RSP, /**< The "rsp" register. */
    DR_REG_RBP, /**< The "rbp" register. */
    DR_REG_RSI, /**< The "rsi" register. */
    DR_REG_RDI, /**< The "rdi" register. */
    
    DR_REG_R8, /**< The "r8" register. */
    DR_REG_R9, /**< The "r9" register. */
    DR_REG_R10, /**< The "r10" register. */
    DR_REG_R11, /**< The "r11" register. */
    
    DR_REG_R12, /**< The "r12" register. */
    DR_REG_R13, /**< The "r13" register. */
    DR_REG_R14, /**< The "r14" register. */
    DR_REG_R15, /**< The "r15" register. */
    
    /* 32-bit general purpose */
    DR_REG_EAX, /**< The "eax" register. */
    DR_REG_ECX, /**< The "ecx" register. */
    DR_REG_EDX, /**< The "edx" register. */
    DR_REG_EBX, /**< The "ebx" register. */
    
    DR_REG_ESP, /**< The "esp" register. */
    DR_REG_EBP, /**< The "ebp" register. */
    DR_REG_ESI, /**< The "esi" register. */
    DR_REG_EDI, /**< The "edi" register. */
    
    DR_REG_R8D, /**< The "r8d" register. */
    DR_REG_R9D, /**< The "r9d" register. */
    DR_REG_R10D, /**< The "r10d" register. */
    DR_REG_R11D, /**< The "r11d" register. */
    
    DR_REG_R12D, /**< The "r12d" register. */
    DR_REG_R13D, /**< The "r13d" register. */
    DR_REG_R14D, /**< The "r14d" register. */
    DR_REG_R15D, /**< The "r15d" register. */
    
    /* 16-bit general purpose */
    DR_REG_AX, /**< The "ax" register. */
    DR_REG_CX, /**< The "cx" register. */
    DR_REG_DX, /**< The "dx" register. */
    DR_REG_BX, /**< The "bx" register. */
    
    DR_REG_SP, /**< The "sp" register. */
    DR_REG_BP, /**< The "bp" register. */
    DR_REG_SI, /**< The "si" register. */
    DR_REG_DI, /**< The "di" register. */
    
    DR_REG_R8W, /**< The "r8w" register. */
    DR_REG_R9W, /**< The "r9w" register. */
    DR_REG_R10W, /**< The "r10w" register. */
    DR_REG_R11W, /**< The "r11w" register. */
    
    DR_REG_R12W, /**< The "r12w" register. */
    DR_REG_R13W, /**< The "r13w" register. */
    DR_REG_R14W, /**< The "r14w" register. */
    DR_REG_R15W, /**< The "r15w" register. */
    
    /* 8-bit general purpose */
    DR_REG_AL, /**< The "al" register. */
    DR_REG_CL, /**< The "cl" register. */
    DR_REG_DL, /**< The "dl" register. */
    DR_REG_BL, /**< The "bl" register. */
    
    DR_REG_AH, /**< The "ah" register. */
    DR_REG_CH, /**< The "ch" register. */
    DR_REG_DH, /**< The "dh" register. */
    DR_REG_BH, /**< The "bh" register. */
    
    DR_REG_R8L, /**< The "r8l" register. */
    DR_REG_R9L, /**< The "r9l" register. */
    DR_REG_R10L, /**< The "r10l" register. */
    DR_REG_R11L, /**< The "r11l" register. */
    
    DR_REG_R12L, /**< The "r12l" register. */
    DR_REG_R13L, /**< The "r13l" register. */
    DR_REG_R14L, /**< The "r14l" register. */
    DR_REG_R15L, /**< The "r15l" register. */
    
    DR_REG_SPL, /**< The "spl" register. */
    DR_REG_BPL, /**< The "bpl" register. */
    DR_REG_SIL, /**< The "sil" register. */
    DR_REG_DIL, /**< The "dil" register. */
    
    /* 64-BIT MMX */
    DR_REG_MM0, /**< The "mm0" register. */
    DR_REG_MM1, /**< The "mm1" register. */
    DR_REG_MM2, /**< The "mm2" register. */
    DR_REG_MM3, /**< The "mm3" register. */
    
    DR_REG_MM4, /**< The "mm4" register. */
    DR_REG_MM5, /**< The "mm5" register. */
    DR_REG_MM6, /**< The "mm6" register. */
    DR_REG_MM7, /**< The "mm7" register. */
    
    /* 128-BIT XMM */
    DR_REG_XMM0, /**< The "xmm0" register. */
    DR_REG_XMM1, /**< The "xmm1" register. */
    DR_REG_XMM2, /**< The "xmm2" register. */
    DR_REG_XMM3, /**< The "xmm3" register. */
    
    DR_REG_XMM4, /**< The "xmm4" register. */
    DR_REG_XMM5, /**< The "xmm5" register. */
	DR_REG_XMM6, /**< The "xmm6" register. */
	DR_REG_XMM7, /**< The "xmm7" register. */

	DR_REG_XMM8, /**< The "xmm8" register. */
	DR_REG_XMM9, /**< The "xmm9" register. */
	DR_REG_XMM10, /**< The "xmm10" register. */
	DR_REG_XMM11, /**< The "xmm11" register. */

	DR_REG_XMM12, /**< The "xmm12" register. */
	DR_REG_XMM13, /**< The "xmm13" register. */
	DR_REG_XMM14, /**< The "xmm14" register. */
	DR_REG_XMM15, /**< The "xmm15" register. */

	/* floating point registers */
	DR_REG_ST0, /**< The "st0" register. */
	DR_REG_ST1, /**< The "st1" register. */
	DR_REG_ST2, /**< The "st2" register. */
	DR_REG_ST3, /**< The "st3" register. */

	DR_REG_ST4, /**< The "st4" register. */
	DR_REG_ST5, /**< The "st5" register. */
	DR_REG_ST6, /**< The "st6" register. */
	DR_REG_ST7, /**< The "st7" register. */

	/* segments (order from "Sreg" description in Intel manual) */
	DR_SEG_ES, /**< The "es" register. */
	DR_SEG_CS, /**< The "cs" register. */
	DR_SEG_SS, /**< The "ss" register. */
	DR_SEG_DS, /**< The "ds" register. */
	DR_SEG_FS, /**< The "fs" register. */
	DR_SEG_GS, /**< The "gs" register. */

	/* debug & control registers (privileged access only; 8-15 for future processors) */
	DR_REG_DR0, /**< The "dr0" register. */
	DR_REG_DR1, /**< The "dr1" register. */
	DR_REG_DR2, /**< The "dr2" register. */
	DR_REG_DR3, /**< The "dr3" register. */

	DR_REG_DR4, /**< The "dr4" register. */
	DR_REG_DR5, /**< The "dr5" register. */
	DR_REG_DR6, /**< The "dr6" register. */
	DR_REG_DR7, /**< The "dr7" register. */

	DR_REG_DR8, /**< The "dr8" register. */
	DR_REG_DR9, /**< The "dr9" register. */
	DR_REG_DR10, /**< The "dr10" register. */
	DR_REG_DR11, /**< The "dr11" register. */

	DR_REG_DR12, /**< The "dr12" register. */
	DR_REG_DR13, /**< The "dr13" register. */
	DR_REG_DR14, /**< The "dr14" register. */
	DR_REG_DR15, /**< The "dr15" register. */

	/* cr9-cr15 do not yet exist on current x64 hardware */
	DR_REG_CR0, /**< The "cr0" register. */
	DR_REG_CR1, /**< The "cr1" register. */
	DR_REG_CR2, /**< The "cr2" register. */
	DR_REG_CR3, /**< The "cr3" register. */

	DR_REG_CR4, /**< The "cr4" register. */
	DR_REG_CR5, /**< The "cr5" register. */
	DR_REG_CR6, /**< The "cr6" register. */
	DR_REG_CR7, /**< The "cr7" register. */

	DR_REG_CR8, /**< The "cr8" register. */
	DR_REG_CR9, /**< The "cr9" register. */
	DR_REG_CR10, /**< The "cr10" register. */
	DR_REG_CR11, /**< The "cr11" register. */

	DR_REG_CR12, /**< The "cr12" register. */
	DR_REG_CR13, /**< The "cr13" register. */
	DR_REG_CR14, /**< The "cr14" register. */
	DR_REG_CR15, /**< The "cr15" register. */

	DR_REG_INVALID, /**< Sentinel value indicating an invalid register. */

	/* 256-BIT YMM */
	DR_REG_YMM0, /**< The "ymm0" register. */
	DR_REG_YMM1, /**< The "ymm1" register. */
	DR_REG_YMM2, /**< The "ymm2" register. */
	DR_REG_YMM3, /**< The "ymm3" register. */

	DR_REG_YMM4, /**< The "ymm4" register. */
	DR_REG_YMM5, /**< The "ymm5" register. */
	DR_REG_YMM6, /**< The "ymm6" register. */
	DR_REG_YMM7, /**< The "ymm7" register. */

	DR_REG_YMM8, /**< The "ymm8" register. */
	DR_REG_YMM9, /**< The "ymm9" register. */
	DR_REG_YMM10, /**< The "ymm10" register. */
	DR_REG_YMM11, /**< The "ymm11" register. */

	DR_REG_YMM12, /**< The "ymm12" register. */
	DR_REG_YMM13, /**< The "ymm13" register. */
	DR_REG_YMM14, /**< The "ymm14" register. */
	DR_REG_YMM15, /**< The "ymm15" register. */


	/*virtual registers for split and joins*/
	DR_REG_VIRTUAL_1,
	DR_REG_VIRTUAL_2,

    
};

enum {
	EFLAGS_CF = 0x00000001, /**< The bit in the eflags register of CF (Carry Flag). */
	EFLAGS_PF = 0x00000004, /**< The bit in the eflags register of PF (Parity Flag). */
	EFLAGS_AF = 0x00000010, /**< The bit in the eflags register of AF (Aux Carry Flag). */
	EFLAGS_ZF = 0x00000040, /**< The bit in the eflags register of ZF (Zero Flag). */
	EFLAGS_SF = 0x00000080, /**< The bit in the eflags register of SF (Sign Flag). */
	EFLAGS_DF = 0x00000400, /**< The bit in the eflags register of DF (Direction Flag). */
	EFLAGS_OF = 0x00000800, /**< The bit in the eflags register of OF (Overflow Flag). */
};

#define DEBUG
#define DEBUG_LEVEL 3

#define ASSERT_MSG(x,s)	      \
	if(!(x)){				  \
		printf s;			  \
		exit(1);			  \
	}

#ifdef DEBUG
#define DEBUG_PRINT(s,l) if(l <= DEBUG_LEVEL) { printf s ; }
#else
#define DEUBG_PRINT(s)
#endif

#define IF_PRINT(x,s) \
	  if((x)){        \
		printf s ;    \
	  	  }   
	



#endif
