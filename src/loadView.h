//
// Created by aag on 4/25/23.
//

#ifndef REACTPP_LOADVIEW_H
#define REACTPP_LOADVIEW_H
#include <fstream>
#include <iostream>
#include <map>
#include <string>
namespace custom {
  /**
   * @brief Custom data type for storing data to be used in views
   * @note just a typedef of std::map<std::string, std::string> (too lazy to type)
   */
typedef std::map<std::string, std::string> data;
}

namespace Core {
  /**
   * @brief Class for loading views
   * @note Views are html files with {{}} for data to be replaced
   */
class View {
private:
  std::string html;
  custom::data data;
  std::string folder = "public/";

  /**
   * @brief Replaces {{key}} with value
   * @param key
   * @param value
   */
  void replace(const std::string &key, const std::string &value) {
    // find key in {{}} and replace with value
    const std::string keyWithBraces = "{{" + key + "}}";

    size_t pos = html.find(keyWithBraces);
    if (pos != std::string::npos) {
      html.replace(pos, keyWithBraces.length(), value);
    }
  }

  /**
   * @brief Loads view from file
   * @param path
   * @param data
   */
  View(const std::string &path, custom::data data) : data(data) {
    std::ifstream file(path);
    if (file.is_open()) {
      std::string line;
      while (std::getline(file, line)) {
        html += line;
      }
      file.close();
    } else {
      html = "Unable to open file: " + path;
    }
  }

public:
  /**
   * @brief Default constructor
   */
  View() = default;


  /** render view
   * @return rendered view
   */
  std::string render() {
    // replace all {{}} with data
    for (auto &[key, value] : (std::map<std::string, std::string>)data) {
      replace(key, value);
    }
    return html;
  }

  /**
   * @brief Set folder for views
   * @param fname
   */
  void setFolder(const std::string &fname) { this->folder = fname; }

  /**
   * @brief Load view from file
   * @param path
   * @param data
   * @return
   */
  View use(const std::string &path, custom::data data) const {
    return View(folder + path, data);
  }
};
} // namespace Core

#endif // REACTPP_LOADVIEW_H
