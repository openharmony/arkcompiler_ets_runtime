let installSetter = false;
let invalidatedInsideCall = false;
let setterReceiver = null;
let setterValue = -1;
let proto = {};

function maybeInstallSetter() {
    if (installSetter) {
        Object.defineProperty(proto, "added", {
            configurable: true,
            set(value) {
                setterReceiver = this;
                setterValue = value;
            }
        });
        invalidatedInsideCall = !ArkTools.arkSteedIsCompiled(storeTransitionAfterCall);
    }
}

function storeTransitionAfterCall(receiver, value) {
    maybeInstallSetter();
    receiver.added = value;
    return value;
}

function newReceiver() {
    return Object.create(proto);
}

for (let i = 0; i < 20000; i++) {
    storeTransitionAfterCall(newReceiver(), i);
}

ArkTools.arkSteedCompileSync(storeTransitionAfterCall);
ArkTools.waitJitCompileFinish(storeTransitionAfterCall);
let ok = ArkTools.arkSteedIsCompiled(storeTransitionAfterCall);

installSetter = true;
let receiver = newReceiver();
let result = storeTransitionAfterCall(receiver, 42);
ok = ok && result === 42;
ok = ok && setterReceiver === receiver && setterValue === 42;
ok = ok && !Object.prototype.hasOwnProperty.call(receiver, "added");
ok = ok && invalidatedInsideCall;
ok = ok && !ArkTools.arkSteedIsCompiled(storeTransitionAfterCall);

print(ok ? "PASS" : "FAIL");
