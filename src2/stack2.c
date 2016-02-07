//                                __                 __
//   _________.__. ____   _______/  |______    ____ |  | __
//  /  ___<   |  |/    \ /  ___/\   __\__  \ _/ ___\|  |/ /
//  \___ \ \___  |   |  \\___ \  |  |  / __ \\  \___|    <
// /____  >/ ____|___|  /____  > |__| (____  /\___  >__|_ \
//      \/ \/         \/     \/            \/     \/     \/
//
// Stackbased audio synthesizer DSL // http://thi.ng/synstack
// (c) 2015-2016 Karsten Schmidt    // ASL2.0 licensed

//      =======
//   =======
// =======
// =======
//   =======
//      =======
//         =======
//           =======
//           =======
//         =======
//      =======

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "core_dict.h"

#define CTSS_VM_TRACE

#ifdef CTSS_VM_TRACE
#define CTSS_LOG(expr) printf expr
#else
#define CTSS_LOG(expr)
#endif

#define CTSS_VM_DS_SIZE 16
#define CTSS_VM_RS_SIZE 8
#define CTSS_VM_MEM_SIZE 0x400 // * sizeof(CTSS_VMValue)
#define CTSS_VM_TIB_SIZE 2
#define CTSS_VM_TIB_TOKEN_SIZE 64
#define CTSS_VM_RIB_SIZE 0x800
#define CTSS_VM_TOKEN_SIZE 64

#define CTSS_OP(name) ctss_vm_op_##name
#define CTSS_DECL_OP(name) static void CTSS_OP(name)(CTSS_VM * vm)
#define CTSS_DEFNATIVE(name, id)                                               \
    CTSS_VMValue *id = ctss_vm_defnative(vm, name, CTSS_OP(id))

typedef struct CTSS_VM CTSS_VM;
typedef struct CTSS_VMOpHeader CTSS_VMOpHeader;
typedef union CTSS_VMValue CTSS_VMValue;
typedef void (*CTSS_VMOp)(CTSS_VM *vm);

typedef enum {
    CTSS_VM_FLAG_HIDDEN = 1,
    CTSS_VM_FLAG_IMMEDIATE = 2,
    CTSS_VM_FLAG_NATIVE = 4,
} CTSS_VMFlag;

typedef enum { CTSS_VM_ERR_OK = 0, CTSS_VM_ERR_UNKNOWN_WORD = 1 } CTSS_VMError;

struct CTSS_VMOpHeader {
    CTSS_VMValue *next;
    char *name;
    uint8_t flags;
};

union CTSS_VMValue {
    CTSS_VMOpHeader *head;
    CTSS_VMValue *addr;
    CTSS_VMOp op;
    int32_t i32;
    float f32;
    char *str;
};

struct CTSS_VM {
    CTSS_VMValue ds[CTSS_VM_DS_SIZE];
    CTSS_VMValue rs[CTSS_VM_RS_SIZE];
    CTSS_VMValue *ip;
    CTSS_VMValue *np;
    CTSS_VMValue *dsp;
    CTSS_VMValue *rsp;
    CTSS_VMValue *latest;
    CTSS_VMValue *here;
    CTSS_VMValue mem[CTSS_VM_MEM_SIZE];
    char tib[CTSS_VM_TIB_SIZE][CTSS_VM_TIB_TOKEN_SIZE];
    char rbuf[CTSS_VM_RIB_SIZE];
    char token[CTSS_VM_TOKEN_SIZE];
    uint32_t readpos;
    uint8_t tibid;
    uint8_t mode;
    CTSS_VMError error;
};

typedef struct {
    CTSS_VMValue val;
    uint8_t err;
} CTSS_VMParseResult;

CTSS_DECL_OP(lit);

void ctss_vm_docolon(CTSS_VM *vm);

void ctss_vm_dump(CTSS_VM *vm);
void ctss_vm_dump_ds(CTSS_VM *vm);

void ctss_vm_init(CTSS_VM *vm) {
    memset(vm->ds, 0, sizeof(CTSS_VMValue) * CTSS_VM_DS_SIZE);
    memset(vm->rs, 0, sizeof(uint32_t) * CTSS_VM_RS_SIZE);
    memset(vm->mem, 0, sizeof(CTSS_VMValue) * CTSS_VM_MEM_SIZE);
    vm->mode = 0;
    vm->tibid = 0;
    vm->dsp = vm->ds;
    vm->rsp = vm->rs;
    vm->ip = vm->mem;
    vm->np = NULL;
    vm->latest = NULL;
    vm->here = vm->mem;
    vm->error = CTSS_VM_ERR_OK;
}

static inline void ctss_vm_push_ds(CTSS_VM *vm, CTSS_VMValue v) {
    *vm->dsp++ = v;
}

static inline CTSS_VMValue *ctss_vm_pop_ds(CTSS_VM *vm) {
    return --vm->dsp;
}

static inline void ctss_vm_push_rs(CTSS_VM *vm, CTSS_VMValue v) {
    *vm->rsp++ = v;
}

static inline CTSS_VMValue *ctss_vm_pop_rs(CTSS_VM *vm) {
    return --vm->rsp;
}

static inline void ctss_vm_push_mem(CTSS_VM *vm, CTSS_VMValue v) {
    *vm->here++ = v;
}

static inline uint8_t ctss_vm_ishidden(CTSS_VMOpHeader *head) {
    return head->flags & CTSS_VM_FLAG_HIDDEN;
}

static inline void ctss_vm_toggle_hidden(CTSS_VMOpHeader *head) {
    head->flags ^= CTSS_VM_FLAG_HIDDEN;
}

static inline uint8_t ctss_vm_isimmediate(CTSS_VMOpHeader *head) {
    return head->flags & CTSS_VM_FLAG_IMMEDIATE;
}

static inline void ctss_vm_set_immediate(CTSS_VMValue *addr, uint8_t state) {
    CTSS_VMOpHeader *hd = (*addr).head;
    if (state) {
        hd->flags |= CTSS_VM_FLAG_IMMEDIATE;
    } else {
        hd->flags &= ~CTSS_VM_FLAG_IMMEDIATE;
    }
}

static inline void ctss_vm_next(CTSS_VM *vm) {
    vm->ip = vm->np++;
}

static inline void ctss_vm_interpret_mode(CTSS_VM *vm) {
    CTSS_LOG(("<interpret>\n"));
    vm->mode = 0;
}

static inline void ctss_vm_compile_mode(CTSS_VM *vm) {
    CTSS_LOG(("<compiling>\n"));
    vm->mode = 1;
}

CTSS_VMValue *ctss_vm_create_header(CTSS_VM *vm, char *name) {
    CTSS_LOG(("new word header: %s, %p\n", name, vm->latest));
    CTSS_VMOpHeader *hd = (CTSS_VMOpHeader *)calloc(1, sizeof(CTSS_VMOpHeader));
    hd->next = vm->latest;
    hd->name = strdup(name);
    vm->latest = vm->here;
    CTSS_VMValue v = {.head = hd};
    ctss_vm_push_mem(vm, v);
    return vm->latest;
}

void ctss_vm_free_header(CTSS_VMOpHeader *hd) {
    free(hd->name);
    free(hd);
}

CTSS_VMValue *ctss_vm_defnative(CTSS_VM *vm, char *name, CTSS_VMOp op) {
    CTSS_VMValue *addr = ctss_vm_create_header(vm, name);
    CTSS_VMValue v = {.op = op};
    ctss_vm_push_mem(vm, v);
    return addr;
}

CTSS_VMValue *ctss_vm_defword(CTSS_VM *vm, char *name, ...) {
    va_list args;
    va_start(args, name);

    CTSS_VMValue *addr = ctss_vm_create_header(vm, name);
    CTSS_VMValue v = {.op = ctss_vm_docolon};
    ctss_vm_push_mem(vm, v);

    CTSS_VMValue *wAddr = va_arg(args, CTSS_VMValue *);
    while (wAddr != NULL) {
        CTSS_LOG(
            ("defword: %s op: %p\n", (*wAddr).head->name, (*(wAddr + 1)).op));
        CTSS_VMValue w = {.op = (*(wAddr + 1)).op}; // cfa
        ctss_vm_push_mem(vm, w);
        wAddr = va_arg(args, CTSS_VMValue *);
    }
    va_end(args);

    return addr;
}

CTSS_VMValue *ctss_vm_find_word(CTSS_VM *vm, char *word, CTSS_VMValue *addr) {
    while (addr != NULL) {
        CTSS_VMOpHeader *hd = (*addr).head;
        if (!ctss_vm_ishidden(hd) && !strcmp(word, hd->name)) {
            return addr;
        }
        addr = hd->next;
    }
    return 0;
}

uint8_t ctss_vm_tib_id(CTSS_VM *vm) {
    uint8_t id = vm->tibid;
    vm->tibid = (vm->tibid + 1) % CTSS_VM_TIB_SIZE;
    return id;
}

void ctss_vm_init_reader(CTSS_VM *vm, char *input) {
    memset(vm->rbuf, 0, CTSS_VM_RIB_SIZE);
    strncpy(vm->rbuf, input, CTSS_VM_RIB_SIZE);
    vm->readpos = 0;
}

static inline char ctss_vm_read_char(CTSS_VM *vm) {
    return vm->rbuf[vm->readpos++];
}

static inline uint8_t ctss_vm_isdelim(char c) {
    return c == ' ' || c == '\n' || c == '\0';
}

char ctss_vm_reader_skip_ws(CTSS_VM *vm) {
    char c;
    do {
        c = ctss_vm_read_char(vm);
    } while (c && ctss_vm_isdelim(c));
    return c;
}

char *ctss_vm_read_token(CTSS_VM *vm) {
    uint32_t pos = 0;
    char c = ctss_vm_reader_skip_ws(vm);
    if (!c) {
        return NULL;
    }
    vm->token[pos] = c;
    while (!ctss_vm_isdelim(c) && pos < CTSS_VM_TOKEN_SIZE - 1) {
        pos++;
        c = ctss_vm_read_char(vm);
        vm->token[pos] = c;
    }
    vm->token[pos] = 0;
    // TODO throw error or return NULL if token buf full
    return vm->token;
}

char *ctss_vm_buffer_token(CTSS_VM *vm, char *token) {
    char *tib = vm->tib[ctss_vm_tib_id(vm)];
    strncpy(tib, token, CTSS_VM_TIB_TOKEN_SIZE);
    return tib;
}

void ctss_vm_docolon(CTSS_VM *vm) {
    ctss_vm_push_rs(vm, *vm->np);
    vm->np = vm->ip + 1;
    ctss_vm_next(vm);
}

void ctss_vm_execute(CTSS_VM *vm) {
    while (vm->ip != NULL) {
        CTSS_LOG(("exe: %p %p\n", vm->ip, (*vm->ip).op));
        // ctss_vm_dump_ds(vm);
        (*vm->ip).op(vm);
    }
}

void ctss_vm_execute_word(CTSS_VM *vm, CTSS_VMValue *addr) {
    CTSS_VMOpHeader *hd = (*addr).head;
    if (!vm->mode || ctss_vm_isimmediate(hd)) {
        vm->ip = addr + 1;
        vm->np = NULL;
        ctss_vm_execute(vm);
    } else {
        CTSS_VMValue v = {.op = (*(addr + 1)).op};
        ctss_vm_push_mem(vm, v);
    }
}

CTSS_VMParseResult ctss_vm_parse_value(char *token) {
    CTSS_VMParseResult res = {.val = {.i32 = 0}, .err = 0};
    int32_t x;
    char *check = NULL;
    x = (int32_t)strtol(token, &check, 10);
    // CTSS_LOG(("parse: %s %d %d\n", token, x, *check));
    if (*check) {
        if (*token == '0' && *(token + 1) == 'x') {
            check = NULL;
            x = (int32_t)strtol(token + 2, &check, 16);
            res.val.i32 = x;
        } else if ((token[0] == '-' || (token[0] >= '0' && token[0] <= '9')) &&
                   token[strlen(token) - 1] == 'f') {
            check = NULL;
            float f = strtof(token, &check);
            res.val.f32 = f;
        } else {
            res.err = 1;
        }
    } else {
        res.val.i32 = x;
    }
    return res;
}

void ctss_vm_execute_literal(CTSS_VM *vm, CTSS_VMValue v) {
    if (!vm->mode) {
        ctss_vm_push_ds(vm, v);
    } else {
        CTSS_VMValue lit = {.op = CTSS_OP(lit)};
        ctss_vm_push_mem(vm, lit);
        ctss_vm_push_mem(vm, v);
    }
}

uint32_t ctss_vm_execute_token(CTSS_VM *vm, char *token) {
    CTSS_VMValue *word = ctss_vm_find_word(vm, token, vm->latest);
    if (word != NULL) {
        ctss_vm_execute_word(vm, word);
        return 0;
    }
    CTSS_VMParseResult res = ctss_vm_parse_value(token);
    if (!res.err) {
        ctss_vm_execute_literal(vm, res.val);
        return CTSS_VM_ERR_OK;
    }
    fprintf(stderr, "Unknown word: %s\n", token);
    vm->error = CTSS_VM_ERR_UNKNOWN_WORD;
    return CTSS_VM_ERR_UNKNOWN_WORD;
}

uint32_t ctss_vm_interpret(CTSS_VM *vm, char *input) {
    ctss_vm_init_reader(vm, input);
    char *token = ctss_vm_read_token(vm);
    uint32_t err = CTSS_VM_ERR_OK;
    while (token && !err) {
        err = ctss_vm_execute_token(vm, token);
        token = ctss_vm_read_token(vm);
    }
    return err;
}

CTSS_DECL_OP(create_header) {
    ctss_vm_create_header(vm, ctss_vm_pop_ds(vm)->str);
    CTSS_VMValue v = {.op = ctss_vm_docolon};
    ctss_vm_push_mem(vm, v);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(toggle_hidden) {
    ctss_vm_toggle_hidden((*vm->latest).head);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(here) {
    CTSS_VMValue v = {.addr = vm->here};
    ctss_vm_push_ds(vm, v);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(set_here) {
    vm->here = ctss_vm_pop_ds(vm)->addr;
    ctss_vm_next(vm);
}

CTSS_DECL_OP(latest) {
    CTSS_VMValue v = {.addr = vm->latest};
    ctss_vm_push_ds(vm, v);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(set_latest) {
    vm->latest = ctss_vm_pop_ds(vm)->addr;
    ctss_vm_next(vm);
}

CTSS_DECL_OP(push_dict) {
    ctss_vm_push_mem(vm, *ctss_vm_pop_ds(vm));
    ctss_vm_next(vm);
}

CTSS_DECL_OP(mem) {
    ctss_vm_push_ds(vm, *(ctss_vm_pop_ds(vm)->addr));
    ctss_vm_next(vm);
}

CTSS_DECL_OP(set_mem) {
    CTSS_VMValue *addr = ctss_vm_pop_ds(vm)->addr;
    *addr = *ctss_vm_pop_ds(vm);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(i32_addr) {
    CTSS_VMValue *dsp = vm->dsp - 1;
    (*dsp).addr = (*dsp).i32 + vm->mem;
    ctss_vm_next(vm);
}

CTSS_DECL_OP(addr_i32) {
    CTSS_VMValue *dsp = vm->dsp - 1;
    (*dsp).i32 = (*dsp).addr - vm->mem;
    ctss_vm_next(vm);
}

CTSS_DECL_OP(immediate) {
    ctss_vm_set_immediate(vm->latest, 1);
}

CTSS_DECL_OP(interpret) {
    ctss_vm_interpret_mode(vm);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(compile) {
    ctss_vm_compile_mode(vm);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(find) {
    ctss_vm_push_ds(
        vm, *ctss_vm_find_word(vm, ctss_vm_pop_ds(vm)->str, vm->latest));
    ctss_vm_next(vm);
}

CTSS_DECL_OP(cfa) {
    CTSS_VMValue *addr = ctss_vm_pop_ds(vm);
    CTSS_VMValue cfa = {.addr = addr->addr + 1};
    ctss_vm_push_ds(vm, cfa);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(lit) {
    ctss_vm_push_ds(vm, *vm->np++);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(ret) {
    vm->np = ctss_vm_pop_rs(vm);
    // CTSS_LOG(("ret: %p\n", vm->np));
    ctss_vm_next(vm);
}

CTSS_DECL_OP(swap) {
    CTSS_VMValue *dsp = vm->dsp - 1;
    CTSS_VMValue a = *dsp;
    *dsp = *(dsp - 1);
    *(dsp - 1) = a;
    ctss_vm_next(vm);
}

CTSS_DECL_OP(dup) {
    *vm->dsp = *(vm->dsp - 1);
    vm->dsp++;
    ctss_vm_next(vm);
}

CTSS_DECL_OP(drop) {
    vm->dsp--;
    ctss_vm_next(vm);
}

CTSS_DECL_OP(over) {
    *vm->dsp = *(vm->dsp - 2);
    vm->dsp++;
    ctss_vm_next(vm);
}

CTSS_DECL_OP(rot) {
    CTSS_VMValue *dsp = vm->dsp - 1;
    CTSS_VMValue c = *(dsp - 2);
    *(dsp - 2) = *(dsp - 1);
    *(dsp - 1) = *dsp;
    *dsp = c;
    ctss_vm_next(vm);
}

CTSS_DECL_OP(rot_inv) {
    CTSS_VMValue *dsp = vm->dsp - 1;
    CTSS_VMValue a = *dsp;
    *dsp = *(dsp - 1);
    *(dsp - 1) = *(dsp - 2);
    *(dsp - 2) = a;
    ctss_vm_next(vm);
}

CTSS_DECL_OP(rpush) {
    ctss_vm_push_rs(vm, *ctss_vm_pop_ds(vm));
    ctss_vm_next(vm);
}

CTSS_DECL_OP(rpop) {
    ctss_vm_push_ds(vm, *ctss_vm_pop_rs(vm));
    ctss_vm_next(vm);
}

CTSS_DECL_OP(branch) {
    vm->np += (*vm->np).i32;
    ctss_vm_next(vm);
}

CTSS_DECL_OP(branch0) {
    if ((*ctss_vm_pop_ds(vm)).i32) {
        vm->np++;
        ctss_vm_next(vm);
    } else {
        CTSS_OP(branch);
    }
}

CTSS_DECL_OP(read_token) {
    char *token = ctss_vm_buffer_token(vm, ctss_vm_read_token(vm));
    CTSS_VMValue v = {.str = token};
    ctss_vm_push_ds(vm, v);
    ctss_vm_next(vm);
}

#define CTSS_DECL_MATH_OP(name, op, aa, bb, type, ctype)                       \
    CTSS_DECL_OP(name##_##type) {                                              \
        CTSS_VMValue *dsp = vm->dsp - 1;                                       \
        ctype a = (*dsp).type;                                                 \
        ctype b = (*(dsp - 1)).type;                                           \
        printf("%f op %f\n", (float)a, (float)b);                              \
        CTSS_VMValue v = {.type = aa op bb};                                   \
        *(dsp - 1) = v;                                                        \
        vm->dsp = dsp;                                                         \
        ctss_vm_next(vm);                                                      \
    }

CTSS_DECL_MATH_OP(add, +, a, b, i32, uint32_t)
CTSS_DECL_MATH_OP(mul, *, a, b, i32, uint32_t)
CTSS_DECL_MATH_OP(sub, -, b, a, i32, uint32_t)
CTSS_DECL_MATH_OP(div, /, b, a, i32, uint32_t)

CTSS_DECL_MATH_OP(add, +, a, b, f32, float)
CTSS_DECL_MATH_OP(mul, *, a, b, f32, float)
CTSS_DECL_MATH_OP(sub, -, b, a, f32, float)
CTSS_DECL_MATH_OP(div, /, b, a, f32, float)

#define CTSS_DECL_CMP_OP(name, op, type, ctype)                                \
    CTSS_DECL_OP(cmp_##name##_##type) {                                        \
        CTSS_VMValue *dsp = vm->dsp - 1;                                       \
        ctype b = (*dsp).type;                                                 \
        ctype a = (*(dsp - 1)).type;                                           \
        CTSS_VMValue v = {.i32 = ((a op b) ? 1 : 0)};                          \
        *(dsp - 1) = v;                                                        \
        vm->dsp = dsp;                                                         \
        ctss_vm_next(vm);                                                      \
    }

CTSS_DECL_CMP_OP(lt, <, i32, uint32_t)
CTSS_DECL_CMP_OP(gt, >, i32, uint32_t)
CTSS_DECL_CMP_OP(le, <=, i32, uint32_t)
CTSS_DECL_CMP_OP(ge, >=, i32, uint32_t)
CTSS_DECL_CMP_OP(eq, ==, i32, uint32_t)
CTSS_DECL_CMP_OP(neq, !=, i32, uint32_t)

CTSS_DECL_CMP_OP(lt, <, f32, float)
CTSS_DECL_CMP_OP(gt, >, f32, float)
CTSS_DECL_CMP_OP(le, <=, f32, float)
CTSS_DECL_CMP_OP(ge, >=, f32, float)
CTSS_DECL_CMP_OP(eq, ==, f32, float)
CTSS_DECL_CMP_OP(neq, !=, f32, float)

CTSS_DECL_OP(i32_f32) {
    (*(vm->dsp - 1)).f32 = (float)((*(vm->dsp - 1)).i32);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(f32_i32) {
    (*(vm->dsp - 1)).i32 = (int32_t)((*(vm->dsp - 1)).f32);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(sinf) {
    (*(vm->dsp - 1)).f32 = sinf((*(vm->dsp - 1)).f32);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(cosf) {
    (*(vm->dsp - 1)).f32 = cosf((*(vm->dsp - 1)).f32);
    ctss_vm_next(vm);
}

void ctss_vm_dump_words(CTSS_VM *vm) {
    CTSS_VMValue *addr = vm->latest;
    while (addr != NULL) {
        CTSS_VMOpHeader *hd = (*addr).head;
        printf("addr: %p %s op: %p next: %p\n", addr, hd->name,
               (*(addr + 1)).op, hd->next);
        addr = hd->next;
    }
}

void ctss_vm_dump_ds(CTSS_VM *vm) {
    for (CTSS_VMValue *i = vm->dsp - 1; i >= vm->ds; i--) {
        printf("<%ld> %d %f (%p)\n", i - vm->ds, (*i).i32, (*i).f32, (*i).addr);
    }
    printf("--\n");
}

void ctss_vm_dump_rs(CTSS_VM *vm) {
    for (CTSS_VMValue *i = vm->rsp - 1; i >= vm->rs; i--) {
        printf("<%p> %p\n", i, (*i).addr);
    }
    printf("--\n");
}

void ctss_vm_dump(CTSS_VM *vm) {
    printf("dsp: %p, rsp: %p, ip: %p, np: %p, latest: %p, here: %p, used: %lu, "
           "err: %u\n",
           vm->dsp, vm->rsp, vm->ip, vm->np, vm->latest, vm->here,
           vm->here - vm->mem, vm->error);
}

CTSS_DECL_OP(dump_vm) {
    ctss_vm_dump(vm);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(dump_words) {
    ctss_vm_dump_words(vm);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(dump_ds) {
    ctss_vm_dump_ds(vm);
    ctss_vm_next(vm);
}

CTSS_DECL_OP(dump_rs) {
    ctss_vm_dump_rs(vm);
    ctss_vm_next(vm);
}

void ctss_vm_init_primitives(CTSS_VM *vm) {
    CTSS_DEFNATIVE("ret", ret);
    CTSS_DEFNATIVE("lit", lit);
    CTSS_DEFNATIVE("create-header", create_header);

    // stack
    CTSS_DEFNATIVE("swap", swap);
    CTSS_DEFNATIVE("dup", dup);
    CTSS_DEFNATIVE("drop", drop);
    CTSS_DEFNATIVE("over", over);
    CTSS_DEFNATIVE("rot", rot);
    CTSS_DEFNATIVE("rot-", rot_inv);
    CTSS_DEFNATIVE(">r", rpush);
    CTSS_DEFNATIVE("r>", rpop);

    // dict
    CTSS_DEFNATIVE("dict-here", here);
    CTSS_DEFNATIVE("dict-here!", set_here);
    CTSS_DEFNATIVE("push-dict", push_dict);
    CTSS_DEFNATIVE("latest", latest);
    CTSS_DEFNATIVE("latest!", set_latest);
    CTSS_DEFNATIVE("find", find);
    CTSS_DEFNATIVE(">cfa", cfa);

    // memory
    CTSS_DEFNATIVE("@", mem);
    CTSS_DEFNATIVE("!", set_mem);
    CTSS_DEFNATIVE("i>a", i32_addr);
    CTSS_DEFNATIVE("a>i", addr_i32);

    // flags
    CTSS_DEFNATIVE("toggle-hidden", toggle_hidden);
    CTSS_DEFNATIVE("immediate!", immediate);
    CTSS_DEFNATIVE("interpret>>", interpret);
    ctss_vm_set_immediate(interpret, 1);
    CTSS_DEFNATIVE("compile>>", compile);

    // word
    CTSS_DEFNATIVE("read-token>", read_token);
    CTSS_VMValue *colon = ctss_vm_defword(vm, ":", read_token, create_header,
                                          toggle_hidden, compile, ret, NULL);
    CTSS_VMValue *semicolon = ctss_vm_defword(
        vm, ";", lit, ret, push_dict, toggle_hidden, interpret, ret, NULL);
    ctss_vm_set_immediate(semicolon, 1);

    // branch
    CTSS_DEFNATIVE("branch", branch);
    CTSS_DEFNATIVE("0branch", branch0);

    // maths
    CTSS_DEFNATIVE("+", add_i32);
    CTSS_DEFNATIVE("*", mul_i32);
    CTSS_DEFNATIVE("-", sub_i32);
    CTSS_DEFNATIVE("/", div_i32);
    CTSS_DEFNATIVE("+f", add_f32);
    CTSS_DEFNATIVE("*f", mul_f32);
    CTSS_DEFNATIVE("-f", sub_f32);
    CTSS_DEFNATIVE("/f", div_f32);

    CTSS_DEFNATIVE("<", cmp_lt_i32);
    CTSS_DEFNATIVE(">", cmp_gt_i32);
    CTSS_DEFNATIVE("<=", cmp_le_i32);
    CTSS_DEFNATIVE(">=", cmp_ge_i32);
    CTSS_DEFNATIVE("==", cmp_eq_i32);
    CTSS_DEFNATIVE("!=", cmp_neq_i32);
    CTSS_DEFNATIVE("<f", cmp_lt_f32);
    CTSS_DEFNATIVE(">f", cmp_gt_f32);
    CTSS_DEFNATIVE("<=f", cmp_le_f32);
    CTSS_DEFNATIVE(">=f", cmp_ge_f32);
    CTSS_DEFNATIVE("==f", cmp_eq_f32);
    CTSS_DEFNATIVE("!=f", cmp_neq_f32);

    CTSS_DEFNATIVE("i>f", i32_f32);
    CTSS_DEFNATIVE("f>i", f32_i32);

    CTSS_DEFNATIVE("sinf", sinf);
    CTSS_DEFNATIVE("cosf", cosf);

    // vector / buffer
    // TODO

    // trace
    CTSS_DEFNATIVE(".vm", dump_vm);
    CTSS_DEFNATIVE(".words", dump_words);
    CTSS_DEFNATIVE(".s", dump_ds);
    CTSS_DEFNATIVE(".r", dump_rs);
}

int main() {
    printf("CTSS_VM:\t%lu\n", sizeof(CTSS_VM));

    CTSS_VM vm;
    ctss_vm_init(&vm);
    ctss_vm_dump(&vm);

    ctss_vm_init_primitives(&vm);

    ctss_vm_dump(&vm);
    ctss_vm_dump_words(&vm);
    // ctss_vm_dump_ds(&vm);

    // ctss_vm_interpret(&vm, ctss_vm_core_dict);
    ctss_vm_interpret(
        &vm, ": pi 3.1415926f ; : madd rot- * + ; 3 5 0xa madd pi cosf");
    ctss_vm_dump(&vm);
    ctss_vm_dump_ds(&vm);
    ctss_vm_dump_rs(&vm);
    return 0;
}
