// @ts-nocheck
class CatchSharedHolder {
    value: number = 0;

    constructor(value: number) {
        "use sendable";
        this.value = value;
    }
}

function storeShared(holder: CatchSharedHolder, value: any): string {
    holder.value = value;
    return "stored";
}

function callStoreWithCatch(holder: CatchSharedHolder, value: any): string {
    try {
        return storeShared(holder, value);
    } catch (error) {
        return "caught";
    }
}

let holder = new CatchSharedHolder(0);
for (let i = 0; i < 20000; i++) {
    storeShared(holder, i);
}

ArkTools.arkSteedCompileSync(storeShared);
ArkTools.waitJitCompileFinish(storeShared);
let ok = ArkTools.isAOTCompiled(storeShared);
ok = ok && callStoreWithCatch(holder, 7) === "stored" && holder.value === 7;
ok = ok && callStoreWithCatch(holder, { invalid: true }) === "caught";
ok = ok && holder.value === 7;
print(ok ? "PASS" : "FAIL");
