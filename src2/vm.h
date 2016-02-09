//           =====
//      =======                                __                 __
//   =======      _________.__. ____   _______/  |______    ____ |  | __
// =======       /  ___<   |  |/    \ /  ___/\   __\__  \ _/ ___\|  |/ /
// =======       \___ \ \___  |   |  \\___ \  |  |  / __ \\  \___|    <
//   =======    /____  >/ ____|___|  /____  > |__| (____  /\___  >__|_ \
//      =======      \/ \/         \/     \/            \/     \/     \/
//         =======
//           =======    A Forth-based audio synthesizer DSL
//           =======    http://thi.ng/synstack
//         =======      (c) 2015-2016 Karsten Schmidt // ASL2.0 licensed
//      =======
//   =====

#pragma once

#include <stdint.h>
#include "vm_conf.h"

#ifdef CTSS_VM_FEATURE_PRINT

#ifndef CTSS_PRINT_FN
#define CTSS_PRINT_FN printf
#endif /* CTSS_PRINT_FN */

#define CTSS_PRINT(expr) CTSS_PRINT_FN expr

#ifdef CTSS_VM_FEATURE_TRACE
#define CTSS_TRACE(expr) CTSS_PRINT_FN expr
#else
#define CTSS_TRACE(expr)
#endif /* CTSS_VM_FEATURE_TRACE */

#else /* CTSS_VM_FEATURE_PRINT */

#define CTSS_PRINT(expr)
#define CTSS_TRACE(expr)

#endif /* CTSS_VM_FEATURE_PRINT */

#ifdef CTSS_VM_FEATURE_BOUNDS
#define CTSS_VM_BOUNDS_CHECK_LO(ptr, buf, id, x)                               \
    if (vm->ptr < vm->buf + x) {                                               \
        vm->error = CTSS_VM_ERR_##id##_UNDERFLOW;                              \
        return;                                                                \
    }

#define CTSS_VM_BOUNDS_CHECK_BOTH(ptr, buf, id, x, y)                          \
    if (vm->ptr < vm->buf + x) {                                               \
        vm->error = CTSS_VM_ERR_##id##_UNDERFLOW;                              \
        return;                                                                \
    } else if (vm->ptr >= vm->buf + CTSS_VM_##id##_SIZE - y) {                 \
        vm->error = CTSS_VM_ERR_##id##_OVERFLOW;                               \
        return;                                                                \
    }
#define CTSS_VM_BOUNDS_CHECK_HI(ptr, buf, id, y)                               \
    if (vm->ptr >= vm->buf + CTSS_VM_##id##_SIZE - y) {                        \
        vm->error = CTSS_VM_ERR_##id##_OVERFLOW;                               \
        return;                                                                \
    }
#else

#define CTSS_VM_BOUNDS_CHECK_LO(ptr, buf, id, x)
#define CTSS_VM_BOUNDS_CHECK_BOTH(ptr, buf, id, x, y)
#define CTSS_VM_BOUNDS_CHECK_HI(ptr, buf, id, y)

#endif /* CTSS_VM_FEATURE_BOUNDS */

#ifndef CTSS_VM_DS_SIZE
#define CTSS_VM_DS_SIZE 4
#endif

#ifndef CTSS_VM_RS_SIZE
#define CTSS_VM_RS_SIZE 8
#endif

#ifndef CTSS_VM_MEM_SIZE
#define CTSS_VM_MEM_SIZE 0x400 // (cells * sizeof(CTSS_VMValue))
#endif

#ifndef CTSS_VM_TIB_SIZE
#define CTSS_VM_TIB_SIZE 2
#endif

#ifndef CTSS_VM_TIB_TOKEN_SIZE
#define CTSS_VM_TIB_TOKEN_SIZE 64
#endif

#ifndef CTSS_VM_RIB_SIZE
#define CTSS_VM_RIB_SIZE 0x800
#endif

#ifndef CTSS_VM_TOKEN_SIZE
#define CTSS_VM_TOKEN_SIZE 64
#endif

#define CTSS_OP(name) ctss_vm_op_##name
#define CTSS_DECL_OP(name) static void CTSS_OP(name)(CTSS_VM * vm)
#define CTSS_DEFNATIVE(name, id)                                               \
    uint32_t id = ctss_vm_defnative(vm, name, CTSS_OP(id))

typedef struct CTSS_VM CTSS_VM;
typedef struct CTSS_VMOpHeader CTSS_VMOpHeader;
typedef union CTSS_VMValue CTSS_VMValue;
typedef void (*CTSS_VMOp)(CTSS_VM *vm);

typedef enum {
    CTSS_VM_ERR_OK = 0,
    CTSS_VM_ERR_UNKNOWN_WORD,
    CTSS_VM_ERR_DS_UNDERFLOW,
    CTSS_VM_ERR_DS_OVERFLOW,
    CTSS_VM_ERR_RS_UNDERFLOW,
    CTSS_VM_ERR_RS_OVERFLOW,
    CTSS_VM_ERR_MEM_OVERFLOW,
    CTSS_VM_ERR_TOKEN_OVERFLOW,
} CTSS_VMError;

typedef enum {
    CTSS_VM_FLAG_HIDDEN = 1,
    CTSS_VM_FLAG_IMMEDIATE = 2,
    CTSS_VM_FLAG_NATIVE = 4,
} CTSS_VMFlag;

struct CTSS_VMOpHeader {
    uint32_t next;
    char *name;
    uint8_t flags;
};

union CTSS_VMValue {
    CTSS_VMOpHeader *head;
    CTSS_VMOp op;
    int32_t i32;
#ifdef CTSS_VM_FEATURE_FLOAT
    float f32;
#endif
#ifdef CTSS_VM_CUSTOM_TYPES
    CTSS_VM_CUSTOM_TYPES
#endif
    char *str;
};

struct CTSS_VM {
    CTSS_VMValue *dsp;
    CTSS_VMValue *rsp;
    CTSS_VMValue ds[CTSS_VM_DS_SIZE];
    CTSS_VMValue rs[CTSS_VM_RS_SIZE];
    uint32_t ip;
    uint32_t np;
    uint32_t latest;
    uint32_t here;
    uint8_t mode;
    CTSS_VMValue mem[CTSS_VM_MEM_SIZE];
    char tib[CTSS_VM_TIB_SIZE][CTSS_VM_TIB_TOKEN_SIZE];
    char rbuf[CTSS_VM_RIB_SIZE];
    char token[CTSS_VM_TOKEN_SIZE];
    uint32_t readpos;
    uint8_t tibid;
    CTSS_VMError error;
};

typedef struct {
    CTSS_VMValue val;
    uint8_t err;
} CTSS_VMParseResult;

void ctss_vm_init(CTSS_VM *vm);
void ctss_vm_init_primitives(CTSS_VM *vm);

uint32_t ctss_vm_make_header(CTSS_VM *vm, char *name);
void ctss_vm_free_header(CTSS_VMOpHeader *hd);

uint32_t ctss_vm_defnative(CTSS_VM *vm, char *name, CTSS_VMOp op);
uint32_t ctss_vm_defword(CTSS_VM *vm, char *name, ...);
uint32_t ctss_vm_find_word(CTSS_VM *vm, char *word, uint32_t addr);

void ctss_vm_init_reader(CTSS_VM *vm, char *input);
char *ctss_vm_read_token(CTSS_VM *vm);
char *ctss_vm_buffer_token(CTSS_VM *vm, char *token);

uint32_t ctss_vm_execute_token(CTSS_VM *vm, char *token);
uint32_t ctss_vm_interpret(CTSS_VM *vm, char *input);

char *ctss_vm_get_error_desc(uint8_t err);
void ctss_vm_dump_error(CTSS_VM *vm);

#ifdef CTSS_VM_FEATURE_PRINT

void ctss_vm_dump(CTSS_VM *vm);
void ctss_vm_dump_mem(CTSS_VM *vm);
void ctss_vm_dump_words(CTSS_VM *vm);
void ctss_vm_dump_ds(CTSS_VM *vm);
void ctss_vm_dump_rs(CTSS_VM *vm);
void ctss_vm_dump_tos_i32(CTSS_VM *vm);
void ctss_vm_dump_tos_i32_hex(CTSS_VM *vm);
void ctss_vm_dump_tos_f32(CTSS_VM *vm);

#endif /* CTSS_VM_FEATURE_PRINT */
