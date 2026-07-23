// @ts-nocheck
class PolySharedA {
    value: SendableArray<number> | null = null;

    constructor(value: SendableArray<number>) {
        "use sendable";
        this.value = value;
    }
}

class PolySharedB {
    padding: number = 0;
    value: SendableArray<number> | null = null;

    constructor(value: SendableArray<number>) {
        "use sendable";
        this.padding = 1;
        this.value = value;
    }
}

function storePolyShared(holder: PolySharedA | PolySharedB, value: SendableArray<number>): number {
    holder.value = value;
    return holder.value[0];
}

let a = new PolySharedA(new SendableArray<number>(0));
let b = new PolySharedB(new SendableArray<number>(0));
for (let i = 0; i < 20000; i++) {
    storePolyShared((i & 1) === 0 ? a : b, new SendableArray<number>(i));
}

ArkTools.arkSteedCompileSync(storePolyShared);
ArkTools.waitJitCompileFinish(storePolyShared);
let ok = ArkTools.isAOTCompiled(storePolyShared);
ok = ok && storePolyShared(a, new SendableArray<number>(11)) === 11;
ok = ok && storePolyShared(b, new SendableArray<number>(12)) === 12;
ok = ok && a.value[0] === 11 && b.value[0] === 12;
print(ok ? "PASS" : "FAIL");
