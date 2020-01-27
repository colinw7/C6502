#ifndef CParser_H
#define CParser_H

#include <string>

class CParser {
 public:
  using uchar = unsigned char;

 public:
  CParser(const std::string &str) :
   str_(str), len_(str.size()) {
  }

  bool eof() const { return pos_ >= len_; }

  bool isChar(char c) const { return (! eof() && str_[pos_] == c); }

  bool isChars(const std::string &str) const {
    int len = str.size();

    for (int i = 0; i < len; ++i)
      if (pos_ + i >= len_ || str_[pos_ + i] != str[i])
        return false;

    return true;
  }

  void skipChar() { ++pos_; }

  bool isDigit   () const { return (! eof() && isdigit(str_[pos_])); }
  bool isXDigit  () const { return (! eof() && std::isxdigit(str_[pos_])); }
  bool isSpace   () const { return (! eof() && std::isspace(str_[pos_])); }
  bool isNonSpace() const { return (! eof() && ! std::isspace(str_[pos_])); }

  char getChar() const { assert(pos_ < len_); return str_[pos_]; }

  char readChar() { assert(pos_ < len_); return str_[pos_++]; }

  void skipSpace   () { while (isSpace   ()) ++pos_; }
  void skipNonSpace() { while (isNonSpace()) ++pos_; }

  bool isString() const { return (isChar('"') || isChar('\'')); }

  void skipString() {
    assert(isString());

    char c = readChar();

    while (! eof()) {
      if      (isChar('\\'))
        skipChar();
      else if (isChar(c))
        break;

      skipChar();
    }

    if (isChar(c))
      skipChar();
  }

  bool readWord(std::string &word) {
    skipSpace();

    int pos1 = pos_;

    while (! eof()) {
      if      (isString())
        skipString();
      else if (isSpace())
        break;
      else
        skipChar();
    }

    int pos2 = pos_;

    if (pos1 == pos2)
      return false;

    word = str_.substr(pos1, pos2 - pos1);

    return true;
  }

  bool readSepWord(std::string &word, char sep) {
    skipSpace();

    int pos1 = pos_;

    while (! eof()) {
      if      (isChar(sep))
        break;
      else if (isString())
        skipString();
      else if (isSpace())
        break;
      else
        skipChar();
    }

    int pos2 = pos_;

    if (isChar(sep)) {
      skipChar();

      skipSpace();
    }

    if (pos1 == pos2)
      return false;

    word = str_.substr(pos1, pos2 - pos1);

    return true;
  }

  std::string toUpper(const std::string &s) const {
    std::string s1 = s;

    std::transform(s1.begin(), s1.end(), s1.begin(),
                   [](unsigned char c){ return std::toupper(c); } // correct
                  );
    return s1;
  }

  ushort getHexValue(uchar &alen) {
    static std::string xchars = "0123456789abcdef";

    alen = 0;

    ushort hvalue = 0;

    while (! eof() && isXDigit()) {
      char c1 = std::tolower(getChar());

      auto p = xchars.find(c1);

      hvalue = hvalue*16 + int(p);

      skipChar();

      ++alen;
    }

    return hvalue;
  }

  ushort getDecValue(uchar &alen) {
    static std::string dchars = "0123456789";

    alen = 0;

    ushort dvalue = 0;

    while (! eof() && isDigit()) {
      auto p = dchars.find(getChar());

      dvalue = dvalue*10 + int(p);

      skipChar();

      ++alen;
    }

    return dvalue;
  }

  bool isValue() const {
    return (isChar('$') || isDigit());
  }

  ushort getValue(uchar &alen) {
    ushort value;

    if      (isChar('$')) {
      skipChar();

      value = getHexValue(alen);
    }
    else if (isDigit()) {
      value = getDecValue(alen);
    }
    else {
      assert(false);
    }

    return value;
  }

  int readLabel(std::string &label) {
    label = "";

    while (! eof()) {
      if (isSpace() || isChar(',') || isChar(')'))
        break;

      label += readChar();
    }

    return (label.size() > 0);
  }

 private:
  std::string str_;
  int         pos_ { 0 };
  int         len_ { 0 };
};

#endif
