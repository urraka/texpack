#ifndef XML_WRITER_HPP
# define XML_WRITER_HPP

# define HEADER "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
# define INDENT "  "
# define NEWLINE "\n"

# include <string>
# include <stack>
# include <fstream>

// Modified by qwertysam, based on XML writer by sebclaeys 
// https://gist.github.com/sebclaeys/1227644/3761c33416d71c20efc300e78ea1dc36221185c5#file-example-cpp-L5
class XMLWriter
{
public:

  XMLWriter(std::ofstream& os) : os(os), tag_open(false), new_line(true) {os << HEADER;}
  ~XMLWriter() {}

  XMLWriter& openElt(const char* tag) {
    this->closeTag();
    if (elt_stack.size() > 0)
      os << NEWLINE;
    this->indent();
    this->os << "<" << tag;
    elt_stack.push(tag);
    tag_open = true;
    new_line = false;
    return *this;
  }

  XMLWriter& closeElt() {
    this->closeTag();
    std::string elt = elt_stack.top();
    this->elt_stack.pop();
    if (new_line)
      {
        os << NEWLINE;
        this->indent();
      }
    new_line = true;
    this->os << "</" << elt << ">";
    return *this;
  }

  XMLWriter& closeAll() {
    while (elt_stack.size())
      this->closeElt();
      
    return *this;
  }

  XMLWriter& attr(const char* key, const char* val) {
    this->os << " " << key << "=\"";
    this->write_escape(val);
    this->os << "\"";
    return *this;
  }

  XMLWriter& attr(const char* key, std::string val) {
    return attr(key, val.c_str());
  }
    
  XMLWriter& attr(const char* key, int val) {
    return attr(key, std::to_string(val));
  }

  XMLWriter& content(const char* val) {
    this->closeTag();
    this->write_escape(val);
    return *this;
  }

private:
  std::ofstream& os;
  bool tag_open;
  bool new_line;
  std::stack<std::string> elt_stack;

  inline void closeTag() {
    if (tag_open)
      {
        this->os << ">";
        tag_open = false;
      }
  }

  inline void indent() {
    for (size_t i = 0; i < elt_stack.size(); i++)
      os << (INDENT);
  }

  inline void write_escape(const char* str) {
    for (; *str; str++)
      switch (*str) {
      //case '&': os << "&amp;"; break;
      //case '<': os << "&lt;"; break;
      //case '>': os << "&gt;"; break;
      //case '\'': os << "&apos;"; break;
      //case '"': os << "&quot;"; break;
      default: os.put(*str); break;
      }
  }
};

#endif /* !XML_WRITER_HPP */