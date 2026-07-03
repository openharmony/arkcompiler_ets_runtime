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

function makeNoise(seed) {
    let result = 0;
    for (let i = 0; i < 64; i++) {
        let tmp = { x: seed + i, y: [seed, i, seed + i] };
        result += tmp.x;
    }
    return result;
}

function storeFresh(holder, marker) {
    holder.value = { marker: marker, payload: [marker, marker + 1, marker + 2] };
}

let holder = new Holder();
for (let i = 0; i < 20000; i++) {
    storeFresh(holder, i);
}

forceFullGC();
let storeICState = ArkTools.getICState(storeFresh, 0, 1);
ArkTools.arkSteedCompileSync(storeFresh);
ArkTools.waitJitCompileFinish(storeFresh);

let ok = storeICState === "mono";
ok = ok && ArkTools.isAOTCompiled(storeFresh);
for (let i = 0; i < 2000; i++) {
    storeFresh(holder, i);
    makeNoise(i);
    forceYoungGC();
    if (holder.value.marker !== i || holder.value.payload[2] !== i + 2) {
        ok = false;
        break;
    }
}

print(ok ? "PASS" : "FAIL");
