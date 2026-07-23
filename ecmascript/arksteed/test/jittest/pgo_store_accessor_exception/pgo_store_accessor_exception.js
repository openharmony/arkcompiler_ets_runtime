let setterValue = -1;
let shouldThrow = false;

let proto = {};
Object.defineProperty(proto, "value", {
    configurable: true,
    set(value) {
        if (shouldThrow) {
            throw new Error("setter failure");
        }
        setterValue = value;
    }
});

function storeAccessor(receiver, value) {
    receiver.value = value;
    return value;
}

let receiver = Object.create(proto);
for (let i = 0; i < 20000; i++) {
    storeAccessor(receiver, i);
}

ArkTools.arkSteedCompileSync(storeAccessor);
ArkTools.waitJitCompileFinish(storeAccessor);
let ok = ArkTools.isAOTCompiled(storeAccessor);

let caught = false;
shouldThrow = true;
try {
    storeAccessor(receiver, 777);
} catch (error) {
    caught = error.message === "setter failure";
}
ok = ok && caught && setterValue === 19999;

print(ok ? "PASS" : "FAIL");
