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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "vm.h"

CTSS_DECL_OP(lit);
CTSS_DECL_OP(docolon);

static uint32_t ctss_vm_cfa_ret;
static uint32_t ctss_vm_cfa_lit;
static CTSS_VMValue ctss_vm_errval = {.i32 = 0xdeadbeef};

static char *ctss_vm_errors[] = {"",
                                 "unknown word",
                                 "DS underflow",
                                 "DS overflow",
                                 "RS underflow",
                                 "RS overflow",
                                 "out of memory",
                                 "token too long",
                                 "unterminated string",
                                 "IO error"};

#define INV_RAND_MAX (float)(1.0 / UINT32_MAX)
#define INV_RAND_MAX2 (float)(2.0 / UINT32_MAX)

static uint32_t ctss_vm_rndX = 0xdecafbad;
static uint32_t ctss_vm_rndY = 0x2fa9d05b;
static uint32_t ctss_vm_rndZ = 0x041f67e3;
static uint32_t ctss_vm_rndW = 0x5c83ec1a;

// xorshift128 - https://en.wikipedia.org/wiki/Xorshift
static inline uint32_t ctss__rand() {
    uint32_t t = ctss_vm_rndX;
    t ^= t << 11;
    t ^= t >> 8;
    ctss_vm_rndX = ctss_vm_rndY;
    ctss_vm_rndY = ctss_vm_rndZ;
    ctss_vm_rndZ = ctss_vm_rndW;
    ctss_vm_rndW ^= ctss_vm_rndW >> 19;
    return ctss_vm_rndW ^= t;
}

static inline float ctss_rand_i32(const int32_t min, const int32_t max) {
    return min + ctss__rand() % (max - min);
}

static inline float ctss_rand_f32(const float min, const float max) {
    return min + (float)ctss__rand() * INV_RAND_MAX * (max - min);
}

static inline float ctss_normrandf() {
    return (float)ctss__rand() * INV_RAND_MAX2 - 1.0f;
}

void ctss_vm_init(CTSS_VM *vm) {
    memset(vm->ds, 0, sizeof(CTSS_VMValue) * CTSS_VM_DS_SIZE);
    memset(vm->rs, 0, sizeof(uint32_t) * CTSS_VM_RS_SIZE);
    memset(vm->mem, 0, sizeof(CTSS_VMValue) * CTSS_VM_MEM_SIZE);
    vm->mode = 0;
    vm->tibid = 0;
    vm->dsp = vm->ds;
    vm->rsp = vm->rs;
    vm->np = 0;
    vm->latest = 0;
    vm->here = 1;
    vm->error = CTSS_VM_ERR_OK;
}

static inline void ctss_vm_push_ds(CTSS_VM *vm, CTSS_VMValue v) {
    CTSS_VM_BOUNDS_CHECK_HI(dsp, ds, DS, 0)
    CTSS_TRACE(("push DS: %x\n", v.i32));
    *vm->dsp++ = v;
}

static inline CTSS_VMValue ctss_vm_pop_ds(CTSS_VM *vm) {
#ifdef CTSS_VM_FEATURE_BOUNDS
    if (vm->dsp <= vm->ds) {
        vm->error = CTSS_VM_ERR_DS_UNDERFLOW;
        return ctss_vm_errval;
    }
#endif
    CTSS_VMValue *v = --vm->dsp;
    CTSS_TRACE(("pop DS: %x\n", v->i32));
    return *v;
}

static inline void ctss_vm_push_rs(CTSS_VM *vm, CTSS_VMValue v) {
    CTSS_VM_BOUNDS_CHECK_HI(rsp, rs, RS, 0)
    CTSS_TRACE(("push RS: %u\n", v.i32));
    *vm->rsp++ = v;
}

static inline CTSS_VMValue ctss_vm_pop_rs(CTSS_VM *vm) {
#ifdef CTSS_VM_FEATURE_BOUNDS
    if (vm->rsp <= vm->rs) {
        vm->error = CTSS_VM_ERR_RS_UNDERFLOW;
        return ctss_vm_errval;
    }
#endif
    CTSS_VMValue *v = --vm->rsp;
    CTSS_TRACE(("pop RS: %u\n", v->i32));
    return *v;
}

static inline void ctss_vm_push_dict(CTSS_VM *vm, CTSS_VMValue v) {
#ifdef CTSS_VM_FEATURE_BOUNDS
    if (vm->here >= CTSS_VM_MEM_SIZE) {
        vm->error = CTSS_VM_ERR_MEM_OVERFLOW;
        return;
    }
#endif
    CTSS_TRACE(("pushdict: %u = %u\n", vm->here, v.i32));
    vm->mem[vm->here] = v;
    vm->here++;
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

static inline void ctss_vm_set_immediate(CTSS_VM *vm, uint32_t addr,
                                         uint8_t state) {
    CTSS_VMOpHeader *hd = vm->mem[addr].head;
    if (state) {
        hd->flags |= CTSS_VM_FLAG_IMMEDIATE;
    } else {
        hd->flags &= ~CTSS_VM_FLAG_IMMEDIATE;
    }
    CTSS_TRACE(("set immediate: %s %u %u\n", hd->name, addr, state));
}

static inline void ctss_vm_next(CTSS_VM *vm) {
    vm->ip = vm->mem[vm->np++].i32;
}

static inline void ctss_vm_interpret_mode(CTSS_VM *vm) {
    CTSS_TRACE(("<interpret>\n"));
    vm->mode = 0;
}

static inline void ctss_vm_compile_mode(CTSS_VM *vm) {
    CTSS_TRACE(("<compiling>\n"));
    vm->mode = 1;
}

uint32_t ctss_vm_make_header(CTSS_VM *vm, char *name) {
    CTSS_TRACE(("==== new word: %s, %u\n", name, vm->here));
    CTSS_VMOpHeader *hd = (CTSS_VMOpHeader *)calloc(1, sizeof(CTSS_VMOpHeader));
    hd->next = vm->latest;
    hd->name = strdup(name);
    vm->latest = vm->here;
    CTSS_VMValue v = {.head = hd};
    ctss_vm_push_dict(vm, v);
    return vm->latest;
}

void ctss_vm_free_header(CTSS_VMOpHeader *hd) {
    free(hd->name);
    free(hd);
}

uint32_t ctss_vm_defnative(CTSS_VM *vm, char *name, CTSS_VMOp op) {
    uint32_t addr = ctss_vm_make_header(vm, name);
    CTSS_VMValue v = {.op = op};
    ctss_vm_push_dict(vm, v);
    return addr;
}

uint32_t ctss_vm_defword(CTSS_VM *vm, char *name, ...) {
    va_list args;
    va_start(args, name);

    uint32_t addr = ctss_vm_make_header(vm, name);
    CTSS_VMValue v = {.op = CTSS_OP(docolon)};
    ctss_vm_push_dict(vm, v);

    uint32_t wAddr;
    while ((wAddr = va_arg(args, uint32_t)) != 0) {
        CTSS_TRACE(
            ("defword: %s addr: %u\n", vm->mem[wAddr].head->name, wAddr));
        CTSS_VMValue w = {.i32 = wAddr + 1}; // cfa
        ctss_vm_push_dict(vm, w);
    }
    va_end(args);

    return addr;
}

uint32_t ctss_vm_find_word(CTSS_VM *vm, char *word, uint32_t addr) {
    while (addr) {
        CTSS_VMOpHeader *hd = vm->mem[addr].head;
        if (!ctss_vm_ishidden(hd) && !strcmp(word, hd->name)) {
            return addr;
        }
        addr = hd->next;
    }
    return 0;
}

uint32_t ctss_vm_word_after(CTSS_VM *vm, uint32_t word) {
    uint32_t addr = vm->latest;
    while (addr > word) {
        uint32_t next = (vm->mem[addr].head)->next;
        if (next == word) {
            return addr;
        }
        addr = next;
    }
    return 0;
}

static uint8_t ctss_vm_tib_id(CTSS_VM *vm) {
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

static char ctss_vm_reader_skip_ws(CTSS_VM *vm) {
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
    if (pos == CTSS_VM_TOKEN_SIZE - 1) {
        vm->error = CTSS_VM_ERR_TOKEN_OVERFLOW;
        return NULL;
    }
    vm->token[pos] = 0;
    return vm->token;
}

char *ctss_vm_buffer_token(CTSS_VM *vm, char *token) {
    char *tib = vm->tib[ctss_vm_tib_id(vm)];
    strncpy(tib, token, CTSS_VM_TIB_TOKEN_SIZE);
    return tib;
}

static void ctss_vm_execute(CTSS_VM *vm) {
    while (vm->ip && !vm->error) {
        CTSS_TRACE(("exe: %s %u %p\n", vm->mem[vm->ip - 1].head->name, vm->ip,
                    vm->mem[vm->ip].op));
        vm->mem[vm->ip].op(vm);
        ctss_vm_next(vm);
    }
    CTSS_TRACE(("<<<< exe done\n"));
    if (vm->error) {
        ctss_vm_dump_error(vm);
    }
}

#ifdef CTSS_VM_FEATURE_INLINE
static uint8_t ctss_vm_maybe_inline(CTSS_VM *vm, uint32_t addr) {
    if (vm->mem[addr + 1].op == ctss_vm_op_docolon) {
        uint32_t next = ctss_vm_word_after(vm, addr);
        if (next) {
            uint32_t len = next - addr;
            CTSS_VMOpHeader *hd = vm->mem[addr].head;
            CTSS_VMOpHeader *hd2 = vm->mem[next].head;
            CTSS_TRACE(
                ("inline candidate: %s (%04x), next: %s (%04x), len: %u\n",
                 hd->name, addr, hd2->name, next, len));
            if (len < CTSS_VM_INLINE_THRESHOLD) {
                uint32_t prev = 0;
                for (uint8_t i = 2; i < len - 1; i++) {
                    CTSS_VMValue word = vm->mem[addr + i];
                    if (word.i32 != ctss_vm_cfa_ret ||
                        prev == ctss_vm_cfa_lit) {
                        CTSS_TRACE(
                            ("inline: %04x: %08x\n", addr + i, word.i32));
                        ctss_vm_push_dict(vm, word);
                        prev = word.i32;
                    } else {
                        break;
                    }
                }
                return 1;
            }
        }
    }
    return 0;
}
#endif

void ctss_vm_execute_word(CTSS_VM *vm, uint32_t addr) {
    CTSS_VMOpHeader *hd = vm->mem[addr].head;
    if (vm->mode == 0 || ctss_vm_isimmediate(hd) != 0) {
        vm->ip = addr + 1;
        vm->np = 0;
        ctss_vm_execute(vm);
    } else {
#ifdef CTSS_VM_FEATURE_INLINE
        if (!ctss_vm_maybe_inline(vm, addr)) {
            CTSS_VMValue v = {.i32 = addr + 1};
            ctss_vm_push_dict(vm, v);
        }
#else
        CTSS_VMValue v = {.i32 = addr + 1};
        ctss_vm_push_dict(vm, v);
#endif
    }
}

static CTSS_VMParseResult ctss_vm_parse_value(char *token) {
    CTSS_VMParseResult res = {.val = {.i32 = 0}, .err = 0};
    int32_t x;
    char *check = NULL;
    x = (int32_t)strtol(token, &check, 10);
    // CTSS_TRACE(("parse: %s %d %d\n", token, x, *check));
    if (*check) {
        if (*token == '0' && *(token + 1) == 'x') {
            check = NULL;
            x = (int32_t)strtol(token + 2, &check, 16);
            res.val.i32 = x;
        }
#ifdef CTSS_VM_FEATURE_FLOAT
        else if ((token[0] == '-' || (token[0] >= '0' && token[0] <= '9')) &&
                 token[strlen(token) - 1] == 'f') {
            check = NULL;
            float f = strtof(token, &check);
            res.val.f32 = f;
        }
#endif
        else {
            res.err = 1;
        }
    } else {
        res.val.i32 = x;
    }
    return res;
}

static void ctss_vm_execute_literal(CTSS_VM *vm, CTSS_VMValue v) {
    if (!vm->mode) {
        ctss_vm_push_ds(vm, v);
    } else {
        CTSS_VMValue lit = {.i32 = ctss_vm_cfa_lit};
        ctss_vm_push_dict(vm, lit);
        ctss_vm_push_dict(vm, v);
    }
}

uint32_t ctss_vm_execute_token(CTSS_VM *vm, char *token) {
    uint32_t word = ctss_vm_find_word(vm, token, vm->latest);
    if (word) {
        ctss_vm_execute_word(vm, word);
    } else {
        CTSS_VMParseResult res = ctss_vm_parse_value(token);
        if (!res.err) {
            ctss_vm_execute_literal(vm, res.val);
        } else {
            CTSS_PRINT(("Unknown word: %s\n", token));
            vm->error = CTSS_VM_ERR_UNKNOWN_WORD;
        }
    }
    return vm->error;
}

uint32_t ctss_vm_interpret(CTSS_VM *vm, char *input) {
    vm->error = CTSS_VM_ERR_OK;
    ctss_vm_init_reader(vm, input);
    char *token = ctss_vm_read_token(vm);
    while (token && !vm->error) {
        ctss_vm_execute_token(vm, token);
        token = ctss_vm_read_token(vm);
    }
    if (vm->error) {
        ctss_vm_dump_error(vm);
    }
    return vm->error;
}

CTSS_DECL_OP(docolon) {
    CTSS_TRACE(("docolon\n"));
    CTSS_VMValue v = {.i32 = vm->np};
    ctss_vm_push_rs(vm, v);
    vm->np = vm->ip + 1;
}

CTSS_DECL_OP(make_header) {
    ctss_vm_make_header(vm, ctss_vm_pop_ds(vm).str);
    CTSS_VMValue v = {.op = ctss_vm_op_docolon};
    ctss_vm_push_dict(vm, v);
}

CTSS_DECL_OP(toggle_hidden) {
    CTSS_VMOpHeader *hd = vm->mem[vm->latest].head;
    ctss_vm_toggle_hidden(hd);
    CTSS_TRACE(("toggle hidden: %s %u\n", hd->name, hd->flags));
}

CTSS_DECL_OP(here) {
    CTSS_VMValue v = {.i32 = vm->here};
    ctss_vm_push_ds(vm, v);
}

CTSS_DECL_OP(set_here) {
    vm->here = ctss_vm_pop_ds(vm).i32;
}

CTSS_DECL_OP(latest) {
    CTSS_VMValue v = {.i32 = vm->latest};
    ctss_vm_push_ds(vm, v);
}

CTSS_DECL_OP(set_latest) {
    vm->latest = ctss_vm_pop_ds(vm).i32;
}

CTSS_DECL_OP(push_dict) {
    ctss_vm_push_dict(vm, ctss_vm_pop_ds(vm));
}

CTSS_DECL_OP(mem) {
    ctss_vm_push_ds(vm, vm->mem[ctss_vm_pop_ds(vm).i32]);
}

CTSS_DECL_OP(set_mem) {
    uint32_t addr = ctss_vm_pop_ds(vm).i32;
    vm->mem[addr] = ctss_vm_pop_ds(vm);
}

/*
  CTSS_DECL_OP(i32_addr) {
  CTSS_VMValue *dsp = vm->dsp - 1;
  (*dsp).addr = (*dsp).i32 + vm->mem;
  }

  CTSS_DECL_OP(addr_i32) {
  CTSS_VMValue *dsp = vm->dsp - 1;
  (*dsp).i32 = (*dsp).addr - vm->mem;
  }
*/

CTSS_DECL_OP(immediate) {
    ctss_vm_set_immediate(vm, vm->latest, 1);
}

CTSS_DECL_OP(interpret) {
    ctss_vm_interpret_mode(vm);
}

CTSS_DECL_OP(compile) {
    ctss_vm_compile_mode(vm);
}

CTSS_DECL_OP(find) {
    CTSS_VMValue v = {
        .i32 = ctss_vm_find_word(vm, ctss_vm_pop_ds(vm).str, vm->latest)};
    ctss_vm_push_ds(vm, v);
}

CTSS_DECL_OP(cfa) {
    CTSS_VMValue cfa = {.i32 = ctss_vm_pop_ds(vm).i32 + 1};
    ctss_vm_push_ds(vm, cfa);
}

CTSS_DECL_OP(lit) {
    ctss_vm_push_ds(vm, vm->mem[vm->np]);
    vm->np++;
}

CTSS_DECL_OP(ret) {
    vm->np = ctss_vm_pop_rs(vm).i32;
}

CTSS_DECL_OP(swap) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    CTSS_VMValue *dsp = vm->dsp - 1;
    CTSS_VMValue a = *dsp;
    *dsp = *(dsp - 1);
    *(dsp - 1) = a;
}

CTSS_DECL_OP(dup) {
    CTSS_VM_BOUNDS_CHECK_BOTH(dsp, ds, DS, 1, 1)
    *vm->dsp = *(vm->dsp - 1);
    vm->dsp++;
}

CTSS_DECL_OP(drop) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1)
    vm->dsp--;
}

CTSS_DECL_OP(over) {
    CTSS_VM_BOUNDS_CHECK_BOTH(dsp, ds, DS, 2, 1)
    *vm->dsp = *(vm->dsp - 2);
    vm->dsp++;
}

CTSS_DECL_OP(rot) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 3)
    CTSS_VMValue *dsp = vm->dsp - 1;
    CTSS_VMValue c = *(dsp - 2);
    *(dsp - 2) = *(dsp - 1);
    *(dsp - 1) = *dsp;
    *dsp = c;
}

CTSS_DECL_OP(rot_inv) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 3)
    CTSS_VMValue *dsp = vm->dsp - 1;
    CTSS_VMValue a = *dsp;
    *dsp = *(dsp - 1);
    *(dsp - 1) = *(dsp - 2);
    *(dsp - 2) = a;
}

CTSS_DECL_OP(rpush) {
    ctss_vm_push_rs(vm, ctss_vm_pop_ds(vm));
}

CTSS_DECL_OP(rpop) {
    ctss_vm_push_ds(vm, ctss_vm_pop_rs(vm));
}

CTSS_DECL_OP(branch) {
    vm->np += vm->mem[vm->np].i32;
}

CTSS_DECL_OP(branch0) {
    if (ctss_vm_pop_ds(vm).i32 != 0) {
        vm->np++;
    } else {
        // CTSS_OP(branch)(vm);
        vm->np += vm->mem[vm->np].i32;
    }
}

CTSS_DECL_OP(call) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1)
    vm->ip = (*--vm->dsp).i32; // FIXME must not call next() for this
}

CTSS_DECL_OP(jump) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1)
    vm->np = (*--vm->dsp).i32;
    CTSS_TRACE(("jump: %04x\n", vm->np));
}

#define CTSS_DECL_MATH_OP(name, op, aa, bb, type, ctype)                       \
    CTSS_DECL_OP(name##_##type) {                                              \
        CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)                                \
        CTSS_VMValue *dsp = vm->dsp - 1;                                       \
        ctype a = (*dsp).type;                                                 \
        ctype b = (*(dsp - 1)).type;                                           \
        (*(dsp - 1)).type = aa op bb;                                          \
        vm->dsp = dsp;                                                         \
    }

#define CTSS_DECL_CMP_OP(name, op, type)                                       \
    CTSS_DECL_OP(cmp_##name##_##type) {                                        \
        CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)                                \
        CTSS_VMValue *dsp = vm->dsp - 1;                                       \
        (*(dsp - 1)).i32 = (((*(dsp - 1)).type op(*dsp).type) ? 1 : 0);        \
        vm->dsp = dsp;                                                         \
    }

CTSS_DECL_MATH_OP(add, +, a, b, i32, int32_t)
CTSS_DECL_MATH_OP(mul, *, a, b, i32, int32_t)
CTSS_DECL_MATH_OP(sub, -, b, a, i32, int32_t)
CTSS_DECL_MATH_OP(div, /, b, a, i32, int32_t)
CTSS_DECL_MATH_OP(mod, %, b, a, i32, int32_t)

CTSS_DECL_MATH_OP(log_and, &&, a, b, i32, int32_t);
CTSS_DECL_MATH_OP(log_or, ||, a, b, i32, int32_t);
CTSS_DECL_OP(not_i32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1)
    (*(vm->dsp - 1)).i32 = !(*(vm->dsp - 1)).i32;
}

CTSS_DECL_OP(bit_and_i32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    CTSS_VMValue *dsp = vm->dsp - 1;
    (*(dsp - 1)).i32 &= (*dsp).i32;
    vm->dsp = dsp;
}

CTSS_DECL_OP(bit_or_i32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    CTSS_VMValue *dsp = vm->dsp - 1;
    (*(dsp - 1)).i32 |= (*dsp).i32;
    vm->dsp = dsp;
}

CTSS_DECL_OP(bit_xor_i32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    CTSS_VMValue *dsp = vm->dsp - 1;
    (*(dsp - 1)).i32 ^= (*dsp).i32;
    vm->dsp = dsp;
}

CTSS_DECL_OP(lsl_i32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    CTSS_VMValue *dsp = vm->dsp - 1;
    (*(dsp - 1)).i32 <<= (*dsp).i32;
    vm->dsp = dsp;
}

CTSS_DECL_OP(asr_i32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    CTSS_VMValue *dsp = vm->dsp - 1;
    (*(dsp - 1)).i32 >>= (*dsp).i32;
    vm->dsp = dsp;
}

CTSS_DECL_OP(lsr_i32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    CTSS_VMValue *dsp = vm->dsp - 1;
    (*(dsp - 1)).i32 = (int32_t)((uint32_t)(*(dsp - 1)).i32 >> (*dsp).i32);
    vm->dsp = dsp;
}

CTSS_DECL_OP(ror_i32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    CTSS_VMValue *dsp = vm->dsp - 1;
    int32_t a = (*dsp).i32;
    uint32_t b = (uint32_t)(*(dsp - 1)).i32;
    (*(dsp - 1)).i32 = (int32_t)((b >> a) | (b & ((1 << a) - 1)) << (31 - a));
    vm->dsp = dsp;
}

CTSS_DECL_CMP_OP(lt, <, i32)
CTSS_DECL_CMP_OP(gt, >, i32)
CTSS_DECL_CMP_OP(le, <=, i32)
CTSS_DECL_CMP_OP(ge, >=, i32)
CTSS_DECL_CMP_OP(eq, ==, i32)
CTSS_DECL_CMP_OP(neq, !=, i32)

CTSS_DECL_OP(rand_i32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    (*(vm->dsp - 2)).i32 =
        ctss_rand_i32((*(vm->dsp - 2)).i32, (*(vm->dsp - 1)).i32);
    vm->dsp--;
}

#ifdef CTSS_VM_FEATURE_FLOAT

CTSS_DECL_MATH_OP(add, +, a, b, f32, float)
CTSS_DECL_MATH_OP(mul, *, a, b, f32, float)
CTSS_DECL_MATH_OP(sub, -, b, a, f32, float)
CTSS_DECL_MATH_OP(div, /, b, a, f32, float)

CTSS_DECL_OP(mod_f32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    CTSS_VMValue *dsp = vm->dsp - 1;
    (*(dsp - 1)).f32 = fmodf((*(dsp - 1)).f32, (*dsp).f32);
    vm->dsp = dsp;
}

CTSS_DECL_CMP_OP(lt, <, f32)
CTSS_DECL_CMP_OP(gt, >, f32)
CTSS_DECL_CMP_OP(le, <=, f32)
CTSS_DECL_CMP_OP(ge, >=, f32)
CTSS_DECL_CMP_OP(eq, ==, f32)
CTSS_DECL_CMP_OP(neq, !=, f32)

CTSS_DECL_OP(i32_f32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1)
    (*(vm->dsp - 1)).f32 = (float)((*(vm->dsp - 1)).i32);
}

CTSS_DECL_OP(f32_i32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1)
    (*(vm->dsp - 1)).i32 = (int32_t)((*(vm->dsp - 1)).f32);
}

CTSS_DECL_OP(sin_f32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1)
    (*(vm->dsp - 1)).f32 = sinf((*(vm->dsp - 1)).f32);
}

CTSS_DECL_OP(cos_f32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1)
    (*(vm->dsp - 1)).f32 = cosf((*(vm->dsp - 1)).f32);
}

CTSS_DECL_OP(pow_f32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    (*(vm->dsp - 2)).f32 = powf((*(vm->dsp - 2)).f32, (*(vm->dsp - 1)).f32);
    vm->dsp--;
}

CTSS_DECL_OP(rand_f32) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    (*(vm->dsp - 2)).f32 =
        ctss_rand_f32((*(vm->dsp - 2)).f32, (*(vm->dsp - 1)).f32);
    vm->dsp--;
}

#endif /* CTSS_VM_FEATURE_FLOAT */

CTSS_DECL_OP(read_token) {
    char *token = ctss_vm_buffer_token(vm, ctss_vm_read_token(vm));
    CTSS_VMValue v = {.str = token};
    ctss_vm_push_ds(vm, v);
}

CTSS_DECL_OP(read_str) {
    uint32_t pos = 0;
    char c = ctss_vm_reader_skip_ws(vm);
    if (!c) {
        vm->error = CTSS_VM_ERR_UNTERMINATED_STRING;
        return;
    }
    vm->token[0] = c;
    while (c != '"' && pos < CTSS_VM_TOKEN_SIZE - 1) {
        pos++;
        c = ctss_vm_read_char(vm);
        vm->token[pos] = c;
    }
    if (pos == CTSS_VM_TOKEN_SIZE - 1) {
        vm->error = CTSS_VM_ERR_TOKEN_OVERFLOW;
        return;
    }
    vm->token[pos] = 0;
    CTSS_VMValue v = {.str = strdup(vm->token)};
    printf("string lit: %s (%p), token: %s (%p)\n", v.str, v.str, vm->token,
           vm->token);
    if (vm->mode) {
        CTSS_VMValue lit = {.i32 = ctss_vm_cfa_lit};
        ctss_vm_push_dict(vm, lit);
        ctss_vm_push_dict(vm, v);
    } else {
        ctss_vm_push_ds(vm, v);
    }
}

CTSS_DECL_OP(read_line_comment) {
    char c;
    do {
        c = ctss_vm_read_char(vm);
    } while (c && c != '\n');
}

CTSS_DECL_OP(print_str) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1)
    CTSS_VMValue *dsp = vm->dsp - 1;
    printf("%s\n", (*dsp).str);
    vm->dsp = dsp;
}

CTSS_DECL_OP(i32_str) {
    static char buf[16];
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1)
    CTSS_VMValue *dsp = vm->dsp - 1;
    snprintf(buf, 16, "%d", (*dsp).i32);
    (*(dsp)).str = strdup(buf);
}

CTSS_DECL_OP(cmp_eq_str) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    CTSS_VMValue *dsp = vm->dsp - 1;
    (*(dsp - 1)).i32 = !strcmp((*(dsp - 1)).str, (*dsp).str);
    vm->dsp = dsp;
}

CTSS_DECL_OP(concat_str) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    CTSS_VMValue *dsp = vm->dsp - 1;
    char *a = (*dsp).str;
    char *b = (*(dsp - 1)).str;
    uint32_t blen = strlen(b) + 1;
    uint32_t clen = blen + strlen(a);
    char *c = malloc(clen);
    strncpy(c, b, blen);
    strcat(c, a);
    CTSS_TRACE(("b: %s (%p), a: %s (%p) -> %s (%p)\n", b, b, a, a, c, c));
    (*(dsp - 1)).str = c;
    vm->dsp = dsp;
}

CTSS_DECL_OP(free_str) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)
    CTSS_VMValue *dsp = vm->dsp - 1;
    free((*dsp).str);
    vm->dsp = dsp;
}

char *ctss_vm_get_error_desc(uint8_t err) {
    return ctss_vm_errors[err];
}

void ctss_vm_dump_error(CTSS_VM *vm) {
    CTSS_PRINT(
        ("error: %u (%s)\n", vm->error, ctss_vm_get_error_desc(vm->error)));
}

#ifdef CTSS_VM_FEATURE_PRINT
void ctss_vm_dump_words(CTSS_VM *vm) {
    uint32_t addr = vm->latest;
    while (addr) {
        CTSS_VMOpHeader *hd = vm->mem[addr].head;
        CTSS_PRINT(("addr: %04x %-16s op: %p next: %04x\n", addr, hd->name,
                    vm->mem[addr + 1].op, hd->next));
        addr = hd->next;
    }
}

void ctss_vm_dump_ds(CTSS_VM *vm) {
    for (CTSS_VMValue *i = vm->dsp - 1; i >= vm->ds; i--) {
        CTSS_PRINT(("<%ld> %d %f\n", i - vm->ds, (*i).i32, (*i).f32));
    }
    CTSS_PRINT(("--\n"));
}

void ctss_vm_dump_rs(CTSS_VM *vm) {
    for (CTSS_VMValue *i = vm->rsp - 1; i >= vm->rs; i--) {
        CTSS_PRINT(("<%ld> %d %f\n", i - vm->rs, (*i).i32, (*i).f32));
    }
    CTSS_PRINT(("--\n"));
}

void ctss_vm_dump_tos_i32(CTSS_VM *vm) {
    CTSS_PRINT(("%d ", ctss_vm_pop_ds(vm).i32));
}

void ctss_vm_dump_tos_i32_hex(CTSS_VM *vm) {
    CTSS_PRINT(("0x%x ", ctss_vm_pop_ds(vm).i32));
}

void ctss_vm_dump_tos_f32(CTSS_VM *vm) {
    CTSS_PRINT(("%f ", ctss_vm_pop_ds(vm).f32));
}

void ctss_vm_dump(CTSS_VM *vm) {
    CTSS_PRINT(("dsp: %ld, rsp: %ld, ip: 0x%04x, np: 0x%04x, latest: 0x%04x, "
                "here: 0x%04x, mode: %u, "
                "err: %u\n",
                vm->dsp - vm->ds, vm->rsp - vm->rs, vm->ip, vm->np, vm->latest,
                vm->here, vm->mode, vm->error));
}

void ctss_vm_dump_mem(CTSS_VM *vm) {
    CTSS_VMValue *addr = vm->mem;
    uint32_t len = CTSS_VM_MEM_SIZE;
    while (len) {
        CTSS_PRINT(("%04lx: ", addr - vm->mem));
        for (uint8_t i = 0; i < 8; i++) {
            CTSS_PRINT(("%08x ", (*addr++).i32));
        }
        CTSS_PRINT(("\n"));
        len -= 8;
    }
}

CTSS_DECL_OP(dump_vm) {
    ctss_vm_dump(vm);
}

CTSS_DECL_OP(dump_mem) {
    ctss_vm_dump_mem(vm);
}

CTSS_DECL_OP(dump_words) {
    ctss_vm_dump_words(vm);
}

CTSS_DECL_OP(dump_ds) {
    ctss_vm_dump_ds(vm);
}

CTSS_DECL_OP(dump_rs) {
    ctss_vm_dump_rs(vm);
}

CTSS_DECL_OP(dump_tos_i32) {
    ctss_vm_dump_tos_i32(vm);
}

CTSS_DECL_OP(dump_tos_i32_hex) {
    ctss_vm_dump_tos_i32_hex(vm);
}

CTSS_DECL_OP(dump_tos_f32) {
    ctss_vm_dump_tos_f32(vm);
}
#endif /* CTSS_VM_FEATURE_PRINT */

void ctss_vm_init_primitives(CTSS_VM *vm) {
    CTSS_DEFNATIVE("ret", ret);
    ctss_vm_cfa_ret = ret + 1;
    CTSS_DEFNATIVE("lit", lit);
    ctss_vm_cfa_lit = lit + 1;
    CTSS_DEFNATIVE("mk-header", make_header);
    CTSS_DEFNATIVE("call", call);
    CTSS_DEFNATIVE("jump", jump);

    // stack
    CTSS_DEFNATIVE("swap", swap);
    CTSS_DEFNATIVE("dup", dup);
    CTSS_DEFNATIVE("drop", drop);
    CTSS_DEFNATIVE("over", over);
    CTSS_DEFNATIVE("rot", rot);
    CTSS_DEFNATIVE("-rot", rot_inv);
    CTSS_DEFNATIVE(">r", rpush);
    CTSS_DEFNATIVE("r>", rpop);

    // dict
    CTSS_DEFNATIVE("here", here);
    CTSS_DEFNATIVE("here!", set_here);
    CTSS_DEFNATIVE(">dict", push_dict);
    CTSS_DEFNATIVE("latest", latest);
    CTSS_DEFNATIVE("latest!", set_latest);
    CTSS_DEFNATIVE("find", find);
    CTSS_DEFNATIVE(">cfa", cfa);

    // memory
    CTSS_DEFNATIVE("@", mem);
    CTSS_DEFNATIVE("!", set_mem);
    // CTSS_DEFNATIVE("i>a", i32_addr);
    // CTSS_DEFNATIVE("a>i", addr_i32);

    // flags
    CTSS_DEFNATIVE("^hidden", toggle_hidden);
    CTSS_DEFNATIVE("immediate!", immediate);
    CTSS_DEFNATIVE("interpret>>", interpret);
    ctss_vm_set_immediate(vm, interpret, 1);
    CTSS_DEFNATIVE("compile>>", compile);

    // word
    CTSS_DEFNATIVE("read-token>", read_token);
    uint32_t colon = ctss_vm_defword(vm, ":", read_token, make_header,
                                     toggle_hidden, compile, ret, 0);
    uint32_t semicolon = ctss_vm_defword(vm, ";", lit, ret, push_dict,
                                         toggle_hidden, interpret, ret, 0);
    ctss_vm_set_immediate(vm, semicolon, 1);

    // branch
    CTSS_DEFNATIVE("branch", branch);
    CTSS_DEFNATIVE("0branch", branch0);

    // maths
    CTSS_DEFNATIVE("+", add_i32);
    CTSS_DEFNATIVE("*", mul_i32);
    CTSS_DEFNATIVE("-", sub_i32);
    CTSS_DEFNATIVE("/", div_i32);
    CTSS_DEFNATIVE("mod", mod_i32);
    CTSS_DEFNATIVE("&", bit_and_i32);
    CTSS_DEFNATIVE("|", bit_or_i32);
    CTSS_DEFNATIVE("^", bit_xor_i32);
    CTSS_DEFNATIVE("<<", lsl_i32);
    CTSS_DEFNATIVE(">>", asr_i32);
    CTSS_DEFNATIVE("u>>", lsr_i32);
    CTSS_DEFNATIVE("ror", ror_i32);

    CTSS_DEFNATIVE("<", cmp_lt_i32);
    CTSS_DEFNATIVE(">", cmp_gt_i32);
    CTSS_DEFNATIVE("<=", cmp_le_i32);
    CTSS_DEFNATIVE(">=", cmp_ge_i32);
    CTSS_DEFNATIVE("==", cmp_eq_i32);
    CTSS_DEFNATIVE("<>", cmp_neq_i32);

    CTSS_DEFNATIVE("\"", read_str);
    ctss_vm_set_immediate(vm, read_str, 1);
    CTSS_DEFNATIVE("\\", read_line_comment);
    ctss_vm_set_immediate(vm, read_line_comment, 1);

    CTSS_DEFNATIVE("s==", cmp_eq_str);
    CTSS_DEFNATIVE("s+", concat_str);
    CTSS_DEFNATIVE("sfree", free_str);
    CTSS_DEFNATIVE("i>s", i32_str);

    CTSS_DEFNATIVE("and", log_and_i32);
    CTSS_DEFNATIVE("or", log_or_i32);
    CTSS_DEFNATIVE("not", not_i32);

    CTSS_DEFNATIVE("rand", rand_i32);

#ifdef CTSS_VM_FEATURE_FLOAT
    CTSS_DEFNATIVE("f+", add_f32);
    CTSS_DEFNATIVE("f*", mul_f32);
    CTSS_DEFNATIVE("f-", sub_f32);
    CTSS_DEFNATIVE("f/", div_f32);
    CTSS_DEFNATIVE("fmod", mod_f32);
    CTSS_DEFNATIVE("f<", cmp_lt_f32);
    CTSS_DEFNATIVE("f>", cmp_gt_f32);
    CTSS_DEFNATIVE("f<=", cmp_le_f32);
    CTSS_DEFNATIVE("f>=", cmp_ge_f32);
    CTSS_DEFNATIVE("f==", cmp_eq_f32);
    CTSS_DEFNATIVE("f<>", cmp_neq_f32);
    CTSS_DEFNATIVE("i>f", i32_f32);
    CTSS_DEFNATIVE("f>i", f32_i32);
    CTSS_DEFNATIVE("fsin", sin_f32);
    CTSS_DEFNATIVE("fcos", cos_f32);
    CTSS_DEFNATIVE("fpow", pow_f32);
    CTSS_DEFNATIVE("frand", rand_f32);
#endif

// vector / buffer
// TODO

#ifdef CTSS_VM_FEATURE_PRINT
    CTSS_DEFNATIVE(".vm", dump_vm);
    CTSS_DEFNATIVE(".mem", dump_mem);
    CTSS_DEFNATIVE(".words", dump_words);
    CTSS_DEFNATIVE(".s", dump_ds);
    CTSS_DEFNATIVE(".r", dump_rs);
    CTSS_DEFNATIVE(".", dump_tos_i32);
    CTSS_DEFNATIVE(".h", dump_tos_i32_hex);
    CTSS_DEFNATIVE(".f", dump_tos_f32);
    CTSS_DEFNATIVE("print", print_str);
#endif
}

#ifdef CTSS_VM_FEATURE_IO
uint8_t ctss_vm_interpret_file(CTSS_VM *vm, char *path, uint32_t bufsize) {
    FILE *file = fopen(path, "r");
    if (!file) {
        return CTSS_VM_ERR_IO;
    }
    char *buf = (char *)malloc(bufsize);
    char *line = buf;
    uint32_t read = 0;
    while (fgets(line, bufsize - read, file) != NULL) {
        uint32_t len = strlen(line);
        line += len;
        read += len;
    }
    CTSS_TRACE(("read file (%u bytes):\n%s", read, buf));
    ctss_vm_interpret(vm, buf);
    free(buf);
    return 0;
}
#endif /* CTSS_VM_FEATURE_IO */
