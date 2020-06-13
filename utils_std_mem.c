#pragma once
#include <execinfo.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils_std_basics.c"


#ifndef mem_bss_max
#define mem_bss_max (1 * 1024 * 1024)
#endif
struct {
    U8 buf[mem_bss_max];
    UInt pos;
} mem_bss = {.pos = 0};


typedef enum MemHeapKind {
    mem_heap_fixed_size,
    mem_heap_pages_malloc,
} MemHeapKind;

typedef struct MemHeap {
    PtrAny ptr;
    UInt cap;
    UInt len;
    MemHeapKind kind;
} MemHeap;

UInt memSize(MemHeap* mem_heap) {
    if (mem_heap == NULL)
        return mem_bss_max;
    switch (mem_heap->kind) {
        case mem_heap_fixed_size: return mem_heap->cap;
        case mem_heap_pages_malloc: {
            UInt ret_size = 0;
            PtrAny ptr = mem_heap->ptr;
            while (ptr != NULL) {
                PtrAny ptr_next = *((PtrAny*)ptr);
                ret_size += mem_heap->cap;
                ptr = ptr_next;
            }
            return ret_size;
        }
        default: exit(mem_heap->kind);
    }
}

void memFree(MemHeap* mem_heap) {
    if (mem_heap == NULL)
        mem_bss.pos = 0;
    else
        switch (mem_heap->kind) {
            case mem_heap_fixed_size: {
                mem_heap->len = 0;
            } break;
            case mem_heap_pages_malloc: {
                PtrAny ptr = mem_heap->ptr;
                while (ptr != NULL) {
                    PtrAny ptr_next = *((PtrAny*)ptr);
                    free(ptr);
                    ptr = ptr_next;
                }
            } break;
            default: exit(mem_heap->kind);
        }
}

PtrAny memAlloc(MemHeap* mem_heap, UInt const size) {
    if (size == 0)
        return NULL;
    MemHeap bss;
    Bool is_bss = (mem_heap == NULL);
    if (is_bss) {
        bss = (MemHeap) {.ptr = &mem_bss, .cap = mem_bss_max, .len = mem_bss.pos, .kind = mem_heap_fixed_size};
        mem_heap = &bss;
    }
    UInt idx = mem_heap->len;
    UInt new_len = mem_heap->len + size;
    if (new_len >= mem_heap->cap)
        switch (mem_heap->kind) {
            case mem_heap_fixed_size: {
                ·fail(str((!is_bss) ? "memAlloc: fixed-size allocator out of memory" : "TODO: increase mem_bss_max"));
            } break;
            case mem_heap_pages_malloc: {
                if ((sizeof(PtrAny) + size) >= mem_heap->cap)
                    ·fail(str("memAlloc: insufficient .cap for mem_heap_pages_malloc"));
                PtrAny next_page = malloc(mem_heap->cap);
                if (next_page == NULL)
                    ·fail(str("memAlloc: heap allocator out of memory"));
                *((PtrAny*)next_page) = mem_heap->ptr;
                mem_heap->ptr = next_page;
                idx = sizeof(PtrAny);
                new_len = idx + size;
            } break;
            default: exit(mem_heap->kind);
        }
    mem_heap->len = new_len;
    if (is_bss)
        mem_bss.pos = new_len;
    return ((U8*)mem_heap->ptr) + idx;
}

U8* memBssAlloc(UInt const num_bytes) {
    return memAlloc(NULL, num_bytes);
}



// macro names prefixed with '·' instead of all upper-case (avoids SCREAM_CODE)

#define ·new(T, ¹mem_heap__) ((T*)memAlloc((¹mem_heap__), sizeof(T)))

#define ·sliceOf(T, ¹mem_heap__, ³initial_len__, ²max_capacity__)                                                                            \
    ((T##s) {                                                                                                                                \
        .len = (³initial_len__),                                                                                                             \
        .at = (T*)(memAlloc((¹mem_heap__), (((²max_capacity__) < (³initial_len__)) ? (³initial_len__) : (²max_capacity__)) * (sizeof(T))))})

#define ·sliceOfPtrs(T, ¹mem_heap__, ³initial_len__, ²max_capacity__)                                                                        \
    {                                                                                                                                        \
        .len = (³initial_len__),                                                                                                             \
        .at = (T**)(memAlloc((¹mem_heap__), (((²max_capacity__) < (³initial_len__)) ? (³initial_len__) : (²max_capacity__)) * (sizeof(T*)))) \
    }

#define ·listOf(T, ¹mem_heap__, ⁵initial_len__, ⁴max_capacity__)                                                                             \
    ((T##s) {                                                                                                                                \
        .len = (⁵initial_len__),                                                                                                             \
        .cap = (((⁴max_capacity__) < (⁵initial_len__)) ? (⁵initial_len__) : (⁴max_capacity__)),                                              \
        .at = (T*)(memAlloc((¹mem_heap__), (((⁴max_capacity__) < (⁵initial_len__)) ? (⁵initial_len__) : (⁴max_capacity__)) * (sizeof(T))))})

#define ·listOfPtrs(T, ¹mem_heap__, ⁵initial_len__, ⁴max_capacity__)                                                                         \
    {                                                                                                                                        \
        .len = (⁵initial_len__), .cap = (((⁴max_capacity__) < (⁵initial_len__)) ? (⁵initial_len__) : (⁴max_capacity__)),                     \
        .at = (T**)(memAlloc((¹mem_heap__), (((⁴max_capacity__) < (⁵initial_len__)) ? (⁵initial_len__) : (⁴max_capacity__)) * (sizeof(T*)))) \
    }




Str newStr(UInt const initial_len, UInt const max_capacity) {
    Str ret_str = (Str) {.len = initial_len, .at = memBssAlloc(max_capacity)};
    return ret_str;
}

Str uintToBuf(PtrAny str_buf, UInt const uint_value, UInt const str_min_len, UInt const base, UInt num_digits) {
    if (num_digits == 0) {
        num_digits = 1;
        UInt n = uint_value;
        while (n >= base) {
            num_digits += 1;
            n /= base;
        }
    }
    UInt const str_len = (num_digits > str_min_len) ? num_digits : str_min_len;
    Str const ret_str = (Str) {.at = str_buf, .len = str_len};
    ret_str.at[str_len] = 0;
    for (UInt i = 0; i < str_len - num_digits; i += 1)
        ret_str.at[i] = '0';

    Bool done = false;
    UInt n = uint_value;
    for (UInt i = ret_str.len; i > 0 && !done;) {
        i -= 1;
        if (n < base) {
            ret_str.at[i] = 48 + n;
            done = true;
        } else {
            ret_str.at[i] = 48 + (n % base);
            n /= base;
        }
        if (base > 10 && ret_str.at[i] > '9')
            ret_str.at[i] += 7;
    }
    return ret_str;
}

Str uIntToStr(UInt const uint_value, UInt const str_min_len, UInt const base) {
    UInt num_digits = 1;
    UInt n = uint_value;
    while (n >= base) {
        num_digits += 1;
        n /= base;
    }

    UInt const str_len = (num_digits > str_min_len) ? num_digits : str_min_len;
    return uintToBuf(newStr(str_len, str_len + 1).at, uint_value, str_min_len, base, num_digits);
}

CStr strZ(Str const str) {
    if (str.at[str.len] == 0)
        return (CStr)str.at;
    U8* buf = memBssAlloc(1 + str.len);
    buf[str.len] = 0;
    for (UInt i = 0; i < str.len; i += 1)
        buf[i] = str.at[i];
    return (CStr)buf;
}

Str strParse(Str const tok) {
    Str ret_str = ·len0(U8);
    if (tok.len >= 2 && tok.at[0] == '\"' && tok.at[tok.len - 1] == '\"') {
        ret_str = newStr(0, tok.len - 1);

        for (UInt i = 1; i < tok.len - 1; i += 1) {
            U8 byte = tok.at[i];
            if (byte == '\\') {
                switch (tok.at[i + 1]) {
                    case 't': byte = '\t'; break;
                    case 'b': byte = '\b'; break;
                    case 'f': byte = '\f'; break;
                    case 'n': byte = '\n'; break;
                    case 'r': byte = '\r'; break;
                    case '\"': byte = '\"'; break;
                    case '\\': byte = '\\'; break;
                    default: return ·len0(U8);
                }
                i += 1;
            }
            ret_str.at[ret_str.len] = byte;
            ret_str.len += 1;
        }
        ret_str.at[ret_str.len] = 0;
    }
    return ret_str;
}

Str strQuot(Str const str) {
    Str ret_str = newStr(1, 3 + (6 * str.len));
    ret_str.at[0] = '\"';
    for (UInt i = 0; i < str.len; i += 1) {
        U8 const chr = str.at[i];
        if (chr >= 32 && chr < 127 && chr != '\\' && chr != '\"')
            ret_str.at[ret_str.len] = chr;
        else {
            ret_str.at[ret_str.len] = '\\';
            ret_str.len += 1;
            switch (chr) {
                case '\t': ret_str.at[ret_str.len] = 't'; break;
                case '\b': ret_str.at[ret_str.len] = 'b'; break;
                case '\f': ret_str.at[ret_str.len] = 'f'; break;
                case '\n': ret_str.at[ret_str.len] = 'n'; break;
                case '\r': ret_str.at[ret_str.len] = 'r'; break;
                case '\"': ret_str.at[ret_str.len] = '\"'; break;
                case '\\': ret_str.at[ret_str.len] = '\\'; break;
                default: {
                    ret_str.at[ret_str.len] = 'u';
                    const Str esc_num_str = uIntToStr(chr, 4, 16);
                    for (UInt c = 0; c < esc_num_str.len; c += 1)
                        ret_str.at[1 + c + ret_str.len] = esc_num_str.at[c];
                    ret_str.len += esc_num_str.len;
                } break;
            }
        }
        ret_str.len += 1;
    }
    ret_str.at[ret_str.len] = '\"';
    ret_str.len += 1;
    ret_str.at[ret_str.len] = 0;
    return ret_str;
}

Str strConcat(Strs const strs, U8 const sep) {
    UInt str_len = 0;
    UInt const sep_len = ((sep == 0) ? 0 : 1);
    ·forEach(Str, str, strs, { str_len += (sep_len + str->len); });

    Str ret_str = newStr(0, 1 + str_len);
    ret_str.at[str_len] = 0;
    ·forEach(Str, str, strs, {
        if (iˇstr != 0 && sep != 0) {
            ret_str.at[ret_str.len] = sep;
            ret_str.len += 1;
        }
        for (UInt i = 0; i < str->len; i += 1)
            ret_str.at[i + ret_str.len] = str->at[i];
        ret_str.len += str->len;
    });
    return ret_str;
}

Str str2(Str const s1, Str const s2) {
    return strConcat((Strs) {.len = 2, .at = ((Str[]) {s1, s2})}, 0);
}

Str str3(Str const s1, Str const s2, Str const s3) {
    return strConcat((Strs) {.len = 3, .at = ((Str[]) {s1, s2, s3})}, 0);
}

Str str4(Str const s1, Str const s2, Str const s3, Str const s4) {
    return strConcat((Strs) {.len = 4, .at = ((Str[]) {s1, s2, s3, s4})}, 0);
}

Str str5(Str const s1, Str const s2, Str const s3, Str const s4, Str const s5) {
    return strConcat((Strs) {.len = 5, .at = ((Str[]) {s1, s2, s3, s4, s5})}, 0);
}

Str str6(Str const s1, Str const s2, Str const s3, Str const s4, Str const s5, Str const s6) {
    return strConcat((Strs) {.len = 6, .at = ((Str[]) {s1, s2, s3, s4, s5, s6})}, 0);
}

Str str7(Str const s1, Str const s2, Str const s3, Str const s4, Str const s5, Str const s6, Str const s7) {
    return strConcat((Strs) {.len = 7, .at = ((Str[]) {s1, s2, s3, s4, s5, s6, s7})}, 0);
}

Str str8(Str const s1, Str const s2, Str const s3, Str const s4, Str const s5, Str const s6, Str const s7, Str const s8) {
    return strConcat((Strs) {.len = 8, .at = ((Str[]) {s1, s2, s3, s4, s5, s6, s7, s8})}, 0);
}



void printChr(U8 const chr) {
    fwrite(&chr, 1, 1, stderr);
}

Str ident(Str const str) {
    Str ret_ident = newStr(0, 4 * str.len);
    Bool all_chars_ok = true;
    for (UInt i = 0; i < str.len; i += 1) {
        U8 c = str.at[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_' || c == '$')
            ·push(ret_ident, c);
        else {
            all_chars_ok = false;
            Str const hex = uIntToStr(c, 1, 16);
            ·push(ret_ident, '-');
            for (UInt j = 0; j < hex.len; j += 1)
                ·push(ret_ident, hex.at[j]);
            ·push(ret_ident, '-');
        }
    }
    if (all_chars_ok) {
        mem_bss.pos -= str.len;
        ret_ident = str;
    } else
        mem_bss.pos -= ((4 * str.len) - ret_ident.len);
    return ret_ident;
}
