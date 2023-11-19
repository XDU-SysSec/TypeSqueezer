#include <stdio.h>
#include <stdint.h> 
#include <limits.h>

#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_process.h"
#include "BPatch_object.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_function.h"
#include "BPatch_point.h"
#include "BPatch_flowGraph.h"
//#include "BPatch_memoryAccessAdapter.h"

#include "PatchCommon.h"
#include "PatchMgr.h"
#include "PatchModifier.h"

#include "Instruction.h"
#include "Operand.h"
#include "Expression.h"
#include "Visitor.h"
#include "Register.h"
#include "BinaryFunction.h"
#include "Immediate.h"
#include "Dereference.h"
// #include "Parsing.h" 
#include "Edge.h"
#include "Symtab.h" 

#include "CFG.h"

#include <string>
#include <set>
#include <vector>
#include <iostream>
#include <algorithm>
#include <fstream>


//lzy 2023
#include <boost/pointer_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/get_pointer.hpp>
#include <boost/detail/lightweight_test.hpp>

using namespace std;
using namespace Dyninst;
using namespace Dyninst::PatchAPI;
using namespace Dyninst::ParseAPI;
using namespace Dyninst::SymtabAPI;
using namespace Dyninst::InstructionAPI;

enum reg_state
{
	R,W,C
};

enum Imm_state
{
	WnoImm,WImm,CW
};

#define X64_ARG 6

map<Address,string> AT_funcs;
map<Address,Address> dcall_AT_pair;
CodeObject *co;
map< Address,vector<int> > backward_cache_width;
map< Address,int > forward_cache_rax;

map<Address , vector<Imm_state>> analyzed_blks_Imm;
map<Address , Imm_state> analyzed_blks_Imm_Ret;

ofstream fout_dcall;
ofstream fout_dcall_ret;

bool operand_is_immediate(InstructionAPI::Operand op)
{
	Immediate::Ptr imm = boost::dynamic_pointer_cast<Immediate,Expression>(op.getValue());
	return imm ? true : false;
}

vector<int> parse_instruction_reverse_width(Instruction::Ptr instr)
{
	vector<int> res(X64_ARG,0);
	int rdi = 0,rsi = 0,rdx = 0,rcx = 0,r8 = 0,r9 = 0;

	Operand op1 = instr->getOperand(0);

	Operation::registerSet regs_written;
	instr->getWriteSet(regs_written);
	for(auto it = regs_written.begin();it != regs_written.end();it++)
	{
		RegisterAST::Ptr reg = *it;
		if(reg->getID().getBaseRegister() == x86_64::rdi)
		{
			res[0] = reg->size();
			if( operand_is_immediate(op1) )
			{
				res[0] = 8;
			}
		}
		else if(reg->getID().getBaseRegister() == x86_64::rsi)
		{
			res[1] = reg->size();
			if( operand_is_immediate(op1) )
			{
				res[1] = 8;
			}
		}
		else if(reg->getID().getBaseRegister() == x86_64::rdx)
		{
			res[2] = reg->size();
			if( operand_is_immediate(op1) )
			{
				res[2] = 8;
			}
		}
		else if(reg->getID().getBaseRegister() == x86_64::rcx)
		{
			res[3] = reg->size();
			if( operand_is_immediate(op1) )
			{
				res[3] = 8;
			}
		}
		else if(reg->getID().getBaseRegister() == x86_64::r8)
		{
			res[4] = reg->size();
			if( operand_is_immediate(op1) )
			{
				res[4] = 8;
			}
		}
		else if(reg->getID().getBaseRegister() == x86_64::r9)
		{
			res[5] = reg->size();
			if( operand_is_immediate(op1) )
			{
				res[5] = 8;
			}
		}
	}

	return res;
}

vector<int> parse_block_reverse_reg_width(ParseAPI::Block *blk,vector<Address> analyzed_blks)
{
	//if this blk has been analyzed(loop detection)
	for(auto it = analyzed_blks.begin();it != analyzed_blks.end();it++)
	{
		if(blk->start() == *it)
		{
			return vector<int>(X64_ARG,8);
		}
	}

	vector<int> res(X64_ARG,0);

	analyzed_blks.push_back(blk->start());

	//get instrs in blk
	ParseAPI::Block::Insns instrs;
	blk->getInsns(instrs);

	//parse each instruction in this basic block in reverse order
	vector< vector<int> > instr_reg_state;
	for(auto instr_it = instrs.rbegin();
	instr_it != instrs.rend();
	instr_it++)
	{
		Dyninst::InstructionAPI::Instruction::Ptr instr = instr_it->second;
		instr_reg_state.push_back(parse_instruction_reverse_width(instr));
	}

	//merge each instruction's reg_state into current block's reg_state
	for(int i = 0;i < instr_reg_state.size();i++)
	{
		vector<int> vec = instr_reg_state[i];
		for(int j = 0;j < X64_ARG;j++)
		{
			if(res[j] == 0)
			{
				res[j] = vec[j];
			}
		}
	}

	// //if r9 is S,then we can return here
	// if(res[5] == reg_state::W)
	// {
	// 	return res;
	// }

	// if current blk is entry blk and has no incoming edges,
	vector<ParseAPI::Function *> funcs;
	blk->getFuncs(funcs);
	ParseAPI::Function *func = funcs[0];
	if(func->entry()->start() == blk->start() && blk->sources().size() == 0)
	{
		return vector<int>(X64_ARG,8);
	}

	//analyze preceded blks
	vector<vector<int>> reg_states;
	for(auto it = blk->sources().begin();it != blk->sources().end();it++)
	{

		ParseAPI::Edge *edge = *it;
		ParseAPI::Block *source_blk;
		//if RET,stop analysis and return(there may be other incoming edges,but we can ignore them)
		if(edge->type() == RET)
		{
			return res;
		}

		//same as RET
		else if(edge->type() == CALL_FT)
		{
			return res;
		}

		//if CALL,continue analysis in each call that call current function
		else if(edge->type() == CALL)
		{
			source_blk = edge->src();
		}

		else
		{
			source_blk = edge->src();
		}

		//speed up analysis
		if(backward_cache_width.count(source_blk->start()))
		{
			reg_states.push_back( backward_cache_width[source_blk->start()] );
		}
		else
		{
			backward_cache_width[source_blk->start()] = parse_block_reverse_reg_width(source_blk,analyzed_blks);
			reg_states.push_back( backward_cache_width[source_blk->start()] );
		}

	}

	/*combine and merge
	如果当前blk中寄存器状态已知，即为此状态
	否则，合并之前基本块的寄存器状态并将其当作当前基本块中的寄存器状态
	合并规则为：有0则为0,否则取最大整数
	*/
	for(int i = 0;i < X64_ARG;i++)
	{
		//当前基本块中寄存器状态未知
		if(res[i] == 0)
		{
			int state = 0;
			for(auto states_it = reg_states.begin();states_it != reg_states.end();states_it++)
			{
				vector<int> states = *states_it;
				if(states[i] == 0)
				{
					state = 0;
					break;
				}
				else
				{
					if(states[i] > state)
					{
						state = states[i];
					}
				}
			}
			res[i] = state;
		}
	}

	return res;

}




vector<bool> checkXor(Block* blk){
	ParseAPI::Block::Insns instrs;
	blk->getInsns(instrs);

	//parse each instruction in this basic block in reverse order
	vector< vector<int> > instr_reg_state;
	int flag = 0;
	vector<bool> res(X64_ARG , false);
	for(auto instr_it = instrs.rbegin();
	instr_it != instrs.rend();
	instr_it++)
	{
		Dyninst::InstructionAPI::Instruction::Ptr instr = instr_it->second;
		if(instr->format().find("xor") != -1){
			flag = 1;
		}
		if(flag == 1){
			Operation::registerSet regWritenSet;
			instr->getWriteSet(regWritenSet);
			for(auto reg : regWritenSet){
				if(reg->getID().getBaseRegister() == x86_64::rdi)
				{
					res[0] = true;
				}
				else if(reg->getID().getBaseRegister() == x86_64::rsi)
				{
					res[1] = true;
				}
				else if(reg->getID().getBaseRegister() == x86_64::rdx)
				{
					res[2] = true;
				}
				else if(reg->getID().getBaseRegister() == x86_64::rcx)
				{
					res[3] = true;
				}
				else if(reg->getID().getBaseRegister() == x86_64::r8)
				{
					res[4] = true;
				}
				else if(reg->getID().getBaseRegister() == x86_64::r9)
				{
					res[5] = true;
				}
			}
		}
	}
	return res;
}


vector<Imm_state> checkImm(Block* blk){
	if(analyzed_blks_Imm.find(blk->start()) != analyzed_blks_Imm.end()){
		return analyzed_blks_Imm.find(blk->start())->second;
	}

	Block::Insns instrs;
	blk->getInsns(instrs);

	vector<Imm_state> stateImm(X64_ARG , CW);
	for(auto it = instrs.rbegin() ; it != instrs.rend() ; it++){
		Instruction::Ptr inst = it->second;
		if(inst->format().find("$") != -1){
			Operation::registerSet regWritenSet;
			for(auto reg : regWritenSet){
				if(reg->getID().getBaseRegister() == x86_64::rdi){
					if(stateImm[0] == Imm_state::CW){
						stateImm[0] == Imm_state::WImm;
					}
				}
				if(reg->getID().getBaseRegister() == x86_64::rsi){
					if(stateImm[1] == Imm_state::CW){
						stateImm[1] == Imm_state::WImm;
					}
				}				
				if(reg->getID().getBaseRegister() == x86_64::rdx){
					if(stateImm[2] == Imm_state::CW){
						stateImm[2] == Imm_state::WImm;
					}
				}
				if(reg->getID().getBaseRegister() == x86_64::rcx){
					if(stateImm[3] == Imm_state::CW){
						stateImm[3] == Imm_state::WImm;
					}
				}
				if(reg->getID().getBaseRegister() == x86_64::r8){
					if(stateImm[4] == Imm_state::CW){
						stateImm[4] == Imm_state::WImm;
					}
				}
				if(reg->getID().getBaseRegister() == x86_64::r9){
					if(stateImm[5] == Imm_state::CW){
						stateImm[5] == Imm_state::WImm;
					}
				}
			}
		}else{
			Operation::registerSet regWritenSet;
			for(auto reg : regWritenSet){
				if(reg->getID().getBaseRegister() == x86_64::rdi){
					if(stateImm[0] == Imm_state::CW){
						stateImm[0] == Imm_state::WnoImm;
					}
				}
				if(reg->getID().getBaseRegister() == x86_64::rsi){
					if(stateImm[1] == Imm_state::CW){
						stateImm[1] == Imm_state::WnoImm;
					}
				}				
				if(reg->getID().getBaseRegister() == x86_64::rdx){
					if(stateImm[2] == Imm_state::CW){
						stateImm[2] == Imm_state::WnoImm;
					}
				}
				if(reg->getID().getBaseRegister() == x86_64::rcx){
					if(stateImm[3] == Imm_state::CW){
						stateImm[3] == Imm_state::WnoImm;
					}
				}
				if(reg->getID().getBaseRegister() == x86_64::r8){
					if(stateImm[4] == Imm_state::CW){
						stateImm[4] == Imm_state::WnoImm;
					}
				}
				if(reg->getID().getBaseRegister() == x86_64::r9){
					if(stateImm[5] == Imm_state::CW){
						stateImm[5] == Imm_state::WnoImm;
					}
				}
			}
		}
	}

	vector<ParseAPI::Function*> funcs;
	blk->getFuncs(funcs);
	ParseAPI::Function* func = funcs[0];
	if(func->entry()->start() == blk->start() && blk->sources().size() == 0)
	{
		analyzed_blks_Imm.insert(pair<Address , vector<Imm_state>>(blk->start() , stateImm));
		return stateImm;
	}

	vector<vector<Imm_state>> Imm_states;
	for(auto edge : blk->sources()){
		ParseAPI::Block* blkToAnalyze;
		if(edge->type() == CALL_FT) continue;
		if(edge->type() == CALL){
			blkToAnalyze == edge->src();
			Imm_states.push_back(checkImm(edge->src()));
		}
	}
	for(int i = 0 ; i < 6 ; i++){
		if(stateImm[i] == CW){
			for(auto it : Imm_states){
				if(it[i] == WnoImm){
					stateImm[i] =WnoImm;
					break;
				}else if(it[i] == Imm_state::CW){
					stateImm[i] = Imm_state::CW;
					break;
				}else{
					stateImm[i] = WImm;
				}
			}
		}
	}
	analyzed_blks_Imm.insert(pair<Address , vector<Imm_state>>(blk->start() , stateImm));
	return stateImm;
}


void parse_icall_reg_width(Block *blk)
{
	backward_cache_width.clear();
	vector<Address> analyzed_blks;
	vector<int> res = parse_block_reverse_reg_width(blk,analyzed_blks);

	vector<bool> resXor = checkXor(blk);

	for(int i = 0 ; i < 6 ; i++){
		if(resXor[i] == true){
			res[i] = 8;
		}
	}

	fout_dcall<<hex<<blk->lastInsnAddr()<<'\t'<<dcall_AT_pair[blk->lastInsnAddr()]<<'\t';
	for(auto it = res.begin();it != res.end();it++)
	{
		fout_dcall<<*it;
	}
	fout_dcall<<'\n';
}

void read_AT_funcs(string fname)
{
	std::ifstream fin(fname,ios::in);
	if(fin)
	{
		string str;
		while(getline(fin,str))
		{
			string func_entry_addr = str.substr(0,6);
			Address AT_addr = (Address)stoi(func_entry_addr,0,16);
			string AT_name = str.substr(9);
			if(AT_funcs.count(AT_addr) == 0)
			{
				AT_funcs[AT_addr] = AT_name;
			}
		}
	}
	else
	{
		cout<<"open file(funcs) err"<<endl;
		exit(0);
	}
	fin.close();
}

void find_dcall_to_AT(string fname)
{
	std::ifstream fin(fname,ios::in);
	if(fin)
	{
		string str;
		while(getline(fin,str))
		{
			//本行包含直接call
			if(str.find("callq") != string::npos && str.find("*") == string::npos)
			{
				Address dcall_addr = stoi(str.substr(2,6),0,16);
				int id1 = str.find_first_of('<');
				string called_func = str.substr(id1 - 7,6);
				if(AT_funcs.count( (Address)stoi(called_func,0,16) ) != 0 )
				{
					dcall_AT_pair[dcall_addr] = (Address)stoi(called_func,0,16);
				}	
			}
		}
	}
	else
	{
		cout<<"open file(funcs) err"<<endl;
		exit(0);
	}
	fin.close();
}

set<ParseAPI::Block *> get_all_dcall_blks(string bin)
{
	Symtab *symtab	= Symtab::findOpenSymtab(string(bin.c_str()));
	vector<SymtabAPI::relocationEntry> fbt;
	symtab->getFuncBindingTable(fbt);
	Address addr = fbt.rbegin()->target_addr();



	set<ParseAPI::Block *> res;
	const CodeObject::funclist &all_functions = co->funcs();
	for(auto func_it = all_functions.begin();func_it != all_functions.end();func_it++)
	{
		ParseAPI::Function *func = *func_it;
		ParseAPI::Function::blocklist blks = func->blocks();
		for(auto blk_it = blks.begin();blk_it != blks.end();blk_it++)
		{
			ParseAPI::Block *blk = *blk_it;
			ParseAPI::Block::edgelist edges = blk->targets();
			for(auto edge_it = edges.begin();edge_it != edges.end();edge_it++)
			{
				ParseAPI::Edge *edge = *edge_it;
				//indirect call
				if(edge->type() == CALL && edge->trg()->start() != (Address)-1)
				{
					if(edge->trg()->start() > addr){
						res.insert(blk);
					}
				}
			}
		}
	}
	return res;
}

int parse_instruction_rax(Instruction::Ptr instr)
{
	int res = 0;
	Operation::registerSet regs_read;
	instr->getReadSet(regs_read);
	for(auto it = regs_read.begin();it != regs_read.end();it++)
	{
		RegisterAST::Ptr reg = *it;
		if(reg->getID().getBaseRegister() == x86_64::rax)
		{
			res = reg->getID().size();
		}
	}

	Operation::registerSet regs_written;
	instr->getWriteSet(regs_written);
	for(auto it = regs_written.begin();it != regs_written.end();it++)
	{
		RegisterAST::Ptr reg = *it;
		if(reg->getID().getBaseRegister() == x86_64::rax)
		{
			res = -1;
		}
	}
	return res;
}

int parse_block_rax_width(ParseAPI::Block *blk,
std::vector<Dyninst::Address> analyzed_blks)
{
	//if blk has been analyzed(loop detected)
	for(auto blk_it = analyzed_blks.begin();blk_it != analyzed_blks.end();blk_it++)
	{
		
		Dyninst::Address start_addr = *blk_it;
		if(start_addr == blk->start())
		{
			return 0;
		}
	}
	int res = 0;

	//add blk to analyzed_blks
	analyzed_blks.push_back(blk->start());

	//analyze current blk
	ParseAPI::Block::Insns instrs;
	blk->getInsns(instrs);

	//parse each instruction in this basic block in order
	vector<int> instr_reg_state;
	for(auto instr_it = instrs.begin();
	instr_it != instrs.end();
	instr_it++)
	{
		Dyninst::InstructionAPI::Instruction::Ptr instr = instr_it->second;
		int tmp = parse_instruction_rax(instr);
		//将这条指令的分析结果记录
		instr_reg_state.push_back(tmp);
	}

	//merge each instruction's reg_state into current block's reg_state
	for(auto sit : instr_reg_state)
	{
		if(sit != 0)
		{
			return sit;
		}
	}

	//analyze current blk's successive blks
	vector<int> reg_states;
	ParseAPI::Block *target_blk;
	for(auto edge_it = blk->targets().begin();edge_it != blk->targets().end();edge_it++)
	{
		ParseAPI::Edge *edge = *edge_it;

		//intra-procedual
		if(edge->type() == RET)
		{
			
			return res;
		}
		//intra-procedual
		else if(edge->type() == CALL)
		{
			return res;
		}
		//intra-procedual
		else if(edge->type() == CALL_FT)
		{
			return res;
		}
		else
		{
			target_blk = edge->trg();
		}

		//可能是ijump？
		if(target_blk->start() == (Address)-1)
		{
			return res;
		}

		//如果target_blk不在当前函数中
		vector<ParseAPI::Function *> cur_funcs,trg_funcs;
		blk->getFuncs(cur_funcs);
		target_blk->getFuncs(trg_funcs);
		if(cur_funcs.size() == 0 || trg_funcs.size() == 0)
		return res;
		if(cur_funcs[0] != trg_funcs[0])
		{
			return res;
		}

		//if we have analyzed this block,use the previous result
		if(forward_cache_rax.count(target_blk->start()))
		{
			reg_states.push_back(forward_cache_rax[target_blk->start()]);
		}
		else
		{
			forward_cache_rax[target_blk->start()] = parse_block_rax_width(target_blk,analyzed_blks);
			reg_states.push_back(forward_cache_rax[target_blk->start()]);
		}
	}

	//combine and path merging
	for(auto it : reg_states)
	{
		if(it <= 0)
		{
			return -1;
		}
		else
		{
			if(res == 0)
			{
				res = it;
			}
			else if(it < res)
			{
				res = it;
			}
		}
	}
	return res;
}





//获取icall返回值宽度
void get_icall_retval_width(Block *blk)
{
    forward_cache_rax.clear();
    Block *icall_next_blk = co->findBlockByEntry(blk->region(),blk->end());
    if(icall_next_blk == nullptr)
    {
        fout_dcall_ret<<hex<<blk->lastInsnAddr()<<'\t'<<dec<<(-1)<<'\n';
    }
    else
    {
        vector<Address> analyzed_blks;
        int res = parse_block_rax_width(icall_next_blk,analyzed_blks);
        fout_dcall_ret<<hex<<blk->lastInsnAddr()<<'\t'<<dec<<res<<'\n';
    }
 
}

int main(int argc,char **argv)
{
    BPatch bpatch;
    BPatch_addressSpace *handle;
    SymtabCodeSource *sts;
	
	CodeRegion *cr;

	string binfile = string(argv[1]);
	read_AT_funcs("../../at/" + binfile + ".at");
	find_dcall_to_AT("../../dump/" + binfile + ".dump");

	fout_dcall.open(binfile + "_dcall");
	fout_dcall_ret.open(binfile + "_dcall_ret");

    handle = bpatch.openBinary(("../../bin/" + binfile).c_str() ,true);

	std::vector<BPatch_object*> objs;
	handle->getImage()->getObjects(objs);
	assert(objs.size() > 0);
    
    string bin = objs[0]->pathName();

	// Create a new binary object 
	sts = new SymtabCodeSource((char*)bin.c_str());
	co = new CodeObject(sts);

	// Parse the binary 
	co->parse();
	const CodeObject::funclist &all_functions = co->funcs();

	//分析所有对AT的直接调用
	set<ParseAPI::Block *> dcall_blks = get_all_dcall_blks(bin);
	for(auto *dcall_blk_it : dcall_blks)
	{
		ParseAPI::Block *blk = dcall_blk_it;
		if(dcall_AT_pair.count( blk->lastInsnAddr() ) != 0 )
		{
			parse_icall_reg_width(blk);
			get_icall_retval_width(blk);
		}
	}
}
