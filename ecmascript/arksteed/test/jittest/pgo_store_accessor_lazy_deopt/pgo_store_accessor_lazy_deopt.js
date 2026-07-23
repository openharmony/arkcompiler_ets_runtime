let mutatePrototype = false;
let setterReceiver = null;
let setterValue = -1;

let root = {};
let proto = {};
Object.defineProperty(proto, "value", {
    configurable: true,
    set(value) {
        setterReceiver = this;
        setterValue = value;
        if (mutatePrototype) {
            Object.setPrototypeOf(proto, root);
        }
    }
});

function storeThroughSetter(receiver, value) {
    receiver.value = value;
    return value + 1;
}

for (let i = 0; i < 20000; i++) {
    storeThroughSetter(Object.create(proto), i);
}

ArkTools.arkSteedCompileSync(storeThroughSetter);
ArkTools.waitJitCompileFinish(storeThroughSetter);
let ok = ArkTools.arkSteedIsCompiled(storeThroughSetter);

mutatePrototype = true;
let receiver = Object.create(proto);
let result = storeThroughSetter(receiver, 41);
ok = ok && result === 42;
ok = ok && setterReceiver === receiver && setterValue === 41;
ok = ok && !ArkTools.arkSteedIsCompiled(storeThroughSetter);

print(ok ? "PASS" : "FAIL");
