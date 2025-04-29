#include <iostream>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <list>
#include <memory>
#include <algorithm>
#include <vector>
#include <map>
#include <array>
#include <set>

/*

  this compiler is currently little endian

  gcc -S -O -fno-asynchronous-unwind-tables -fcf-protection=none main.c

*/

#include "c-compiler-lib/C_compiler.hpp"

#include "assemble_ir.hpp"

#include "../assembler/assemble.hpp"

#include "../disassembler/disassemble_instruction.hpp"

int main(int argc, char* argv[])
{
  outputFilename = "a.out";

  /*#ifndef NDEBUG
  {
    argc = 4;

    char* args[4] = {
      "./C_compiler",
      "../tests/counting.c",
      "-O",
      "-I../include"
    };
    argv = args;
  }
  #endif // NDEBUG*/

  if (handleArgs(argc, argv))
  {
    return -1;
  }

  std::string fileText = loadFile(inputFilename);

  if (doPreprocess)
  {
    fileText = preprocess(fileText);
    std::cout << "Preprocessed:\n" << fileText << '\n';
  }

  if (doCompile)
  {
    std::list<Token> code = lex(fileText);
    std::cout << "Tokens:\n";

    for (const Token& token: code)
    {
      std::cout << Token::typeStrings[token.type] << ": \"" << token.data << "\"\n";
    }
  
    Program AST = parse(code);

    /*if (optimize)
    {
      optimizeAST(AST);
    }*/

    removeSubExpressions(AST);
  
    std::cout << "AST:\n";
    PrintAST printer(AST);

    std::vector<Operation> asmCode = generateIR(AST);

    if (optimize)
    {
      optimizeIR(asmCode);
    }

    fileText = printIR(asmCode);
    std::cout << "Intermediate Representation:\n" << fileText << '\n';

    std::vector<std::string> assembly = assembleIR(asmCode);

    std::vector<uint8_t> binary = assemble(assembly);

    std::cout << "Assembly: \n";
    for (uint16_t i = 0; i < binary.size()-1; i += 2)
    {
      std::cout << disassembleInstruction(binary[i], binary[i+1]) << "\n";
    }

    std::filebuf file;
    file.open(outputFilename, std::ios::out | std::ios::binary);

    file.sputn((char*)binary.data(), binary.size());

    file.close();
  }

  return 0;
}

