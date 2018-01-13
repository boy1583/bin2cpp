// main.cpp : Defines the entry point for the console application.
//

#include "targetver.h"

#include "errorcodes.h"
#include "SegmentGenerator.h"
#include "StringGenerator.h"
#include "ArrayGenerator.h"

#include <stdlib.h>
#include <stdio.h>
#include <sstream>

#include "common.h"
#include "argumentparser.h"
#include "logger.h"
#include "md5support.h"

//#define ENABLE_BREAKPOINT_DEBUGGING

#ifdef ENABLE_BREAKPOINT_DEBUGGING
#include <windows.h>
#endif

using namespace bin2cpp;

//pre-declarations
bin2cpp::ErrorCodes processFile(const std::string & inputFile, bin2cpp::IGenerator * generator, const std::string & functionIdentifier, const size_t & chunkSize, bool overrideExisting, const std::string & iOutputFilePath);

void printHeader()
{
  printf("bin2cpp v%s - Convert binary files into C++ source code.\n", bin2cpp::getVersionString() );
  printf("Copyright (C) 2013-%d end2endzone.com. All rights reserved.\n", bin2cpp::getCopyrightYear());
  printf("bin2cpp is open source software, see http://github.com/end2endzone/bin2cpp \n");
}

void printUsage()
{
  //usage string in docopt format. See http://docopt.org/
  static const char usage[] = 
    "Usage:\n"
    "  bin2cpp --file=<path> --output=<path> --headerfile=<name> --identifier=<name> [--generator=<name>] [--chunksize=<value>] [--override] [--noheader] [--quiet]\n"
    "  bin2cpp --help\n"
    "  bin2cpp --version\n"
    "\n"
    "Options:\n"
    "  --help               Display this help message.\n"
    "  --version            Display this application version.\n"
    "  --file=<path>        Path of the input file used for embedding as a C++ source code.\n"
    "  --output=<path>      Output folder where to create generated code. ie: .\\generated_files\n"
    "  --headerfile=<name>  File name of the generated C++ Header file. ie: SplashScreen.h\n"
    "  --generator=<name>   Name of the generator to use. Possible values are 'segment', 'string' and 'array'. [default: segment].\n"
    "  --identifier=<name>  Identifier of the function name that is used to get an instance of the file. ie: SplashScreen\n"
    "  --chunksize=<value>  Size in bytes of each string segments (bytes per row). [default: 200].\n"
    "  --override           Tells bin2cpp to overwrite the destination files.\n"
    "  --noheader           Do not print program header to standard output.\n"
    "  --quiet              Do not log any message to standard output.\n"
    "\n";
  printf("%s", usage);
}

int main(int argc, char* argv[])
{
  #ifdef ENABLE_BREAKPOINT_DEBUGGING
  MessageBox(NULL, "", "", MB_OK);
  #endif

  //help
  std::string dummy;
  if (bin2cpp::parseArgument("help", dummy, argc, argv))
  {
    printHeader();
    printUsage();
    return bin2cpp::ErrorCodes::Success;
  }

  //noheader
  bool noheader = false;
  if (bin2cpp::parseArgument("noheader", dummy, argc, argv))
  {
    noheader = true;
  }

  //quiet
  bool quiet = false;
  if (bin2cpp::parseArgument("quiet", dummy, argc, argv))
  {
    quiet = true;
  }

  //force noheader if quiet
  if (quiet)
    noheader = true;

  bin2cpp::setQuietMode(quiet);

  //version
  if (bin2cpp::parseArgument("version", dummy, argc, argv))
  {
    if (!noheader)
      printHeader();
    return bin2cpp::ErrorCodes::Success;
  }

  if (!noheader && !quiet)
    printHeader();

  //mandatory arguments
  std::string inputFile;
  std::string outputFolder;
  std::string headerFilename;
  std::string functionIdentifier;

  if (!bin2cpp::parseArgument("file", inputFile, argc, argv))
  {
    bin2cpp::ErrorCodes error = bin2cpp::ErrorCodes::MissingArguments;
    bin2cpp::log(bin2cpp::LOG_ERROR, "%s (file)", getErrorCodeDescription(error));
    printUsage();
    return error;
  }

  if (!bin2cpp::parseArgument("output", outputFolder, argc, argv))
  {
    bin2cpp::ErrorCodes error = bin2cpp::ErrorCodes::MissingArguments;
    bin2cpp::log(bin2cpp::LOG_ERROR, "%s (output)", getErrorCodeDescription(error));
    printUsage();
    return error;
  }

  if (!bin2cpp::parseArgument("headerfile", headerFilename, argc, argv))
  {
    bin2cpp::ErrorCodes error = bin2cpp::ErrorCodes::MissingArguments;
    bin2cpp::log(bin2cpp::LOG_ERROR, "%s (headerfile)", getErrorCodeDescription(error));
    printUsage();
    return error;
  }

  if (!bin2cpp::parseArgument("identifier", functionIdentifier, argc, argv))
  {
    bin2cpp::ErrorCodes error = bin2cpp::ErrorCodes::MissingArguments;
    bin2cpp::log(bin2cpp::LOG_ERROR, "%s (identifier)", getErrorCodeDescription(error));
    printUsage();
    return error;
  }

  //optional arguments
  static const size_t DEFAULT_CHUNK_SIZE = 200;
  size_t chunkSize = DEFAULT_CHUNK_SIZE;
  bool overrideExisting = false;

  size_t tmpChunkSize = 0;
  if (bin2cpp::parseArgument("chunksize", tmpChunkSize, argc, argv))
  {
    chunkSize = tmpChunkSize;
  }

  if (bin2cpp::parseArgument("override", dummy, argc, argv))
  {
    overrideExisting = true;
  }

  //select generator
  bin2cpp::SegmentGenerator segmentGenerator;
  bin2cpp::StringGenerator stringGenerator;
  bin2cpp::ArrayGenerator arrayGenerator;
  bin2cpp::IGenerator * generator = NULL;

  std::string generatorName;
  if (bin2cpp::parseArgument("generator", generatorName, argc, argv))
  {
    if (generatorName == "segment")
    {
      generator = &segmentGenerator;
    }
    else if (generatorName == "string")
    {
      generator = &stringGenerator;
    }
    else if (generatorName == "array")
    {
      generator = &arrayGenerator;
    }

    //validate generator selection
    if (generator == NULL)
    {
      bin2cpp::ErrorCodes error = bin2cpp::ErrorCodes::MissingArguments;
      bin2cpp::log(bin2cpp::LOG_ERROR, "%s, unknown values for 'generator' argument!", getErrorCodeDescription(error));
      printUsage();
      return error;
    }
  }

  //apply default generator
  if (generator == NULL)
  {
    generator = &segmentGenerator;
  }

  // printing info
  std::string info;
  info << "Embedding \"" << inputFile << "\"";
  if (chunkSize != DEFAULT_CHUNK_SIZE)
  {
    info << " using chunks of ";
    info << chunkSize;
    info << " bytes";
  }
  if (overrideExisting)
    info << " overriding existing files";
  info << "...";
  bin2cpp::log(bin2cpp::LOG_INFO, info.c_str());

  //prepare output files path
  std::string outputHeaderPath = outputFolder + "\\" + headerFilename;
  std::string outputCppPath = outputFolder + "\\" + headerFilename;         bin2cpp::strReplace(outputCppPath, ".h", ".cpp");
  std::string cppFilename = headerFilename;                                 bin2cpp::strReplace(cppFilename, ".h", ".cpp");
  
  bin2cpp::ErrorCodes headerResult = bin2cpp::ErrorCodes::Success;
  bin2cpp::ErrorCodes cppResult = bin2cpp::ErrorCodes::Success;

  //process files
  headerResult = processFile(inputFile, generator, functionIdentifier, chunkSize, overrideExisting, outputHeaderPath);
  switch(headerResult)
  {
  case bin2cpp::ErrorCodes::Success:
  case bin2cpp::ErrorCodes::OutputFilesSkipped:
    //ok
    break;
  default:
    return headerResult;
  };
  
  cppResult = processFile(inputFile, generator, functionIdentifier, chunkSize, overrideExisting, outputCppPath);
  switch(cppResult)
  {
  case bin2cpp::ErrorCodes::Success:
  case bin2cpp::ErrorCodes::OutputFilesSkipped:
    //ok
    break;
  default:
    return cppResult;
  };

  //success
  return bin2cpp::ErrorCodes::Success;
}

bin2cpp::ErrorCodes processFile(const std::string & inputFile, bin2cpp::IGenerator * generator, const std::string & functionIdentifier, const size_t & chunkSize, bool overrideExisting, const std::string & iOutputFilePath)
{
  uint64_t lastModifiedDate = getFileModifiedDate(iOutputFilePath);
  std::string filename = getFilename(iOutputFilePath.c_str());
  std::string extension = getFileExtention(iOutputFilePath);

  bin2cpp::ErrorCodes result = bin2cpp::ErrorCodes::Success;

  if (bin2cpp::fileExists(iOutputFilePath.c_str()))
  {
    uint64_t outputModifiedDate = getOutputFileModifiedDate(iOutputFilePath);
    bool outputFileOutdated = (outputModifiedDate == 0 || lastModifiedDate > outputModifiedDate);
    if (outputFileOutdated)
    {
      //should we force override flag ?
      if (overrideExisting)
      {
        //no problem, user has already choosen to update the output files.
      }
      else
      {
        //force overriding output files.
        std::string message;
        message << "Output file \'" << filename << "\' is out of date. Forcing override flag";
        bin2cpp::log(bin2cpp::LOG_INFO, message.c_str());
        overrideExisting = true;
      }
    }
    else if (lastModifiedDate == outputModifiedDate)
    {
      //output file already up to date.
      result = bin2cpp::ErrorCodes::OutputFilesSkipped;
    }
    else if (!overrideExisting)
    {
      //fail if not overriding output file
      bin2cpp::ErrorCodes error = bin2cpp::ErrorCodes::OutputFileAlreadyExist;
      bin2cpp::log(bin2cpp::LOG_ERROR, "%s (%s)", getErrorCodeDescription(error), iOutputFilePath.c_str());
      return error;
    }
  }

  //generate file
  if (result == bin2cpp::ErrorCodes::Success)
  {
    bin2cpp::log(bin2cpp::LOG_INFO, "Writing file \"%s\"...", iOutputFilePath.c_str());
    if (extension == ".h")
    {
      //generate header
      result = generator->createHeaderEmbededFile(inputFile.c_str(), iOutputFilePath.c_str(), functionIdentifier.c_str());
    }
    else
    {
      //generate cpp
      result = generator->createCppEmbeddedFile(inputFile.c_str(), iOutputFilePath.c_str(), functionIdentifier.c_str(), chunkSize);
    }
  }
  if (result == bin2cpp::ErrorCodes::Success)
  {
    //OK
  }
  else if (result == bin2cpp::ErrorCodes::OutputFilesSkipped)
  {
    bin2cpp::log(bin2cpp::LOG_WARNING, "%s", getErrorCodeDescription(result));
  }
  else
  {
    bin2cpp::log(bin2cpp::LOG_ERROR, "%s", getErrorCodeDescription(result));
    bin2cpp::log(bin2cpp::LOG_ERROR, "Embedding failed!");
    return result;
  }
  
  return bin2cpp::ErrorCodes::Success;
}
