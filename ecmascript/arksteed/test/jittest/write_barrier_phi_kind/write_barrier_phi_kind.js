class Holder {
    constructor() {
        this.value = null;
    }
}

function forceFullGC() {
    ArkTools.forceFullGC();
    if (ArkTools.GC !== undefined) {
        ArkTools.GC.startGC("full", undefined, true);
    }
}

function forceYoungGC() {
    if (ArkTools.GC !== undefined) {
        ArkTools.GC.startGC("young", undefined, true);
        return;
    }
    ArkTools.gc();
}

function storeMixedPhi(holder, flag, marker) {
    let value = 7;
    if (flag) {
        value = { marker: marker, payload: [marker, marker + 1] };
    }
    holder.value = value;
    return holder.value;
}

let holder = new Holder();

for (let i = 0; i < 20000; i++) {
    storeMixedPhi(holder, (i & 1) === 0, i);
}

let ok = true;
ArkTools.arkSteedCompileSync(storeMixedPhi);
ArkTools.waitJitCompileFinish(storeMixedPhi);
ok = ok && ArkTools.isAOTCompiled(storeMixedPhi);

ok = ok && storeMixedPhi(holder, false, 42) === 7;
forceFullGC();
storeMixedPhi(holder, true, 99);
forceYoungGC();
ok = ok && holder.value.marker === 99 && holder.value.payload[1] === 100;

print(ok ? "PASS" : "FAIL");
