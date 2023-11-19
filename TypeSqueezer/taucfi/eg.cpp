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

enum Imm_state{
	WnoImm , WImm , CW
};

#define X64_ARG 6

CodeObject *co;
vector<Address> plt_funcs;
Address exit_addr;		//exit@plt is diffenrent from other plt entries(a call to it will nenver return)
// map< Address,vector<reg_state> > forward_cache;
// map< Address,vector<reg_state> > backward_cache;
map< Address,vector<int> > forward_cache_width;
map< Address,vector<int> > backward_cache_width;
map< Address,int> forward_cache_rax;
map< Address,int> backward_cache_rax;


map<Address , vector<Imm_state>> analyzed_blks_Imm;
map<Address , Imm_state> analyzed_blks_Imm_Ret;



std::ofstream fout_icall;
std::ofstream fout_AT_funcs;
ofstream fout_icall_ret;
ofstream fout_AT_ret;


Imm_state checkImmRet(Block* blk){
	// cout<<hex<<"checkImm\t"<<blk->start()<<endl;
	if(analyzed_blks_Imm_Ret.find(blk->start()) != analyzed_blks_Imm_Ret.end()){
		return analyzed_blks_Imm_Ret.find(blk->start())->second;
	}

	Block::Insns instrs;
	blk->getInsns(instrs);

	Imm_state stateImm = Imm_state::CW;
	for(auto it = instrs.rbegin() ; it != instrs.rend() ; it++){
		Instruction::Ptr inst = it->second;
		if(inst->format().find("$") != -1){
			Operation::registerSet regWritenSet;
			for(auto reg : regWritenSet){
				if(reg->getID().getBaseRegister() == x86_64::rax){
					if(stateImm == Imm_state::CW){
						stateImm == Imm_state::WImm;
					}
				}
			}
		}else{
			Operation::registerSet regWritenSet;
			for(auto reg : regWritenSet){
				if(reg->getID().getBaseRegister() == x86_64::rax){
					if(stateImm == Imm_state::CW){
						stateImm == Imm_state::WnoImm;
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
		analyzed_blks_Imm_Ret.insert(pair<Address , Imm_state>(blk->start() , stateImm));
		return stateImm;
	}

	vector<Imm_state> Imm_states;
	for(auto edge : blk->sources()){
		ParseAPI::Block* blkToAnalyze;
		if(edge->type() == CALL_FT) continue;
		if(edge->type() == CALL){
			blkToAnalyze == edge->src();
			// cout<<123<<endl;
			// cout<<hex<<edge->src()->start()<<'\t'<<edge->trg()->start()<<endl;
			// cout<<edge->src()->start()<<endl;
			Imm_states.push_back(checkImmRet(edge->src()));
		}
	}
	if(stateImm == CW){
		for(auto it : Imm_states){
			if(it == WnoImm){
				stateImm =WnoImm;
				break;
			}else if(it == Imm_state::CW){
				stateImm = Imm_state::CW;
				break;
			}else{
				stateImm = WImm;
			}
		}
	}
	analyzed_blks_Imm_Ret.insert(pair<Address , Imm_state>(blk->start() , stateImm));
	// cout<<blk->start();
	// for(int i = 0 ; i < 6 ; i ++){
	// 	cout<<stateImm[i]<<'\t';
	// }
	// cout<<endl;
	return stateImm;




}



bool checkXorRet(Block* blk){
	ParseAPI::Block::Insns instrs;
	blk->getInsns(instrs);

	//parse each instruction in this basic block in reverse order
	int flag = 0;
	bool res = false;
	for(auto instr_it = instrs.begin();
	instr_it != instrs.end();
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
				if(reg->getID().getBaseRegister() == x86_64::rax)
				{
					res = true;
				}
			}
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
	// cout<<hex<<"checkImm\t"<<blk->start()<<endl;
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
			// cout<<123<<endl;
			// cout<<hex<<edge->src()->start()<<'\t'<<edge->trg()->start()<<endl;
			// cout<<edge->src()->start()<<endl;
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
	// cout<<blk->start();
	// for(int i = 0 ; i < 6 ; i ++){
	// 	cout<<stateImm[i]<<'\t';
	// }
	// cout<<endl;
	return stateImm;
}

set<ParseAPI::Block *> get_all_icall_blks()
{
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
				if(edge->type() == CALL && edge->trg()->start() == (Address)-1)
				{
					res.insert(blk);
				}
			}
		}
	}

	return res;
}

void parse_plt_entries(string bin)
{
	Symtab *symtab	= Symtab::findOpenSymtab(string(bin.c_str()));
	vector<SymtabAPI::relocationEntry> fbt;
	symtab->getFuncBindingTable(fbt);

	for(auto it = fbt.begin();it != fbt.end();it++)
	{
		SymtabAPI::relocationEntry plt_enrty = *it;

		if(plt_enrty.name() == string("exit"))
		{
			exit_addr = plt_enrty.target_addr();
		}

		co->parse((Address)((*it).target_addr()), true);
		plt_funcs.push_back( (Address)(*it).target_addr() );
	}
}

vector<int> parse_instruction_width(Instruction::Ptr instr)
{
	vector<int> res(X64_ARG,0);
	int rdi = 0,rsi = 0,rdx = 0,rcx = 0,r8 = 0,r9 = 0;

	Operation::registerSet regs_read;
	instr->getReadSet(regs_read);
	for(auto it = regs_read.begin();it != regs_read.end();it++)
	{
		RegisterAST::Ptr reg = *it;
		if(reg->getID().getBaseRegister() == x86_64::rdi)
		{
			res[0] = reg->getID().size();
		}
		else if(reg->getID().getBaseRegister() == x86_64::rsi)
		{
			res[1] = reg->getID().size();
		}
		else if(reg->getID().getBaseRegister() == x86_64::rdx)
		{
			res[2] = reg->getID().size();
		}
		else if(reg->getID().getBaseRegister() == x86_64::rcx)
		{
			res[3] = reg->getID().size();
		}
		else if(reg->getID().getBaseRegister() == x86_64::r8)
		{
			res[4] = reg->getID().size();
		}
		else if(reg->getID().getBaseRegister() == x86_64::r9)
		{
			res[5] = reg->getID().size();
		}
	}

	Operation::registerSet regs_written;
	instr->getWriteSet(regs_written);
	for(auto it = regs_written.begin();it != regs_written.end();it++)
	{
		RegisterAST::Ptr reg = *it;
		if(reg->getID().getBaseRegister() == x86_64::rdi)
		{
			res[0] = -1;
		}
		else if(reg->getID().getBaseRegister() == x86_64::rsi)
		{
			res[1] = -1;
		}
		else if(reg->getID().getBaseRegister() == x86_64::rdx)
		{
			res[2] = -1;
		}
		else if(reg->getID().getBaseRegister() == x86_64::rcx)
		{
			res[3] = -1;
		}
		else if(reg->getID().getBaseRegister() == x86_64::r8)
		{
			res[4] = -1;
		}
		else if(reg->getID().getBaseRegister() == x86_64::r9)
		{
			res[5] = -1;
		}
	}

	return res;
}

//-1代表写，0代表没有读写，8、16、32、64代表读的位数
vector<int> parse_block_reg_width(ParseAPI::Block *blk,
std::vector<Dyninst::Address> analyzed_blks,
std::vector<Dyninst::Address>fts)	//fts:fall_through_stack(using Dyninst::Address to identify basic block)
{

	std::vector<Dyninst::ParseAPI::Function *> f;
	blk->getFuncs(f);

	//if blk has been analyzed(loop detected)
	for(auto blk_it = analyzed_blks.begin();blk_it != analyzed_blks.end();blk_it++)
	{
		
		Dyninst::Address start_addr = *blk_it;
		if(start_addr == blk->start())
		{
			return vector<int>(X64_ARG,0);
		}
	}
	vector<int> res(X64_ARG,0);

	//add blk to analyzed_blks
	analyzed_blks.push_back(blk->start());

	/*analyze current blk
	if current blk is R or W,analysis stop
	otherwise,analysis continue in successive blks
	*/
	ParseAPI::Block::Insns instrs;
	blk->getInsns(instrs);

	//parse each instruction in this basic block in order
	vector< vector<int> > instr_reg_state;
	for(auto instr_it = instrs.begin();
	instr_it != instrs.end();
	instr_it++)
	{
		Dyninst::InstructionAPI::Instruction::Ptr instr = instr_it->second;
		vector<int> tmp = parse_instruction_width(instr);
		//任意一个寄存器状态为读或写，将这条指令的分析结果记录
		for(auto it = tmp.begin();it != tmp.end();it++)
		{
			if(*it != 0)
			{
				instr_reg_state.push_back(tmp);
				break;
			}
		}
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


	//analyze current blk's successive blks
	vector< vector<int> > reg_states;
	ParseAPI::Block *target_blk;
	for(auto edge_it = blk->targets().begin();edge_it != blk->targets().end();edge_it++)
	{
		
		ParseAPI::Edge *edge = *edge_it;
		if(edge->type() == RET)
		{
			
			if(fts.empty())
			{
				return res;
			}
			target_blk = co->findBlockByEntry(blk->region(),fts.back());
			fts.pop_back();
			if(forward_cache_width.count(target_blk->start()))
			{
				reg_states.push_back(forward_cache_width[target_blk->start()]);
			}
			else
			{
				forward_cache_width[target_blk->start()] = parse_block_reg_width(target_blk,analyzed_blks,fts);
				reg_states.push_back(forward_cache_width[target_blk->start()]);
			}
			break;
		}
		else if(edge->type() == CALL)
		{
			//if indirect call,to be conservative,we should assume all regs are W
			if(edge->trg()->start() == (Dyninst::Address)-1 )
			{
				return vector<int>(X64_ARG,-1);
			}


			std::vector<ParseAPI::Function *> funcs;
			blk->getFuncs(funcs);
			ParseAPI::Function *func = funcs[0];


			bool is_plt = false;
			for(auto it = plt_funcs.begin();it != plt_funcs.end();it++)
			{
				//if direct call to plt entries
				if(edge->trg()->start() == *it)
				{
					//if direct call to exit@plt,return current state
					if(edge->trg()->start() == exit_addr)
					{
						return res;

					}
					//if not direct call to exit@plt,next blk to analyze is fall_through blk

					target_blk = co->findBlockByEntry(func->region(),blk->end());

					//if target_blk not exist,assume this direct call may not return,return current state
					if(target_blk == NULL)
					{
						return res;
					}

					//if target_blk is other function's blk,assume this direct call not return,return current state
					if(!func->contains(target_blk))
					{
						return res;
					}

					is_plt = true;
					
					break;
				}
			}


			//if not direct call to plt entries,we should follow the direct call
			if(!is_plt)
			{
				fts.push_back(blk->end());
				target_blk = co->findBlockByEntry(func->region(),edge->trg()->start());
			}
			

			/*direct call only has one successive blk,Dyninst is wrong here
			for example
			  405bed:	e8 2e bc ff ff       	callq  401820 <spec_fwrite>
  			  405bf2:	41 39 c6             	cmp    %eax,%r14d
			for assembly code above,Dyninst will concule that the bb ends with the direct call(0x405bed) has 2 \
			successive blks(0x401820 and 0x405bf2).However,for our analysis,successive blk is only 0x401820.So \
			we should break here
			*/
			if(forward_cache_width.count(target_blk->start()))
			{
				reg_states.push_back(forward_cache_width[target_blk->start()]);
			}
			else
			{
				forward_cache_width[target_blk->start()] = parse_block_reg_width(target_blk,analyzed_blks,fts);
				reg_states.push_back(forward_cache_width[target_blk->start()]);
			}
			break;

		}
		else if(edge->type() == CALL_FT)
		{
			continue;
		}
		else
		{
			if(edge->trg()->start() == (Dyninst::Address)-1 ) return res;
			target_blk = edge->trg();
			//djmp to plt
			for(auto it = plt_funcs.begin();it != plt_funcs.end();it++)
			{
				if(target_blk->start() == *it)
				{
					return res;
				}
			}
		}

		//if we have analyzed this block,use the previous result
		if(forward_cache_width.count(target_blk->start()))
		{
			reg_states.push_back(forward_cache_width[target_blk->start()]);
		}
		else
		{
			forward_cache_width[target_blk->start()] = parse_block_reg_width(target_blk,analyzed_blks,fts);
			reg_states.push_back(forward_cache_width[target_blk->start()]);
		}


	}

	/*combine and path merging
	only when current blk's state is C,we look at successive blks' state
	only if all successive blks' state is R,state is R
	*/

	for(int i = 0;i < X64_ARG;i++)
	{
		//如果寄存器在当前blk中没被读写过，用后续的状态替代当前状态
		if(res[i] == 0)
		{
			int combined_res = 0;
			for(auto states_it = reg_states.begin();states_it != reg_states.end();states_it++)
			{
				vector<int> states = *states_it; 
				int state = states[i];
				//如果某条路径上寄存器被写，那么此寄存器一定没有用来保存参数
				if(state == -1)
				{
					combined_res = -1;
					break;
				}
				//某条路径上寄存器没有读写，因为保守原则，处理方式与寄存器被写相同
				else if(state == 0)
				{
					combined_res = -1;
					break;
				}
				else
				{
					if(combined_res == 0)
					{
						combined_res = state;
					}
					else
					{
						if(state < combined_res)
						{
							combined_res = state;
						}
					}
				}
			}
			res[i] = combined_res;
		}
		
	}

	return res;
}

void parse_function_reg_width(ParseAPI::Function *func)
{
	forward_cache_width.clear();
	Block *blk = func->entry();
	vector<Address> analyzed_blks,fts;
	vector<int> tt = parse_block_reg_width(blk,analyzed_blks,fts);
	fout_AT_funcs<<hex<<func->addr()<<'\t';
	for(auto it2 = tt.begin();it2 != tt.end();it2++)
	{
		fout_AT_funcs<<dec<<*it2<<' ';
	}
	fout_AT_funcs<<endl;
}


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
	if(funcs.size() == 0)
	{
		return res;
	}
	ParseAPI::Function *func = funcs[0];
	if(func->entry()->start() == blk->start() && blk->sources().size() == 0)
	{
		return res;
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
			// if(edge->src()->start() == (Dyninst::Address)-1 ) continue;
			source_blk = edge->src();
			if(source_blk->start() == -1){
				continue;
			}
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



void parse_icall_reg_width(Block *blk)
{
	backward_cache_width.clear();
	vector<Address> analyzed_blks;
	vector<int> res = parse_block_reverse_reg_width(blk,analyzed_blks);

	vector<bool> resXor = checkXor(blk);
	// vector<Imm_state> resImm = checkImm(blk);

	for(int i = 0 ; i < 6 ; i++){
		if(resXor[i] == true){
			res[i] = 8;
		}
		// if(resImm[i] == WImm){
		// 	res[i] = 8;
		// }
	}



	fout_icall<<hex<<blk->lastInsnAddr()<<'\t';
	for(auto it = res.begin();it != res.end();it++)
	{
		fout_icall<<*it;
	}
	fout_icall<<endl;
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
	vector<Address> analyzed_blks;
	int res = parse_block_rax_width(icall_next_blk,analyzed_blks);
	fout_icall_ret<<hex<<blk->lastInsnAddr()<<'\t'<<dec<<res<<'\n';
}

//0代表不是写，大于0代表写的字节数
int parse_instruction_reverse_rax(Instruction::Ptr instr)
{
	int res = 0;
	Operand op1 = instr->getOperand(0);
	Operation::registerSet regs_written;
	instr->getWriteSet(regs_written);
	for(auto it = regs_written.begin();it != regs_written.end();it++)
	{
		RegisterAST::Ptr reg = *it;
		if(reg->getID().getBaseRegister() == x86_64::rax)
		{
			res = reg->getID().size();
			if( operand_is_immediate(op1) )
			{
				res = 8;
			}
		}
	}
	return res;
}

int parse_block_reverse_rax_width(Block *blk,vector<Address> analyzed_blks)
{
	//if this blk has been analyzed(loop detection)
	for(auto it = analyzed_blks.begin();it != analyzed_blks.end();it++)
	{
		if(blk->start() == *it)
		{
			return 8;
		}
	}


	int res = 0;

	analyzed_blks.push_back(blk->start());

	//get instrs in blk
	ParseAPI::Block::Insns instrs;
	blk->getInsns(instrs);

	//parse each instruction in this basic block in reverse order
	vector<int> instr_reg_state;
	for(auto instr_it = instrs.rbegin();
	instr_it != instrs.rend();
	instr_it++)
	{
		Dyninst::InstructionAPI::Instruction::Ptr instr = instr_it->second;
		instr_reg_state.push_back(parse_instruction_reverse_rax(instr));
	}

	//merge each instruction's reg_state into current block's reg_state
	for(auto sit : instr_reg_state)
	{
		if(sit > 0)
		{
			return sit;
		}
	}


	// if current blk is entry blk,stop,because this is a intra-procedual analysis
	vector<ParseAPI::Function *> funcs;
	blk->getFuncs(funcs);
	ParseAPI::Function *func = funcs[0];
	if(func->entry()->start() == blk->start())
	{
		return 8;
	}



	//analyze preceded blks
	vector<int> reg_states;
	for(auto it = blk->sources().begin();it != blk->sources().end();it++)
	{
		ParseAPI::Edge *edge = *it;
		ParseAPI::Block *source_blk;


		//intra-procedual
		if(edge->type() == RET)
		{
			return res;
		}

		else if(edge->type() == CALL_FT)
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
			source_blk = edge->src();
			if(source_blk->start() == (Address)-1)
			{
				continue;
			}
		}


		//如果target_blk不在当前函数中
		vector<ParseAPI::Function *> cur_funcs,src_funcs;
		blk->getFuncs(cur_funcs);
		source_blk->getFuncs(src_funcs);
		if(cur_funcs.size() == 0 || src_funcs.size() == 0)
		{
			return res;
		}
		if(cur_funcs[0] != src_funcs[0])
		{
			return res;
		}


		//speed up analysis
		if(backward_cache_rax.count(source_blk->start()))
		{
			reg_states.push_back( backward_cache_rax[source_blk->start()] );
		}
		else
		{
			backward_cache_rax[source_blk->start()] = parse_block_reverse_rax_width(source_blk,analyzed_blks);
			reg_states.push_back( backward_cache_rax[source_blk->start()] );
		}

	}

	/*combine and merge
	如果当前blk中寄存器状态已知，即为此状态
	否则，合并之前基本块的寄存器状态并将其当作当前基本块中的寄存器状态
	合并规则为：有0则为0,否则取最大整数
	*/
	for(auto it : reg_states)
	{
		if(it == 0)
		{
			return it;
		}
		else
		{
			if(res == 0)
			{
				res = it;
			}
			else if(it > res)
			{
				res = it;
			}
		}
	}

	return res;
}

void get_AT_retval_width(ParseAPI::Function *func)
{
	ParseAPI::Function::const_blocklist exit_blks = func->exitBlocks();
	vector<int> res;
	vector<bool> resXor;
	// vector<Imm_state> resImm;
	for(auto bit : exit_blks)
	{
		vector<Address> analyzed_blks;
		backward_cache_rax.clear();
		res.push_back(parse_block_reverse_rax_width(bit,analyzed_blks));

		// resImm.push_back(checkImmRet(bit));
		resXor.push_back(checkXorRet(bit));
	}
	int max_res = 0;
	fout_AT_ret<<hex<<func->entry()->start()<<'\t'<<dec;
	for(auto rit : res)
	{
		if(rit == 0)
		{
			fout_AT_ret<<"0\n";
			return;
		}
		else
		{
			if(max_res == 0)
			{
				max_res = rit;
			}
			else if(rit > max_res)
			{
				max_res = rit;
			}
		}
	}


	for(auto it : resXor){
		if(it == true){
			fout_AT_ret<<"8\n";
			return;
		}
	}
	// for(auto it : resImm){
	// 	if(it == Imm_state::WImm){
	// 		fout_AT_ret<<"8\n";
	// 		return;
	// 	}
	// }





	fout_AT_ret<<max_res<<'\n';
}

int main(int argc,char **argv)
{
    BPatch bpatch;
    BPatch_addressSpace *handle;
    SymtabCodeSource *sts;
	
	CodeRegion *cr;

	if(argc != 2)
	{
		cout<<"usage : ./eg binary-to-analyze"<<endl;
		exit(0);
	}

	string target_binary = string(argv[1]);
	fout_icall.open(target_binary + "_icall");
	fout_AT_funcs.open(target_binary + "_AT_funcs");
	fout_icall_ret.open(target_binary + "_icall_ret");
	fout_AT_ret.open(target_binary + "_AT_ret");
    handle = bpatch.openBinary(("../bin/" + target_binary).c_str(),true);
	// handle = bpatch.openBinary("bzip2",true);

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

	//parse plt entries
	parse_plt_entries(bin);

	set<ParseAPI::Block *> icall_blks = get_all_icall_blks();
	for(auto icall_blk_it = icall_blks.begin();icall_blk_it != icall_blks.end();icall_blk_it++)
	{
		ParseAPI::Block *blk = *icall_blk_it;
		parse_icall_reg_width(blk);
		get_icall_retval_width(blk);

	}

	ParseAPI::Function *func_to_analyze;
	for(auto it = all_functions.begin();it != all_functions.end();it++)
	{
	// if((*it)->addr() == 0x420310) continue;
		func_to_analyze = *it;
		// cout<<hex<<(*it)->addr()<<endl;
		parse_function_reg_width(func_to_analyze);
		// cout<<func_to_analyze->name()<<endl;
		get_AT_retval_width(func_to_analyze);
	}
}
