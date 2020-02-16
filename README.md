# Static Taint Analysis In RUST




Rust is a new system programming language that offers a practical and safe alternative to C. Following its relatively new release there is an absence of any major debugging framework which helps the user to track the Information flow of his/her code. In this paper, we have designed a utility to provide a basic static taint analysis in RUST Programing Language.

## 1. INTRODUCTION

Taint analysis can be seen as a form of Information Flow Analysis. A gentle definition provided by Dorothy Denning[1] is “Information flows from object x to object y, denoted x→y, whenever information stored in x is transferred to, object y”. If the source of the value of the object X is untrustworthy, we say that X is tainted. Now wherever information flows out of X to any other object we mark that object Tainted. After the trace has completed, we output the objects which were marked tainted directly or indirectly through X. 

## 2. Implementation

At its heart, RUST uses LLVM to run optimization passes before actually generating an executable. This gave us the opportunity to use a predefined and trustworthy structure as LLVM (a library for programmatically creating machine-native code) for static taint analysis. LLVM provides us the interface to traverse through the code in terms of a list. Each function contains the set of instructions (which were our primary focus).

To basically get bitcode file of RUST using rustc command and run our LLVM pass externally on that file. We passed a file containing variable names that are taint sources to LLVM pass. Initially, we read the file and create a map of function Name with tainted source. Now, we traverse over each function one by one, find the instruction where this variable is initialized (basically an alloc instruction tells us this) and traverse over all the instructions where this variable is used in that function. Now, if the variable is passed to a function, we mark that function tainted with this  specific argument number. Now, we traverse over all those functions that are marked tainted and check if this function’s tainted argument is passed to any other function or not. We recursively go over all those functions whose implementation is inside the file. In the end, we will end with all those function calls whose implementation is not inside the file, that means these are the system calls with possible information leakage (we made this assumption.) Now, in the end, we print all those Function Names where these system calls were made with the possibility of information leakage in that function.


## 3. Testing


We performed different types of test single function call [T1], nested function call [T2], passing tainted values standard data structures [T3] and passing a tainted message to standard socket [T4]. All of the test went fine.


