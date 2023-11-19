#the result would be an overapproximation

import os
import sys
#import re

TARGET = ''

func_list = {}
AT_funcs = {}

def addPltInDynsym():
    objdump_h = open('../misc/tmp/' + TARGET + '_h.tmp','r')
    size = ''
    offset = ''
    addr = ''
    dynsize = ''
    dynoffset = ''
    dynaddr = ''
    for line in objdump_h.readlines():
        if ' .plt ' in line:
            i = 4
            while i < len(line):
                if line[i] == ' ':
                    break
                i += 1
            while i < len(line):
                if line[i] != ' ':
                    break
                i += 1
            while i < len(line):
                size += line[i]
                if line[i] == ' ':
                    break
                i += 1
            while i < len(line):
                if line[i] != ' ':
                    break
                i += 1
            while i < len(line):
                addr += line[i]
                if line[i] == ' ':
                    break
                i += 1
            while i < len(line):
                if line[i] != ' ':
                    break
                i += 1
            while i < len(line):
                if line[i] == ' ':
                    break
                i += 1
            while i < len(line):
                if line[i] != ' ':
                    break
                i += 1
            while i < len(line):
                offset += line[i]
                if line[i] == ' ':
                    break
                i += 1
        if '.dynsym' in line:
            i = 4
            while i < len(line):
                if line[i] == ' ':
                    break
                i += 1
            while i < len(line):
                if line[i] != ' ':
                    break
                i += 1
            while i < len(line):
                dynsize += line[i]
                if line[i] == ' ':
                    break
                i += 1
            while i < len(line):
                if line[i] != ' ':
                    break
                i += 1
            while i < len(line):
                dynaddr += line[i]
                if line[i] == ' ':
                    break
                i += 1
            while i < len(line):
                if line[i] != ' ':
                    break
                i += 1
            while i < len(line):
                if line[i] == ' ':
                    break
                i += 1
            while i < len(line):
                if line[i] != ' ':
                    break
                i += 1
            while i < len(line):
                dynoffset += line[i]
                if line[i] == ' ':
                    break
                i += 1        
    objdump_h.close()
    if(size + offset + addr == ''):
        return
    if(dynsize + dynoffset + dynaddr == ''):
        return
    minAddr = int(addr , 16)
    maxAddr = int(addr , 16) + int(size , 16)
    dynoffset = int(dynoffset , 16)
    dynaddr = int(dynaddr , 16)
    dynsize = int(dynsize , 16)
    fp = open('../bin/' + TARGET,'rb')
    i = dynoffset
    ttt = []
    cnt = 0

    while i < dynoffset + dynsize:
        fp.seek(i)
        b8 = fp.read(8)
        #print(b8.hex())
        if(int.from_bytes(b8 , byteorder='little') <= maxAddr) and (int.from_bytes(b8 , byteorder='little') >= minAddr):
            if is_func_entry_addr(b8):

                if b8 in ttt:
                    pass
                else:
                    ttt.append(b8)
                    cnt += 1
                    
                i += 1
                continue
        i += 1
    
    

def parse_one_line(line):
    i = 4
    size = ''
    offset = ''
    addr = ''
    while i < len(line):
        if line[i] == ' ':
            break
        i += 1
    while i < len(line):
        if line[i] != ' ':
            break
        i += 1
    while i < len(line):
        size += line[i]
        if line[i] == ' ':
            break
        i += 1
    while i < len(line):
        if line[i] != ' ':
            break
        i += 1
    while i < len(line):
        addr += line[i]
        if line[i] == ' ':
            break
        i += 1
    while i < len(line):
        if line[i] != ' ':
            break
        i += 1
    while i < len(line):
        if line[i] == ' ':
            break
        i += 1
    while i < len(line):
        if line[i] != ' ':
            break
        i += 1
    while i < len(line):
        offset += line[i]
        if line[i] == ' ':
            break
        i += 1
    return size,addr,offset
    
def get_all_funcs():
    fp = open('../dump/' + TARGET + '.dump','r')
    for line in fp.readlines():
        if '>:' in line:
            start = line.index('<')
            end = line.index('>')
            func_list[ int( line[:start - 1] ,16 ) ] = line[start + 1:end]

def is_func_entry_addr(s):
    addr = 0
    power = 1
    for ch in s:
        addr = ch * power + addr
        power = power * 256
    if addr in func_list:
        if not addr in AT_funcs:
            AT_funcs[addr] = func_list[addr]
        return True
    return False

def get_elf_info():
    data_info = []
    code_info = []
    objdump_h = open('../misc/tmp/' + TARGET + '_h.tmp','r')
    pre_line = ''
    for line in objdump_h.readlines():
        if 'DATA' in line:
             if 'dynsym' in pre_line:
                 pre_line = line
                 continue
             data_info.append(parse_one_line(pre_line))
        if 'CODE' in line:
            code_info.append(parse_one_line(pre_line))
        pre_line = line
    return data_info,code_info

def find_AT_funcs(data_info,code_info):
    fp = open('../bin/' + TARGET,'rb')

    # in data section,function entry address is 8bytes
    for sec in data_info:
        cnt = 0
        size = int(sec[0],16)
        addr = int(sec[1],16)
        offset = int(sec[2],16)
        #if addr == 0x4002d8:
            #continue
        i = offset

        ttt = []

        while i < offset + size:
            fp.seek(i)
            b8 = fp.read(8)
            if is_func_entry_addr(b8):

                if b8 in ttt:
                    pass
                else:
                    ttt.append(b8)
                    cnt += 1
                    
                i += 1
                continue
            i += 1
        # print(hex(addr),cnt)
    # print(len(AT_funcs))
    # exit(0)
    #     print(hex(addr),len(AT_funcs))
    # exit(0)
    # in code section,function entry address is 4bytes
    for sec in code_info:
        size = int(sec[0],16)
        addr = int(sec[1],16)
        offset = int(sec[2],16)
        i = offset
        while i < offset + size:
            fp.seek(i)
            b4 = fp.read(4)
            if is_func_entry_addr(b4):
                i += 1
                continue
            i += 1
    
    fp.close()

if __name__ == '__main__':
    TARGET = sys.argv[1]
    data_info,code_info = get_elf_info()
    os.system('objdump -h ../bin/' + TARGET +' > ../misc/tmp/' + TARGET + '_h.tmp')
    get_all_funcs()    
    find_AT_funcs(data_info,code_info)
    addPltInDynsym()
    # for item in data_info:
    #     print(item)
    # for item in code_info:
    #     print(item)
    # print(len(func_list))
    # for item in func_list.items():
    #     print(item)
    # print(len(AT_funcs))
    for item in AT_funcs.items():
        print(str(hex(item[0]))[2:],',',item[1])

