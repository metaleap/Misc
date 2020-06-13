#pragma once

#include "utils_std_basics.c"
#include "utils_toks.c"


typedef enum JsonValueKind {
    json_invalid,
    json_null,
    json_bool,
    json_number,
    json_string,
    json_array,
    json_object,
} JsonValueKind;


struct JsonValue;
typedef struct JsonValue JsonValue;
typedef ·SliceOf(JsonValue) JsonValues;
struct JsonValue {
    JsonValueKind kind;
    union {
        Bool boolean;
        I64 number;
        Str string;
        JsonValues arr;
        struct {
            Strs keys;
            JsonValues vals;
        } obj;
    } of;
};

JsonValue jsonNewStr(Str const string) {
    return (JsonValue) {.kind = json_string, .of = {.string = string}};
}

JsonValue jsonNewArr(UInt const capacity) {
    return (JsonValue) {.kind = json_array, .of = {.arr = ·sliceOf(JsonValue, NULL, 0, capacity)}};
}

JsonValue jsonNewObj(UInt const num_fields) {
    return (JsonValue) {.kind = json_object,
                        .of = {.obj = {.keys = ·sliceOf(Str, NULL, 0, num_fields), .vals = ·sliceOf(JsonValue, NULL, 0, num_fields)}}};
}

JsonValue jsonNewNum(I64 const number) {
    return (JsonValue) {.kind = json_number, .of = {.number = number}};
}

JsonValue jsonNewBool(Bool const boolean) {
    return (JsonValue) {.kind = json_bool, .of = {.boolean = boolean}};
}

JsonValue jsonNewNull() {
    return (JsonValue) {.kind = json_null};
}

JsonValue* jsonObjVal(JsonValue const* const json_obj, Str const key, JsonValueKind const must_kind) {
    for (UInt i = 0; i < json_obj->of.obj.keys.len; i += 1)
        if (strEql(json_obj->of.obj.keys.at[i], key) && (must_kind == json_invalid || must_kind == json_obj->of.obj.vals.at[i].kind))
            return &json_obj->of.obj.vals.at[i];
    return NULL;
}

ºI64 jsonObjValNum(JsonValue const* const json_obj, Str const key) {
    JsonValue* const json_val = jsonObjVal(json_obj, key, json_number);
    if (json_val != NULL)
        return ·got(I64, json_val->of.number);
    return ·none(I64);
}

ºBool jsonObjValBool(JsonValue const* const json_obj, Str const key) {
    JsonValue* const json_val = jsonObjVal(json_obj, key, json_bool);
    if (json_val != NULL)
        return ·got(Bool, json_val->of.boolean);
    return ·none(Bool);
}

Str jsonObjValStr(JsonValue const* const json_obj, Str const key) {
    JsonValue* const json_val = jsonObjVal(json_obj, key, json_string);
    if (json_val != NULL)
        return json_val->of.string;
    return ·len0(U8);
}



JsonValue jsonParse(Tokens toks, Str const full_src);

JsonValue jsonParseArray(Tokens const toks, Str const full_src) {
    JsonValue ret_json = {.kind = json_invalid};

    ºUInt const tok_end_idx = toksIndexOfMatchingBracket(toks);
    if (tok_end_idx.got && tok_end_idx.it == toks.len - 1) {
        Tokenss const subs = toksSplit(·slice(Token, toks, 1, tok_end_idx.it), tok_kind_sep_comma);
        ret_json = jsonNewArr(subs.len);
        ·forEach(Tokens, sub_toks, subs, {
            ·push(ret_json.of.arr, jsonParse(*sub_toks, full_src));
            if (·last(ret_json.of.arr)->kind == json_invalid) {
                ret_json.kind = json_invalid;
                break;
            }
        });
    }

    return ret_json;
}

JsonValue jsonParseObject(Tokens const toks, Str const full_src) {
    JsonValue ret_json = {.kind = json_invalid};

    ºUInt const tok_end_idx = toksIndexOfMatchingBracket(toks);
    if (tok_end_idx.got && tok_end_idx.it == toks.len - 1) {
        Tokenss const subs = toksSplit(·slice(Token, toks, 1, tok_end_idx.it), tok_kind_sep_comma);
        ret_json = jsonNewObj(subs.len);
        ·forEach(Tokens, sub_toks, subs, {
            Tokens const cur = *sub_toks;
            if ((cur.len >= 3) && (cur.at[1].kind == tok_kind_ident) && strEq(":", tokSrc(&cur.at[1], full_src), 1)) {
                Str const key = strParse(tokSrc(&cur.at[0], full_src));
                if (key.at == NULL) {
                    ret_json.kind = json_invalid;
                    break;
                } else {
                    ·push(ret_json.of.obj.keys, key);
                    ·push(ret_json.of.obj.vals, jsonParse(·slice(Token, cur, 2, cur.len), full_src));
                    if (·last(ret_json.of.obj.vals)->kind == json_invalid) {
                        ret_json.kind = json_invalid;
                        break;
                    }
                }
            } else {
                ret_json.kind = json_invalid;
                break;
            }
        });
    }
    return ret_json;
}

JsonValue jsonParse(Tokens const toks, Str const full_src) {
    JsonValue ret_json = {.kind = json_invalid};

    if (toks.len > 1)
        switch (toks.at[0].kind) {
            case tok_kind_sep_bsquare_open: ret_json = jsonParseArray(toks, full_src); break;
            case tok_kind_sep_bcurly_open: ret_json = jsonParseObject(toks, full_src); break;
            default: break;
        }
    else if (toks.len == 1) {
        Str const tok_src = tokSrc(&toks.at[0], full_src);
        switch (toks.at[0].kind) {
            case tok_kind_ident: {
                if (strEql(tok_src, strL("null", 4)))
                    ret_json.kind = json_null;
                else if (strEql(tok_src, strL("true", 4))) {
                    ret_json.kind = json_bool;
                    ret_json.of.boolean = true;
                } else if (strEql(tok_src, strL("false", 5))) {
                    ret_json.kind = json_bool;
                    ret_json.of.boolean = false;
                }
            } break;
            case tok_kind_lit_num_prefixed: {
                ºU64 maybe_uint = uInt64Parse(tok_src);
                if (maybe_uint.got) {
                    ret_json.kind = json_number;
                    ret_json.of.number = maybe_uint.it;
                }
            } break;
            case tok_kind_lit_str_qdouble: {
                ret_json.of.string = strParse(tok_src);
                if (ret_json.of.string.at != NULL)
                    ret_json.kind = json_string;
            } break;
            default: break;
        }
    }

    return ret_json;
}

Str jsonWrite(Str buf, UInt const buf_cap, JsonValue const* const json) {
    if (buf.len < buf_cap)
        switch (json->kind) {
            case json_invalid: ·fail(str("jsonWrite: new bug")); break;
            case json_null: {
                for (UInt i = 0; i < 4 && (buf.len < buf_cap); i += 1)
                    ·push(buf, "null"[i]);
            } break;
            case json_bool: {
                if (json->of.boolean)
                    for (UInt i = 0; i < 4 && (buf.len < buf_cap); i += 1)
                        ·push(buf, "true"[i]);
                else
                    for (UInt i = 0; i < 5 && (buf.len < buf_cap); i += 1)
                        ·push(buf, "false"[i]);
            } break;
            case json_number: {
                Str const num_str = uIntToStr(json->of.number, 1, 10);
                for (UInt i = 0; i < num_str.len && (buf.len < buf_cap); i += 1)
                    ·push(buf, num_str.at[i]);
            } break;
            case json_string: {
                Str const str_quot = strQuot(json->of.string);
                for (UInt i = 0; i < str_quot.len && (buf.len < buf_cap); i += 1)
                    ·push(buf, str_quot.at[i]);
            } break;
            case json_array: {
                if (buf.len < buf_cap)
                    ·push(buf, '[');
                for (UInt i = 0; i < json->of.arr.len && (buf.len < buf_cap); i += 1) {
                    if (i != 0)
                        ·push(buf, ',');
                    if (buf.len < buf_cap)
                        buf = jsonWrite(buf, buf_cap, &json->of.arr.at[i]);
                }
                if (buf.len < buf_cap)
                    ·push(buf, ']');
            } break;
            case json_object: {
                if (buf.len < buf_cap)
                    ·push(buf, '{');
                for (UInt i = 0; i < json->of.obj.vals.len && (buf.len < buf_cap); i += 1) {
                    if (i != 0)
                        ·push(buf, ',');
                    Str const str_quot = strQuot(json->of.obj.keys.at[i]);
                    for (UInt j = 0; j < str_quot.len && (buf.len < buf_cap); j += 1)
                        ·push(buf, str_quot.at[j]);
                    if (buf.len < buf_cap)
                        ·push(buf, ':');
                    if (buf.len < buf_cap)
                        buf = jsonWrite(buf, buf_cap, &json->of.obj.vals.at[i]);
                }
                if (buf.len < buf_cap)
                    ·push(buf, '}');
            } break;
            default: ·fail(uIntToStr(json->kind, 1, 10));
        }

    if (buf.len >= buf_cap)
        ·fail(str("TODO: increase buf_cap due to output"));
    buf.at[buf.len] = 0;
    return buf;
}
