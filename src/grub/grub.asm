; grub.asm
section .text
global read_sector

; Function to read a sector from the disk
; Arguments:
;   - drive:  DL register (0x00 for first hard drive)
;   - lba:    CX register (logical block address)
;   - buffer: BX register (pointer to the buffer)
;   - count:  AL register (number of sectors to read)
read_sector:
    ; Set up registers for BIOS interrupt
    mov ah, 0x02        ; AH = 0x02 (read sectors)
    mov al, [esp + 4]   ; AL = number of sectors to read (from stack)
    mov bx, [esp + 8]   ; BX = pointer to buffer (from stack)
    mov dl, [esp + 12]  ; DL = drive number (from stack)

    ; Convert LBA to CH and CL
    xor cx, cx          ; Clear CX
    mov ax, [esp + 16]  ; Load LBA from stack
    mov ch, al          ; CH = high byte of LBA
    shr ax, 8           ; Shift right to get low byte
    mov cl, al          ; CL = low byte of LBA (sector number)

    int 0x13           ; Call BIOS interrupt

    ; Return from the function
    ret