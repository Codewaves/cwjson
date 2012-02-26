cwjson - C++ JSON parser
========================

Description
-----------

cwjson is a small, fast and safe C++ JSON parser. Currently it supports UTF8 parsing/generation, 
but can be upgraded to UTF16/UTF32 in the future.

Please contact me via [github] or [my blog] if you have any suggestions, questions or bug reports.

Requirements
------------

- C++ compiler
- C++ Standard library
- Enabled exceptions

Installation
-----------

cwjson parser doesn't require installation. Add two source files to your project and include cwjson.h to the header search path.

Documentation
-------------

Coming soon.

Memory allocations
------------------

IMPORTANT: all pointers/references returned by cwjson library point to internal data 
structure, you MUST NEVER delete or free them. DO NOT store them for late use,
or do it with great caution. If you remove parent value from the data tree, all
child pointers will be invalid.  

The only exception to this rule is "clone()" method. "clone()" method makes exact copy of value with all its children 
and return pointer to a copy. It's your responsibility to delete that memory using C++ standard library "delete".

All cwjson linkXXXXXXXX() methods accept pointers to value and that value will be owned by cwjson and deleted in the 
future. linkXXXXXXXX() methods can throw an exception. It's your job to prevent memory leaks or other problems. 
NEVER DO the following:

- NEVER Pass local object pointer or temporary pointer to link functions

- NEVER Use same pointer more than once. (If you want insert value copy, use setValue(), insertValue() or pushValue() they 
  will create copy for you)

- NEVER Pass unguarded pointer. In case of exception you will leak the memory. Use std::auto_ptr or other techniques.

         std::auto_ptr guard(pointer);
         linkXXXXX(pointer);
         guard.release();

You are warned!

Performance
-----------

Try to avoid using Object/Array setValue()/pushValue()/insertValue() methods. They clone values internally and hurt 
the performance. These methods are useful if you want to copy parts of the tree. If you need to insert large object 
with many children, consider using linkXXXXXXXX() methods instead. 


JSON example
------------

    {
       "Image" : {
          "Width" : 800,
          "Height" : 600,
          "Title" : "View from 15th Floor",
          "32bit" : true,
          "Thumbnail": {
             "Url" : "http://www.example.com/image/481989943",
             "Height" : 125,
             "Width" : 100
          },
          "IDs" : [116, 943, 234, 38793]
       }
    }

Basic parsing and data access
-----------------------------

If you are parsing JSON object with known structure, you can use basic data access methods. It's easy to use and safe.

      // Load, receive or somehow get the JSON data into 
      // std::string or char * buffer
      
      try
      {
         cwjson::Root root(buffer);

         cwjson::Object    &image = root.getObject().getObject("Image");
         const std::string &title = image.getString("Title").getValueStr();
         int                width = (int)image.getNumber("Width").getValue();
         int                height = (int)image.getNumber("Height").getValue();

         std::cout << title << " " << width << "x" << height << std::endl; 

         cwjson::Array &ids = image.getArray("IDs");
         for (int i = 0; i < ids.childCount(); ++i)
            std::cout << ids.getNumber(i).getValue() << " ";

         std::cout << std::endl;
      }
      catch (cwjson::JsonError &e)
      {
         // Parsing or data access exception. cwjson will throw an exception 
         // if it can't find value with specified name, array index is out of bounds
         // or trying to cast the value to incorrect type.

         std::cout << "Json Exception: " << e.what() << std::endl;
      }

Program will print the following:

      View from 15th Floor 800x600
      116 943 234 38793

null value handling
-------------------

JSON data can contain "null" values. Let assume that "Thumbnails" object can be null. Trying to access it, following code

      std::cout << image.getObject("Thumbnail").getString("Url").getValueStr();

will throw an exception "value is not an object" because "Thumbnail" is not an object. Use "isNull" method to check the value. 

      if (!image.isNull("Thumbnail"))
         std::cout << image.getObject("Thumbnail").getString("Url").getValueStr();

JSON generation
---------------

Let compose JSON example. 

      try
      {
         cwjson::Root root;

         cwjson::Object &rootObject = root.createRootObject();
         cwjson::Object &image      = rootObject.createObject("Image");
         image.setNumber("Width", 800);
         image.setNumber("Height", 600);
         image.setString("Title", "View from 15th Floor");
         image.setBoolean("32bit", true);

         cwjson::Object &thumbnail = image.createObject("Thumbnail");
         thumbnail.setString("Url", "http://www.example.com/image/481989943");
         thumbnail.setNumber("Width", 125);
         thumbnail.setNumber("Height", 100);

         cwjson::Array &ids = image.createArray("IDs");
         ids.pushNumber(116);
         ids.pushNumber(943);
         ids.pushNumber(234);
         ids.pushNumber(38793);

         std::ostringstream out;
         root.print(out, true);  // "true" means formated output
      }
      catch (cwjson::JsonError &e)
      {
         std::cout << "Json Exception: " << e.what() << std::endl;
      }

At the program end "out" stringstream will contain formated JSON object string.

JSON object tree traversal
--------------------------

If you don't know the JSON data structure, you can use generic value access methods to traverse the tree. It's 
less safe comparing to normal data access, because you need to be careful with data pointers. 

JSON data tree is built from the six main objects - Object, Array, String, Number, Boolean and Null. All these objects 
have same base class Value. You can use Value object methods to traverse the tree. Each Value object is a tree node 
and have parent, children and siblings. Use getType() method to find out the type of the object and appropriate method 
to convert Value object to derived class object.

Recursively walk JSON data tree:

      void recurseValue(cwjson::Value *value)
      {
         switch (value->getType())
         {
            case cwjson::TypeRoot:
               recurseValue(value->firstChild());
               break;

            case cwjson::TypeObject:
            case cwjson::TypeArray:
            {
               cwjson::Value *item = value->firstChild();
               while (item)
               {
                  recurseValue(item);
                  item = item->nextSibling();
               }
            }
            break;

            case cwjson::TypeString:
               std::cout << value->getNameStr() << " : " << value->toString().getValueStr() << std::endl;
               break;

            case cwjson::TypeNumber:
               std::cout << value->getNameStr() << " : " << value->toNumber().getValue() << std::endl;
               break;

            ...
            // Boolean and Null types
            ...
         }
      }
  
      int main(int argc, char* argv[])
      {
         // Load, receive or somehow get the JSON data into 
         // std::string or char * buffer

         try
         {
            cwjson::Root root(buffer);
            recurseValue(&root);
         }
         catch (cwjson::JsonError &e)
         {
            // Parsing exception. cwjson also throws an exception if you try cast Value to 
            // incorrect type.

            std::cout << "Json Exception: " << e.what() << std::endl;
         }
      }

There is another method to traverse the tree. Create a new class derived from cwjson::Visitor, overload methods 
and pass class instance to traverse() method. traverse() method will recursively walk all Value children and call 
your overloaded methods. If value type is ether String, Number, Boolean or Null appropriate visit() method will be called. 
If value type is Object or Array, enter() method is called before traversing children and exit() method is called after 
traversing is done. Return "true" to continue traversing. Return "false" in enter() to skip all Object/Array children. Return 
"false" in visit()/exit() methods to skip all remaining parent children.

Following example produces same result, but code is safe and simple.

      class TestVisitor : public cwjson::Visitor
      {
      public:
         bool visit(const cwjson::String &value) 
         { 
            std::cout << value.getNameStr() << " : " << value.toString().getValueStr() << std::endl;
            return true;
         }

         bool visit(const cwjson::Number &value) 
         {
            std::cout << value.getNameStr() << " : " << value.toNumber().getValue() << std::endl;
            return true; 
         }
      };

      int main(int argc, char* argv[])
      {
         // Load, receive or somehow get the JSON data into 
         // std::string or char * buffer

         try
         {
            cwjson::Root root(buffer);
            root.traverse(TestVisitor());
         }
         catch (cwjson::JsonError &e)
         {
            // Parsing exception.
            std::cout << "Json Exception: " << e.what() << std::endl;
         }
      }

Changing existing JSON tree
---------------------------

Simply get reference or pointer to an existing value and use setName()/setValue()/removeValue() methods. You 
can change JSON data in any way you want, for example - parse input, modify data, generate JSON object string.     


Author
------

Sergej Kravcenko  
[http://digital.codewaves.com]  
sergej@codewaves.com  

[github]: https://github.com/Codewaves/cwjson
[my blog]: http://digital.codewaves.com/
[http://digital.codewaves.com]: http://digital.codewaves.com/