OPTION CASEMAP:NONE
EXTERN TestResolveAGS:PROC
.code
PUBLIC GetOriginalAGSProcByName
GetOriginalAGSProcByName PROC FRAME
    sub rsp, 28h
    .allocstack 28h
    .endprolog
    call TestResolveAGS
    ; Force clobbering of the registers an ordinary resolver is allowed to use.
    ; The caller must preserve all original arguments around this resolver call.
    mov rcx, 0BAD00001h
    mov rdx, 0BAD00002h
    mov r8, 0BAD00003h
    mov r9, 0BAD00004h
    pxor xmm0, xmm0
    pxor xmm1, xmm1
    pxor xmm2, xmm2
    pxor xmm3, xmm3
    add rsp, 28h
    ret
GetOriginalAGSProcByName ENDP
END
