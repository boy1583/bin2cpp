#include "BaseGenerator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <stdlib.h>
#include <sstream>

#include "common.h"

namespace bin2cpp
{

  BaseGenerator::BaseGenerator()
  {
  }

  BaseGenerator::~BaseGenerator()
  {
  }

  std::string BaseGenerator::getGetterFunctionName(const char * iFunctionIdentifier)
  {
    //Uppercase function identifier
    std::string functionIdentifier = iFunctionIdentifier;
    functionIdentifier[0] = (char)toupper(functionIdentifier[0]);

    std::string getter;
    getter.append("get");
    getter.append(functionIdentifier);
    getter.append("File");
    return getter;
  }

  std::string BaseGenerator::getHeaderFilePath(const char * iOutputFolder, const char * iHeaderFilename)
  {
    //Build header file path
    std::string headerPath = iOutputFolder;
    if (headerPath[headerPath.length()-1] != '\\')
      headerPath.append("\\");
    headerPath.append(iHeaderFilename);

    return headerPath;
  }

  std::string BaseGenerator::getCppFilePath(const char * iOutputFolder, const char * iHeaderFilename)
  {
    std::string headerPath = getHeaderFilePath(iOutputFolder, iHeaderFilename);

    //Build cpp file file
    std::string cppPath = headerPath;
    strReplace(cppPath, ".h", ".cpp");

    return cppPath;
  }

  bin2cpp::ErrorCodes BaseGenerator::createHeaderEmbededFile(const char * iOutputFolder, const char * iHeaderFilename, const char * iFunctionIdentifier, bool iOverrideExisting)
  {
    std::string outputHeaderPath = std::string(iOutputFolder) + "\\" + iHeaderFilename;

    FILE * header = fopen(outputHeaderPath.c_str(), "w");
    if (!header)
      return bin2cpp::ErrorCodes::UnableToCreateOutputFiles;

    fprintf(header, "/**\n");
    fprintf(header, " * This file was generated by bin2cpp v%s.\n", getVersionString() );
    fprintf(header, " * Copyright (C) 2013-%d end2endzone.com. All rights reserved.\n", bin2cpp::getCopyrightYear());
    fprintf(header, " * Do not modify this file.\n");
    fprintf(header, " */\n");
    fprintf(header, "#pragma once\n");
    fprintf(header, "#include <stddef.h>\n");
    fprintf(header, "namespace bin2cpp\n");
    fprintf(header, "{\n");
    fprintf(header, "  #ifndef BIN2CPP_EMBEDDEDFILE_CLASS\n");
    fprintf(header, "  #define BIN2CPP_EMBEDDEDFILE_CLASS\n");
    fprintf(header, "  class File\n");
    fprintf(header, "  {\n");
    fprintf(header, "  public:\n");
    fprintf(header, "    virtual size_t getSize() const = 0;\n");
    fprintf(header, "    virtual const char * getFilename() const = 0;\n");
    fprintf(header, "    virtual const char * getBuffer() const = 0;\n");
    fprintf(header, "    virtual const char * getMd5() const = 0;\n");
    fprintf(header, "    virtual bool save(const char * iFilename) const = 0;\n");
    fprintf(header, "  };\n");
    fprintf(header, "  #endif\n");
    fprintf(header, "  const File & %s();\n", getGetterFunctionName(iFunctionIdentifier).c_str());
    fprintf(header, "}; //bin2cpp\n");

    fclose(header);

    return bin2cpp::ErrorCodes::Success;
  }

}; //bin2cpp