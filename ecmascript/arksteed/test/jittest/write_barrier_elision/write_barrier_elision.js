class Holder {
    constructor() {
        this.value = 0;
        this.other = null;
    }
}

function storeSmi(holder) {
    holder.value = 123;
}

function storeRoot(holder) {
    holder.other = null;
}

function storePhiSmi(holder, flag) {
    let value = 1;
    if (flag) {
        value = 2;
    }
    holder.value = value;
}

function storeLoopPhiSmi(holder, count) {
    let value = 1;
    for (let i = 0; i < count; i++) {
        holder.value = value;
        value = 2;
    }
    return holder.value;
}

let holder = new Holder();
for (let i = 0; i < 20000; i++) {
    storeSmi(holder);
    storeRoot(holder);
    storePhiSmi(holder, (i & 1) === 0);
    storeLoopPhiSmi(holder, 2);
}

let ok = ArkTools.getICState(storeSmi, 0, 1) === "mono" &&
    ArkTools.getICState(storeRoot, 0, 1) === "mono";
ArkTools.arkSteedCompileSync(storeSmi);
ArkTools.arkSteedCompileSync(storeRoot);
ArkTools.arkSteedCompileSync(storePhiSmi);
ArkTools.arkSteedCompileSync(storeLoopPhiSmi);
ArkTools.waitJitCompileFinish(storeSmi);
ArkTools.waitJitCompileFinish(storeRoot);
ArkTools.waitJitCompileFinish(storePhiSmi);
ArkTools.waitJitCompileFinish(storeLoopPhiSmi);
ok = ok && ArkTools.isAOTCompiled(storeSmi) && ArkTools.isAOTCompiled(storeRoot) &&
    ArkTools.isAOTCompiled(storePhiSmi) && ArkTools.isAOTCompiled(storeLoopPhiSmi);

storeSmi(holder);
storeRoot(holder);
storePhiSmi(holder, true);
ok = ok && storeLoopPhiSmi(holder, 3) === 2;
ok = ok && holder.value === 2 && holder.other === null;

print(ok ? "PASS" : "FAIL");
