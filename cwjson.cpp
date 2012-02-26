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

#include "cwjson.h"

namespace cwjson {


void Value::insertValueInt(Value *value)
{
   if (!m_lastChild)
   {
      m_length = 1;
      m_firstChild = m_lastChild = value;
   }
   else
   {
      m_lastChild->m_next = value;
      value->m_prev       = m_lastChild;
      m_lastChild         = value;
      m_length++;
   }

   value->m_parent = this;
}

void Value::insertValueBeforeInt(Value *before, Value *value)
{
   value->m_parent = this;
   value->m_prev   = before->m_prev;
   value->m_next   = before;
   before->m_prev  = value;

   if (value->m_prev)
      value->m_prev->m_next = value;
   else
      m_firstChild = value;

   m_length++;
}

Value *Value::swapValueInt(Value *value)
{
   value->m_parent = m_parent;
   value->m_next   = m_next;
   value->m_prev   = m_prev;

   if (m_prev)
      m_prev->m_next = value;
   else
      m_parent->m_firstChild = value;

   if (m_next)
      m_next->m_prev = value;
   else
      m_parent->m_lastChild = value;

   return this;
}

Value *Value::removeValueInt(Value *value)
{
   if (value->m_next)
      value->m_next->m_prev = value->m_prev;
   else
      m_lastChild = value->m_prev;

   if (value->m_prev)
      value->m_prev->m_next = value->m_next;
   else
      m_firstChild = value->m_next;

   m_length--;
   return value;
}

bool Value::isNull() const 
{ 
   return getType() == TypeNull; 
}

const Value &Object::getValue(const char *name) const
{
   Value *it = m_firstChild;
   while (it)
   {
      if (it->getNameStr() == name)
         return *it;
      it = it->m_next;
   }

   throw JsonNull(std::string("value not found: ") + name);
}

Value *Object::linkValue(const char *name, Value *value)
{
   if (!value)
      return 0;

   if (value->m_next || value->m_parent || value->m_prev)
      JsonError("value is already linked to JSON object");

   Value *it = m_firstChild;
   while (it)
   {
      if (it->getNameStr() == name)
      {
         value->setName(name);
         delete it->swapValueInt(value);
         return value;
      }
      it = it->m_next;
   }

   value->setName(name);
   insertValueInt(value);
   return value;
}

void Object::linkValueSafe(const char *name, Value *value)
{
   std::auto_ptr<Value> safe(value);
   linkValue(name, value);
   safe.release();
}

Value &Object::setValue(const char *name, Value &value)
{
   Value *newv = value.clone();
   linkValueSafe(name, newv);
   return *newv;
}

void Object::removeValue(const char *name)
{
   Value *it = m_firstChild;
   while (it)
   {
      if (it->getNameStr() == name)
      {
         delete removeValueInt(it);
         return;
      }
      it = it->m_next;
   }
}

Object &Object::createObject(const char *name) 
{
   Object *newo = new Object();
   linkValueSafe(name, newo); 
   return *newo;
}

Array &Object::createArray(const char *name) 
{
   Array *newa = new Array();
   linkValueSafe(name, newa); 
   return *newa;
}

Object *Object::clone() const
{
   std::auto_ptr<Object> ptr(new Object());

   Value *it = m_firstChild;
   while (it)
   {
      Value *copy = it->clone();
      copy->setName(it->m_name);
      ptr->insertValueInt(copy);
      it = it->m_next;
   }

   return ptr.release();
}

const Value &Array::getValue(int idx) const
{
   int    i = 0;
   Value *it = m_firstChild;
   while (it)
   {
      if (i++ >= idx)
         return *it;
      it = it->m_next;
   }

   throw JsonNull(std::string("index out of range"));
}

Value *Array::linkValueInt(Value *value, int position, int where)
{
   if (!value)
      return 0;

   if (value->m_next || value->m_parent || value->m_prev)
      JsonError("value is already linked to JSON object");

   if (0 == where)
      insertValueInt(value);      
   else
   {
      int    i  = 0;
      Value *it = m_firstChild;
      while (it)
      {
         if (i++ >= position)
         {
            if (1 == where)
            {
               insertValueBeforeInt(it, value);
               return value;
            }
            else
            {
               delete it->swapValueInt(value);
               return value;
            }
         }
         it = it->m_next;
      }

      throw JsonNull(std::string("index out of range"));
   }

   return value;
}

Value *Array::linkValueSafe(Value *value, int position, int where)
{
   std::auto_ptr<Value> safe(value);
   linkValueInt(value, position, where);
   return safe.release();
}

void Array::removeValue(int position)
{
   int    i  = 0;
   Value *it = m_firstChild;
   while (it)
   {
      if (i++ >= position)
      {
         delete removeValueInt(it);
         return;
      }
      it = it->m_next;
   }

   throw JsonNull(std::string("index out of range"));
}

Array *Array::clone() const
{
   std::auto_ptr<Array> ptr(new Array());

   Value *it = m_firstChild;
   while (it)
   {
      Value *copy = it->clone();
      ptr->insertValueInt(copy);
      it = it->m_next;
   }

   return ptr.release();
}

Root *Root::clone() const
{
   std::auto_ptr<Root> ptr(new Root());

   if (m_firstChild)
      ptr->linkValue(m_firstChild->clone());

   return ptr.release();
}

void Root::parse(const char *json)
{
   if (!json)
      return;

   if (m_firstChild)
      delete m_firstChild;
   m_firstChild = 0;

   std::string empty;
   parse_value(this, empty, json);
}

const char *Root::parse_value(Value *parent, std::string &name, const char *ptr)
{
   ptr = whitespace(ptr);

   switch (*ptr)
   {
   case '{':
      {
         ptr = skip(ptr, 1);

         Object *object = new Object(name);
         parent->insertValueInt(object);

         ptr = whitespace(ptr);
         if (*ptr == '}')
            return skip(ptr, 1);

         while (1)
         {
            std::string valueName;
            ptr = whitespace(ptr);
            ptr = parse_string(valueName, ptr);

            ptr = whitespace(ptr);
            if (*ptr != ':')
               throw JsonError("expected ':' before object value");
            ptr = skip(ptr, 1);

            ptr = parse_value(object, valueName, ptr);
            ptr = whitespace(ptr);

            if (*ptr == ',')
            {
               ptr = skip(ptr, 1);
               continue;
            }

            if (*ptr == '}')
               break;

            throw JsonError("expected '}' or ',' after object element");
         }

         return skip(ptr, 1);
      }
      break;
   case '[':
      {
         ptr = skip(ptr, 1);

         Array *array = new Array(name);
         parent->insertValueInt(array);

         ptr = whitespace(ptr);
         if (*ptr == ']')
            return skip(ptr, 1);

         while (1)
         {
            std::string empty;

            ptr = parse_value(array, empty, ptr);
            ptr = whitespace(ptr);

            if (*ptr == ',')
            {
               ptr = skip(ptr, 1);
               continue;
            }

            if (*ptr == ']')
               break;
            
            throw JsonError("expected ']' or ',' after array element");
         }

         return skip(ptr, 1);
      }
      break;
   case '\"':
      {
         std::string string;
         ptr = parse_string(string, ptr);
         parent->insertValueInt(new String(name, string));
         return ptr;
      }
      break;
   case 't':
      {
         if (strncmp(ptr, "true", 4) == 0)
         {
            parent->insertValueInt(new Boolean(name, true));
            return skip(ptr, 4);
         }
      }
      break;
   case 'f':
      {
         if (strncmp(ptr, "false", 5) == 0)
         {
            parent->insertValueInt(new Boolean(name, false));
            return skip(ptr, 5);
         }
      }
      break;
   case 'n':
      {
         if (strncmp(ptr, "null", 4) == 0)
         {
            parent->insertValueInt(new Null(name));
            return skip(ptr, 4);
         }
      }
      break;
   case '-':
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
      {
         double number;
         ptr = parse_number(number, ptr);
         parent->insertValueInt(new Number(name, number));
         return ptr;
      }
      break;
   default:
      break;
   }

   throw JsonError("unexpected character");
}

const char *Root::parse_number(double &value, const char *ptr)
{
   double      number = 0;
   double      sign = 1;
   int         frac = 0;
   int         expSign = 1;
   int         exp = 0;

   if (*ptr == '-')
   {
      sign = -1;
      ptr = skip(ptr, 1);
   }

   if (*ptr == '0')
   {
      ptr = skip(ptr, 1);
      if (isDigit(*ptr))
         throw JsonError("leading zeros are not allowed");
   }
   else if (isDigit(*ptr))
   {
      while (isDigit(*ptr))
      {
         number = number * 10.0 + (*ptr - '0');
         ptr = skip(ptr, 1);
      }
   }

   if (*ptr == '.')
   {
      ptr = skip(ptr, 1);
      if (!isDigit(*ptr))
         throw JsonError("expected digit after '.'");

      while (isDigit(*ptr))
      {
         number = number * 10.0 + (*ptr - '0');
         frac  += 1;
         ptr = skip(ptr, 1);
      }
   }

   if (*ptr == 'e' || *ptr == 'E')
   {
      ptr = skip(ptr, 1);
      if (*ptr == '-')
      {
         expSign = -1;
         ptr = skip(ptr, 1);
      } 
      else if (*ptr == '+')
         ptr = skip(ptr, 1);

      if (!isDigit(*ptr))
         throw JsonError("expected digit after 'e' or 'E'");

      while (isDigit(*ptr))
      {
         exp = exp * 10 + (*ptr - '0');
         ptr = skip(ptr, 1);
      }
   }

   number = sign * number;
   exp    = exp * expSign - frac;

   double e10 = 10.0;
   int    e   = exp;
   
   if (e < 0) 
      e = -e;
  
   while (e) 
   {
      if (e & 1) 
      {
         if (exp < 0)
            number /= e10;
         else
            number *= e10;
      }

      e >>= 1;
      e10 *= e10;
   }

   value = number;

   return ptr;
}

const char *Root::parse_string(std::string &value, const char *ptr)
{
   value = "";
   ptr = skip(ptr, 1);

   if (*ptr == '\"')
      return skip(ptr, 1);

   const char *start = ptr; 

   while (*ptr != '\"' && *ptr)
   {
      if (*ptr != '\\')
         ptr = skip(ptr, 1);
      else
      {
         if (ptr != start)
            value.append(start, ptr - start);

         ptr = skip(ptr, 1);

         switch (*ptr)
         {
         case 'b':
            value += '\b';
            ptr = skip(ptr, 1);
            break;
         case 'f':
            value += '\f';
            ptr = skip(ptr, 1);
            break;
         case 'n':
            value += '\n';
            ptr = skip(ptr, 1);
            break;
         case 'r':
            value += '\r';
            ptr = skip(ptr, 1);
            break;
         case 't':
            value += '\t';
            ptr = skip(ptr, 1);
            break;
         case 'u':
            {
               ptr = skip(ptr, 1);

               int unicode;
               ptr = parse_unicode(unicode, ptr);

               if ((unicode >= 0xDC00 && unicode <= 0xDFFF) || unicode == 0)	
                  throw JsonError("bad unicode character");

               if (unicode >= 0xD800 && unicode <= 0xDBFF )
               {
                  if (*ptr != '\\')
                     throw JsonError("expected second unicode surrogate part");
                  ptr = skip(ptr, 1);
                  if (*ptr != 'u')
                     throw JsonError("expected second unicode surrogate part");
                  ptr = skip(ptr, 1);

                  int unicode2;
                  ptr = parse_unicode(unicode2, ptr);

                  unicode = 0x10000 + (((unicode & 0x3FF) << 10) | (unicode2 & 0x3FF));
               }

               int len = 4;
               if (unicode < 0x80)
               {
                  len = 1;
                  value += unicode;
               }
               else if (unicode < 0x800)
               {
                  len = 2;
                  value += 0xc0 | (unicode >> 6);
               }
               else if (unicode < 0x10000)
               {
                  len = 3;
                  value += 0xe0 | (unicode >> 12);
               }
               else
               {
                  len = 4;
                  value += 0xf0 | (unicode >> 18);
               }

               for (int i = len; i > 1; --i)
               {
                  int shift = (i - 2) * 6;
                  value += 0x80 | ((unicode >> shift) & 0x3f);
               }
            }
            break;
         default:
            value += *ptr;
            ptr = skip(ptr, 1);
            break;
         }

         start = ptr;
      }
   }

   if (ptr != start)
      value.append(start, ptr - start);

   if (0 == *ptr)
      return ptr;

   return skip(ptr, 1);
}

const char *Root::parse_unicode(int &value, const char *ptr)
{
   value = 0;

   for (int i = 0; i < 4; ++i)
   {
      char hex = *ptr;

      if ((hex >= '0' && hex <= '9'))
         value = (value << 4) + (hex - '0');
      else if ((hex >= 'a' && hex <= 'f'))
         value = (value << 4) + (hex - 'a' + 10);
      else if ((hex >= 'A' && hex <= 'F'))
         value = (value << 4) + (hex - 'A' + 10);
      else
         throw JsonError("bad escaped character");

      ptr = skip(ptr, 1);
   }

   return ptr;
}

const Array &Root::getArray() const
{
   if (!m_firstChild || m_firstChild->getType() != TypeArray)
      throw JsonError("value is not an array");

   return m_firstChild->toArray();
}

const Object &Root::getObject() const
{ 
   if (!m_firstChild || m_firstChild->getType() != TypeObject)
      throw JsonError("value is not an object");

   return m_firstChild->toObject();
}

Value *Root::linkValue(Value *value) 
{
   if (m_firstChild)
      delete m_firstChild;
   m_firstChild = value;
   return value;
}

Value &Root::setValue(Value &value)
{
   Value *newv = value.clone();
   
   if (m_firstChild)
      delete m_firstChild;
   m_firstChild = newv;
   return *newv;
}

bool Printer::enter(const Value &value) 
{
   if (value.previousSibling())
   {
      m_out << ',';
      printLineBreak();
   }
   printIndent();
   printName(value);

   if (value.getType() == TypeArray)
      m_out << '[';
   else if (value.getType() == TypeObject)
      m_out << '{';

   printLineBreak();
   m_depth++;

   return true; 
}

bool Printer::visit(const Boolean &value) 
{
   printSeparator(value);
   printName(value);

   if (value.getValue())
      m_out << "true";
   else
      m_out << "false";

   return true; 
}

bool Printer::visit(const Null &value) 
{
   printSeparator(value);
   printName(value);
   m_out << "null";
   return true;
}

bool Printer::visit(const String &value)
{
   printSeparator(value);
   printName(value);
   printEscapedString(value.getValueStr());

   return true;
}

bool Printer::visit(const Number &value)
{
   printSeparator(value);
   printName(value);
   m_out << std::setprecision(std::numeric_limits<double>::digits10 + 1) << value.getValue();

   return true;
}

bool Printer::exit(const Value &value) 
{
   m_depth--;
   printLineBreak();
   printIndent();

   if (value.getType() == TypeArray)
      m_out << ']';
   else if (value.getType() == TypeObject)
      m_out << '}';

   return true; 
}

void Printer::printEscapedString(const std::string &value)
{
   m_out << '\"';

   const char *str = value.c_str();
   const char *ptr = str;

   while (*ptr)
   {
      if ((unsigned char)*ptr > 31 && *ptr != '\"' && *ptr != '\\')
         ptr++;
      else
      {
         if (ptr != str)
            m_out.write(str, ptr - str);
         
         switch (*ptr)
         {
         case '\\':
            m_out << "\\\\";
            break;
         case '\"':
            m_out << "\\\"";
            break;
         case '\b':
            m_out << "\\b";
            break;
         case '\f':
            m_out << "\\f";
            break;
         case '\n':
            m_out << "\\n";
            break;
         case '\r':
            m_out << "\\r";
            break;
         case '\t':
            m_out << "\\t";
            break;
         default:
            const char *hex = {"0123456789abcdef"};
            m_out << "\\u00";
            m_out << hex[(unsigned char)*ptr >> 4];
            m_out << hex[(unsigned char)*ptr & 0x0f];
            break;
         }

         str = ++ptr;
      }
   }

   if (ptr != str)
      m_out.write(str, ptr - str);
   
   m_out << '\"';
}

};