#ifndef NETBOOT_H
#define NETBOOT_H

#define NETBOOT_DIR_SIZE 64

typedef struct { 
    char          be_name[32];  /* asciiZ   */ 
    unsigned int  be_offset;    /* 4K pages */ 
    unsigned int  be_type;      /* BE_*     */ 
    unsigned int  be_size;      /* 4K pages */ 
    unsigned int  be_vsize;     /* 4K pages */ 
    unsigned int  be_extra0; 
    unsigned int  be_extra1; 
    unsigned int  be_extra2; 
    unsigned int  be_extra3; 
} boot_entry; 

typedef struct { 
    boot_entry bd_entry[NETBOOT_DIR_SIZE]; 
} boot_dir; 

#define BE_TYPE_NONE         0  /* empty entry            */ 
#define BE_TYPE_DIRECTORY    1  /* directory (entry 0)    */ 
#define BE_TYPE_BOOTSTRAP    2  /* bootstrap code object  */ 
#define BE_TYPE_CODE         3  /* executable code object */ 
#define BE_TYPE_DATA         4  /* raw data object        */ 
#define BE_TYPE_ELF32        5  /* 32bit ELF object       */ 

/* for BE_TYPE_CODE / BE_TYPE_BOOTSTRAP */ 
#define be_code_vaddr be_extra0 /* virtual address (rel offset 0)     */ 
#define be_code_ventr be_extra1 /* virtual entry point (rel offset 0) */ 

#endif


