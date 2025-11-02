#include "ConfigFile.hpp"

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace {

std::string joinPaths(const std::string &base, const std::string &leaf) {
  if (leaf.empty())
    return leaf;
  if (!leaf.empty() && leaf[0] == '/')
    return leaf;
  if (base.empty())
    return leaf;
  if (base[base.size() - 1] == '/')
    return base + leaf;
  return base + "/" + leaf;
}

} // namespace

ConfigFile::ConfigFile() : path_(), size_(0) {}

int ConfigFile::getTypePath(const std::string &path) {
  struct stat st;
  if (stat(path.c_str(), &st) == -1)
    return -1;
  if (S_ISREG(st.st_mode))
    return 1;
  if (S_ISDIR(st.st_mode))
    return 2;
  return 3;
}

int ConfigFile::checkFile(const std::string &path, int mode) {
  return access(path.c_str(), mode);
}

int ConfigFile::isFileExistAndReadable(const std::string &path,
                                       const std::string &index) {
  if (index.empty())
    return 0;

  if (getTypePath(index) == 1 && checkFile(index, R_OK) == 0)
    return 1;

  const std::string candidate = joinPaths(path, index);
  if (getTypePath(candidate) == 1 && checkFile(candidate, R_OK) == 0)
    return 1;

  return 0;
}

std::string ConfigFile::readFile(const std::string &path) {
  std::ifstream file(path.c_str());
  if (!file.is_open()) {
    path_.clear();
    size_ = 0;
    return std::string();
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  const std::string content = buffer.str();
  path_ = path;
  size_ = static_cast<int>(content.size());
  return content;
}

const std::string &ConfigFile::getPath() const { return path_; }

int ConfigFile::getSize() const { return size_; }
