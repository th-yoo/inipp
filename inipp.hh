// inipp.hh - minimalistic ini file parser class in single header file
//
// Copyright (c) 2009, Florian Wagner <florian@wagner-flo.net>.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef INIPP_HH
#define INIPP_HH

#define INIPP_VERSION "1.0"

#include <string>
#include <unordered_map>
#include <fstream>
#include <stdexcept>
#include <sstream>

namespace inipp
{
  class inifile;
  class inisection;

  class unknown_entry_error : public std::runtime_error
  {
    public:
      inline unknown_entry_error(const std::string& key)
        : std::runtime_error("Unknown entry '" + key + "'.")
      { /* empty */ };

      inline unknown_entry_error(const std::string& key,
                                 const std::string& section)
        : std::runtime_error("Unknown entry '" + key + "' in section '" +
                             section + "'.")
      { /* empty */ };
  };

  class unknown_section_error : public std::runtime_error
  {
    public:
      inline unknown_section_error(const std::string& section)
        : std::runtime_error("Unknown section '" + section + "'.")
      { /* empty */ };
  };

  class syntax_error : public std::runtime_error
  {
    public:
      inline syntax_error(const std::string& msg)
        : std::runtime_error(msg)
      { /* empty */ };
  };

  class inisection
  {
    friend class inifile;

    public:
      inline std::string name() const;
      inline std::string get(const std::string& key) const;
      inline std::string dget(const std::string& key,
                              const std::string& default_value) const;

      template<typename T>
      T getval( const std::string& key
	      , const T def) const
      {
	  std::string src;
	  try {
		  src = get(key);
	  } catch (...) {
		  return def;
	  }

	  std::istringstream i{src};
	  char c;
	  T rv;
	  if ((i>>std::boolalpha>>rv) && !(i>>c))
		  return rv;
	  return def;
      }

      const std::string getval( const std::string& key
			      , const char* def) const
      try {
	      return dget(key, def);
      } catch (...) { return def; }

      const std::string getval( const std::string& key
			      , const std::string& def) const
      try {
	      return dget(key, def);
      } catch (...) { return def; }


    protected:
      inline inisection(const std::string& section, const inifile& ini);

      const std::string _section;
      const inifile& _ini;
  };

  class inifile
  {
    public:
      explicit inline inifile(std::ifstream& infile);
      explicit inline inifile(std::ifstream&& infile);

      inline std::string get(const std::string& section,
                             const std::string& key) const;
      inline std::string get(const std::string& key) const;

      inline std::string dget(const std::string& section,
                              const std::string& key,
                              const std::string& default_value) const;
      inline std::string dget(const std::string& key,
                              const std::string& default_value) const;
      inline inisection section(const std::string& section) const;

      // borrow from mcmtroffaes/inipp
      // non-pointer built-in type
      // TODO: type check
      template<typename T>
      T getval( const std::string& sec
	      , const std::string& key
	      , const T def) const
      {
	  std::string src;
	  try {
		  src = get(sec, key);
	  } catch (...) {
		  return def;
	  }

	  std::istringstream i{src};
	  char c;
	  T rv;
	  if ((i>>std::boolalpha>>rv) && !(i>>c))
		  return rv;
	  return def;
      }

      const std::string getval( const std::string& sec
			      , const std::string& key
			      , const char* def) const
      try {
	      return dget(sec, key, def);
      } catch (...) { return def; }

      const std::string getval( const std::string& sec
			      , const std::string& key
			      , const std::string& def) const
      try {
	      return dget(sec, key, def);
      } catch (...) { return def; }

      // TODO: copy, move

      // https://www.reddit.com/r/programming/comments/7f0ljb/check_out_my_new_c_library_to_parse_creating_ini/
      // TODO: overload operator []

    protected:
      typedef std::unordered_map<std::string, std::string> kv_t;
      typedef std::unordered_map<std::string, kv_t> kkv_t;
      kkv_t sections_;
      kv_t defaultsection_;
  };

  namespace private_
  {
    inline std::string& rstrip(std::string& s, const std::string& c_to_strip);
    inline std::string& lstrip(std::string& s, const std::string& c_to_strip);

    inline std::string& trim(std::string&& str,
                            const std::string& whitespace = " \t\n\r\f\v");
    inline std::string& trim(std::string& str,
                            const std::string& whitespace = " \t\n\r\f\v");


    inline bool split(const std::string& in, const std::string& sep,
                      std::string& first, std::string& second);

    inline std::string& strip_comment(std::string& s, const std::string& mark);
    inline std::string& strip_comment(std::string& s);
  }

  inifile::inifile(std::ifstream&& infile)
	  : inifile(infile) {}

  inifile::inifile(std::ifstream& infile) {
    kv_t* cursec = &this->defaultsection_;
    std::string line;

    while(std::getline(infile, line)) {
      // trim line
      line = private_::trim(line);
      // remove comments
      private_::strip_comment(line);

      // ignore empty lines
      if(line.empty()) {
        continue;
      }

      // section?
      if(line[0] == '[') {
        if(line[line.size() - 1] != ']') {
          throw syntax_error("The section '" + line +
                             "' is missing a closing bracket.");
        }

        line = private_::trim(line.substr(1, line.size() - 2));
        cursec = &this->sections_[line];
        continue;
      }

      // entry: split by "=", trim and set
      std::string key;
      std::string value;

      if(private_::split(line, "=", key, value)) {
        (*cursec)[private_::trim(key)] = private_::trim(value);
        continue;
      }

      // throw exception on invalid line
      throw syntax_error("The line '" + line + "' is invalid.");
    }
  }

  std::string inifile::get(const std::string& section,
                           const std::string& key) const {
    if(!this->sections_.count(section)) {
      throw unknown_section_error(section);
    }

    if(!this->sections_.find(section)->second.count(key)) {
      throw unknown_entry_error(section, key);
    }

    return this->sections_.find(section)->second.find(key)->second;
  }

  std::string inifile::get(const std::string& key) const {
    if(!this->defaultsection_.count(key)) {
      throw unknown_entry_error(key);
    }

    return this->defaultsection_.find(key)->second;
  };

  std::string inifile::dget(const std::string& section,
                            const std::string& key,
                            const std::string& default_value) const {
    try {
      return this->get(section, key);
    }
    catch(unknown_section_error& ex) { /* ignore */ }
    catch(unknown_entry_error& ex) { /* ignore */ }

    return default_value;
  }

  std::string inifile::dget(const std::string& key,
                            const std::string& default_value) const {
    try {
      return this->get(key);
    }
    catch(unknown_entry_error& ex) { /* ignore */ }

    return default_value;
  }

  inisection inifile::section(const std::string& section) const {
    if(!this->sections_.count(section)) {
      throw unknown_section_error(section);
    }

    return inisection(section, *this);
  };

  inisection::inisection(const std::string& section, const inifile& ini)
    : _section(section),
      _ini(ini) {
    /* empty */
  }

  inline std::string inisection::name() const {
    return this->_section;
  }

  inline std::string inisection::get(const std::string& key) const {
    return this->_ini.get(this->_section, key);
  }

  inline std::string inisection::dget(const std::string& key,
                                      const std::string& default_val) const {
    return this->_ini.dget(this->_section, key, default_val);
  }

  inline std::string& private_::rstrip( std::string& s
		  		      , const std::string& c_to_strip)
  {
    size_t endpos = s.find_last_not_of(c_to_strip);
    if (endpos == std::string::npos) return s.erase();
    return s.erase(endpos+1);
  }

  inline std::string& private_::lstrip( std::string& s
		  		      , const std::string& c_to_strip)
  {
    size_t startpos = s.find_first_not_of(c_to_strip);
    if (startpos == std::string::npos) return s.erase();
    return s.erase(0, startpos);
  }

  inline std::string& private_::trim(std::string&& str,
                                    const std::string& whitespace) {
	  return rstrip(lstrip(str, whitespace), whitespace);
  }

  inline std::string& private_::trim(std::string& str,
                                    const std::string& whitespace) {
	  return rstrip(lstrip(str, whitespace), whitespace);
  }



  inline bool private_::split(const std::string& in, const std::string& sep,
                              std::string& first, std::string& second) {
    size_t eqpos = in.find(sep);

    if(eqpos == std::string::npos) {
      return false;
    }

    first = in.substr(0, eqpos);
    second = in.substr(eqpos + sep.size(), in.size() - eqpos - sep.size());

    return true;
  }

  inline std::string& private_::strip_comment(std::string& s, const std::string& mark)
  {
	  size_t pos = s.find_first_of(mark);
	  if (pos != std::string::npos) s.erase(pos);
	  return s;
  }

  inline std::string& private_::strip_comment(std::string& s)
  {
	  return strip_comment(strip_comment(s, "#"), ";");
  }
}

#endif /* INIPP_HH */
