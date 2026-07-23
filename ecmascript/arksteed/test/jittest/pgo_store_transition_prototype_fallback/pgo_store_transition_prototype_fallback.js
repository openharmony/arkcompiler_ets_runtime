function storePrototypeTransition(holder, value) {
    holder.added = value;
    return holder.added;
}

let prototypeHolders = [];
for (let i = 0; i < 257; i++) {
    class Holder {}
    prototypeHolders.push(Holder.prototype);
}

storePrototypeTransition(prototypeHolders[0], 0);

ArkTools.arkSteedCompileSync(storePrototypeTransition);
ArkTools.waitJitCompileFinish(storePrototypeTransition);
let ok = ArkTools.isAOTCompiled(storePrototypeTransition);
for (let i = 0; i < 256; i++) {
    let holder = prototypeHolders[i + 1];
    ok = ok && storePrototypeTransition(holder, i) === i;
    ok = ok && holder.added === i;
}

print(ok ? "PASS" : "FAIL");
