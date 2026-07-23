// @ts-nocheck
class SharedHolder {
    value: SendableArray<number> | null = null;

    constructor(value: SendableArray<number>) {
        "use sendable";
        this.value = value;
    }
}

function storeShared(holder: SharedHolder, value: SendableArray<number>): number {
    holder.value = value;
    return holder.value[0];
}

let holder = new SharedHolder(new SendableArray<number>(0));
for (let i = 0; i < 20000; i++) {
    storeShared(holder, new SendableArray<number>(i));
}

let ok = ArkTools.getICState(storeShared, 0, 1) === "mono";
ArkTools.arkSteedCompileSync(storeShared);
ArkTools.waitJitCompileFinish(storeShared);
ok = ok && ArkTools.isAOTCompiled(storeShared);

for (let i = 0; i < 2000; i++) {
    let value = new SendableArray<number>(i);
    storeShared(holder, value);
    if ((i & 127) === 0) {
        ArkTools.triggerSharedGC("shared");
    }
}

ok = ok && holder.value[0] === 1999;
print(ok ? "PASS" : "FAIL");
