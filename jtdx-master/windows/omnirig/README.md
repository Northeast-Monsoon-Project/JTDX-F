# OmniRig ActiveQt wrapper

These files were generated for the original OmniRig 1.x COM interface with Qt 5.15 `dumpcpp`, from [VE3NEA's OmniRig type library](https://github.com/VE3NEA/OmniRig/blob/master/OmniRig.tlb). This preserves the upstream JTDX OmniRig integration without requiring a registered COM server at build time. The end user still installs OmniRig separately when using that radio backend.

The generator's optional documentation lookup needed to check failed `GetIDsOfNames` / `GetDocumentation` results under Wine; no interface members were removed. The generated source uses a relative `OmniRig.h` include and lowercase `oaidl.h` for case-sensitive cross builds. To regenerate, run `dumpcpp -o OmniRig OmniRig.tlb`, then normalize those two include paths. Qt's generator and ActiveQt source retain their own BSD licensing notices.
