# Environment Setup and Compilation

## Environment Configuration

Use Ubuntu 18.04 or 20.04. For details about how to set up the environment, see:

[Setting Up Ubuntu Development Environment with Installation Package and Building Source Code](https://developer.huawei.com/consumer/cn/training/course/video/C101639988048536240)

## Compilation

1.  First compilation:

    ```
    ./build.sh --product-name hispark_taurus_standard
    ```

2.  Compile an ARK runtime after the first compilation:

    x64：
	```
    ./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages --build-target ark_ts2abc_build
    ```

	arm64：
	```
	./build.sh --product-name ohos_arm64 --build-target ark_js_vm --build-target ld-musl-aarch64.so.1
	```

	arm32:
	```
	./build.sh --product-name rk3568 --build-target ark_js_runtime --build-target ld-musl-arm.so.1
	```

3.  Compile the ARK frontend after the first compilation:

    ```
    ./build.sh --product-name hispark_taurus_standard --build-target ark_ts2abc_build
    ```

**NOTE**:  Run the compilation commands in the project root directory.

The binary files related to ARK are available in the following paths:

```
out/hispark_taurus/arkcompiler/runtime_core/
out/hispark_taurus/arkcompiler/ets_frontend/
out/hispark_taurus/arkcompiler/ets_runtime/
out/hispark_taurus/clang_x64/arkcompiler/runtime_core/
out/hispark_taurus/clang_x64/arkcompiler/ets_frontend/
out/hispark_taurus/clang_x64/arkcompiler/ets_runtime
```
