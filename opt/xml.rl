// This file is originally from libgdx, the original AUTHORS file referred to
// below is at http://libgdx.googlecode.com/svn/trunk/gdx/AUTHORS

// Translated from the original Java version
// Conrad Steenberg <conrad.steenberg@gmail.com>
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
import crack.cont.hashmap HashMap, OrderedHashMap;
import crack.exp.file File;
import crack.io FStr, cout, cerr, StringFormatter, Reader;
import crack.lang AppendBuffer, InvalidResourceError, Buffer, Formatter,
                  WriteBuffer, Exception, IndexError, KeyError, CString;
import crack.math min, strtof;
import crack.runtime memmove, mmap, munmap, Stat, fopen, PROT_READ, MAP_PRIVATE,
                    stat, fileno;
import crack.sys strerror;

uint indentWidth = 4;

class ParseError : Exception {
    oper init(String text) : Exception(text) {}
    oper init() {}
}

/// Lightweight XML parser. Supports a subset of XML features: elements,
/// attributes, text, predefined entities, CDATA, mixed content. Namespaces are
/// parsed as part of the element or attribute name. Prologs and doctypes are
/// ignored. Only 8-bit character encodings are supported. <br> The default
/// behavior is to parse the XML into a DOM. Extends this class and override
/// methods to perform event driven parsing. When this is done, the parse methods
/// will return null.
/// @author Nathan Sweet
class XmlReader {

    byteptr data;
    uintz data_size = 0, eof = 0, p, pe, cs, ts, te, act;
    int line = 1, col = 1;
    String attributeName = null;
    bool hasBody = false;

    /* EOF char used to flush out that last token. This should be a whitespace
    * token. */

    uint LAST_CHAR = 0;

    String bufString(byteptr buf, uint size, bool takeOwnership){
        retval := String(buf, size, takeOwnership);
#~         cout `bufString=0x$(radix(uintz(retval.buffer), 16)), retval='$retval'\n`;
        return retval;
    }

    class Element {
        String _name;
        HashMap[String, String] _attributes;
        Array[Element] _children;
        String _text;
        Element _parent;

        oper init(String name, Element parent): _name=name, _parent=parent{
            
        }

        String getName() {
            return _name;
        }

        HashMap[String, String] getAttributes () {
            return _attributes;
        }

        void toString(Formatter fmt, String indent) {
            fmt `$indent<$_name`;
            if (_attributes) {
                for (entry :in _attributes)
                    fmt ` $(entry.key)="$(entry.val)"`;
            }

            if (!_children && (!_text || _text.size == 0))
                fmt.write("/>");
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
            if (!_children) _children = Array[Element](8);
            _children.append(element);
        }

        String getText() {
            return _text;
        }

        void setText(String text) {
            this._text = text;
        }

        void setAttribute(String name, String value) {
            if (!_attributes) _attributes = HashMap[String, String]();
            _attributes[name] = value;
        }

        void set(String name, String value) {
            setAttribute(name, value);
        }

// TODO improve index-based element removals
#~         void removeChild(int index) {
#~             if (_children != null) _children.removeIndex(index);
#~         }
#~ 
#~         void removeChild(Element child) {
#~             if (_children != null) _children.removeValue(child, true);
#~         }
#~ 
#~         void remove() {
#~             _parent.removeChild(this);
#~         }

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

    Array[Element] _elements = {};
    Element __root, __current;
    StringFormatter __textBuffer = {64};

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
#~         print_elements();
        if (true) {
            Element child = {name, __current};
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
        __root = _elements.pop();
        __current = _elements.count() > 0 ? _elements[-1] : null;
    }

    %%{
    machine xml;

        action buffer { s = p; }
        action elementStart {
            byte c = data[s];
            if (c == b'?' || c == b'!') {
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
                    while (data[p - 2] != b']' || data[p - 1] != b']' || data[p] != b'>')
                        p++;
                    _text(bufString(data+s, p - s - 2, false));
                } else if (pe - p > 4 &&
                    data[s + 1] == b'-' && 
                    data[s + 2] == b'-' &&
                    data[s + 3] != b'>'
                ) {
                    while (p<pe) {
                        if (data[++p] == b'>' && data[p-1] == b'-' && data[p-2] == b'-')
                            break;
                    }
                    if (p==pe)
                        throw ParseError(FStr() `Unmatched comment open element near $(bufString(data+s, min(80,p-s), false))`);
                }
                else {
                    while (data[p] != b'>')
                        p++;
                }
                fgoto elementBody;
            }
            hasBody = true;
            _open(bufString(data+s, p - s, false));
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
            attributeName = bufString(data+s, p - s, false);
        }
        action attribute {
            _setAttribute(attributeName, bufString(data+s, p - s, false));
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
                        __textBuffer.write(bufString(data+s, entityStart - s - 1, false));
                        String name = bufString(data+entityStart, __ci - entityStart - 1, false);
                        String value = _entity(name);
                        __textBuffer.write(value ? value : name);
                        s = __ci;
                        entityFound = true;
                        break;
                    }
                }
                if (entityFound) {
                    if (s < end) __textBuffer.write(bufString(data+s, end - s, false));
                    _text(__textBuffer.string());
                    __textBuffer = StringFormatter(64);
                } else
                    _text(bufString(data+s, end - s, false));
            }
        }

        attribute = ^(space | [/>=])+ >buffer %attributeName space* '=' space*
            (('\'' ^'\''* >buffer %attribute '\'') | ('"' ^'"'* >buffer %attribute '"'));
        element = '<' space* ^(space | [/>])+ >buffer %elementStart (space+ attribute)*
            :>> (space* ('/' %elementEndSingle)? space* '>' @element);
        elementBody := space* <: ((^'<'+ >buffer %text) <: space*)?
            element? :>> ('<' space* '/' ^'>'+ '>' @elementEnd);
        main := space* element space*;

    }%%

    %% write data;

    void reset(){
        p = 0;

        %% write init;

    }

    oper init () { }

    Element _parse() {    // Do the first read. 
        if (data is null)
            InvalidResourceError(FStr() `Error parsing XML, null data pointer supplied`);
        uint s;
        pe = data_size;

        // ------ Start exec ---------------------------------------------------------
        %% write exec;
        // ------ End exec -----------------------------------------------------------

        /* Check if we failed. */
        if ( cs == xml_error ) {
            /* Machine failed before finding a token. */
            throw ParseError("PARSE ERROR");
        }

        if (p < pe) {
            uint lineNumber = 1;
            for (uint i = 0; i < p; i++)
                if (data[i] == b'\n') lineNumber++;
            throw InvalidResourceError(FStr() `Error parsing XML on line $lineNumber near: $(bufString(data+p, uint(min(32, pe - p)), false ))`);
        } else if (_elements.count() != 0) {
            Element element = _elements.pop();
            _elements.deleteAll();
            throw InvalidResourceError(FStr() `Error parsing XML, unclosed element: $(element.getName())`);
        }
        Element __root = this.__root;
        this.__root = null;
        return __root;
    }

    Element parse(String xml) {
        reset();
        data = xml.buffer;
        data_size = xml.size;
        return _parse();
    }
    
    Element parseFile(String fname) {
        reset();
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

            data = byteptr(mmap(null, statInfo.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
            data_size = statInfo.st_size;
            if (uintz(data) != uintz(0)-1){
                Element retval = _parse();
                munmap(data, statInfo.st_size);
                return retval;
            }
            else
                throw InvalidResourceError(FStr() `$fname: $(strerror())`);
        }
        return null;
    }
    
}
