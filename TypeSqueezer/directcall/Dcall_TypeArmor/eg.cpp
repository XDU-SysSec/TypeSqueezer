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

#define X64_ARG 6

CodeObject *co;
map<Address,string> AT_funcs;
map<Address,Address> dcall_AT_pair;
map< Address,vector<reg_state> > backward_cache;
map< Address,reg_state > forward_cache_rax;

ofstream fout_dcall,fout_dcall_ret;

//used in callsite analysis,S is set,T is not set(we could use reg_state::W to represent S and reg_state::C to represent T)
vector<reg_state> parse_instruction_ST(InstructionAPI::Instruction::Ptr instr)
{
	//defalut state is reg_state::C
	vector<reg_state> res(X64_ARG,reg_state::C);

	set<Dyninst::InstructionAPI::RegisterAST::Ptr> regs_write;

	instr->getWriteSet(regs_write);

	//rdi
	for(auto reg_it = regs_write.begin();reg_it != regs_write.end();reg_it++)
	{
		Dyninst::InstructionAPI::RegisterAST::Ptr reg = *reg_it;
		if(reg->getID().name() == string("x86_64::rdi") || reg->getID().name() == string("x86_64::edi") || reg->getID().name() == string("x86_64::di") || reg->getID().name() == string("x86_64::dil"))
		{
			res[0] = reg_state::W;
			break;
		}
	}


	//rsi
	for(auto reg_it = regs_write.begin();reg_it != regs_write.end();reg_it++)
	{
		Dyninst::InstructionAPI::RegisterAST::Ptr reg = *reg_it;
		if(reg->getID().name() == string("x86_64::rsi") || reg->getID().name() == string("x86_64::esi") || reg->getID().name() == string("x86_64::si") || reg->getID().name() == string("x86_64::sil"))
		{
			res[1] = reg_state::W;
			break;
		}
	}

	//rdx
	for(auto reg_it = regs_write.begin();reg_it != regs_write.end();reg_it++)
	{
		Dyninst::InstructionAPI::RegisterAST::Ptr reg = *reg_it;
		if(reg->getID().name() == string("x86_64::rdx") || reg->getID().name() == string("x86_64::edx") || reg->getID().name() == string("x86_64::dx") || reg->getID().name() == string("x86_64::dl"))
		{
			res[2] = reg_state::W;
			break;
		}
	}

	//rcx
	for(auto reg_it = regs_write.begin();reg_it != regs_write.end();reg_it++)
	{
		Dyninst::InstructionAPI::RegisterAST::Ptr reg = *reg_it;
		if(reg->getID().name() == string("x86_64::rcx") || reg->getID().name() == string("x86_64::ecx") || reg->getID().name() == string("x86_64::cx") || reg->getID().name() == string("x86_64::cl"))
		{
			res[3] = reg_state::W;
			break;
		}
	}

	//r8
	for(auto reg_it = regs_write.begin();reg_it != regs_write.end();reg_it++)
	{
		Dyninst::InstructionAPI::RegisterAST::Ptr reg = *reg_it;
		if(reg->getID().name() == string("x86_64::r8") || reg->getID().name() == string("x86_64::r8d") || reg->getID().name() == string("x86_64::r8w") || reg->getID().name() == string("x86_64::r8b"))
		{
			res[4] = reg_state::W;
			break;
		}
	}

	//r9
	for(auto reg_it = regs_write.begin();reg_it != regs_write.end();reg_it++)
	{
		Dyninst::InstructionAPI::RegisterAST::Ptr reg = *reg_it;
		if(reg->getID().name() == string("x86_64::r9") || reg->getID().name() == string("x86_64::r9d") || reg->getID().name() == string("x86_64::r9w") || reg->getID().name() == string("x86_64::r9b"))
		{
			res[5] = reg_state::W;
			break;
		}
	}

	return res;
}

vector<reg_state> parse_block_reverse(ParseAPI::Block *blk,vector<Address> analyzed_blks)
{
	//if this blk has been analyzed(loop detection)
	for(auto it = analyzed_blks.begin();it != analyzed_blks.end();it++)
	{
		if(blk->start() == *it)
		{
			return vector<reg_state>(X64_ARG,reg_state::W);
		}
	}

	vector<reg_state> res = vector<reg_state>(X64_ARG,reg_state::C);

	analyzed_blks.push_back(blk->start());

	//get instrs in blk
	ParseAPI::Block::Insns instrs;
	blk->getInsns(instrs);

	//parse each instruction in this basic block in reverse order
	vector< vector<reg_state> > instr_reg_state;
	for(auto instr_it = instrs.rbegin();
	instr_it != instrs.rend();
	instr_it++)
	{
		Dyninst::InstructionAPI::Instruction::Ptr instr = instr_it->second;
		instr_reg_state.push_back(parse_instruction_ST(instr));
	}

	//merge each instruction's reg_state into current block's reg_state
	for(int i = 0;i < instr_reg_state.size();i++)
	{
		vector<reg_state> vec = instr_reg_state[i];
		for(int j = 0;j < X64_ARG;j++)
		{
			if(res[j] == reg_state::C)
			{
				res[j] = vec[j];
			}
		}
	}

	// //if r9 is S,then we can return here
	if(res[5] == reg_state::W)
	{
		return res;
	}

	// if current blk is entry blk and has no incoming edges,
	vector<ParseAPI::Function *> funcs;
	blk->getFuncs(funcs);
	ParseAPI::Function *func = funcs[0];
	if(func->entry()->start() == blk->start() && blk->sources().size() == 0)
	{
		return vector<reg_state>(X64_ARG,reg_state::W);
	}

	//analyze preceded blks
	vector<vector<reg_state>> reg_states;
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
		if(backward_cache.count(source_blk->start()))
		{
			reg_states.push_back( backward_cache[source_blk->start()] );
		}
		else
		{
			backward_cache[source_blk->start()] = parse_block_reverse(source_blk,analyzed_blks);
			reg_states.push_back( backward_cache[source_blk->start()] );
		}

	}

	/*combine and merge path
	only when current blk's state is T,we look at preceded blks
	only if all preceded blks' state is W,state is W
	*/
	for(int i = 0;i < X64_ARG;i++)
	{
		if(res[i] == reg_state::C)
		{
			for(auto states_it = reg_states.begin();states_it != reg_states.end();states_it++)
			{
				vector<reg_state> states = *states_it;
				res[i] = states[i];
				if(states[i] != reg_state::W)
				{
					break;
				}
			}
		}
	}

	return res;

}

void get_dcall_args(ParseAPI::Block *blk)
{
	vector<ParseAPI::Function *> funcs;
	blk->getFuncs(funcs);
	ParseAPI::Function *func = funcs[0];
	vector<Address> analyzed_blks;
	vector<reg_state> res = parse_block_reverse(blk,analyzed_blks);
	fout_dcall<<hex<<blk->lastInsnAddr()<<'\t'<<dcall_AT_pair[blk->lastInsnAddr()]<<'\t';
	for(auto it : res)
	{
		if(it == reg_state::R)
		{
			fout_dcall<<'R';
		}
		else if(it == reg_state::W)
		{
			fout_dcall<<'W';
		}
		if(it == reg_state::C)
		{
			fout_dcall<<'C';
		}
	}
	fout_dcall<<'\n';
}

void read_AT_funcs(string fname)
{
	std::ifstream fin(fname,ios::in);	//analyze result/nginx-0.8.54(real)/nginx-0.8_at");
	if(fin)
	{
		string str;
		while(getline(fin,str))
		{
			string func_entry_addr = str.substr(0,6);
			Address AT_addr = stoi(func_entry_addr,0,16);
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
	std::ifstream fin(fname,ios::in);	//analyze result/nginx-0.8.54(real)/nginx-0.8_at");
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

//return a reg_state that indicate rax's state
reg_state parse_instruction_rax(Dyninst::InstructionAPI::Instruction::Ptr instr)
{
	//defalut state is reg_state::C
	reg_state res = reg_state::C;

	set<Dyninst::InstructionAPI::RegisterAST::Ptr> regs_read;
	set<Dyninst::InstructionAPI::RegisterAST::Ptr> regs_write;

	instr->getReadSet(regs_read);
	instr->getWriteSet(regs_write);

	//rax
	bool rax_is_read = false;
	bool rax_is_write = false;
	for(auto reg_it = regs_read.begin();reg_it != regs_read.end();reg_it++)
	{
		
		Dyninst::InstructionAPI::RegisterAST::Ptr reg = *reg_it;
		if(reg->getID().name() == string("x86_64::rax") || reg->getID().name() == string("x86_64::eax") || reg->getID().name() == string("x86_64::ax") || reg->getID().name() == string("x86_64::al"))
		{
			res = reg_state::R;
			rax_is_read = true;
			break;
		}
	}
	for(auto reg_it = regs_write.begin();reg_it != regs_write.end();reg_it++)
	{
		Dyninst::InstructionAPI::RegisterAST::Ptr reg = *reg_it;
		if(reg->getID().name() == string("x86_64::rax") || reg->getID().name() == string("x86_64::eax") || reg->getID().name() == string("x86_64::ax") || reg->getID().name() == string("x86_64::al"))
		{
			res = reg_state::W;
			rax_is_write = true;
			break;
		}
	}
	if(rax_is_read & rax_is_write)
	{
		res = reg_state::W;
	}
	return res;

}

//analysis is intra-procedual
reg_state parse_block_rax(ParseAPI::Block *blk,
std::vector<Dyninst::Address> analyzed_blks,
std::vector<Dyninst::Address>fts)	//fts:fall_through_stack(using Dyninst::Address to identify basic block)
{

	//if blk has been analyzed(loop detected)
	for(auto blk_it = analyzed_blks.begin();blk_it != analyzed_blks.end();blk_it++)
	{
		
		Dyninst::Address start_addr = *blk_it;
		if(start_addr == blk->start())
		{
			return reg_state::C;
		}
	}

	reg_state res = reg_state::C;

	//add blk to analyzed_blks
	analyzed_blks.push_back(blk->start());

	/*analyze current blk
	if current blk is R or W,analysis stop
	otherwise,analysis continue in successive blks
	*/
	ParseAPI::Block::Insns instrs;
	blk->getInsns(instrs);

	//parse each instruction in this basic block in order
	vector<reg_state> instr_reg_state;
	for(auto instr_it = instrs.begin();
	instr_it != instrs.end();
	instr_it++)
	{
		Instruction::Ptr instr = instr_it->second;
		reg_state tmp = parse_instruction_rax(instr);
		instr_reg_state.push_back(tmp);
	}

	//merge each instruction's reg_state into current block's reg_state
	for(auto sit : instr_reg_state)
	{
		if(sit != reg_state::C)
		{
			return sit;
		}
	}

	//analyze current blk's successive blks
	vector<reg_state> reg_states;
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
			forward_cache_rax[target_blk->start()] = parse_block_rax(target_blk,analyzed_blks,fts);
			reg_states.push_back(forward_cache_rax[target_blk->start()]);
		}
	}

	/*combine and path merging
	only when current blk's state is C,we look at successive blks' state
	only if all successive blks' state is R,state is R
	*/
	for(auto sit : reg_states)
	{
		if(sit != reg_state::R)
		{
			return reg_state::W;
		}
	}

	return reg_state::R;
}

void icall_is_nonvoid(Block *blk)
{
	Block *icall_next_blk = co->findBlockByEntry(blk->region(),blk->end());

	if(icall_next_blk == nullptr)
	{
		fout_dcall_ret<<hex<<blk->lastInsnAddr()<<'\t'<<dcall_AT_pair[blk->lastInsnAddr()]<<'\t'<<"\tnot nonvoid\n";
	}
	else
	{
		vector<Address> analyzed_blks,fts;
		reg_state res = parse_block_rax(icall_next_blk,analyzed_blks,fts);

		vector<ParseAPI::Function *> funcs;
		blk->getFuncs(funcs);
		ParseAPI::Function *func = funcs[0];
		fout_dcall_ret<<hex<<blk->lastInsnAddr()<<'\t'<<dcall_AT_pair[blk->lastInsnAddr()]<<'\t';

		if(res == reg_state::R)
		{
			fout_dcall_ret<<"\tnonvoid\n";
		}
		else
		{
			fout_dcall_ret<<"\tnot nonvoid\n";
		}
		return;
	}	
}

int main(int argc , char* argv[])
{
    BPatch bpatch;
    BPatch_addressSpace *handle;
    SymtabCodeSource *sts;
	
	CodeRegion *cr;

	string binFileName = argv[1];


	read_AT_funcs(("../../at/" + binFileName + ".at").c_str());
	find_dcall_to_AT(("../../dump/" + binFileName + ".dump").c_str());

	fout_dcall.open((binFileName +  "_dcall").c_str());
	fout_dcall_ret.open((binFileName + "_dcall_ret").c_str());

    handle = bpatch.openBinary(("../../bin/" + binFileName).c_str(),true);
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
			get_dcall_args(blk);
			icall_is_nonvoid(blk);
		}
	}

	exit(0);
}
