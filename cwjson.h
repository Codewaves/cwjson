/*
   Copyright (c) 2012 Sergej Kravcenko
 
   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#ifndef __CWJSON_H__
#define __CWJSON_H__

#include <stdio.h>
#include <string.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <limits>
#include <memory>
#include <math.h>

namespace cwjson {

enum ValueType
{
   TypeRoot,
   TypeObject,
   TypeArray,
   TypeString,
   TypeNumber,
   TypeBoolean,
   TypeNull
};

class JsonError : public std::runtime_error
{
public:
   JsonError(const std::string &err) : std::runtime_error(err) {}
};

class JsonNull : public JsonError
{
public:
   JsonNull(const std::string &err) : JsonError(err) {}
};

class Object;
class Root;
class Array;
class Value;
class Number;
class String;
class Boolean;
class Null;

class Visitor
{
public:
   virtual ~Visitor() {}

   virtual bool enter(const Value &value) { return true; }
   virtual bool visit(const Value &value) { return true; }
   virtual bool visit(const String &value) { return true; }
   virtual bool visit(const Number &value) { return true; }
   virtual bool visit(const Boolean &value) { return true; }
   virtual bool visit(const Null &value) { return true; }
   virtual bool exit(const Value &value) { return true; }
};

class Value
{
   friend class Root;
   friend class Array;
   friend class Object;

public:
   Value() : m_parent(0), m_firstChild(0), m_lastChild(0), m_prev(0), m_next(0), m_length(0) { }
   virtual ~Value() 
   {
      Value *it = m_firstChild;
      while (it)
      {
         Value *next = it->m_next;
         delete it;
         it = next;
      }
   }

protected:
   Value(std::string &name) : m_parent(0), m_firstChild(0), m_lastChild(0), m_prev(0), m_next(0), m_length(0) { m_name.swap(name); }
   void   insertValueInt(Value *value);
   void   insertValueBeforeInt(Value *before, Value *value);
   Value *swapValueInt(Value *value);
   Value *removeValueInt(Value *value);
   void   swapName(std::string &name) { m_name.swap(name); }

public:
   virtual ValueType    getType() const = 0;
   const std::string   &getNameStr() const { return m_name; }
   const char          *getName() const { return m_name.c_str(); }
   void                 setName(std::string &name) { m_name = name; }
   void                 setName(const char *name) { m_name = name; }
   bool                 isNull() const;
   int                  childCount() const { return m_length; }

   const Value   *parent() const { return m_parent; }
   Value         *firstChild() { return m_firstChild; }
   const Value   *firstChild() const { return m_firstChild; }
   Value         *lastChild() { return m_lastChild; }
   const Value   *lastChild() const { return m_lastChild; }
   Value         *nextSibling() { return m_next; }
   const Value   *nextSibling() const { return m_next; }
   Value         *previousSibling() { return m_prev; }
   const Value   *previousSibling() const { return m_prev; }

   virtual Boolean &toBoolean() { throw JsonNull("value is not a boolean"); }
   virtual Number  &toNumber()  { throw JsonNull("value is not a number"); }
   virtual String  &toString()  { throw JsonNull("value is not a string"); }
   virtual Array   &toArray()   { throw JsonNull("value is not an array"); }
   virtual Object  &toObject()  { throw JsonNull("value is not an object"); }

   virtual const Boolean &toBoolean() const { throw JsonNull("value is not a boolean"); }
   virtual const Number  &toNumber() const  { throw JsonNull("value is not a number"); }
   virtual const String  &toString() const  { throw JsonNull("value is not a string"); }
   virtual const Array   &toArray() const   { throw JsonNull("value is not an array"); }
   virtual const Object  &toObject() const  { throw JsonNull("value is not an object"); }

   virtual bool   traverse(Visitor &visitor) const = 0;
   virtual Value *clone() const = 0;

private:
   Value(const Value &);
   void operator=(const Value &);

protected:
   Value *m_parent;
   Value *m_firstChild;
   Value *m_lastChild;
   Value *m_prev;
   Value *m_next;
   int    m_length;

   std::string m_name;
};

class Number : public Value
{
   friend class Root;

public:
   Number(double value) : m_value(value) {}
   virtual ~Number() {}

   ValueType      getType() const { return TypeNumber; }
   double         getValue() const { return m_value; }
   void           setValue(double value) { m_value = value; } 
   Number        &toNumber() { return *this; }
   const Number  &toNumber() const { return *this; }

   bool    traverse(Visitor &visitor) const { return visitor.visit(*this); }
   Number *clone() const { return new Number(m_value); }

private:
   Number(std::string &name, double value) : Value(name), m_value(value) {}

private:
   double m_value;
};

class String : public Value
{
   friend class Root;

public:
   String(const std::string &value) : m_value(value) { }
   String(const char *value) : m_value(value) { }

   virtual ~String() {}

   ValueType          getType() const { return TypeString; }
   const std::string &getValueStr() const { return m_value; }
   const char        *getValue() const { return m_value.c_str(); }
   void               setValue(const std::string &value) { m_value = value; }
   void               setValue(const char *value) { m_value = value; } 
   String            &toString() { return *this; }
   const String      &toString() const { return *this; }

   bool    traverse(Visitor &visitor) const { return visitor.visit(*this); }
   String *clone() const { return new String(m_value); }

private:
   String(std::string &name, std::string &value) : Value(name) { m_value.swap(value); }

private:
   std::string m_value;
};

class Boolean : public Value
{
   friend class Root;

public:
   Boolean(bool value) : m_value(value) {}
   virtual ~Boolean() {}

   ValueType      getType() const { return TypeBoolean; }
   bool           getValue() const { return m_value; }
   void           setValue(bool value) { m_value = value; }
   Boolean       &toBoolean() { return *this; }
   const Boolean &toBoolean() const { return *this; }

   bool     traverse(Visitor &visitor) const { return visitor.visit(*this); }
   Boolean *clone() const { return new Boolean(m_value); }

private:
   Boolean(std::string &name, bool value) : Value(name), m_value(value) {}

private:
   bool m_value;
};

class Null : public Value
{
   friend class Root;

public:
   Null() {}
   virtual ~Null() {}

   ValueType getType() const { return TypeNull; }

   bool   traverse(Visitor &visitor) const { return visitor.visit(*this); }
   Null  *clone() const { return new Null(); }

private:
   Null(std::string &name) : Value(name) {}
};

class Object : public Value
{
   friend class Root;

public:
   Object() {}
   virtual ~Object() {}

   ValueType     getType() const { return TypeObject; }
   Object       &toObject() { return *this; }
   const Object &toObject() const { return *this; }
   
   Value         &getValue(const char *name) { return const_cast<Value &>((const_cast<const Object *>(this))->getValue(name)); }
   const Value   &getValue(const char *name) const;
   Object        &getObject(const char *name) { return getValue(name).toObject(); }
   const Object  &getObject(const char *name) const { return getValue(name).toObject(); }
   Array         &getArray(const char *name) { return getValue(name).toArray(); }
   const Array   &getArray(const char *name) const { return getValue(name).toArray(); }
   String        &getString(const char *name) { return getValue(name).toString(); }
   const String  &getString(const char *name) const { return getValue(name).toString(); }
   Number        &getNumber(const char *name) { return getValue(name).toNumber(); }
   const Number  &getNumber(const char *name) const { return getValue(name).toNumber(); }
   Boolean       &getBoolean(const char *name) { return getValue(name).toBoolean(); }
   const Boolean &getBoolean(const char *name) const { return getValue(name).toBoolean(); }

   Value         &getValue(const std::string &name) { return getValue(name.c_str()); }
   const Value   &getValue(const std::string &name) const { return getValue(name.c_str()); }
   Object        &getObject(const std::string &name) { return getValue(name).toObject(); }
   const Object  &getObject(const std::string &name) const { return getValue(name).toObject(); }
   Array         &getArray(const std::string &name) { return getValue(name).toArray(); }
   const Array   &getArray(const std::string &name) const { return getValue(name).toArray(); }
   String        &getString(const std::string &name) { return getValue(name).toString(); }
   const String  &getString(const std::string &name) const { return getValue(name).toString(); }
   Number        &getNumber(const std::string &name) { return getValue(name).toNumber(); }
   const Number  &getNumber(const std::string &name) const { return getValue(name).toNumber(); }
   Boolean       &getBoolean(const std::string &name) { return getValue(name).toBoolean(); }
   const Boolean &getBoolean(const std::string &name) const { return getValue(name).toBoolean(); }
   
   bool           isNull(const char *name) const { return getValue(name).isNull(); }
   bool           isNull(const std::string &name) const { return isNull(name.c_str()); }

   Value         *linkValue(const char *name, Value *value);
   Value         *linkValue(const std::string &name, Value *value) { return linkValue(name.c_str(), value); }

   Value         &setValue(const char *name, Value &value);
   Value         &setValue(const std::string &name, Value &value) { return setValue(name.c_str(), value); }
   void           setNumber(const char *name, double value) { linkValueSafe(name, new Number(value)); }
   void           setNumber(const std::string &name, double value) { setNumber(name.c_str(), value); }
   void           setString(const char *name, const char *value) { linkValueSafe(name, new String(value)); }
   void           setString(const std::string &name, const char *value) { setString(name.c_str(), value); }
   void           setString(const char *name, const std::string &value) { linkValueSafe(name, new String(value)); }
   void           setString(const std::string &name, const std::string &value) { setString(name.c_str(), value); }
   void           setBoolean(const char *name, bool value) { linkValueSafe(name, new Boolean(value)); }
   void           setBoolean(const std::string &name, bool value) { setBoolean(name.c_str(), value); }
   void           setNull(const char *name) { linkValueSafe(name, new Null()); }
   void           setNull(const std::string &name) { setNull(name.c_str()); }
   
   Object        &createObject(const char *name);
   Object        &createObject(const std::string &name) { return createObject(name.c_str()); }
   Array         &createArray(const char *name);
   Array         &createArray(const std::string &name) { return createArray(name.c_str()); }

   void           removeValue(const char *name);
   void           removeValue(const std::string &name) { removeValue(name.c_str()); }

   bool traverse(Visitor &visitor) const
   {
      if (visitor.enter(*this))
      {
         Value *it = m_firstChild;
         while (it)
         {
            if (!it->traverse(visitor))
               break;
            it = it->m_next;
         }
      }

      return visitor.exit(*this);
   }
   Object *clone() const;

private:
   Object(std::string &name) : Value(name) {}
   void linkValueSafe(const char *name, Value *value);

private:
};

class Array : public Value
{
   friend class Root;

public:
   Array() {}
   virtual ~Array() {}

   ValueType    getType() const { return TypeArray; }
   Array       &toArray() { return *this; }
   const Array &toArray() const { return *this; }

   Value         &getValue(int idx) { return const_cast<Value &>((const_cast<const Array *>(this))->getValue(idx)); }
   const Value   &getValue(int idx) const;
   Object        &getObject(int idx) { return getValue(idx).toObject(); }
   const Object  &getObject(int idx) const { return getValue(idx).toObject(); }
   Array         &getArray(int idx) { return getValue(idx).toArray(); }
   const Array   &getArray(int idx) const { return getValue(idx).toArray(); }
   String        &getString(int idx) { return getValue(idx).toString(); }
   const String  &getString(int idx) const { return getValue(idx).toString(); }
   Number        &getNumber(int idx) { return getValue(idx).toNumber(); }
   const Number  &getNumber(int idx) const { return getValue(idx).toNumber(); }
   Boolean       &getBoolean(int idx) { return getValue(idx).toBoolean(); }
   const Boolean &getBoolean(int idx) const { return getValue(idx).toBoolean(); }

   bool           isNull(int idx) const { return getValue(idx).isNull(); }

   Value         *linkValueBack(Value *value) { return linkValueInt(value, 0, 0); }
   Value         *linkValueBefore(Value *value, int position) { return linkValueInt(value, position, 1); }
   Value         *linkValueAt(Value *value, int position) { return linkValueInt(value, position, 2); }

   Value         &pushValue(Value &value) { Value *newv = value.clone(); linkValueSafe(newv, 0, 0); return *newv; }
   Object        &pushNewObject() { Object *newo = new Object(); linkValueSafe(newo, 0, 0); return *newo; }
   Array         &pushNewArray() { Array *newa = new Array(); linkValueSafe(newa, 0, 0); return *newa; }
   void           pushString(const char *value) { linkValueSafe(new String(value), 0, 0); }
   void           pushString(const std::string &value) { linkValueSafe(new String(value), 0, 0); }
   void           pushNumber(double value) { linkValueSafe(new Number(value), 0, 0); }
   void           pushBoolean(bool value) { linkValueSafe(new Boolean(value), 0, 0); }
   void           pushNull(bool value) { linkValueSafe(new Null(), 0, 0); }

   Value         &insertValue(int before, Value &value) { Value *newv = value.clone(); linkValueSafe(newv, before, 1); return *newv; }
   Object        &insertNewObject(int before) { Object *newo = new Object(); linkValueSafe(newo, before, 1); return *newo; }
   Array         &insertNewArray(int before) { Array *newa = new Array(); linkValueSafe(newa, before, 1); return *newa; }
   void           insertString(int before, const char *value) { linkValueSafe(new String(value), before, 1); }
   void           insertString(int before, const std::string &value) { linkValueSafe(new String(value), before, 1); }
   void           insertNumber(int before, double value) { linkValueSafe(new Number(value), before, 1); }
   void           insertBoolean(int before, bool value) { linkValueSafe(new Boolean(value), before, 1); }
   void           insertNull(int before, bool value) { linkValueSafe(new Null(), before, 1); }

   Value         &setValue(int position, Value &value) { Value *newv = value.clone(); linkValueSafe(newv, position, 2); return *newv; }
   Object        &setNewObject(int position) { Object *newo = new Object(); linkValueSafe(newo, position, 2); return *newo; }
   Array         &setNewArray(int position) { Array *newa = new Array(); linkValueSafe(newa, position, 2); return *newa; }
   void           setString(int position, const char *value) { linkValueSafe(new String(value), position, 2); }
   void           setString(int position, const std::string &value) { linkValueSafe(new String(value), position, 2); }
   void           setNumber(int position, double value) { linkValueSafe(new Number(value), position, 2); }
   void           setBoolean(int position, bool value) { linkValueSafe(new Boolean(value), position, 2); }
   void           setNull(int position, bool value) { linkValueSafe(new Null(), position, 2); }

   void           removeValue(int position);

   bool traverse(Visitor &visitor) const
   {
      if (visitor.enter(*this))
      {
         Value *it = m_firstChild;
         while (it)
         {
            if (!it->traverse(visitor))
               break;
            it = it->m_next;
         }
      }

      return visitor.exit(*this);
   }
   Array *clone() const;

private:
   Array(std::string &name) : Value(name) {}
   Value *linkValueInt(Value *value, int position, int where);
   Value *linkValueSafe(Value *value, int position, int where);

private:
};

class Printer : public Visitor
{
public:
   Printer(std::ostream &out) : m_out(out), m_depth(0), m_format(false) {}

   bool enter(const Value &value);
   bool visit(const Boolean &value);
   bool visit(const String &value);
   bool visit(const Number &value);
   bool visit(const Null &value);
   bool exit(const Value &value);

   void setFormating(bool format, const char *tab, const char *lineBreak)
   {
      m_format    = format;
      m_lineBreak = lineBreak;
      m_tab       = tab;
   }

private:
   void printEscapedString(const std::string &value);
   void printName(const Value &value)
   {
      if (value.parent() && value.parent()->getType() == TypeObject)
      {
         printEscapedString(value.getNameStr());
         if (m_format)
            m_out << " : ";
         else
            m_out << ":";
      }
   }

   void printIndent()
   {
      if (m_format)
      {
         for (int i = 0; i < m_depth; ++i)
            m_out << m_tab;
      }
   }

   void printLineBreak()
   {
      if (m_format)
         m_out << m_lineBreak;
   }

   void printSeparator(const Value &value)
   {
      if (value.previousSibling())
      {
         m_out << ',';
         printLineBreak();
      }
      printIndent();
   }

private:
   std::ostream &m_out;
   int           m_depth;
   bool          m_format;
   std::string   m_tab;
   std::string   m_lineBreak;
};

class Root : public Value
{
public:
   Root() {}
   Root(const char *json) { parse(json); }
   Root(std::string &json) { parse(json.c_str()); }

   virtual ~Root() {}

   ValueType    getType() const { return TypeRoot; }

   Array        &getArray() { return const_cast<Array &>((const_cast<const Root *>(this))->getArray()); }
   Object       &getObject() { return const_cast<Object &>((const_cast<const Root *>(this))->getObject()); }
   const Array  &getArray() const;
   const Object &getObject() const;

   Value        *linkValue(Value *value);
   Value        &setValue(Value &value);
   Object       &createRootObject() { Object *newo = new Object(); linkValue(newo);  return *newo; }
   Array        &createRootArray() { Array *newa = new Array(); linkValue(newa);  return *newa; }

   Root         *clone() const;

   void parse(const char *json);
   void parse(std::string &json) { parse(json.c_str()); }
   bool traverse(Visitor &visitor) const
   {
      if (m_firstChild)
         m_firstChild->traverse(visitor);
      return true;
   }

   void print(std::ostream &out, bool format = false)
   {
      Printer printer(out);
      printer.setFormating(format, "   ", "\n");

      traverse(printer);
   }

private:
   const char *whitespace(const char *ptr) const
   {
      while (*ptr == 0x20 || *ptr == 0x09 || *ptr == 0x0A || *ptr == 0x0D) 
         ++ptr; 
      return ptr; 
   }

   const char *skip(const char *ptr, size_t count) const
   {
      return ptr + count;
   }

   bool isDigit(char digit) const
   {
      if (digit >= '0' && digit <= '9')
         return true;
      return false;
   }

   const char *parse_value(Value *parent, std::string &name, const char *ptr);
   const char *parse_number(double &value, const char *ptr);
   const char *parse_string(std::string &value, const char *ptr);
   const char *parse_unicode(int &value, const char *ptr);
};

}

#endif