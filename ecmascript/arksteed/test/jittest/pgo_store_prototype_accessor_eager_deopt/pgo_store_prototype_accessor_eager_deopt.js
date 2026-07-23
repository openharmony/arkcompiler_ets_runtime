let setterReceiver = null;
let setterValue = -1;

let proto = {};
Object.defineProperty(proto, "value", {
    configurable: true,
    set(value) {
        setterReceiver = this;
        setterValue = value;
    }
});

function storePrototypeAccessor(receiver, value) {
    receiver.value = value;
    return value;
}

for (let i = 0; i < 20000; i++) {
    storePrototypeAccessor(Object.create(proto), i);
}

ArkTools.arkSteedCompileSync(storePrototypeAccessor);
ArkTools.waitJitCompileFinish(storePrototypeAccessor);
let ok = ArkTools.isAOTCompiled(storePrototypeAccessor);

let receiver = Object.create(proto);
ok = ok && storePrototypeAccessor(receiver, 42) === 42;
ok = ok && setterReceiver === receiver && setterValue === 42;

let replacementProto = {};
Object.defineProperty(replacementProto, "value", {
    configurable: true,
    set(value) {
        setterReceiver = this;
        setterValue = value + 1;
    }
});
Object.setPrototypeOf(proto, replacementProto);
delete proto.value;

let deoptReceiver = Object.create(proto);
ok = ok && storePrototypeAccessor(deoptReceiver, 100) === 100;
ok = ok && setterReceiver === deoptReceiver && setterValue === 101;

print(ok ? "PASS" : "FAIL");
