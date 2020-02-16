#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/CommandLine.h"
#include <map>
#include <set>
#include <string>
#include <fstream>

#define DIRTY 1
#define CLEAN 0
std::map< std::string, std::pair<std::string,int> > variableState;

using namespace llvm;

void ReadFile();
void getAllUses(Instruction *,Instruction * = NULL);
void printAllWarnings();
cl::opt<std::string> OutputFilename("xx", cl::desc("Specify output filename"), cl::value_desc("filename"));
std::set<Instruction*> all_tainted_instructions;
std::map<Function*, std::set<int> > functions_call;
std::map<Function*,  int> functions_returntype;

namespace {
	
  struct SkeletonPass : public ModulePass {
    static char ID;
    SkeletonPass() : ModulePass(ID) {}
	virtual bool runOnModule(Module &M) {
    	ReadFile();
    	all_tainted_instructions.clear();
	    for (auto &F : M) {
			// errs() << F.getName() << "\n";
	    	for (auto &B : F) {
	    		for (auto &I : B) {
	    			if(!strcmp(I.getOpcodeName(), "alloca")){
	    				std::pair<std::string, int> varTempState = std::make_pair(F.getName(),DIRTY);
	    				if (variableState.find(I.getName()) != variableState.end() 
	    					&&  variableState[I.getName()] == varTempState)
	    				{
    						for (User *U : I.users()) {
								if (auto eachUse = dyn_cast<Instruction>(U)) {
								   getAllUses(eachUse,&I);
								}    				
							}
	    				}
	    			}
	    			
	    		}
	    	}
	    }
    	printAllWarnings();
	    return false;
	}
  };
}

void getAllFunctionUses(Function* F, std::set<int> args){
	int i =0;
	if(F->isDeclaration()){
		return;
	}
	for (auto a = F->arg_begin(); a != F->arg_end(); a++)
	{
		if (args.find(i) != args.end())
		{
			// errs() << F->getName() << "\n";
			for (User *U : a->users()) {
				if (auto eachUse = dyn_cast<Instruction>(U)) {
				   getAllUses(eachUse);
				}    				
			}
		}
		i++;
	}
}


void getAllUses(Instruction *I, Instruction *parent_inst ){

	if(!strcmp(I->getOpcodeName(), "call"))
	{
   		for (int i = 0; i < I->getNumOperands()-1; ++i){
   			if (!parent_inst || I->getOperand(i) == parent_inst)
   			{
   				if (functions_call.find(cast<CallInst>(*I).getCalledFunction()) == functions_call.end())
   				{
   					std::set<int> dirty_args;
   					dirty_args.insert(i);
   					// errs() <<*I << " " <<  I->getNumOperands() << " " << *I->getOperand(0) << "\n";
   					functions_call[cast<CallInst>(*I).getCalledFunction()] = dirty_args;
   					getAllFunctionUses(cast<CallInst>(*I).getCalledFunction(), dirty_args);
   				} else {
   					functions_call[cast<CallInst>(*I).getCalledFunction()].insert(i);
   				}
   			}
   		}
	}
	
	if (all_tainted_instructions.find(I) != all_tainted_instructions.end())
	{
		return;
	}
	all_tainted_instructions.insert(I);
	// errs() << *I << "\n";
	for (User *U : I->users()) {
		if (auto eachUse = dyn_cast<Instruction>(U)) {
		   getAllUses(eachUse,I);
		}    				
	}
	
	for (int i = 0; i < I->getNumOperands(); ++i)
	{
   		for (User *U : I->getOperand(i)->users()) {
			if (auto eachUse = dyn_cast<Instruction>(U)) {
			   getAllUses(eachUse,I);
			}    				
		}
	}
}
//Reading Varaible States from a file named variable_state
void ReadFile(){
	std::ifstream file;
	file.open(OutputFilename.c_str());
	if (!file)
	{
		errs() << "Unable to Open File "<< OutputFilename.c_str() << "\n";
	}

	std::string funcName;
	std::string varName;

	std::string temp;
	while(file >> temp){
		if (temp == "func")
		{
			file >> funcName;
		} else {
			varName = temp;
			file >> temp;
			int state = (temp == "dirty") ? 1 : 0;
			variableState[varName] = make_pair(funcName,state);
		}
	}

	file.close();
}		

std::set<std::string> system_calls;
void printAllWarnings(){
	system_calls.insert("_ZN3std2io5stdio6_print17ha644d8a6c5fabe29E");
	system_calls.insert("_ZN4core3fmt10ArgumentV13new17h89028b04c3739a11E");
	system_calls.insert("_ZN4core3fmt10ArgumentV13new17had43cc926454e634E");
	system_calls.insert("_ZN59_$LT$std..net..tcp..TcpStream$u20$as$u20$std..io..Write$GT$5write17ha37c77f5005864d1E");

	for (auto *I : all_tainted_instructions)
	{
		// errs() << *I << "\n";
		if(!strcmp(I->getOpcodeName(), "call") && 
			system_calls.find(cast<CallInst>(*I).getCalledFunction()->getName()) != system_calls.end())
		{
			// errs() << *I << "\n";
			errs() << "warning: " << I->getFunction()->getName() << " has possible information leakage!" << "\n";
		}	
		if(!strcmp(I->getOpcodeName(), "invoke") && 
			system_calls.find(cast<InvokeInst>(*I).getCalledFunction()->getName()) != system_calls.end())
		{
			// errs() << *I << "\n";
			// errs() << I->getFunction()->getName() << " ---- " << cast<CallInst>(*I).getCalledFunction()->getName() << "\n";
			errs() << "warning: " << I->getFunction()->getName() << " has possible information leakage!" << "\n";
		}	
	}
}

//Registering the Pass
char SkeletonPass::ID = 0;

static RegisterPass<SkeletonPass> X("skeleton", "Static Taint Analysis", false,  false);
