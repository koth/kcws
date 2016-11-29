// -*- mode: c++; c-basic-offset: 4; -*-

// Author: Hong Jiang <hong@hjiang.net>
// Contributors:
//   Sean Middleditch <sean@middleditch.us>
//   rlyeh <https://github.com/r-lyeh>

#include "jsonxx.h"

#include <cctype>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <limits>

// Snippet that creates an assertion function that works both in DEBUG & RELEASE mode.
// JSONXX_ASSERT(...) macro will redirect to this. assert() macro is kept untouched.
#if defined(NDEBUG) || defined(_NDEBUG)
#   define JSONXX_REENABLE_NDEBUG
#   undef  NDEBUG
#   undef _NDEBUG
#endif
#include <stdio.h>
#include <cassert>
void jsonxx::assertion( const char *file, int line, const char *expression, bool result ) {
    if( !result ) {
        fprintf( stderr, "[JSONXX] expression '%s' failed at %s:%d -> ", expression, file, line );
        assert( 0 );
    }
}
#if defined(JSONXX_REENABLE_NDEBUG)
#   define  NDEBUG
#   define _NDEBUG
#endif
#include <cassert>

namespace jsonxx {

//static_assert( sizeof(unsigned long long) < sizeof(long double), "'long double' cannot hold 64bit values in this compiler :(");

bool match(const char* pattern, std::istream& input);
bool parse_array(std::istream& input, Array& array);
bool parse_bool(std::istream& input, Boolean& value);
bool parse_comment(std::istream &input);
bool parse_null(std::istream& input);
bool parse_number(std::istream& input, Number& value);
bool parse_object(std::istream& input, Object& object);
bool parse_string(std::istream& input, String& value);
bool parse_identifier(std::istream& input, String& value);
bool parse_value(std::istream& input, Value& value);

// Try to consume characters from the input stream and match the
// pattern string.
bool match(const char* pattern, std::istream& input) {
    input >> std::ws;
    const char* cur(pattern);
    char ch(0);
    while(input && !input.eof() && *cur != 0) {
        input.get(ch);
        if (ch != *cur) {
            input.putback(ch);
            if( parse_comment(input) )
                continue;
            while (cur > pattern) {
                cur--;
                input.putback(*cur);
            }
            return false;
        } else {
            cur++;
        }
    }
    return *cur == 0;
}

bool parse_string(std::istream& input, String& value) {
    char ch = '\0', delimiter = '"';
    if (!match("\"", input))  {
        if (Parser == Strict) {
            return false;
        }
        delimiter = '\'';
        if (input.peek() != delimiter) {
            return false;
        }
        input.get(ch);
    }
    while(!input.eof() && input.good()) {
        input.get(ch);
        if (ch == delimiter) {
            break;
        }
        if (ch == '\\') {
            input.get(ch);
            switch(ch) {
                case '\\':
                case '/':
                    value.push_back(ch);
                    break;
                case 'b':
                    value.push_back('\b');
                    break;
                case 'f':
                    value.push_back('\f');
                    break;
                case 'n':
                    value.push_back('\n');
                    break;
                case 'r':
                    value.push_back('\r');
                    break;
                case 't':
                    value.push_back('\t');
                    break;
                case 'u': {
                        int i;
                        std::stringstream ss;
                        for( i = 0; (!input.eof() && input.good()) && i < 4; ++i ) {
                            input.get(ch);
                            ss << std::hex << ch;
                        }
                        if( input.good() && (ss >> i) )
                            value.push_back(i);
                    }
                    break;
                default:
                    if (ch != delimiter) {
                        value.push_back('\\');
                        value.push_back(ch);
                    } else value.push_back(ch);
                    break;
            }
        } else {
            value.push_back(ch);
        }
    }
    if (input && ch == delimiter) {
        return true;
    } else {
        return false;
    }
}

bool parse_identifier(std::istream& input, String& value) {
    input >> std::ws;

    char ch = '\0', delimiter = ':';
    bool first = true;

    while(!input.eof() && input.good()) {
        input.get(ch);

        if (ch == delimiter) {
            input.unget();
            break;
        }

        if(first) {
            if ((ch != '_' && ch != '$') &&
                    (ch < 'a' || ch > 'z') &&
                    (ch < 'A' || ch > 'Z')) {
                return false;
            }
            first = false;
        }
        if(ch == '_' || ch == '$' ||
            (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9')) {
            value.push_back(ch);            
        }
        else if(ch == '\t' || ch == ' ') {
            input >> std::ws;
        }
    }
    if (input && ch == delimiter) {
        return true;
    } else {
        return false;
    }
}

bool parse_number(std::istream& input, Number& value) {
    input >> std::ws;
    std::streampos rollback = input.tellg();
    input >> value;
    if (input.fail()) {
        input.clear();
        input.seekg(rollback);
        return false;
    }
    return true;
}

bool parse_bool(std::istream& input, Boolean& value) {
    if (match("true", input))  {
        value = true;
        return true;
    }
    if (match("false", input)) {
        value = false;
        return true;
    }
    return false;
}

bool parse_null(std::istream& input) {
    if (match("null", input))  {
        return true;
    }
    if (Parser == Strict) {
        return false;
    }
    return (input.peek()==',');
}

bool parse_array(std::istream& input, Array& array) {
    return array.parse(input);
}

bool parse_object(std::istream& input, Object& object) {
    return object.parse(input);
}

bool parse_comment(std::istream &input) {
    if( Parser == Permissive )
    if( !input.eof() && input.peek() == '/' )
    {
        char ch0(0);
        input.get(ch0);

        if( !input.eof() )
        {
            char ch1(0);
            input.get(ch1);

            if( ch0 == '/' && ch1 == '/' )
            {
                // trim chars till \r or \n
                for( char ch(0); !input.eof() && (input.peek() != '\r' && input.peek() != '\n'); )
                    input.get(ch);

                // consume spaces, tabs, \r or \n, in case no eof is found
                if( !input.eof() )
                    input >> std::ws;
                return true;
            }

            input.unget();
            input.clear();
        }

        input.unget();
        input.clear();
    }

    return false;
}

bool parse_value(std::istream& input, Value& value) {
    return value.parse(input);
}



template<>
 bool Value::is<Value>() const {
    return true;
}

template<>
bool Value::is<Null>() const {
  return type_ == NULL_;
}

template<>
 bool Value::is<Boolean>() const {
  return type_ == BOOL_;
}

template<>
 bool Value::is<String>() const {
  return type_ == STRING_;
}

template<>
 bool Value::is<Number>() const {
  return type_ == NUMBER_;
}

template<>
 bool Value::is<Array>() const {
  return type_ == ARRAY_;
}

template<>
 bool Value::is<Object>() const {
  return type_ == OBJECT_;
}
    
template<>
 Value& Value::get<Value>() {
    return *this;
}
    
template<>
 const Value& Value::get<Value>() const {
    return *this;
}

template<>
 bool& Value::get<Boolean>() {
  JSONXX_ASSERT(is<Boolean>());
  return bool_value_;
}

template<>
 std::string& Value::get<String>() {
  JSONXX_ASSERT(is<String>());
  return *string_value_;
}

template<>
 Number& Value::get<Number>() {
  JSONXX_ASSERT(is<Number>());
  return number_value_;
}

template<>
 Array& Value::get<Array>() {
  JSONXX_ASSERT(is<Array>());
  return *array_value_;
}

template<>
 Object& Value::get<Object>() {
  JSONXX_ASSERT(is<Object>());
  return *object_value_;
}

template<>
 const Boolean& Value::get<Boolean>() const {
  JSONXX_ASSERT(is<Boolean>());
  return bool_value_;
}

template<>
 const String& Value::get<String>() const {
  JSONXX_ASSERT(is<String>());
  return *string_value_;
}

template<>
 const Number& Value::get<Number>() const {
  JSONXX_ASSERT(is<Number>());
  return number_value_;
}

template<>
 const Array& Value::get<Array>() const {
  JSONXX_ASSERT(is<Array>());
  return *array_value_;
}

template<>
 const Object& Value::get<Object>() const {
  JSONXX_ASSERT(is<Object>());
  return *object_value_;
}


Object::Object() : value_map_() {}

Object::~Object() {
    reset();
}

bool Object::parse(std::istream& input, Object& object) {
    object.reset();

    if (!match("{", input)) {
        return false;
    }
    if (match("}", input)) {
        return true;
    }

    do {
        std::string key;
        if(UnquotedKeys == Enabled) {
            if (!parse_identifier(input, key)) {
                if (Parser == Permissive) {
                    if (input.peek() == '}')
                        break;
                }
                return false;
            }
        }
        else {
            if (!parse_string(input, key)) {
                if (Parser == Permissive) {
                    if (input.peek() == '}')
                        break;
                }
                return false;
            }
        }
        if (!match(":", input)) {
            return false;
        }
        Value* v = new Value();
        if (!parse_value(input, *v)) {
            delete v;
            break;
        }
        object.value_map_[key] = v;
    } while (match(",", input));


    if (!match("}", input)) {
        return false;
    }

    return true;
}

Value::Value() : type_(INVALID_) {}

void Value::reset() {
    if (type_ == STRING_) {
        delete string_value_;
        string_value_ = 0;
    }
    else if (type_ == OBJECT_) {
        delete object_value_;
        object_value_ = 0;
    }
    else if (type_ == ARRAY_) {
        delete array_value_;
        array_value_ = 0;
    }
}

bool Value::parse(std::istream& input, Value& value) {
    value.reset();

    std::string string_value;
    if (parse_string(input, string_value)) {
        value.string_value_ = new std::string();
        value.string_value_->swap(string_value);
        value.type_ = STRING_;
        return true;
    }
    if (parse_number(input, value.number_value_)) {
        value.type_ = NUMBER_;
        return true;
    }

    if (parse_bool(input, value.bool_value_)) {
        value.type_ = BOOL_;
        return true;
    }
    if (parse_null(input)) {
        value.type_ = NULL_;
        return true;
    }
    if (input.peek() == '[') {
        value.array_value_ = new Array();
        if (parse_array(input, *value.array_value_)) {
            value.type_ = ARRAY_;
            return true;
        }
        delete value.array_value_;
    }
    value.object_value_ = new Object();
    if (parse_object(input, *value.object_value_)) {
        value.type_ = OBJECT_;
        return true;
    }
    delete value.object_value_;
    return false;
}

Array::Array() : values_() {}

Array::~Array() {
    reset();
}

bool Array::parse(std::istream& input, Array& array) {
    array.reset();

    if (!match("[", input)) {
        return false;
    }
    if (match("]", input)) {
        return true;
    }

    do {
        Value* v = new Value();
        if (!parse_value(input, *v)) {
            delete v;
            break;
        }
        array.values_.push_back(v);
    } while (match(",", input));

    if (!match("]", input)) {
        return false;
    }
    return true;
}

static std::ostream& stream_string(std::ostream& stream,
                                   const std::string& string) {
    stream << '"';
    for (std::string::const_iterator i = string.begin(),
                 e = string.end(); i != e; ++i) {
        switch (*i) {
            case '"':
                stream << "\\\"";
                break;
            case '\\':
                stream << "\\\\";
                break;
            case '/':
                stream << "\\/";
                break;
            case '\b':
                stream << "\\b";
                break;
            case '\f':
                stream << "\\f";
                break;
            case '\n':
                stream << "\\n";
                break;
            case '\r':
                stream << "\\r";
                break;
            case '\t':
                stream << "\\t";
                break;
            default:
                if (*i < 32) {
                    stream << "\\u" << std::hex << std::setw(4) <<
                            std::setfill('0') << static_cast<int>(*i) << std::dec <<
                            std::setw(0);
                } else {
                    stream << *i;
                }
        }
    }
    stream << '"';
    return stream;
}

}  // namespace jsonxx

std::ostream& operator<<(std::ostream& stream, const jsonxx::Value& v) {
    using namespace jsonxx;
    if (v.is<Number>()) {
        return stream << v.get<Number>();
    } else if (v.is<String>()) {
        return stream_string(stream, v.get<std::string>());
    } else if (v.is<Boolean>()) {
        if (v.get<Boolean>()) {
            return stream << "true";
        } else {
            return stream << "false";
        }
    } else if (v.is<Null>()) {
        return stream << "null";
    } else if (v.is<Object>()) {
        return stream << v.get<Object>();
    } else if (v.is<Array>()){
        return stream << v.get<Array>();
    }
    // Shouldn't reach here.
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const jsonxx::Array& v) {
    stream << "[";
    jsonxx::Array::container::const_iterator
        it = v.values().begin(),
        end = v.values().end();
    while (it != end) {
        stream << *(*it);
        ++it;
        if (it != end) {
            stream << ", ";
        }
    }
    return stream << "]";
}

std::ostream& operator<<(std::ostream& stream, const jsonxx::Object& v) {
    stream << "{";
    jsonxx::Object::container::const_iterator
        it = v.kv_map().begin(),
        end = v.kv_map().end();
    while (it != end) {
        jsonxx::stream_string(stream, it->first);
        stream << ": " << *(it->second);
        ++it;
        if (it != end) {
            stream << ", ";
        }
    }
    return stream << "}";
}


namespace jsonxx {
namespace {

typedef unsigned char byte;

//template<bool quote>
std::string escape_string( const std::string &input, const bool quote = false ) {
    static std::string map[256], *once = 0;
    if( !once ) {
        // base
        for( int i = 0; i < 256; ++i ) {
            map[ i ] = std::string() + char(i);
        }
        // non-printable
        for( int i = 0; i < 32; ++i ) {
            std::stringstream str;
            str << "\\u" << std::hex << std::setw(4) << std::setfill('0') << i;
            map[ i ] = str.str();
        }
        // exceptions
        map[ byte('"') ] = "\\\"";
        map[ byte('\\') ] = "\\\\";
        map[ byte('/') ] = "\\/";
        map[ byte('\b') ] = "\\b";
        map[ byte('\f') ] = "\\f";
        map[ byte('\n') ] = "\\n";
        map[ byte('\r') ] = "\\r";
        map[ byte('\t') ] = "\\t";

        once = map;
    }
    std::string output;
    output.reserve( input.size() * 2 + 2 ); // worst scenario
    if( quote ) output += '"';
    for( std::string::const_iterator it = input.begin(), end = input.end(); it != end; ++it )
        output += map[ byte(*it) ];
    if( quote ) output += '"';
    return output;
}


namespace json {

    std::string remove_last_comma( const std::string &_input ) {
        std::string input( _input );
        size_t size = input.size();
        if( size > 2 )
            if( input[ size - 2 ] == ',' )
                input[ size - 2 ] = ' ';
        return input;
    }

    std::string tag( unsigned format, unsigned depth, const std::string &name, const jsonxx::Value &t) {
        std::stringstream ss;
        const std::string tab(depth, '\t');

        if( !name.empty() )
            ss << tab << '\"' << escape_string( name ) << '\"' << ':' << ' ';
        else
            ss << tab;

        switch( t.type_ )
        {
            default:
            case jsonxx::Value::NULL_:
                ss << "null";
                return ss.str() + ",\n";

            case jsonxx::Value::BOOL_:
                ss << ( t.bool_value_ ? "true" : "false" );
                return ss.str() + ",\n";

            case jsonxx::Value::ARRAY_:
                ss << "[\n";
                for(Array::container::const_iterator it = t.array_value_->values().begin(),
                    end = t.array_value_->values().end(); it != end; ++it )
                  ss << tag( format, depth+1, std::string(), **it );
                return remove_last_comma( ss.str() ) + tab + "]" ",\n";

            case jsonxx::Value::STRING_:
                ss << '\"' << escape_string( *t.string_value_ ) << '\"';
                return ss.str() + ",\n";

            case jsonxx::Value::OBJECT_:
                ss << "{\n";
                for(Object::container::const_iterator it=t.object_value_->kv_map().begin(),
                    end = t.object_value_->kv_map().end(); it != end ; ++it)
                  ss << tag( format, depth+1, it->first, *it->second );
                return remove_last_comma( ss.str() ) + tab + "}" ",\n";

            case jsonxx::Value::NUMBER_:
                // max precision
                ss << std::setprecision(std::numeric_limits<long double>::digits10 + 1);
                ss << t.number_value_;
                return ss.str() + ",\n";
        }
    }
} // namespace jsonxx::anon::json

namespace xml {

std::string escape_attrib( const std::string &input ) {
    static std::string map[256], *once = 0;
    if( !once ) {
        for( int i = 0; i < 256; ++i )
            map[ i ] = "_";
        for( int i = int('a'); i <= int('z'); ++i )
            map[ i ] = std::string() + char(i);
        for( int i = int('A'); i <= int('Z'); ++i )
            map[ i ] = std::string() + char(i);
        for( int i = int('0'); i <= int('9'); ++i )
            map[ i ] = std::string() + char(i);
        once = map;
    }
    std::string output;
    output.reserve( input.size() ); // worst scenario
    for( std::string::const_iterator it = input.begin(), end = input.end(); it != end; ++it )
        output += map[ byte(*it) ];
    return output;
}

std::string escape_tag( const std::string &input, unsigned format ) {
    static std::string map[256], *once = 0;
    if( !once ) {
        for( int i = 0; i < 256; ++i )
            map[ i ] = std::string() + char(i);
        map[ byte('<') ] = "&lt;";
        map[ byte('>') ] = "&gt;";

        switch( format )
        {
            default:
                break;

            case jsonxx::JXML:
            case jsonxx::JXMLex:
            case jsonxx::JSONx:
            case jsonxx::TaggedXML:
                map[ byte('&') ] = "&amp;";
                break;
        }

        once = map;
    }
    std::string output;
    output.reserve( input.size() * 5 ); // worst scenario
    for( std::string::const_iterator it = input.begin(), end = input.end(); it != end; ++it )
        output += map[ byte(*it) ];
    return output;
}

std::string open_tag( unsigned format, char type, const std::string &name, const std::string &attr = std::string(), const std::string &text = std::string() ) {
    std::string tagname;
    switch( format )
    {
        default:
            return std::string();

        case jsonxx::JXML:
            if( name.empty() )
                tagname = std::string("j son=\"") + type + '\"';
            else
                tagname = std::string("j son=\"") + type + ':' + escape_string(name) + '\"';
            break;

        case jsonxx::JXMLex:
            if( name.empty() )
                tagname = std::string("j son=\"") + type + '\"';
            else
                tagname = std::string("j son=\"") + type + ':' + escape_string(name) + "\" " + escape_attrib(name) + "=\"" + escape_string(text) + "\"";
            break;

        case jsonxx::JSONx:
            if( !name.empty() )
                tagname = std::string(" name=\"") + escape_string(name) + "\"";
            switch( type ) {
                default:
                case '0': tagname = "json:null" + tagname; break;
                case 'b': tagname = "json:boolean" + tagname; break;
                case 'a': tagname = "json:array" + tagname; break;
                case 's': tagname = "json:string" + tagname; break;
                case 'o': tagname = "json:object" + tagname; break;
                case 'n': tagname = "json:number" + tagname; break;
            }
            break;

        case jsonxx::TaggedXML: // @TheMadButcher
            if( !name.empty() )
                tagname = escape_attrib(name);
            else
                tagname = "JsonItem";
            switch( type ) {
                default:
                case '0': tagname += " type=\"json:null\""; break;
                case 'b': tagname += " type=\"json:boolean\""; break;
                case 'a': tagname += " type=\"json:array\""; break;
                case 's': tagname += " type=\"json:string\""; break;
                case 'o': tagname += " type=\"json:object\""; break;
                case 'n': tagname += " type=\"json:number\""; break;
            }

            if( !name.empty() )
                tagname += std::string(" name=\"") + escape_string(name) + "\"";

            break;
    }

    return std::string("<") + tagname + attr + ">";
}

std::string close_tag( unsigned format, char type, const std::string &name ) {
    switch( format )
    {
        default:
            return std::string();

        case jsonxx::JXML:
        case jsonxx::JXMLex:
            return "</j>";

        case jsonxx::JSONx:
            switch( type ) {
                default:
                case '0': return "</json:null>";
                case 'b': return "</json:boolean>";
                case 'a': return "</json:array>";
                case 'o': return "</json:object>";
                case 's': return "</json:string>";
                case 'n': return "</json:number>";
            }
            break;

        case jsonxx::TaggedXML: // @TheMadButcher
            if( !name.empty() )
                return "</"+escape_attrib(name)+">";
            else
                return "</JsonItem>";
    }
}

std::string tag( unsigned format, unsigned depth, const std::string &name, const jsonxx::Value &t, const std::string &attr = std::string() ) {
    std::stringstream ss;
    const std::string tab(depth, '\t');

    switch( t.type_ )
    {
        default:
        case jsonxx::Value::NULL_:
            return tab + open_tag( format, '0', name, " /" ) + '\n';

        case jsonxx::Value::BOOL_:
            ss << ( t.bool_value_ ? "true" : "false" );
            return tab + open_tag( format, 'b', name, std::string(), format == jsonxx::JXMLex ? ss.str() : std::string() )
                       + ss.str()
                       + close_tag( format, 'b', name ) + '\n';

        case jsonxx::Value::ARRAY_:
            for(Array::container::const_iterator it = t.array_value_->values().begin(),
                end = t.array_value_->values().end(); it != end; ++it )
              ss << tag( format, depth+1, std::string(), **it );
            return tab + open_tag( format, 'a', name, attr ) + '\n'
                       + ss.str()
                 + tab + close_tag( format, 'a', name ) + '\n';

        case jsonxx::Value::STRING_:
            ss << escape_tag( *t.string_value_, format );
            return tab + open_tag( format, 's', name, std::string(), format == jsonxx::JXMLex ? ss.str() : std::string() )
                       + ss.str()
                       + close_tag( format, 's', name ) + '\n';

        case jsonxx::Value::OBJECT_:
            for(Object::container::const_iterator it=t.object_value_->kv_map().begin(),
                end = t.object_value_->kv_map().end(); it != end ; ++it)
              ss << tag( format, depth+1, it->first, *it->second );
            return tab + open_tag( format, 'o', name, attr ) + '\n'
                       + ss.str()
                 + tab + close_tag( format, 'o', name ) + '\n';

        case jsonxx::Value::NUMBER_:
            // max precision
            ss << std::setprecision(std::numeric_limits<long double>::digits10 + 1);
            ss << t.number_value_;
            return tab + open_tag( format, 'n', name, std::string(), format == jsonxx::JXMLex ? ss.str() : std::string() )
                       + ss.str()
                       + close_tag( format, 'n', name ) + '\n';
    }
}

// order here matches jsonxx::Format enum
const char *defheader[] = {
    "",

    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
         JSONXX_XML_TAG "\n",

    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
         JSONXX_XML_TAG "\n",

    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
         JSONXX_XML_TAG "\n",

    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
         JSONXX_XML_TAG "\n"
};

// order here matches jsonxx::Format enum
const char *defrootattrib[] = {
    "",

    " xsi:schemaLocation=\"http://www.datapower.com/schemas/json jsonx.xsd\""
        " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
        " xmlns:json=\"http://www.ibm.com/xmlns/prod/2009/jsonx\"",

    "",

    "",

    ""
};

} // namespace jsonxx::anon::xml

} // namespace jsonxx::anon

std::string Object::json() const {
    using namespace json;

    jsonxx::Value v;
    v.object_value_ = const_cast<jsonxx::Object*>(this);
    v.type_ = jsonxx::Value::OBJECT_;

    std::string result = tag( jsonxx::JSON, 0, std::string(), v );

    v.object_value_ = 0;
    return remove_last_comma( result );
}

std::string Object::xml( unsigned format, const std::string &header, const std::string &attrib ) const {
    using namespace xml;
    JSONXX_ASSERT( format == jsonxx::JSONx || format == jsonxx::JXML || format == jsonxx::JXMLex || format == jsonxx::TaggedXML );

    jsonxx::Value v;
    v.object_value_ = const_cast<jsonxx::Object*>(this);
    v.type_ = jsonxx::Value::OBJECT_;

    std::string result = tag( format, 0, std::string(), v, attrib.empty() ? std::string(defrootattrib[format]) : attrib );

    v.object_value_ = 0;
    return ( header.empty() ? std::string(defheader[format]) : header ) + result;
}

std::string Array::json() const {
    using namespace json;

    jsonxx::Value v;
    v.array_value_ = const_cast<jsonxx::Array*>(this);
    v.type_ = jsonxx::Value::ARRAY_;

    std::string result = tag( jsonxx::JSON, 0, std::string(), v );

    v.array_value_ = 0;
    return remove_last_comma( result );
}

std::string Array::xml( unsigned format, const std::string &header, const std::string &attrib ) const {
    using namespace xml;
    JSONXX_ASSERT( format == jsonxx::JSONx || format == jsonxx::JXML || format == jsonxx::JXMLex || format == jsonxx::TaggedXML );

    jsonxx::Value v;
    v.array_value_ = const_cast<jsonxx::Array*>(this);
    v.type_ = jsonxx::Value::ARRAY_;

    std::string result = tag( format, 0, std::string(), v, attrib.empty() ? std::string(defrootattrib[format]) : attrib );

    v.array_value_ = 0;
    return ( header.empty() ? std::string(defheader[format]) : header ) + result;
}

bool validate( std::istream &input ) {

    // trim non-printable chars
    for( char ch(0); !input.eof() && input.peek() <= 32; )
        input.get(ch);

    // validate json
    if( input.peek() == '{' )
    {
        jsonxx::Object o;
        if( parse_object( input, o ) )
            return true;
    }
    else
    if( input.peek() == '[' )
    {
        jsonxx::Array a;
        if( parse_array( input, a ) )
            return true;
    }

    // bad json input
    return false;
}

bool validate( const std::string &input ) {
    std::istringstream is( input );
    return jsonxx::validate( is );
}

std::string reformat( std::istream &input ) {

    // trim non-printable chars
    for( char ch(0); !input.eof() && input.peek() <= 32; )
        input.get(ch);

    // validate json
    if( input.peek() == '{' )
    {
        jsonxx::Object o;
        if( parse_object( input, o ) )
            return o.json();
    }
    else
    if( input.peek() == '[' )
    {
        jsonxx::Array a;
        if( parse_array( input, a ) )
            return a.json();
    }

    // bad json input
    return std::string();
}

std::string reformat( const std::string &input ) {
    std::istringstream is( input );
    return jsonxx::reformat( is );
}

std::string xml( std::istream &input, unsigned format ) {
    using namespace xml;
    JSONXX_ASSERT( format == jsonxx::JSONx || format == jsonxx::JXML || format == jsonxx::JXMLex || format == jsonxx::TaggedXML );

    // trim non-printable chars
    for( char ch(0); !input.eof() && input.peek() <= 32; )
        input.get(ch);

    // validate json, then transform
    if( input.peek() == '{' )
    {
        jsonxx::Object o;
        if( parse_object( input, o ) )
            return o.xml(format);
    }
    else
    if( input.peek() == '[' )
    {
        jsonxx::Array a;
        if( parse_array( input, a ) )
            return a.xml(format);
    }

    // bad json, return empty xml
    return defheader[format];
}

std::string xml( const std::string &input, unsigned format ) {
    std::istringstream is( input );
    return jsonxx::xml( is, format );
}


Object::Object(const Object &other) {
  import(other);
}
Object::Object(const std::string &key, const Value &value) {
  import(key,value);
}
void Object::import( const Object &other ) {
  odd.clear();
  if (this != &other) {
    // default
    container::const_iterator
        it = other.value_map_.begin(),
        end = other.value_map_.end();
    for (/**/ ; it != end ; ++it) {
      container::iterator found = value_map_.find(it->first);
      if( found != value_map_.end() ) {
        delete found->second;
      }
      value_map_[ it->first ] = new Value( *it->second );
    }
  } else {
    // recursion is supported here
    import( Object(*this) );
  }
}
void Object::import( const std::string &key, const Value &value ) {
  odd.clear();
  container::iterator found = value_map_.find(key);
  if( found != value_map_.end() ) {
    delete found->second;
  }
  value_map_[ key ] = new Value( value );
}
Object &Object::operator=(const Object &other) {
  odd.clear();
  if (this != &other) {
    reset();
    import(other);
  }
  return *this;
}
Object &Object::operator<<(const Value &value) {
  if (odd.empty()) {
    odd = value.get<String>();
  } else {
    import( Object(odd, value) );
    odd.clear();
  }
  return *this;
}
Object &Object::operator<<(const Object &value) {
  import( std::string(odd),value);
  odd.clear();
  return *this;
}
size_t Object::size() const {
  return value_map_.size();
}
bool Object::empty() const {
  return value_map_.size() == 0;
}
const std::map<std::string, Value*> &Object::kv_map() const {
  return value_map_;
}
std::string Object::write( unsigned format ) const {
  return format == JSON ? json() : xml(format);
}
void Object::reset() {
  container::iterator i;
  for (i = value_map_.begin(); i != value_map_.end(); ++i) {
    delete i->second;
  }
  value_map_.clear();
}
bool Object::parse(std::istream &input) {
  return parse(input,*this);
}
bool Object::parse(const std::string &input) {
  std::istringstream is( input );
  return parse(is,*this);
}


Array::Array(const Array &other) {
  import(other);
}
Array::Array(const Value &value) {
  import(value);
}
void Array::import(const Array &other) {
  if (this != &other) {
    // default
    container::const_iterator
        it = other.values_.begin(),
        end = other.values_.end();
    for (/**/ ; it != end; ++it) {
      values_.push_back( new Value(**it) );
    }
  } else {
    // recursion is supported here
    import( Array(*this) );
  }
}
void Array::import(const Value &value) {
  values_.push_back( new Value(value) );
}
size_t Array::size() const {
  return values_.size();
}
bool Array::empty() const {
  return values_.size() == 0;
}
void Array::reset() {
  for (container::iterator i = values_.begin(); i != values_.end(); ++i) {
    delete *i;
  }
  values_.clear();
}
bool Array::parse(std::istream &input) {
  return parse(input,*this);
}
bool Array::parse(const std::string &input) {
  std::istringstream is(input);
  return parse(is,*this);
}
Array &Array::operator<<(const Array &other) {
  import(other);
  return *this;
}
Array &Array::operator<<(const Value &value) {
  import(value);
  return *this;
}
Array &Array::operator=(const Array &other) {
  if( this != &other ) {
    reset();
    import(other);
  }
  return *this;
}
Array &Array::operator=(const Value &value) {
  reset();
  import(value);
  return *this;
}

Value::Value(const Value &other) : type_(INVALID_) {
  import( other );
}
bool Value::empty() const {
  if( type_ == INVALID_ ) return true;
  if( type_ == STRING_ && string_value_ == 0 ) return true;
  if( type_ == ARRAY_ && array_value_ == 0 ) return true;
  if( type_ == OBJECT_ && object_value_ == 0 ) return true;
  return false;
}
bool Value::parse(std::istream &input) {
  return parse(input,*this);
}
bool Value::parse(const std::string &input) {
  std::istringstream is( input );
  return parse(is,*this);
}

}  // namespace jsonxx
