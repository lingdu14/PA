#include "cpu/exec.h"

make_EHelper(mov);

make_EHelper(operand_size);

make_EHelper(inv);
make_EHelper(nemu_trap);

make_EHelper(call);
make_EHelper(sub);
make_EHelper(xor);
make_EHelper(push);
make_EHelper(pop);
make_EHelper(ret);

make_EHelper(pusha);
make_EHelper(popa);


// ============ 根据报错日志，还需要补充下面这些框架引用的声明 ============
make_EHelper(add);
make_EHelper(adc);

make_EHelper(sbb);
make_EHelper(and);
make_EHelper(or);

make_EHelper(cmp);
make_EHelper(inc);
make_EHelper(dec);
make_EHelper(jmp);
make_EHelper(jcc);
make_EHelper(jmp_rm);
make_EHelper(call_rm);
make_EHelper(imul3);

make_EHelper(shl);
make_EHelper(shr);
make_EHelper(sar);
make_EHelper(test);
make_EHelper(not);
make_EHelper(neg);
make_EHelper(mul);
make_EHelper(imul1);
make_EHelper(div);
make_EHelper(idiv);
make_EHelper(lea);
make_EHelper(nop);
make_EHelper(cwtl);
make_EHelper(cltd);
make_EHelper(leave);
make_EHelper(setcc);
make_EHelper(imul2);
make_EHelper(movzx);
make_EHelper(movsx);
