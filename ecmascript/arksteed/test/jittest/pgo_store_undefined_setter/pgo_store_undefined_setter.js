let receiver = {};
Object.defineProperty(receiver, "value", {
    configurable: true,
    get() {
        return 1;
    }
});

function storeUndefinedSetter(object, value) {
    object.value = value;
    return value;
}

for (let i = 0; i < 20000; i++) {
    try {
        storeUndefinedSetter(receiver, i);
    } catch (error) {
    }
}

ArkTools.arkSteedCompileSync(storeUndefinedSetter);
ArkTools.waitJitCompileFinish(storeUndefinedSetter);
let ok = ArkTools.isAOTCompiled(storeUndefinedSetter);

let caught = false;
try {
    storeUndefinedSetter(receiver, 42);
} catch (error) {
    caught = error instanceof TypeError;
}
ok = ok && caught;

print(ok ? "PASS" : "FAIL");
