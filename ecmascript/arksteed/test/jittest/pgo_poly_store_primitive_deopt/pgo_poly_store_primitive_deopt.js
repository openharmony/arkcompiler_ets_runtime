class PrimitiveHolderA {
    constructor(value) {
        this.value = value;
    }
}

class PrimitiveHolderB {
    constructor(value) {
        this.padding = 0;
        this.value = value;
    }
}

function storePolyPrimitive(holder, value) {
    holder.value = value;
    return holder.value;
}

let a = new PrimitiveHolderA(0);
let b = new PrimitiveHolderB(0);
for (let i = 0; i < 20000; i++) {
    storePolyPrimitive((i & 1) === 0 ? a : b, i);
}

ArkTools.arkSteedCompileSync(storePolyPrimitive);
ArkTools.waitJitCompileFinish(storePolyPrimitive);
let ok = ArkTools.isAOTCompiled(storePolyPrimitive);
let threw = false;
try {
    storePolyPrimitive(1, 7);
} catch (error) {
    threw = error instanceof TypeError;
}
ok = ok && threw;

print(ok ? "PASS" : "FAIL");
