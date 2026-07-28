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

function computeExpected(seed) {
    let d0 = seed * 1.125 + 0.25;
    let d1 = seed * 1.25 + 0.5;
    let d2 = seed * 1.375 + 0.75;
    let d3 = seed * 1.5 + 1.0;
    let d4 = seed * 1.625 + 1.25;
    let d5 = seed * 1.75 + 1.5;
    let d6 = seed * 1.875 + 1.75;
    let d7 = seed * 2.0 + 2.0;
    return d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7;
}

function storeWithFprPressure(holder, seed) {
    let value = { marker: seed };
    let d0 = seed * 1.125 + 0.25;
    let d1 = seed * 1.25 + 0.5;
    let d2 = seed * 1.375 + 0.75;
    let d3 = seed * 1.5 + 1.0;
    let d4 = seed * 1.625 + 1.25;
    let d5 = seed * 1.75 + 1.5;
    let d6 = seed * 1.875 + 1.75;
    let d7 = seed * 2.0 + 2.0;

    holder.value = value;

    if (holder.value !== value) {
        return -1;
    }
    return d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7;
}

let holder = new Holder();
for (let i = 0; i < 20000; i++) {
    storeWithFprPressure(holder, i + 0.125);
}

forceFullGC();
let ok = true;
ArkTools.arkSteedCompileSync(storeWithFprPressure);
ArkTools.waitJitCompileFinish(storeWithFprPressure);
ok = ok && ArkTools.isAOTCompiled(storeWithFprPressure);

for (let i = 0; i < 128; i++) {
    let seed = i + 0.125;
    let result = storeWithFprPressure(holder, seed);
    forceYoungGC();
    if (result !== computeExpected(seed) || holder.value.marker !== seed) {
        ok = false;
        break;
    }
}

print(ok ? "PASS" : "FAIL");
