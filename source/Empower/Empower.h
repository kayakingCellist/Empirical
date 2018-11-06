/**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2018
 *
 *  @file  Empower.h
 *  @brief A scripting language built inside of C++
 *
 *  Empower is a scripting language built inside of Empirical to simplify and the use of fast
 *  run-time interpreting.  Internally, and Empower object will track all of the types used and 
 *  all of the variables declared, ensuring that they interact correctly.
 * 
 *  Developer Notes:
 *  @todo After a memory image is locked down, we can optimize it by re-ordering variables, etc.
 *        to group identical types together, or isolate those that are trivially constructable or
 *        destructable. 
 *  
 *  @todo Setup a template wrapper that adds annotations on to another class using a MemoryImage
 *  @todo Allow multiple memory spaces in Empower; basically scopes or namespaces.
 *  @todo Allow nested namespaces to be branched at any level (so outer levels share vars)
 */

#ifndef EMP_EMPOWER_H
#define EMP_EMPOWER_H

#include <functional>
#include <map>
#include <string>

#include "../base/Ptr.h"
#include "../base/vector.h"
#include "../meta/TypeID.h"
#include "../tools/debug.h"

namespace emp {

  class Empower {
  public:
    using byte_t = unsigned char;

    static constexpr size_t undefined_id = (size_t) -1;

    /// A MemoryImage is a full set of variable values stored in an Empower instance.
    /// Any number of memory images can be created for a single Empower instance, but
    /// they must all be the same size and refer to the same set of variables.
    class MemoryImage {
    private:
      emp::vector<byte_t> memory;      ///< The specific memory values.
      emp::Ptr<Empower> empower_ptr;   ///< A pointer back to Empower instance this memory uses.

    public:
      MemoryImage(emp::Ptr<Empower> _ptr) : memory(), empower_ptr(_ptr) { ; }
      MemoryImage(const MemoryImage &);
      MemoryImage(MemoryImage &&);
      ~MemoryImage();

      const emp::vector<byte_t> & GetMemory() const { return memory; }
      Empower & GetEmpower() { return *empower_ptr; }
      const Empower & GetEmpower() const { return *empower_ptr; }

      template <typename T> emp::Ptr<T> GetPtr(size_t pos) {
        return reinterpret_cast<T*>(&memory[pos]);
      }
      template <typename T> T & GetRef(size_t pos) {
        return *(reinterpret_cast<T*>(&memory[pos]));
      }
      template <typename T> const T & GetRef(size_t pos) const {
        return *(reinterpret_cast<const T*>(&memory[pos]));
      }

      byte_t & operator[](size_t pos) { return memory[pos]; }
      const byte_t & operator[](size_t pos) const { return memory[pos]; }
      size_t size() const { return memory.size(); }
      void resize(size_t new_size) { memory.resize(new_size); }
    };


    /// A Var is an internal variable that has a run-time determined type (which is tracked).
    class Var {
    private:
      size_t info_id;                 ///< Which variable ID is this var associated with?
      size_t mem_pos;                 ///< Where is this variable in a memory image?
      emp::Ptr<MemoryImage> mem_ptr;  ///< Which memory image is variable using (by default)

    public:
      Var(size_t _id, size_t _pos, MemoryImage & mem)
	      : info_id(_id), mem_pos(_pos), mem_ptr(&mem) { ; }
      Var(const Var &) = default;

      template <typename T>
      T & Restore() {
        // Make sure function is restoring the correct type.
        emp_assert( mem_ptr->GetEmpower().vars[info_id].type_id ==
		    mem_ptr->GetEmpower().GetTypeID<T>() );

      	// Convertthis memory to a reference that can be returned.
        return mem_ptr->GetRef<T>(mem_pos);
      }
    };

  protected:    
    /// Information about a single Empower variable, including its type, name, and where to
    /// find it in a memory image.
    struct VarInfo {
      size_t type_id;          ///< What type is this variable?
      std::string var_name;    ///< What is the unique name for this variable?
      size_t mem_pos;          ///< Where in memory is this variable stored?

      VarInfo(size_t _id, const std::string & _name, size_t _pos)
       : type_id(_id), var_name(_name), mem_pos(_pos) { ; }
    };

    using cconstruct_fun_t = std::function<void(const VarInfo &, const MemoryImage &, MemoryImage &)>;
    using copy_fun_t = std::function<void(const VarInfo &, const MemoryImage &, MemoryImage &)>;
    using destruct_fun_t = std::function<void(const VarInfo &, MemoryImage &)>;

    /// Information about a single type used in Empower.
    struct TypeInfo {
      size_t type_id;          ///< Unique value for this type.
      std::string type_name;   ///< Name of this type (from std::typeid)
      size_t mem_size;         ///< Bytes needed for this type (from sizeof)      
      
      cconstruct_fun_t cconstruct_fun; ///< Function to run copy constructor on var of this type.
      copy_fun_t copy_fun;             ///< Function to copy var of this type across memory images.
      destruct_fun_t destruct_fun;     ///< Function to run destructor on var of this type.
      
      // Core conversion functions for this type.
      std::function<double(Var &)> to_double;      ///< Fun to convert type to double (empty=>none)
      std::function<std::string(Var &)> to_string; ///< Fun to convert type to string (empty=>none)
      
      TypeInfo(size_t _id, const std::string & _name, size_t _size,
               const cconstruct_fun_t & cc_fun,
               const copy_fun_t & c_fun, const destruct_fun_t & d_fun)
	      : type_id(_id), type_name(_name), mem_size(_size)
        , cconstruct_fun(cc_fun)
        , copy_fun(c_fun), destruct_fun(d_fun) { ; }
    };


    /// ------ INTERNAL VARIABLES ------

    MemoryImage memory;             ///< The Default memory image.
    emp::vector<VarInfo> vars;      ///< Information about all vars used.
    emp::vector<TypeInfo> types;    ///< Information about all types used.

    std::map<std::string, size_t> var_map;   ///< Map variable names to index in vars
    std::map<size_t, size_t> type_map;       ///< Map type names (from typeid) to index in types

    /// When build a copy of a MemoryImage, we must make sure to properly construct each variable.
    void CopyConstruct(const MemoryImage & from_image, MemoryImage & to_image) {
      /// Loop through all variables stored in this memory image and copy each of them.
      for (const VarInfo & v : vars) {
        const TypeInfo & type = types[v.type_id];
      	type.cconstruct_fun(v, from_image, to_image);
      }
    }

    /// When copying a MemoryImage, we must make sure to properly copy each variable.  Requires
    /// that all variables have already been constructed and are just being copied into!
    void Copy(const MemoryImage & from_image, MemoryImage & to_image) {
      /// Loop through all variables stored in this memory image and copy each of them.
      for (const VarInfo & v : vars) {
        const TypeInfo & type = types[v.type_id];
      	type.copy_fun(v, from_image, to_image);
      }
    }

    /// When deleting a MemoryImage, we must make sure to run the destructors on each
    /// internal variable contained.
    void Destruct(MemoryImage & image) {
      /// Loop through all variables stored in this memory image and destruct each of them.
      for (const VarInfo & v : vars) {
        const TypeInfo & type = types[v.type_id];
      	type.destruct_fun(v, image);
      }
    }

  public:
    Empower() : memory(this), vars(), types(), var_map(), type_map() { ; }
    ~Empower() { ; }

    /// Convert a type (provided as a template argument) to its index in types vector.
    /// If type is not already in Empower, add it.
    template <typename T>
    size_t GetTypeID() {
      using base_t = typename std::decay<T>::type;

      size_t type_hash = GetTypeValue<base_t>();
      std::string type_name = typeid(base_t).name();

      // If this type already exists stop here!
      auto type_it = type_map.find(type_hash);
      if (type_it != type_map.end()) return type_it->second;

      size_t type_id = types.size();
      size_t mem_size = sizeof(base_t);
      cconstruct_fun_t cconstruct_fun =
        [](const VarInfo & var_info, const MemoryImage & from_image, MemoryImage & to_image) {
          const size_t mem_pos = var_info.mem_pos;
          new (to_image.GetPtr<T>(mem_pos).Raw()) T(from_image.GetRef<T>(mem_pos));
        };
      copy_fun_t copy_fun = [](const VarInfo & var_info, const MemoryImage & from_image, MemoryImage & to_image) {
        const size_t mem_pos = var_info.mem_pos;
        to_image.GetRef<T>(mem_pos) = from_image.GetRef<T>(mem_pos);
      };
      destruct_fun_t destruct_fun = [](const VarInfo & var_info, MemoryImage & mem) {
	      mem.GetPtr<T>(var_info.mem_pos)->~T();
      };

      types.emplace_back(type_id, type_name, mem_size, cconstruct_fun, copy_fun, destruct_fun);
      type_map[type_hash] = type_id;

      return type_id;
    }

    template <typename T>
    Var NewVar(const std::string & name, const T & value) {
      size_t type_id = GetTypeID<T>();                ///< Get ID for type (create if needed)
      TypeInfo & type_info = types[type_id];          ///< Create ref to type info for easy access.
      size_t var_id = vars.size();                    ///< New var details go at end of var vector.
      size_t mem_start = memory.size();               ///< Start new var at current end of memory.
      vars.emplace_back(type_id, name, mem_start);    ///< Add this VarInfo to our records.
      memory.resize(mem_start + type_info.mem_size);  ///< Resize memory to fit new variable.
      var_map[name] = var_id;                         ///< Link the name of this variable to id.

      /// Construct new variable contents in place, where space was allocated.
      new (memory.GetPtr<T>(mem_start)) T(value);


      return Var(var_id, mem_start, memory);
    }
  };

  // Define copy constructor.
  Empower::MemoryImage::MemoryImage(const MemoryImage & image) {
    memory.resize(image.memory.size());              // Default to image's memory.
    empower_ptr = image.empower_ptr;    // Copy pointer back to empower object.

    // Run through all copy constructors.
    empower_ptr->CopyConstruct(image, *this);
  }

  // Define move constructor.
  Empower::MemoryImage::MemoryImage(MemoryImage && image) {
    memory = std::move(image.memory);   // Move over memory
    empower_ptr = image.empower_ptr;    // Copy pointer back to empower object.
    image.empower_ptr = nullptr;        // Make this memory as inactive.
  }


  // Define functions from MemoryImage that need to refer back to it functions in Empower.
  Empower::MemoryImage::~MemoryImage() {
    // If memory still active, make sure to destruct it.
    if (empower_ptr) empower_ptr->Destruct(*this);
  }

}

#endif