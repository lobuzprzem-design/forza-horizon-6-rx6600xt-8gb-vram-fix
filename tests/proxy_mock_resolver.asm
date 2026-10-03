OPTION CASEMAP:NONE
.code
ClobberForwardingArguments PROC
    pxor xmm0, xmm0
    pxor xmm1, xmm1
    pxor xmm2, xmm2
    pxor xmm3, xmm3
    xor rcx, rcx
    xor rdx, rdx
    xor r8, r8
    xor r9, r9
    ret
ClobberForwardingArguments ENDP
END
