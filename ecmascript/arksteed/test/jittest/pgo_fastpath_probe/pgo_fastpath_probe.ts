class ProbeCtor {
    value: any;

    constructor(value: any) {
        this.value = value;
    }
}

function storeProbe(obj: any, value: any) {
    obj.value = value;
    return obj.value;
}

function instanceProbe(obj: any) {
    return obj instanceof ProbeCtor ? 1 : 0;
}

let obj = new ProbeCtor({ payload: 0 });
let inst = new ProbeCtor({ payload: 1 });

for (let i = 0; i < 20000; i++) {
    storeProbe(obj, inst);
    instanceProbe(inst);
}

print(ArkTools.getICState(storeProbe, 0, 1));
print(ArkTools.isStableHClass(obj));
print(ArkTools.isStableHClass(ProbeCtor));

ArkTools.arkSteedCompileSync(storeProbe);
ArkTools.arkSteedCompileSync(instanceProbe);
ArkTools.waitJitCompileFinish(storeProbe);
ArkTools.waitJitCompileFinish(instanceProbe);
print(ArkTools.isAOTCompiled(storeProbe));
print(ArkTools.isAOTCompiled(instanceProbe));

print(storeProbe(obj, inst) === inst ? 42 : 0);
print(instanceProbe(inst));
