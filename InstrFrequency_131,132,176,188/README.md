# Instruction Frequency Analysis Pass for LLVM/Clang

 Instruction Frequency Analysis is a specialized
 technique in the LLVM compiler framework that
 categorizes and counts various instruction types
 across a program's functions, such as arithmetic
 operations, logical operations, memory accesses,
 and control flow instructions. By generating a
 detailed frequency table for each function, this
 analysis provides valuable insights into the code's structure and execution patterns, aiding in
 performance optimization.

NAME | USN 
--- | --- 
**S DHANUSH** | **1RV21CS131**
**S MOHAMMED ASHIQ** | **1RV21CS132**
**TANMAY S LAL** | **1RV21CS176**
**VIANY D** | **1RV21CS188**

Steps to run :

Step 1: 

>Prerequisite installation to be done or made sure is present 

```sudo apt install llvm clang llvm-dev```

Step 2:

>Build a tool . A Tool is created named "InstrPassTool" in the same directory .

# InstrPassTool

    InstrPassTool is a tool that analyzes LLVM instructions within a compiled C program. This README file will guide you through the installation process and how to compile and run the tool on a Unix-based system.


```clang++ `llvm-config --cxxflags --ldflags --system-libs --libs all` -o InstrPassTool InstrPassTool.cpp```

Step 3:

>Insert your sample code inside any filename  named as "<filename.c>"

Eg : Here it is "sample.c"

Step 4:

>Generate result in <filename.ic> file

```./InstrPassTool sample.c```

Eg : Here result is obtained in "sample.ic"