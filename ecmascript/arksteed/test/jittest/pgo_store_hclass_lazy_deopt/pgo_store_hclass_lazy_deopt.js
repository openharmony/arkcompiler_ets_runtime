let transitionReceiver = false;
let invalidatedInsideCall = false;

function maybeTransitionReceiver(receiver) {
    if (transitionReceiver) {
        receiver.extra = 1;
        invalidatedInsideCall = !ArkTools.arkSteedIsCompiled(storeAroundCall);
    }
}

function storeAroundCall(receiver, value) {
    receiver.value = value;
    maybeTransitionReceiver(receiver);
    receiver.value = value + 1;
    return receiver.value;
}

function newReceiver() {
    return { value: 0 };
}

for (let i = 0; i < 20000; i++) {
    storeAroundCall(newReceiver(), i);
}

ArkTools.arkSteedCompileSync(storeAroundCall);
ArkTools.waitJitCompileFinish(storeAroundCall);
let ok = ArkTools.arkSteedIsCompiled(storeAroundCall);

transitionReceiver = true;
let receiver = newReceiver();
let result = storeAroundCall(receiver, 41);
ok = ok && result === 42;
ok = ok && receiver.value === 42 && receiver.extra === 1;
ok = ok && invalidatedInsideCall;
ok = ok && !ArkTools.arkSteedIsCompiled(storeAroundCall);

print(ok ? "PASS" : "FAIL");
