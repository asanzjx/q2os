;/***************************************************
;
; q2os boot
;
;***************************************************/


	org	0x7c00	

BaseOfStack	equ	0x7c00

; Base ofLo ader equ 0x100和off se tOfLo ad er equ 0x00组合成了Loader 程序的起始物理地址，这个组合必须经过实模式的地址变换公式才能生成物理地址，即BaseofLoader << 4 + OffsetOfLoader = 0x10000.
BaseOfLoader	equ	0x1000
OffsetOfLoader	equ	0x00

; RootDirsectors equ 14定义了根目录占用的扇区数，这个数值是根据FAT12文件系统提供的信息经过计算而得，即(BPB_RootEntCnt *32+ BPB_BytesPerSec- 1)/ BPB_Bytes PerSec = (224X32 + 512 - )1 / 512 = 140
RootDirSectors	equ	14
; SectorNumofRootDirStart equ19定义了根目录的起始扇区号，这个数值也是通过计算而得，即保留扇区数+FAT表扇区数*FAT表份数= 1 + 9 * 2 = 19，因为扇区编号的计数值从0 开始，故根目录的起始扇区号为19
SectorNumOfRootDirStart	equ	19
; SectorNumofFAT1st art equ 1代表了FAT1表的起始扇区号，在FAT1表前面只有一个保留扇区(引导扇区)，而且它的扇 区编号是0，那么FAT1表的起始扇区号理应为1。
SectorNumOfFAT1Start	equ	1
; SectorBalance equ 17 用于平衡文件(或者目录)的起始簇号与数据区起始簇号的差值。更通俗点说，因数据区对应的有效簇号是2(FAT[2])，为了正确计算出FAT表项对应的数据区起始扇区号，则必须将FAT表项值减2，或者将数据区的起始簇号/ 扇区号减2(仅在每簇由一个扇区组成时可用)。本程序暂时采用 一种更取巧的方法是，将根目录起始扇区号减2(19-2=17)，进而 间接把数据区的起始扇区号(数据区起始扇区号=根目录起始扇区号+根目录所占扇区数)减2
SectorBalance	equ	17	

	jmp	short Label_Start
	nop
	BS_OEMName	db	'q2osboot'
	BPB_BytesPerSec	dw	512
	BPB_SecPerClus	db	1
	BPB_RsvdSecCnt	dw	1
	BPB_NumFATs	db	2
	BPB_RootEntCnt	dw	224
	BPB_TotSec16	dw	2880
	BPB_Media	db	0xf0
	BPB_FATSz16	dw	9
	BPB_SecPerTrk	dw	18
	BPB_NumHeads	dw	2
	BPB_HiddSec	dd	0
	BPB_TotSec32	dd	0
	BS_DrvNum	db	0
	BS_Reserved1	db	0
	BS_BootSig	db	0x29
	BS_VolID	dd	0
	BS_VolLab	db	'boot loader'
	BS_FileSysType	db	'FAT12   '

Label_Start:

	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	BaseOfStack

;=======	clear screen

	mov	ax,	0600h
	mov	bx,	0700h
	mov	cx,	0
	mov	dx,	0184fh
	int	10h

;=======	set focus

	mov	ax,	0200h
	mov	bx,	0000h
	mov	dx,	0000h
	int	10h

;=======	display on screen : Start Booting......

	mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0000h
	mov	cx,	10
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartBootMessage
	int	10h

;=======	reset floppy

	xor	ah,	ah
	xor	dl,	dl
	int	13h

;=======	search loader.bin
	mov	word	[SectorNo],	SectorNumOfRootDirStart

Lable_Search_In_Root_Dir_Begin:

	cmp	word	[RootDirSizeForLoop],	0
	jz	Label_No_LoaderBin
	dec	word	[RootDirSizeForLoop]	
	mov	ax,	00h
	mov	es,	ax
	mov	bx,	8000h
	mov	ax,	[SectorNo]
	mov	cl,	1
	call	Func_ReadOneSector
	mov	si,	LoaderFileName
	mov	di,	8000h
	cld
	mov	dx,	10h
	
Label_Search_For_LoaderBin:

	cmp	dx,	0
	jz	Label_Goto_Next_Sector_In_Root_Dir
	dec	dx
	mov	cx,	11

Label_Cmp_FileName:

	cmp	cx,	0
	jz	Label_FileName_Found
	dec	cx
	lodsb	
	cmp	al,	byte	[es:di]
	jz	Label_Go_On
	jmp	Label_Different

Label_Go_On:
	
	inc	di
	jmp	Label_Cmp_FileName

Label_Different:

	and	di,	0ffe0h
	add	di,	20h
	mov	si,	LoaderFileName
	jmp	Label_Search_For_LoaderBin

Label_Goto_Next_Sector_In_Root_Dir:
	
	add	word	[SectorNo],	1
	jmp	Lable_Search_In_Root_Dir_Begin
	
;=======	display on screen : ERROR:No LOADER Found

Label_No_LoaderBin:

	mov	ax,	1301h
	mov	bx,	008ch
	mov	dx,	0100h
	mov	cx,	21
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	NoLoaderMessage
	int	10h
	jmp	$

;=======	found loader.bin name in root director struct

Label_FileName_Found:

	mov	ax,	RootDirSectors
	and	di,	0ffe0h
	add	di,	01ah
	mov	cx,	word	[es:di]
	push	cx
	add	cx,	ax
	add	cx,	SectorBalance
	mov	ax,	BaseOfLoader
	mov	es,	ax
	mov	bx,	OffsetOfLoader
	mov	ax,	cx

Label_Go_On_Loading_File:
	push	ax
	push	bx
	mov	ah,	0eh
	mov	al,	'.'
	mov	bl,	0fh
	int	10h
	pop	bx
	pop	ax

	mov	cl,	1
	call	Func_ReadOneSector
	pop	ax
	call	Func_GetFATEntry
	cmp	ax,	0fffh
	jz	Label_File_Loaded
	push	ax
	mov	dx,	RootDirSectors
	add	ax,	dx
	add	ax,	SectorBalance
	add	bx,	[BPB_BytesPerSec]
	jmp	Label_Go_On_Loading_File

Label_File_Loaded:
	
	jmp	BaseOfLoader:OffsetOfLoader

;=======	read one sector from floppy
; BIOS中断服务程序INT13h 的主功能号AH=02h实现软盘扇区的读取操作，该中断服务程序的各寄存器参数说明如下。
; INT13h，AH =02h 功能:读取磁盘扇区。
; AL = 读入的扇区数(必须非0);
; CH = 磁道号(柱面号)的低8位;
; CL = 扇区号 1 ~ 63 (bit0 ~ 5)，磁道号(柱面号)的高 2 位 (bit6 ~ 7 ，只对硬盘有效);
; DH = 磁头号;
; DL = 驱动器号(如果操作的是硬盘驱动器，bit7必须被置位);
; ES:BX=>数据缓冲区
;
; 模块Func_Read oneSec tor功能:读取磁盘扇区。
; AX = 待读取的磁盘起始扇区号;
; CL = 读人的扇区数量;
; ES:BX => 目标缓冲区起始地址
;================

Func_ReadOneSector:
	
	push	bp
	mov	bp,	sp
	sub	esp,	2
	mov	byte	[bp - 2],	cl
	push	bx
	mov	bl,	[BPB_SecPerTrk]
	div	bl
	inc	ah
	mov	cl,	ah
	mov	dh,	al
	shr	al,	1
	mov	ch,	al
	and	dh,	1
	pop	bx
	mov	dl,	[BS_DrvNum]
Label_Go_On_Reading:
	mov	ah,	2
	mov	al,	byte	[bp - 2]
	int	13h
	jc	Label_Go_On_Reading
	add	esp,	2
	pop	bp
	ret

;=======	get FAT Entry

Func_GetFATEntry:

	push	es
	push	bx
	push	ax
	mov	ax,	00
	mov	es,	ax
	pop	ax
	mov	byte	[Odd],	0
	mov	bx,	3
	mul	bx
	mov	bx,	2
	div	bx
	cmp	dx,	0
	jz	Label_Even
	mov	byte	[Odd],	1

Label_Even:

	xor	dx,	dx
	mov	bx,	[BPB_BytesPerSec]
	div	bx
	push	dx
	mov	bx,	8000h
	add	ax,	SectorNumOfFAT1Start
	mov	cl,	2
	call	Func_ReadOneSector
	
	pop	dx
	add	bx,	dx
	mov	ax,	[es:bx]
	cmp	byte	[Odd],	1
	jnz	Label_Even_2
	shr	ax,	4

Label_Even_2:
	and	ax,	0fffh
	pop	bx
	pop	es
	ret

;=======	tmp variable

RootDirSizeForLoop	dw	RootDirSectors
SectorNo		dw	0
Odd			db	0

;=======	display messages
; BIOS 中断服务程序必须明确提供显示的字符串的长度，不需要判读字符串结尾处的字符:10' 

StartBootMessage:	db	"Start Boot"
NoLoaderMessage:	db	"ERROR:No LOADER Found"
LoaderFileName:		db	"LOADER  BIN",0

;=======	fill zero until whole sector

	times	510 - ($ - $$)	db	0
	dw	0xaa55

