#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

#include <string>

class ConfigFile {
public:
  ConfigFile();

  static int getTypePath(const std::string &path);
  static int checkFile(const std::string &path, int mode);
  static int isFileExistAndReadable(const std::string &path,
                                    const std::string &index);

  std::string readFile(const std::string &path);
  const std::string &getPath() const;
  int getSize() const;

private:
  std::string path_;
  int size_;
};

#endif
