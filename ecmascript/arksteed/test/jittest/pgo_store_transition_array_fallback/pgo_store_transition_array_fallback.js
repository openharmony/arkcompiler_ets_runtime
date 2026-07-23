function makeArrayBase() {
    return [1, 2, 3];
}

function storeArrayTransition(holder, value) {
    holder.added = value;
    return holder.added;
}

for (let i = 0; i < 20000; i++) {
    storeArrayTransition(makeArrayBase(), i);
}

ArkTools.arkSteedCompileSync(storeArrayTransition);
ArkTools.waitJitCompileFinish(storeArrayTransition);
let ok = ArkTools.isAOTCompiled(storeArrayTransition);
for (let i = 0; i < 256; i++) {
    let holder = makeArrayBase();
    ok = ok && storeArrayTransition(holder, i) === i;
    ok = ok && holder.length === 3 && holder.added === i;
}

print(ok ? "PASS" : "FAIL");
