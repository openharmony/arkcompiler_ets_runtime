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

function expectedChecksum(seed) {
    let sum = seed;
    for (let i = 1; i <= 12; i++) {
        sum += seed + i;
    }
    sum += seed + 1;
    sum += seed + 2;
    sum += seed + 3;
    sum += (seed * 2) | 0;
    return sum;
}

function storeWithPressure(holder, seed) {
    let live0 = { value: seed + 1 };
    let live1 = { value: seed + 2 };
    let live2 = { value: seed + 3 };
    let live3 = { value: seed + 4 };
    let live4 = { value: seed + 5 };
    let live5 = { value: seed + 6 };
    let live6 = { value: seed + 7 };
    let live7 = { value: seed + 8 };
    let live8 = { value: seed + 9 };
    let live9 = { value: seed + 10 };
    let live10 = { value: seed + 11 };
    let live11 = { value: seed + 12 };
    let intLive = (seed * 2) | 0;
    let doubleLive = seed + 0.5;
    let value = { marker: seed, payload: [seed + 1, seed + 2, seed + 3] };

    holder.value = value;

    let sum = value.marker + value.payload[0] + value.payload[1] + value.payload[2] + intLive;
    sum += live0.value + live1.value + live2.value + live3.value;
    sum += live4.value + live5.value + live6.value + live7.value;
    sum += live8.value + live9.value + live10.value + live11.value;
    if (holder.value !== value || doubleLive - seed !== 0.5) {
        return -1;
    }
    return sum;
}

let holder = new Holder();
let warm = 0;
for (let i = 0; i < 20000; i++) {
    warm += storeWithPressure(holder, i) & 1;
}

forceFullGC();
let ok = ArkTools.getICState(storeWithPressure, 0, 1) === "mono";
ArkTools.arkSteedCompileSync(storeWithPressure);
ArkTools.waitJitCompileFinish(storeWithPressure);
ok = ok && ArkTools.isAOTCompiled(storeWithPressure);

for (let i = 0; i < 1000; i++) {
    let result = storeWithPressure(holder, i);
    forceYoungGC();
    if (result !== expectedChecksum(i) ||
        holder.value.marker !== i ||
        holder.value.payload[0] !== i + 1 ||
        holder.value.payload[2] !== i + 3) {
        ok = false;
        break;
    }
}

print(ok ? "PASS" : "FAIL");
