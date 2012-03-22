// INI file parser
// (C) Conrad Steenberg <conrad.steenberg@gmail.com>
// 2/11/2012

import crack.io FStr;
import crack.lang Formatter, AppendBuffer, Buffer;
import crack.cont.treemap TreeMap;
import crack.cont.array Array;
import crack.ascii escape;

class IniError {
  String msg;
  uint line, col;

  oper init(String msg, uint line, uint col): msg = msg, line = line, col = col
  { }

  void formatTo(Formatter fmt){
    fmt `$msg:$line:$col`;
  }
}

class IniParser {

    uint maxErr = 1;
    Array[IniError] _errors;
    TreeMap[String, TreeMap[String, String]] _resultMap;
    TreeMap[String, String] _sectionMap;

    %%{
        machine IniParser;

        write data;

        # Actions -------------------------------------------------------------
        action hold {
          fhold;
        }

        action errorHandler {
          if (p > okp)
            _errors.append(IniError(FStr() `Syntax error near $(inputString.slice(okp, p))`, line, p-lineOffset-1));
          if (_errors.count() >= maxErr) return _resultMap;
        }

        action keyError {
          if (p > okp) {
            String msg;
            if (data[p] == b'=')
              msg = "Invalid key syntax ";
            else
              msg = "Invalid value syntax ";
            _errors.append(IniError(msg + inputString.slice(okp, p), line, p-lineOffset-1));
          }
          if (_errors.count() >= maxErr) return _resultMap;
        }

        action incLineNum {
          if (lineOffset < p && data[p] < 32) line+=1;
          lineOffset = p+1;
          okp = p+1;
        }

        action commentHandler {
          okp = p+1;
        }

        action startSection {
          if (p==lineOffset) {
            if (_sectionMap != null && sectionName != null){
              _resultMap[sectionName] = _sectionMap;
            }
            _sectionMap = null;
            sectionName = null;
            okp = p+1;
          }
        }

        action endSection {
          sectionName = inputString.slice(okp, p);
          _sectionMap = TreeMap[String, String]();
          _resultMap[sectionName] = _sectionMap;
          okp = p+1;
        }

        action keyEnd {
          if (marker < lineOffset) {
            key = inputString.slice(lineOffset, p);
            marker = p;
          }
          okp = p+1;
        }

        action valueStart {
          appendBuf.size = 0;
          okp = p+1;
        }

        action regularString {
            if (p>okp){
              appendBuf.extend(data + uintz(okp), p - okp);
              okp = p;
          }
        }

        action parseNumberStart {
          okp = p;
        }

        action parseNumber {
            chr = (data[okp] - 48) *64;
            chr += (data[okp+1] - 48)*8;
            chr += (data[okp+2] - 48);
            appendBuf.append(chr);

            okp = p + 1;
            fexec p + 1;
        }

        action escapeString {
            if (p > okp)
                appendBuf.extend(data + uintz(okp), p - okp - 1);
            chr = data[p];
            if (chr == b'"' || chr == b'\\' || chr == b'/')
                appendBuf.append(chr);
            else if (chr == b"b")
                appendBuf.append(b"\b");
            else if (chr == b"n")
                appendBuf.append(b"\n");
            else if (chr == b"f")
                appendBuf.append(b"\f");
            else if (chr == b"r")
                appendBuf.append(b"\r");
            else if (chr == b"t")
                appendBuf.append(b"\t");
            okp = p + 1;
            fexec p + 1;
        }

        action valueEnd {
          if (p > okp)
              appendBuf.extend(data + uintz(okp), p - okp);
          value = String(appendBuf, 0, appendBuf.size);

          _sectionMap[key] = value;
          okp = p+1;
        }

        action emptyLine {
          okp = p+1;
        }

        # Machine definitions --------------------------------------------------
        varAlpha = [a-zA-Z_];
        varAlphaNum = [a-zA-Z_0-9\-]*;
        varGen = [a-zA-Z_0-9\-\[\]]*;
        varName = varAlpha varAlphaNum;
        varNameGen = varAlpha varGen;

        ws = [ \t]+;
        eol = [\r\n];
        eolComment = ([\r\n] >commentHandler >incLineNum);

        comment = ';' (any -- eol)* eolComment;

        section = '[' >startSection varName ']' >endSection ws? 
                   eol >incLineNum @hold;

        keyval = varNameGen (ws? >keyEnd) '=' >keyEnd >valueStart (ws? @valueStart)
                 (   (0..0x1f -- eol) >keyError
                  | '\\' >regularString ["\\/bfnrt] >escapeString
                  | '\\' >regularString ([0..3] >parseNumberStart [0-7]{2}) @parseNumber
                  | (^(0..0x1f |'\\'))+
                  )*
                  eol >valueEnd >errorHandler >incLineNum;

        emptyline = ws? eol >commentHandler >errorHandler >incLineNum;

        errorkey = (any -- '=')+ '=' >keyError;

        main := (emptyline | comment | section | keyval)*;
    }%%

    # parser method ------------------------------------------------------------
    oper init() {}

    TreeMap[String, TreeMap[String, String]] parse(String inputString, uint p0, uint pe0) {
        AppendBuffer appendBuf = {128}; // String to hold the value
        _errors = Array[IniError]();      // reset _errors

        // Placeholders for section, key, and value strings
        String sectionName = null, key = null, value = null;

        // column and line counters for error messages
        uint line = 1, col = 1;

        byteptr data = inputString.buffer;
        uint data_size = inputString.size;

        uint p = p0, pe = pe0, okp = 0, eof = data_size;
        uint cs = IniParser_error, lineOffset, marker;
        byte chr;

        %% write init;

        %% write exec;

        return _resultMap;
    }

    TreeMap[String, TreeMap[String, String]] parse(String inputString) {
        _resultMap = TreeMap[String, TreeMap[String, String]]();
        TreeMap[String, String] _sectionMap = null;

        return parse(inputString, 0, inputString.size);
    }

    void reset(){
      _resultMap = null;
      _sectionMap = null;
      _errors =  null;
    }

    TreeMap[String, TreeMap[String, String]] results(){
      return _resultMap;
    }

    Array[IniError] errors(){
      return _errors;
    }

    void formatTo(Formatter fmt){
      if (_resultMap is null)
        fmt.write('null');
      else {
        for (section :in _resultMap){
          fmt `[$(section.key)]\n`;
          for (vitem :in section.val)
            fmt `$(vitem.key)=$(escape(vitem.val, 32, 255))\n`
        }
      }
    }

}

iniParser := IniParser();
