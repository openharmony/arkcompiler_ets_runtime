// @ts-nocheck
class SharedCounter {
    value: number = 0;

    constructor(value: number) {
        "use sendable";
        this.value = value;
    }
}

function storeSharedPrimitive(holder: SharedCounter, value: number): number {
    holder.value = value;
    return holder.value;
}

let holder = new SharedCounter(0);
for (let i = 0; i < 20000; i++) {
    storeSharedPrimitive(holder, i);
}

let ok = ArkTools.getICState(storeSharedPrimitive, 0, 1) === "mono";
ArkTools.arkSteedCompileSync(storeSharedPrimitive);
ArkTools.waitJitCompileFinish(storeSharedPrimitive);
ok = ok && ArkTools.isAOTCompiled(storeSharedPrimitive);

for (let i = 0; i < 2048; i++) {
    let result = storeSharedPrimitive(holder, i);
    if ((i & 255) === 0) {
        ArkTools.triggerSharedGC((i & 511) === 0 ? "shared" : "shared_full");
    }
    if (result !== i || holder.value !== i) {
        ok = false;
        break;
    }
}

ArkTools.triggerSharedGC("shared_full");
ok = ok && holder.value === 2047;
print(ok ? "PASS" : "FAIL");
