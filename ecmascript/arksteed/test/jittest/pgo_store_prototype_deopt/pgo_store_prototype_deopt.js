function storeInherited(receiver, value) {
    receiver.value = value;
    return receiver.value;
}

let proto = { value: 0 };
storeInherited(Object.create(proto), 0);

ArkTools.arkSteedCompileSync(storeInherited);
ArkTools.waitJitCompileFinish(storeInherited);
let receiver = Object.create(proto);
let result = storeInherited(receiver, 42);
let ok = ArkTools.isAOTCompiled(storeInherited) && result === 42 && receiver.value === 42;

let setterReceiver = null;
let setterValue = -1;
Object.defineProperty(proto, "value", {
    configurable: true,
    set(value) {
        setterReceiver = this;
        setterValue = value;
    }
});
let deoptReceiver = Object.create(proto);
let deoptResult = storeInherited(deoptReceiver, 99);
ok = ok && deoptResult === undefined && setterReceiver === deoptReceiver && setterValue === 99;
ok = ok && !Object.prototype.hasOwnProperty.call(deoptReceiver, "value");

print(ok ? "PASS" : "FAIL");
