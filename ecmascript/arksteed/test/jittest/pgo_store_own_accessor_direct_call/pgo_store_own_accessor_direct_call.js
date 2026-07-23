let setterReceiver = null;
let setterValue = -1;

let receiver = {};
Object.defineProperty(receiver, "value", {
    configurable: true,
    set(value) {
        setterReceiver = this;
        setterValue = value;
    }
});

function storeOwnAccessor(object, value) {
    object.value = value;
    return value;
}

for (let i = 0; i < 20000; i++) {
    storeOwnAccessor(receiver, i);
}

ArkTools.arkSteedCompileSync(storeOwnAccessor);
ArkTools.waitJitCompileFinish(storeOwnAccessor);
let ok = ArkTools.isAOTCompiled(storeOwnAccessor);

ok = ok && storeOwnAccessor(receiver, 42) === 42;
ok = ok && setterReceiver === receiver && setterValue === 42;

print(ok ? "PASS" : "FAIL");
