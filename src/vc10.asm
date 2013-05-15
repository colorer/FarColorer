;vc10.asm

;for VC2010 and Windows 2000
;http://tedwvc.wordpress.com/2010/11/07/how-to-get-visual-c-2010-mfc-applications-to-run-on-windows-2000/


.486
.model flat

EXTERNDEF __imp__EncodePointer@4 : DWORD
EXTERNDEF __imp__DecodePointer@4 : DWORD

.const
align 4
__imp__EncodePointer@4 dd dummy
__imp__DecodePointer@4 dd dummy

.code
dummy proc
mov eax, [esp+4]
ret 4
dummy endp

end
