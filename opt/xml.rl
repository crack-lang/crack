// This file is originally from libgdx, the original AUTHORS file referred to
// below is at http://libgdx.googlecode.com/svn/trunk/gdx/AUTHORS

// Translated from the original Java version
// Copyright 2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// 2/27/2012

// *****************************************************************************
// * Copyright 2011 See AUTHORS file.
// * 
// * Licensed under the Apache License, Version 2.0 (the "License");
// * you may not use this file except in compliance with the License.
// * You may obtain a copy of the License at
// * 
// *   http://www.apache.org/licenses/LICENSE-2.0
// * 
// * Unless required by applicable law or agreed to in writing, software
// * distributed under the License is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// * See the License for the specific language governing permissions and
// * limitations under the License.
// *****************************************************************************

import crack.ascii parseInt, parseBool, radix;
import crack.cont.array Array;
import crack.cont.hashmap OrderedHashMap;
import crack.cont.list List;
import crack.exp.file File;
import crack.io FStr, cout, cerr, StringFormatter, Reader, StringWriter;
import crack.lang AppendBuffer, InvalidResourceError, Buffer, Formatter,
                  WriteBuffer, Exception, IndexError, KeyError, CString;
import crack.math min, strtof;
import crack.runtime memmove, mmap, munmap, Stat, fopen, PROT_READ, MAP_PRIVATE,
                    stat, fileno;
import crack.sys strerror;
import crack.io.readers PageBufferString, PageBufferReader, PageBuffer;

uint indentWidth = 4;

class ParseError : Exception {
    oper init(String text) : Exception(text) {}
    oper init() {}
}

uint ELEMENT = 0;
uint COMMENT = 1;
uint DOCINFO = 2;
uint CDATA = 3;

/// An XML element class possibly containing a DOM tree
class Element {
    String _name;
    OrderedHashMap[String, String] _attributes;
    List[Element] _children;
    String _text;
    Element _parent;
    uint _type;

    oper init(String name, Element parent): _name=name, _parent=parent,
                                            _type = ELEMENT{
    }

    oper init(String name, Element parent, uint type):  _name=name,
                                                        _parent=parent,
                                                        _type = type{
    }

    String getName() {
        return _name;
    }

    uint getType() {
        return _type;
    }

    List[Element] getChildren () {
      if (!_children) return List[Element]![];
      else return _children;
    }

    OrderedHashMap[String, String] getAttributes () {
        return _attributes;
    }

    void toString(Formatter fmt, String indent) {
        fmt `$indent<`;
        if (_type == COMMENT && _name.size ==0) fmt.write('!--');
        else if (_type == DOCINFO) fmt.write('?');
        fmt `$_name`;
        if (_attributes) {
            for (entry :in _attributes)
                fmt ` $(entry.key)="$(entry.val)"`;
        }

        if (_type == COMMENT) {
            if (_text && _text.size >0)
                fmt.write(_text);
            fmt.write('-->');
        }
        else if (!_children && (!_text || _text.size == 0)){
            if (_type == DOCINFO) fmt.write('?>\n');
            else fmt.write("/>");
        }
        else {
            fmt.write(">\n");
            childIndent := indent + " "*indentWidth;
            if (_text && _text.size > 0) {
                fmt `$childIndent$_text\n`;
            }
            if (_children) {
                for (child :in _children) {
                    child.toString(fmt, childIndent);
                    fmt.write("\n");
                }
            }
            fmt `$indent</$_name>`;
        }
    }

    void formatTo(Formatter fmt){
      toString(fmt, "");
    }

    uint getChildCount () {
        if (!_children) return 0;
        return _children.count();
    }

    Element getChild (uint i) {
        if (!_children) throw IndexError(FStr() `Element has no children: $_name`);
        return _children[i];
    }

    void addChild(Element element) {
        if (!_children) _children = List[Element]();
        _children.append(element);
    }

    String getText() {
        return _text;
    }

    void setText(String text) {
        this._text = text;
    }

    void setAttribute(String name, String value) {
        if (!_attributes) _attributes = OrderedHashMap[String, String]();
        _attributes[name] = value;
    }

    void set(String name, String value) {
        setAttribute(name, value);
    }

    void removeChild(int index) {
        if (_children != null) _children.delete(index);
    }

    void removeChild(Element child) {
        if (_children != null) _children.delete(child);
    }

    void remove() {
        _parent.removeChild(this);
    }

    Element getParent() {
        return _parent;
    }

    /// @param name the name of the child {@link Element}
    /// @return the first child having the given name or null, does not recurse
    Element getChildByName(String name) {
        if (!_children) return null;
        for (uint i = 0; i < _children.count(); i++) {
            Element element = _children[i];
            if (element._name == name) return element;
        }
        return null;
    }

    /// @param name the name of the child {@link Element}
    /// @return the first child having the given name or null, recurses
    Element getChildByNameRecursive (String name) {
        if (!_children) return null;
        for (uint i = 0; i < _children.count(); i++) {
            Element element = _children[i];
            if (element._name == name) return element;
            Element found = element.getChildByNameRecursive(name);
            if (found) return found;
        }
        return null;
    }

    /// @param name the name of the children
    /// @return the children with the given name or an empty {@link Array}
    Array[Element] getChildrenByName(String name) {
        Array[Element] result = {};
        if (!_children) return result;
        for (uint i = 0; i < _children.count(); i++) {
            Element child = _children[i];
            if (child._name == name) result.append(child);
        }
        return result;
    }

    void getChildrenByNameRecursively (String name, Array[Element] result) {
        if (!_children) return;
        for (uint i = 0; i < _children.count(); i++) {
            Element child = _children[i];
            if (child._name == name) result.append(child);
            child.getChildrenByNameRecursively(name, result);
        }
    }

    /// @param name the name of the children
    /// @return the children with the given name or an empty {@link Array}
    Array[Element] getChildrenByNameRecursively (String name) {
        Array[Element] result = {};
        getChildrenByNameRecursively(name, result);
        return result;
    }

    /// Returns the attribute value with the specified name, or if no
    /// attribute is found, the text of a child with the name.
    String get(String name, String defaultValue) {
        if (_attributes) {
            String value = _attributes.get(name);
            if (value) return value;
        }
        Element child = getChildByName(name);
        if (!child) return defaultValue;
        String value = child.getText();
        if (!value) return defaultValue;
        return value;
    }

    /// Returns the attribute value with the specified name, or if no attribute
    /// is found, the text of a child with the name.
    /// @throws KeyError if no attribute or child was not found.
    String get(String name) {
        String value = get(name, null);
        if (!value) throw KeyError(FStr() `Element $(this._name) doesn't have attribute or child: $name`);
        return value;
    }

    /// Returns the attribute value with the specified name, or if no attribute is found, the text of a child with the name.
    /// @throws KeyError if no attribute or child was not found.
    int getInt (String name, int defaultValue) {
        String value = get(name, null);
        if (!value) return defaultValue;
        return parseInt(value);
    }

    /// Returns the attribute value with the specified name, or if no
    /// attribute is found, the text of a child with the name.
    /// @throws KeyError if no attribute or child was not found.
    int getInt(String name) {
        String value = get(name, null);
        if (!value) throw KeyError(FStr() `Element $(this._name)  doesn't have attribute or child: $name`);
        return parseInt(value);
    }

    /// Returns the attribute value with the specified name, or if no attribute is found, the text of a child with the name.
    /// @throws KeyError if no attribute or child was not found.
    float getFloat (String name, float defaultValue) {
        String value = get(name, null);
        if (!value) return defaultValue;
        return strtof(value);
    }

    /// Returns the attribute value with the specified name, or if no attribute is found, the text of a child with the name.
    /// @throws KeyError if no attribute or child was not found.
    float getFloat (String name) {
        String value = get(name, null);
        if (!value) throw KeyError(FStr() `Element $(this._name) doesn't have attribute or child: $name`);
        return strtof(value);
    }

    /// Returns the attribute value with the specified name, or if no attribute is found, the text of a child with the name.
    /// @throws KeyError if no attribute or child was not found.
    bool getBool (String name) {
        String value = get(name, null);
        if (!value) throw KeyError(FStr() `Element $(this._name) doesn't have attribute or child: $name`);
        return parseBool(value);
    }

    /// Returns the attribute value with the specified name, or if no attribute is found, the text of a child with the name.
    /// @throws KeyError if no attribute or child was not found.
    bool getBool (String name, bool defaultValue) {
        String value = get(name, null);
        if (!value) return defaultValue;
        return parseBool(value);
    }
}


/// Lightweight XML parser. Supports a subset of XML features: elements,
/// attributes, text, predefined entities, CDATA, mixed content. Namespaces are
/// parsed as part of the element or attribute name. Prologs and doctypes are
/// ignored. Only 8-bit character encodings are supported. <br> The default
/// behavior is to parse the XML into a DOM. Extend this class and override
/// methods to perform event driven parsing. When this is done, the parse methods
/// will return null.
/// @author Nathan Sweet
class XmlReader {

    PageBuffer data;
    uint data_size = 0, eof = 0, p, pe, cs, ts, te, act, okp, bufsize = 1024*1024;
    int line = 1, col = 1;
    String attributeName = null;
    bool hasBody = false;

    /* EOF char used to flush out that last token. This should be a whitespace
    * token. */

    uint LAST_CHAR = 0, _level;

    void _readTo(uint i){
        if (i>data.size){
            try {
                b:=data[i];
            }
            catch (IndexError ex){
            }
            pe = data.size;
        }
    }

    Array[Element] _elements = {};
    Element __root, __current;
    StringWriter __textBuffer = {64};

    void _text(String text) {
        String existing = __current.getText();
        __current.setText(existing ? existing + text : text);
    }

    void print_elements(){
        cout `**** elements = [`;
        for (el :in _elements)
            cout `$(el._name), `;
        cout `]\n`;
    }

    void _open(String name) {
        if (true) {
            _level++;
            Element child = {name, __current};
            Element parent = __current;
            if (parent) parent.addChild(child);
            _elements.append(child);
            __current = child;
        }
    }

    void _open(String name, String text, uint type) {
        if (true) {
            Element child = {name, __current, type};
            child.setText(text);
            Element parent = __current;
            if (parent) parent.addChild(child);
            _elements.append(child);
            __current = child;
        }
    }

    void _setAttribute (String name, String value) {
        __current.setAttribute(name, value);
    }

    String _entity(String name) {
        if (name =="lt") return "<";
        if (name == "gt") return ">";
        if (name == "amp") return "&";
        if (name == "apos") return "'";
        if (name == "quot") return "\"";
        return null;
    }

    void _close(uint s) {
        _level--;
        __root = _elements[-1].getParent() ? _elements.pop().getParent() : null; 
        __current = _elements.count()  > 0 && _elements[-1].getType() != DOCINFO ? _elements[-1] : null;
    }

    %%{
    machine xml;

        action buffer { _readTo(p); s = p; }
        action elementStart {
            byte c = data[s];
            if (c == b'!') {
                _readTo(p+10);
                if (
                    data[s + 1] == b'[' && //
                    data[s + 2] == b'C' && //
                    data[s + 3] == b'D' && //
                    data[s + 4] == b'A' && //
                    data[s + 5] == b'T' && //
                    data[s + 6] == b'A' && //
                    data[s + 7] == b'['
                ) {
                    s += 8;
                    p = s + 2;
                    while (p<pe && (data[p - 2] != b']' || data[p - 1] != b']' || data[p] != b'>')){ // TODO optimize this
                        _readTo(p+1); // update pe
                        p++;
                    }
                    _text(data.substr(s, p - s - 2));
                } else if (pe - p > 4 &&
                    data[s + 1] == b'-' && 
                    data[s + 2] == b'-' &&
                    data[s + 3] != b'>'
                ) {
                    while (p<pe) {
                        _readTo(++p); // update pe
                        if (data[p] == b'>' && data[p-1] == b'-' && data[p-2] == b'-'){
                            break;
                        }
                    }
                    if (p==pe)
                        throw ParseError(FStr() `Unmatched comment open element near $(data.substr(s, min(80, p-s)))`);
                    _open("!--", data.substr(s+3, p - s-5), COMMENT);
                    hasBody = false;
                    _close(s);
                    s=p;
                }
                else {
                    while (p<pe && data[p] != b'>'){
                        _readTo(++p);
                    }
                }
                fgoto elementBody;
            }
            else {
                if (c == b'?') {
                    if (_level>0) throw ParseError("Document metadata must appear at the toplevel of the document");
                    hasBody = false;
                    _open(data.substr( s + 1, p - s - 1), "", DOCINFO);
                }
                else {
                    hasBody = true;
                    _open(data.substr( s, p - s));
                }
            }
        }

        action elementEndSingle {
            hasBody = false;
            _close(s);
            s=p;
            fgoto elementBody;
        }

        action elementEnd {
            _close(s);
            s=p;
            fgoto elementBody;
        }
        action element {
            if (hasBody) fgoto elementBody;
        }
        action attributeName {
            attributeName = data.substr(s, p - s);
        }
        action attribute {
            _setAttribute(attributeName, data.substr(s, p - s));
        }

        action text {
            if (true) { // Crack doesn't have nested blocks yet
                uint end = p;
                while (end != s) {
                    if (data[end - 1] == b' ' ||
                        data[end - 1] == b'\t' ||
                        data[end - 1] == b'\n' ||
                        data[end - 1] == b'\r') {
                        end--;
                        continue;
                    }
                    break;
                }
                uint __ci = s;
                bool entityFound = false;
                while (__ci != end) {
                    if (data[__ci++] != b'&') continue;
                    uint entityStart = __ci;
                    while (__ci != end) {
                        if (data[__ci++] != b';') continue;
                        __textBuffer.write(data.substr(s, entityStart - s - 1));
                        String name = data.substr(entityStart, __ci - entityStart - 1);
                        String value = _entity(name);
                        __textBuffer.write(value ? value : name);
                        s = __ci;
                        entityFound = true;
                        break;
                    }
                }
                if (entityFound) {
                    if (s < end) __textBuffer.write(data.substr(s, end - s));
                    _text(__textBuffer.string());
                    __textBuffer = StringWriter(64);
                } else
                    _text(data.substr(s, end - s));
            }
        }

        attribute = ^(space | [/>=])+ >buffer %attributeName space* '=' space*
            (('\'' ^'\''* >buffer %attribute '\'') | ('"' ^'"'* >buffer %attribute '"'));
        element = '<' space* ^(space | [/>])+ >buffer %elementStart (space+ attribute)*
            :>> (space* (('/'|'?') %elementEndSingle)? space* '>' @element);
        elementBody := space* <: ((^'<'+ >buffer %text) <: space*)?
            element? :>> ('<' space* '/' ^'>'+ '>' @elementEnd);
        main := space* element space*;

    }%%

    %% write data;

    void reset(){
        p = 0;
        _elements.clear();

        %% write init;

    }

    oper init () { }

    Array[Element] _parse() {    // Do the first read. 
        if (data is null)
            InvalidResourceError(FStr() `Error parsing XML, null data pointer supplied`);
        uint s, parseLoops = 0;
        pe = data_size;
        cs = xml_start;
        while (parseLoops <2 && p < pe){
        // ------ Start exec ---------------------------------------------------------
        %% write exec;
        // ------ End exec -----------------------------------------------------------
            _readTo(pe+1); // Update pe just in case we got stuck at the end of a page by accident
            if (p < pe ) parseLoops++;
        }

        /* Check if we failed. */
        if ( cs == xml_error ) {
            /* Machine failed before finding a token. */
            throw ParseError(data.substr(s, p - s));
        }

        if (p < pe) {
            uint lineNumber = 1;
            for (uint i = 0; i < p; i++)
                if (data[i] == b'\n') lineNumber++;
            throw InvalidResourceError(FStr() `Error parsing XML on line $lineNumber near: $(data.substr(p, min(32, pe-p)))`);
        }
        uint namedElems = 0;
        for (uint ei = 0; ei < _elements.count(); ei++){
            eName := _elements[ei].getName();
            if (_elements[ei].getParent())
                throw InvalidResourceError(FStr() `Error parsing XML, unclosed element: $eName`);
        }

        this.__root = null;
        return _elements;
    }

    Array[Element] parse(String xml) {
        data = PageBufferString(xml);
        data_size = xml.size;
        return _parse();
    }

    Array[Element] parse(Reader r) {
        data = PageBufferReader(r); // Reads one block
        data_size = data.size; 
        return _parse();
    }


    Array[Element] parseFile(String fname) {
        Stat statInfo = {};
        n := CString(fname);
        statErrors := stat(n.buffer, statInfo);
        if (!statErrors){
            mode := "r";
            file := fopen(n.buffer, mode.buffer);

            if (file is null) {
                throw InvalidResourceError(FStr() `$fname: $(strerror())`);
            }
            fd := fileno(file);

            data_size = statInfo.st_size;
            tdata := mmap(null, statInfo.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
            data = PageBufferString(String(byteptr(tdata), data_size, false));
            
            if (uintz(tdata) != uintz(0)-1){
                Array[Element] retval = _parse();
                munmap(tdata, data_size);
                return retval;
            }
            else
                throw InvalidResourceError(FStr() `$fname: $(strerror())`);
        }
        return null;
    }

    void formatTo(Formatter fmt){
      for (element :in _elements)
        fmt.format(element);
    }
}
