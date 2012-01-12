//
// A JSON parser based on the Ragel state machine from http://flori.github.com/json/
// This is a derivative work as defined by the license at 
// http://www.ruby-lang.org/en/LICENSE.txt
// The LGPLv3 license of Crack fulfills the requirements of 2.a and 3.a of that license

// To convert the Ragel source file to a .crk file use Ragel from
// www.bitbucket.org/hengestone/ragel-crack until the patch -s merged using
// ragel -K -F0 json.rl -o json.crk

// (C) Conrad Steenberg <conrad.steenberg@gmail.com>
// 12/10/2011

// For more info on JSON, see http://json.org/

import crack.runtime exit, memmove;
import crack.lang WriteBuffer, AppendBuffer, ManagedBuffer, Buffer, Exception, 
                  Writer, CString, Formatter;
import crack.io cout, cerr, cin, FStr, StandardFormatter, StringWriter;
import crack.cont.array Array;
import crack.cont.hashmap HashMap;
import crack.cont.treemap TreeMap;
import crack.math atoi, INFINITY, NAN, strtof, fpclassify, FP_INFINITE, FP_NAN,
                        FP_NORMAL, FP_ZERO, sign;
@import crack.ann define;

// Define a formatter class to override string formatting
class JsonFormatter : StandardFormatter {
    oper init(Writer rep) : StandardFormatter(rep) {}

    String encodeString(String data) {
        AppendBuffer buf = {data.size + 2};

        buf.append(b'"');
        for (uint i = 0; i < data.size; ++i) {
            ch := data.buffer[i];
            if (ch == b'"' || ch == b'\\'){
                buf.append(b'\\');
                buf.append(ch);
            } else if (ch == b'\n'){
                buf.append(b'\\');
                buf.append(b'n');
            } else if (ch == b'\t'){
                buf.append(b'\\');
                buf.append(b't');
            } else if (ch == 12){ // FF
                buf.append(b'\\');
                buf.append(b'f');
            } else if (ch == b'\r'){
                buf.append(b'\\');
                buf.append(b'r');
            } else if (ch == b'/'){
                buf.append(b'\\');
                buf.append(b'/');
            } else if (ch == 8){ // BS
                buf.append(b'\\');
                buf.append(b'b');
            } else if (ch < 32 || ch > 127) {
                buf.append(b'\\');
                buf.append(b'0' + (ch >> 6));
                buf.append(b'0' + ((ch & 56) >> 3));
                buf.append(b'0' + (ch & 7));
            } else {
                buf.append(ch)
            }
        }
        buf.append(b'"');

        buf.size = buf.pos;
        return String(buf, true);
    }

    void format(StaticString data) {
        write(encodeString(data));
    }

    void format(String data) {
        write(encodeString(data));
    }

    void format(float32 value) {
        int fptype = fpclassify(value);

        if (fptype == FP_NORMAL || fptype == FP_ZERO) StandardFormatter.format(value);
        else {
            if (value < 0) write('-');
            if (fptype == FP_NAN) write('NaN');
            else if (fptype == FP_INFINITE) write('Infinity');
        }
    }

    void format(float64 value) {
        int fptype = fpclassify(value);
        
        if (fptype == FP_NORMAL || fptype == FP_ZERO) StandardFormatter.format(value);
        else {
            if (value < 0) write('-');
            if (fptype == FP_NAN) write('NaN');
            else if (fptype == FP_INFINITE) write('Infinity');
        }
    }

    // For general objects, format() just calls the object's writeTo()
    // method.
    void format(Object obj) {
        if (obj is null)
            write(NULL);
        else if (obj.isa(String))
            format(String.cast(obj));
        else
            obj.writeTo(this);
    }
}

class JsonStringFormatter : JsonFormatter {
    StringWriter _writer;
    oper init() : JsonFormatter (null) {
        _writer = StringWriter();
        rep = _writer;
    }

    // Return a string containing everything that has been written so far.
    String string() {
        retval := _writer.string();
        _writer = StringWriter();
        rep = _writer;
        return retval;
    }
}


@define writeValue() {
    void writeTo(Formatter fmt) {
        fmt.format(value);
    }
}

class unexpectedToken : Exception {
    oper init(byteptr data, uint p, uint pe){
        cerr `$p: $(String(Buffer(data + uintz(p), pe - p)))\n`;
    }
}

class parseException : Exception {
    oper init(String text0, uint line, uint col){
        text = FStr() `$(text0):$line:$col`;
    }
}

class JsonObject : HashMap[String, Object] {
}

class JsonArray : Array[Object] {
}

class JsonScalar {
}

class JsonInt : JsonScalar {
    int value;
    oper init(int value): value = value {}
    @writeValue()
}

class JsonFloat : JsonScalar {
    float value;
    oper init(float value): value = value {}
    @writeValue()
}

class JsonBool : JsonScalar {
    bool value;
    oper init(bool value): value = value {}
    @writeValue()
}

class JsonParser {

  /* EOF char used to flush out that last token. This should be a whitespace
   * token. */

    uint LAST_CHAR = 0, EVIL = 6666;

    ManagedBuffer buf;
    AppendBuffer append_buf = {128};
    uint data_size = 0, eof = 0, have = 0, maxNesting = 10, currentNesting = 0;
    uint line = 1, col = 1;
    byteptr data;
    bool allowNaN = true, quirksMode = true;


    class ParserResult {

        /**
         * The result of the successful parsing. Should never be
         * <code>null</code>.
         */
        Object result;

        /**
         * The point where the parser returned.
         */
        uint p;

        oper init(Object result, uint p) : result = result, p = p {
        }

        @define writeJsonValue(Type){
            if (result.isa(Type)) { 
                fmt.format(Type.cast(result));
            }
        }

        void writeTo(Formatter fmt) {
            if (result is null){
              fmt.format(fmt.NULL);
              return;
            }

            @writeJsonValue(JsonInt)
            else @writeJsonValue(JsonBool)
            else @writeJsonValue(JsonFloat)
            else @writeJsonValue(JsonObject)
            else @writeJsonValue(JsonArray)
            else @writeJsonValue(String)
            else fmt.format('UNKNOWN');
        }

    }

    %%{
        machine JSON_common;

        cr                  = '\n';
        cr_neg              = [^\n];
        ws                  = [ \t\r\n];
        c_comment           = '/*' ( any* - (any* '*/' any* ) ) '*/';
        cpp_comment         = '//' cr_neg* cr;
        comment             = c_comment | cpp_comment;
        ignore              = ws | comment;
        name_separator      = ':';
        value_separator     = ',';
        Vnull               = 'null';
        Vfalse              = 'false';
        Vtrue               = 'true';
        VNaN                = 'NaN';
        VInfinity           = 'Infinity';
        VMinusInfinity      = '-Infinity';
        begin_value         = [nft"\-[{NI] | digit;
        begin_object        = '{';
        end_object          = '}';
        begin_array         = '[';
        end_array           = ']';
        begin_string        = '"';
        begin_name          = begin_string;
        begin_number        = ('-' | digit);
    }%%

//------------------------------------------------------------------------------


    String bufferString(uint bi, uint ei){
        return String(Buffer(data + uintz(bi), ei - bi));
    }

    int _atoi(uint bi, uint ei){
        return atoi(CString(data + uintz(bi), ei - bi, false));
    }

    // Forward declarations
    ParserResult parseValue(uint p, uint pe);
    ParserResult parseFloat(uint p, uint pe);
    ParserResult parseInteger(uint p, uint pe);
    ParserResult parseString(uint p, uint pe);
    ParserResult parseArray(uint p, uint pe);
    ParserResult parseObject(uint p, uint pe);

    %%{
        machine JSON_value;
        include JSON_common;

        write data;

        action parse_null {
            result = null;
        }

        action parse_false {
            result = JsonBool(false);
        }

        action parse_true {
            result = JsonBool(true);
        }

        action parse_nan {
            if (allowNaN) {
                result = JsonFloat(NAN);
            } else {
                throw unexpectedToken(data, p - 2, pe);
            }
        }

        action parse_infinity {
            if (allowNaN) {
                result = JsonFloat(INFINITY)
            } else {
                throw unexpectedToken(data, p - 7, pe);
            }
        }

        action parse_number {
            if (pe > fpc + 9 && bufferString(p, p+9) == "-Infinity") {
                if (allowNaN) {
                    result = JsonFloat(-INFINITY);
                    fexec p + 10;
                    fhold;
                    fbreak;
                } else {
                    throw unexpectedToken(data, p, pe);
                }
            }

            res = parseFloat(fpc, pe);
            if (!(res is null)) {
                result = res.result;
                fexec res.p;
            }

            res = parseInteger(fpc, pe);
            if (!(res is null)) {
                result = res.result;
                fexec res.p;
            }
            fhold;
            fbreak;
        }

        action parse_string {
            res = parseString(fpc, pe);
            if (res is null) {
                fhold;
                fbreak;
            } else {
                result = res.result;
                fexec res.p;
            }
        }

        action parse_array {
            currentNesting++;
            res = parseArray(fpc, pe);
            currentNesting--;
            if (res is null) {
                fhold;
                fbreak;
            } else {
                result = res.result;
                fexec res.p;
            }
        }

        action parse_object {
            currentNesting++;
            res = parseObject(fpc, pe);
            currentNesting--;
            if (res is null) {
                fhold;
                fbreak;
            } else {
                result = res.result;
                fexec res.p;
            }
        }

        action exit {
            fhold;
            fbreak;
        }

        main := ( Vnull @parse_null |
                  Vfalse @parse_false |
                  Vtrue @parse_true |
                  VNaN @parse_nan |
                  VInfinity @parse_infinity |
                  begin_number >parse_number |
                  begin_string >parse_string |
                  begin_array >parse_array |
                  begin_object >parse_object
                ) %*exit;
    }%%

    ParserResult parseValue(uint p, uint pe) {
        Object result = null;
        ParserResult res = null;
        uint cs = EVIL;

        %% write init;

        %% write exec;

        if (cs >= JSON_value_first_final){
            return ParserResult(result, p);
        }

        return null;
    }

    %%{
        machine JSON_integer;

        write data;

        action exit {
            fhold;
            fbreak;
        }

        main := '-'? ( '0' | [1-9][0-9]* ) ( ^[0-9]? @exit );
    }%%

    ParserResult parseInteger(uint p, uint pe) {
        uint cs = EVIL;
        uint memo = p;

        %% write init;

        %% write exec;

        if (cs < JSON_integer_first_final) {
            return null;
        }

        JsonInt number = { atoi(CString(data + uintz(memo), p - memo, false)) };

        return ParserResult(number, p + 1);
    }

    %%{
        machine JSON_float;
        include JSON_common;

        write data;

        action exit {
            fhold;
            fbreak;
        }

        main := '-'?
                ( ( ( '0' | [1-9][0-9]* ) '.' [0-9]+ ( [Ee] [+\-]?[0-9]+ )? )
                | ( ( '0' | [1-9][0-9]* ) ( [Ee] [+\-]? [0-9]+ ) ) )
                ( ^[0-9Ee.\-]? @exit );
    }%%

    ParserResult parseFloat(uint p, uint pe) {
        uint cs = EVIL;
        uint memo = p;

        %% write init;

        %% write exec;

        if (cs < JSON_float_first_final) {
            return null;
        }

        JsonFloat number = { strtof(CString(data + uintz(memo), p - memo, false)) };
        return ParserResult(number, p + 1);
    }

    %%{
        machine JSON_string;
        include JSON_common;

        write data;
        
        action startString {
            memo++;
        }
        
        action regularString {
            append_buf.extend(data + uintz(memo), p - memo);
            memo = p;
        }

        action parse_number_start {
            memo = p;
        }

        action parse_number {
            chr = (data[memo] - 48) *64;
            chr += (data[memo+1] - 48)*8;
            chr += (data[memo+2] - 48);
            append_buf.append(chr);
            memo = p + 1;
            fexec p + 1;
        }

        action escapeString {
            if (p > memo)
                append_buf.extend(data + uintz(memo), p - memo - 1);
            chr = data[p];
            if (chr == b'"' || chr == b'\\' || chr == b'/')
                append_buf.append(chr);
            else if (chr == b"b")
                append_buf.append(b"\b");
            else if (chr == b"n")
                append_buf.append(b"\n");
            else if (chr == b"f")
                append_buf.append(b"\f");
            else if (chr == b"r")
                append_buf.append(b"\r");
            else if (chr == b"t")
                append_buf.append(b"\t");
            memo = p + 1;
            fexec p + 1;
        }

        action parse_string {
            if (p > memo)
                append_buf.extend(data + uintz(memo), p - memo);
            result = String(append_buf, append_buf.pos, false);
            fexec p + 1;
        }

        action exit {
            fhold;
            fbreak;
        }

        main := '"' >startString
                ( ( ^(["\\]|0..0x1f)
                  | '\\' ["\\/bfnrt] @escapeString
                  | '\\' ([0..3] >parse_number_start [0-7]{2}) @parse_number
                  )* %parse_string
                ) '"' @exit;
    }%%

    ParserResult parseString(uint p, uint pe) {
        append_buf.pos = 0;
        String result = null;
        uint cs = EVIL;
        byte chr;

        uint memo = p;

        %% write init;

        %% write exec;

        if (cs >= JSON_string_first_final && !(result is null)) {
            return ParserResult(result, p + 1);
        } else {
            return null;
        }
    }

    %%{
        machine JSON_array;
        include JSON_common;

        write data;

        action parse_elem {
            res = parseValue(fpc, pe);
            if (res is null) {
                fhold;
                fbreak;
            } else {
                result.append(res.result);
                fexec res.p;
            }
        }

        action exit {
            fhold;
            fbreak;
        }

        next_element = value_separator ignore* begin_value >parse_elem;

        main := begin_array
                ignore*
                ( ( begin_value >parse_elem
                    ignore* )
                  ( ignore*
                    next_element
                    ignore* )* )?
                ignore*
                end_array @exit;
    }%%

    ParserResult parseArray(uint p, uint pe) {
        if (maxNesting > 0 && currentNesting > maxNesting) {
            throw parseException(FStr() `Nesting of $currentNesting is too deep`, line, col);
        }

        JsonArray result = {};
        ParserResult res = null;

        uint cs = EVIL;

        %% write init;

        %% write exec;

        if (cs >= JSON_array_first_final) {
            return ParserResult(result, p + 1);
        } else {
            throw unexpectedToken(data, p, pe);
        }
    }

    %%{
        machine JSON_object;
        include JSON_common;

        write data;

        action parse_value {
            res = parseValue(fpc, pe);
            if (res is null) {
                fhold;
                fbreak;
            } else {
                if (!(lastName is null))
                    result[lastName] = res.result;
                else
                    throw parseException(FStr() `No key for mapping`, line, col);
                fexec res.p;
            }
        }

        action parse_name {
            res = parseString(fpc, pe);

            if (res is null) {
                throw parseException(FStr() `Expected a string while parsing object key, got $(bufferString(p, pe))`, line, col);
            } else {
                lastName = String.cast(res.result);
                fexec res.p;
            }
        }

        action exit {
            fhold;
            fbreak;
        }
        
        pair      = ignore* begin_name >parse_name ignore* name_separator
                    ignore* begin_value >parse_value;
        next_pair = ignore* value_separator pair;

        main := (
          begin_object (pair (next_pair)*)? ignore* end_object
        ) @exit;
    }%%

    ParserResult parseObject(uint p, uint pe) {
        String lastName = null;
        ParserResult res = null;
        if (maxNesting > 0 && currentNesting > maxNesting) {
            throw parseException(FStr() `Nesting of $currentNesting is too deep`, line, col);
        }

        JsonObject result = {};

        uint cs = EVIL;

        %% write init;

        %% write exec;

        if (cs < JSON_object_first_final) {
            return null;
        }
        return ParserResult(result, p + 1);
    }

    %%{
        machine JSON;
        include JSON_common;

        write data;

        action parse_object {
            currentNesting = 1;
            res = parseObject(fpc, pe);
            if (res is null) {
                fhold;
                fbreak;
            } else {
                result = res.result;
                fexec res.p;
            }
        }

        action parse_array {
            currentNesting = 1;
            res = parseArray(fpc, pe);
            if (res is null) {
                fhold;
                fbreak;
            } else {
                result = res.result;
                fexec res.p;
            }
        }

        main := ignore*
                ( begin_object >parse_object
                | begin_array >parse_array )
                ignore*;
    }%%

    ParserResult parseStrict() {
        Object result = null;
        ParserResult res = null;

        uint cs = EVIL;
        uint p = 0;
        uint pe = p + data_size;

        %% write init;

        %% write exec;

        if (cs >= JSON_first_final && p == pe) {
            return res;
        } else {
            throw unexpectedToken(data, p, pe);
        }
    }

    %%{
        machine JSON_quirks_mode;
        include JSON_common;

        write data;

        action parse_value {
            res = parseValue(fpc, pe);
            if (res is null) {
                fhold;
                fbreak;
            } else {
                result = res.result;
                fexec res.p;
            }
        }

        main := ignore* ( begin_value >parse_value) ignore*;
    }%%

    ParserResult parseQuirksMode() {
        Object result = null;
        ParserResult res = null;

        uint cs = EVIL;
        uint p = 0;
        uint pe = p + data_size;

        %% write init;

        %% write exec;

        if (cs >= JSON_quirks_mode_first_final && p == pe) {
            return res;
        } else {
            throw unexpectedToken(data, p, pe);
        }
    }

    oper init() {
    }

    ParserResult parse() {
        if (quirksMode) {
            return parseQuirksMode();
        } else {
            return parseStrict();
        }
    }

//------------------------------------------------------------------------------

    ParserResult parse(String buf0) {
        data = buf0.buffer;
        data_size = buf0.size;
        return parse();
    }
}
