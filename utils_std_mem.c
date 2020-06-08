#pragma once
#include "utils_std_basics.c"
#include <execinfo.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


#ifndef mem_bss_max
#define mem_bss_max (1 * 1024 * 1024)
#endif
struct {
    U8 buf[mem_bss_max];
    UInt pos;
} mem_bss = {.pos = 0};


// macro names prefixed with '·' instead of all upper-case (avoids SCREAM_CODE)


#define ·new(T) ((T*)memBssAlloc(sizeof(T)))

#define ·sliceOf(T, ³initial_len__, ²max_capacity__)                                                                                         \
    ((T##s) {.len = (³initial_len__),                                                                                                        \
             .at = (T*)(memBssAlloc((((²max_capacity__) < (³initial_len__)) ? (³initial_len__) : (²max_capacity__)) * (sizeof(T))))})

#define ·sliceOfPtrs(T, ³initial_len__, ²max_capacity__)                                                                                     \
    {                                                                                                                                        \
        .len = (³initial_len__),                                                                                                             \
        .at = (T**)(memBssAlloc((((²max_capacity__) < (³initial_len__)) ? (³initial_len__) : (²max_capacity__)) * (sizeof(T*))))             \
    }

#define ·listOf(T, ⁵initial_len__, ⁴max_capacity__)                                                                                          \
    ((T##s) {.len = (⁵initial_len__),                                                                                                        \
             .cap = (((⁴max_capacity__) < (⁵initial_len__)) ? (⁵initial_len__) : (⁴max_capacity__)),                                         \
             .at = (T*)(memBssAlloc((((⁴max_capacity__) < (⁵initial_len__)) ? (⁵initial_len__) : (⁴max_capacity__)) * (sizeof(T))))})

#define ·listOfPtrs(T, ⁵initial_len__, ⁴max_capacity__)                                                                                      \
    {                                                                                                                                        \
        .len = (⁵initial_len__), .cap = (((⁴max_capacity__) < (⁵initial_len__)) ? (⁵initial_len__) : (⁴max_capacity__)),                     \
        .at = (T**)(memBssAlloc((((⁴max_capacity__) < (⁵initial_len__)) ? (⁵initial_len__) : (⁴max_capacity__)) * (sizeof(T*))))             \
    }



U8* memBssAlloc(UInt const num_bytes) {
    if (num_bytes == 0)
        return NULL;
    UInt const new_pos = mem_bss.pos + num_bytes;
    if (new_pos >= mem_bss_max - 1)
        ·fail(str("out of memory: increase mem_bss_max!"));
    U8* const mem_ptr = &mem_bss.buf[mem_bss.pos];
    mem_bss.pos = new_pos;
    return mem_ptr;
}

Str newStr(UInt const initial_len, UInt const max_capacity, Bool const zeroed) {
    Str ret_str = (Str) {.len = initial_len, .at = memBssAlloc(max_capacity)};
    if (zeroed)
        for (UInt i = 0; i < max_capacity; i += 1)
            ret_str.at[i] = 0;
    return ret_str;
}

Str uIntToStr(UInt const uInt_value, UInt const str_min_len, UInt const base) {
    UInt num_digits = 1;
    UInt n = uInt_value;
    while (n >= base) {
        num_digits += 1;
        n /= base;
    }
    n = uInt_value;

    UInt const str_len = (num_digits > str_min_len) ? num_digits : str_min_len;
    Str const ret_str = newStr(str_len, str_len + 1, false);
    ret_str.at[str_len] = 0;
    for (UInt i = 0; i < str_len - num_digits; i += 1)
        ret_str.at[i] = '0';

    Bool done = false;
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
        ret_str = newStr(0, tok.len - 1, false);

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
    Str ret_str = newStr(1, 3 + (6 * str.len), false);
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

    Str ret_str = newStr(0, 1 + str_len, false);
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
    Str ret_ident = newStr(0, 4 * str.len, false);
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
