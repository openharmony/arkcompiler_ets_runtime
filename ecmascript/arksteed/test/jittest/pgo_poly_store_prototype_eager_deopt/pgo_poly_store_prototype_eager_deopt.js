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

function storePoly(receiver, value) {
    receiver.value = value;
    return value;
}

let own = { value: 0, marker: 0 };
let inherited = Object.create(proto);
for (let i = 0; i < 20000; i++) {
    storePoly((i & 1) === 0 ? own : inherited, i);
}

ArkTools.arkSteedCompileSync(storePoly);
ArkTools.waitJitCompileFinish(storePoly);
let ok = ArkTools.isAOTCompiled(storePoly);

let ownCheck = { value: 0, marker: 1 };
ok = ok && storePoly(ownCheck, 42) === 42 && ownCheck.value === 42;

let accessorCheck = Object.create(proto);
ok = ok && storePoly(accessorCheck, 55) === 55;
ok = ok && setterReceiver === accessorCheck && setterValue === 55;

Object.defineProperty(proto, "value", {
    configurable: true,
    writable: true,
    value: 1
});
let deoptCheck = Object.create(proto);
ok = ok && storePoly(deoptCheck, 99) === 99 && deoptCheck.value === 99;
ok = ok && Object.prototype.hasOwnProperty.call(deoptCheck, "value");

print(ok ? "PASS" : "FAIL");
