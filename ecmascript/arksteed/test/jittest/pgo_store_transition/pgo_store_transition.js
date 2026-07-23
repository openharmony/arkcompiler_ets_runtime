function makeTransitionBase() {
    return { base: 1 };
}

function storeTransition(holder, value) {
    holder.added = value;
    return holder.added;
}

for (let i = 0; i < 20000; i++) {
    storeTransition(makeTransitionBase(), { marker: i });
}

ArkTools.arkSteedCompileSync(storeTransition);
ArkTools.waitJitCompileFinish(storeTransition);
let ok = ArkTools.isAOTCompiled(storeTransition);

for (let i = 0; i < 2048; i++) {
    let holder = makeTransitionBase();
    let value = { marker: i };
    ok = ok && storeTransition(holder, value) === value;
    ok = ok && holder.base === 1 && holder.added.marker === i;
    if ((i & 127) === 0) {
        ArkTools.gc();
    }
}

print(ok ? "PASS" : "FAIL");
