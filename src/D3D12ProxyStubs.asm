OPTION CASEMAP:NONE

EXTERN GetOriginalProcByName:PROC
EXTERN GetOriginalProcByOrdinal:PROC
EXTERN ProxyMissingD3D12Export:PROC

.data
name_SetAppCompatStringPointer db "SetAppCompatStringPointer",0
name_D3D12GetDebugInterface db "D3D12GetDebugInterface",0
name_D3D12CoreCreateLayeredDevice db "D3D12CoreCreateLayeredDevice",0
name_D3D12CoreGetLayeredDeviceSize db "D3D12CoreGetLayeredDeviceSize",0
name_D3D12CoreRegisterLayers db "D3D12CoreRegisterLayers",0
name_D3D12CreateRootSignatureDeserializer db "D3D12CreateRootSignatureDeserializer",0
name_D3D12CreateVersionedRootSignatureDeserializer db "D3D12CreateVersionedRootSignatureDeserializer",0
name_D3D12DeviceRemovedExtendedData db "D3D12DeviceRemovedExtendedData",0
name_D3D12EnableExperimentalFeatures db "D3D12EnableExperimentalFeatures",0
name_D3D12GetInterface db "D3D12GetInterface",0
name_D3D12PIXEventsReplaceBlock db "D3D12PIXEventsReplaceBlock",0
name_D3D12PIXGetThreadInfo db "D3D12PIXGetThreadInfo",0
name_D3D12PIXNotifyWakeFromFenceSignal db "D3D12PIXNotifyWakeFromFenceSignal",0
name_D3D12PIXReportCounter db "D3D12PIXReportCounter",0
name_D3D12SerializeRootSignature db "D3D12SerializeRootSignature",0
name_D3D12SerializeVersionedRootSignature db "D3D12SerializeVersionedRootSignature",0
name_GetBehaviorValue db "GetBehaviorValue",0

.code

JMP_BY_NAME MACRO procName, nameLabel
    LOCAL missingExport
procName PROC FRAME
    sub rsp, 88h
    .allocstack 88h
    .endprolog
    mov [rsp+20h], rcx
    mov [rsp+28h], rdx
    mov [rsp+30h], r8
    mov [rsp+38h], r9
    movdqu [rsp+40h], xmm0
    movdqu [rsp+50h], xmm1
    movdqu [rsp+60h], xmm2
    movdqu [rsp+70h], xmm3
    lea rcx, nameLabel
    call GetOriginalProcByName
    mov r10, rax
    mov rcx, [rsp+20h]
    mov rdx, [rsp+28h]
    mov r8,  [rsp+30h]
    mov r9,  [rsp+38h]
    movdqu xmm0, [rsp+40h]
    movdqu xmm1, [rsp+50h]
    movdqu xmm2, [rsp+60h]
    movdqu xmm3, [rsp+70h]
    test r10, r10
    jz missingExport
    add rsp, 88h
    jmp r10
missingExport:
    call ProxyMissingD3D12Export
    int 3
procName ENDP
ENDM

D3D12Ordinal99 PROC FRAME
    sub rsp, 88h
    .allocstack 88h
    .endprolog
    mov [rsp+20h], rcx
    mov [rsp+28h], rdx
    mov [rsp+30h], r8
    mov [rsp+38h], r9
    movdqu [rsp+40h], xmm0
    movdqu [rsp+50h], xmm1
    movdqu [rsp+60h], xmm2
    movdqu [rsp+70h], xmm3
    mov ecx, 99
    call GetOriginalProcByOrdinal
    mov r10, rax
    mov rcx, [rsp+20h]
    mov rdx, [rsp+28h]
    mov r8,  [rsp+30h]
    mov r9,  [rsp+38h]
    movdqu xmm0, [rsp+40h]
    movdqu xmm1, [rsp+50h]
    movdqu xmm2, [rsp+60h]
    movdqu xmm3, [rsp+70h]
    test r10, r10
    jz ordinalMissing
    add rsp, 88h
    jmp r10
ordinalMissing:
    call ProxyMissingD3D12Export
    int 3
D3D12Ordinal99 ENDP

JMP_BY_NAME SetAppCompatStringPointer, name_SetAppCompatStringPointer
JMP_BY_NAME D3D12GetDebugInterface, name_D3D12GetDebugInterface
JMP_BY_NAME D3D12CoreCreateLayeredDevice, name_D3D12CoreCreateLayeredDevice
JMP_BY_NAME D3D12CoreGetLayeredDeviceSize, name_D3D12CoreGetLayeredDeviceSize
JMP_BY_NAME D3D12CoreRegisterLayers, name_D3D12CoreRegisterLayers
JMP_BY_NAME D3D12CreateRootSignatureDeserializer, name_D3D12CreateRootSignatureDeserializer
JMP_BY_NAME D3D12CreateVersionedRootSignatureDeserializer, name_D3D12CreateVersionedRootSignatureDeserializer
JMP_BY_NAME D3D12DeviceRemovedExtendedData, name_D3D12DeviceRemovedExtendedData
JMP_BY_NAME D3D12EnableExperimentalFeatures, name_D3D12EnableExperimentalFeatures
JMP_BY_NAME D3D12GetInterface, name_D3D12GetInterface
JMP_BY_NAME D3D12PIXEventsReplaceBlock, name_D3D12PIXEventsReplaceBlock
JMP_BY_NAME D3D12PIXGetThreadInfo, name_D3D12PIXGetThreadInfo
JMP_BY_NAME D3D12PIXNotifyWakeFromFenceSignal, name_D3D12PIXNotifyWakeFromFenceSignal
JMP_BY_NAME D3D12PIXReportCounter, name_D3D12PIXReportCounter
JMP_BY_NAME D3D12SerializeRootSignature, name_D3D12SerializeRootSignature
JMP_BY_NAME D3D12SerializeVersionedRootSignature, name_D3D12SerializeVersionedRootSignature
JMP_BY_NAME GetBehaviorValue, name_GetBehaviorValue

END
