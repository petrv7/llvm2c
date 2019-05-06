#include "Func.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Support/raw_ostream.h>

#include "../type/Type.h"

#include <utility>
#include <cstdint>
#include <string>
#include <fstream>
#include <set>

const static std::set<std::string> STDLIB_FUNCTIONS = {"atof", "atoi", "atol", "strtod", "strtol", "strtoul", "calloc",
                                                       "free", "malloc", "realloc", "abort", "atexit", "exit", "getenv",
                                                       "system", "bsearch", "qsort", "abs", "div", "labs", "ldiv",
                                                       "rand", "srand", "mblen", "mbstowcs", "mbtowc", "wcstombs", "wctomb",
                                                       "strtoll", "strtoull", "realpath"};

const static std::set<std::string> STRING_FUNCTIONS = {"memchr", "memcmp", "memcpy", "memmove", "memset", "strcat", "strncat",
                                                       "strchr", "strcmp", "strncmp", "strcoll", "strcpy", "strncpy", "strcspn",
                                                       "strerror", "strlen", "strpbrk", "strrchr", "strspn", "strstr", "strtok",
                                                       "strxfrm", "strsep", "strnlen", "strncasecmp", "strcasecmp", "stpcpy",
                                                       "strdup"};

const static std::set<std::string> STDIO_FUNCTIONS = {"fclose", "clearerr", "feof", "ferror", "fflush", "fgetpos", "fopen",
                                                      "fread", "freopen", "fseek", "fsetpos", "ftell", "fwrite", "remove",
                                                      "rename", "rewind", "setbuf", "setvbuf", "tmpfile", "tmpnam", "fprintf",
                                                      "sprintf", "vfprintf", "vsprintf", "fscanf", "scanf", "sscanf", "fgetc",
                                                      "fgets", "fputc", "fputs", "getc", "getchar", "gets", "putc", "putchar",
                                                      "puts", "ungetc", "perror", "snprintf", "vsnprintf", "printf", "pclose",
                                                      "popen", "fileno", "fseeko"};

const static std::set<std::string> PTHREAD_FUNCTIONS = {"pthread_attr_destroy", "pthread_attr_getdetachstate", "pthread_attr_getguardsize",
                                                        "pthread_attr_getinheritsched", "pthread_attr_getschedparam", "pthread_attr_getschedpolicy",
                                                        "pthread_attr_getscope", "pthread_attr_getstackaddr", "pthread_attr_getstacksize",
                                                        "pthread_attr_init", "pthread_attr_setdetachstate", "pthread_attr_setguardsize",
                                                        "pthread_attr_setinheritsched", "pthread_attr_setschedparam", "pthread_attr_setschedpolicy",
                                                        "pthread_attr_setscope", "pthread_attr_setstackaddr", "pthread_attr_setstacksize",
                                                        "pthread_cancel", "pthread_cleanup_push", "pthread_cleanup_pop", "pthread_cond_broadcast",
                                                        "pthread_cond_destroy", "pthread_cond_init", "pthread_cond_signal", "pthread_cond_timedwait",
                                                        "pthread_cond_wait", "pthread_condattr_destroy", "pthread_condattr_getpshared",
                                                        "pthread_condattr_init", "pthread_condattr_setpshared","pthread_create",  "pthread_detach",
                                                        "pthread_equal", "pthread_exit", "pthread_getconcurrency", "pthread_getschedparam","pthread_getspecific",
                                                        "pthread_join", "pthread_key_create", "pthread_key_delete", "pthread_mutex_destroy",
                                                        "pthread_mutex_getprioceiling", "pthread_mutex_init", "pthread_mutex_lock",
                                                        "pthread_mutex_setprioceiling", "pthread_mutex_trylock", "pthread_mutex_unlock",
                                                        "pthread_mutexattr_destroy", "pthread_mutexattr_getprioceiling", "pthread_mutexattr_getprotocol",
                                                        "pthread_mutexattr_getpshared", "pthread_mutexattr_gettype", "pthread_mutexattr_init",
                                                        "pthread_mutexattr_setprioceiling", "pthread_mutexattr_setprotocol", "pthread_mutexattr_setpshared",
                                                        "pthread_mutexattr_settype", "pthread_once","pthread_rwlock_destroy", "pthread_rwlock_init",
                                                        "pthread_rwlock_rdlock", "pthread_rwlock_tryrdlock", "pthread_rwlock_trywrlock",
                                                        "pthread_rwlock_unlock", "pthread_rwlock_wrlock","pthread_rwlockattr_destroy",
                                                        "pthread_rwlockattr_getpshared", "pthread_rwlockattr_init", "pthread_rwlockattr_setpshared",
                                                        "pthread_self","pthread_setcancelstate",  "pthread_setcanceltype", "pthread_setconcurrency",
                                                        "pthread_setschedparam", "pthread_setspecific", "pthread_testcancel"};

Func::Func(const llvm::Function* func, Program* program, bool isDeclaration, bool isExtern) {
    this->program = program;
    function = func;
    this->isDeclaration = isDeclaration;
    this->isExtern = isExtern;
    returnType = getType(func->getReturnType());

    parseFunction();
}

std::string Func::getBlockName(const llvm::BasicBlock* block) {
    auto iter = blockMap.find(block);
    if (iter == blockMap.end()) {
        std::string blockName = "block";
        blockName += std::to_string(blockCount);
        blockMap[block] = std::make_unique<Block>(blockName, block, this);
        blockCount++;

        return blockName;
    }

    return iter->second->blockName;
}

Expr* Func::getExpr(const llvm::Value* val) {
    if (exprMap.find(val) == exprMap.end()) {
        if (auto F = llvm::dyn_cast<llvm::Function>(val)) {
            createExpr(val, std::make_unique<Value>("&" + F->getName().str(), getType(F->getReturnType())));
            return exprMap.find(val)->second.get();
        }
    } else {
        return exprMap.find(val)->second.get();
    }

    return program->getGlobalVar(val);
}

void Func::createExpr(const llvm::Value* val, std::unique_ptr<Expr> expr) {
    exprMap[val] = std::move(expr);
}

std::string Func::getVarName() {
    std::string varName = "var";
    varName += std::to_string(varCount);
    varCount++;

    return varName;
}

void Func::parseFunction() {
    const llvm::Value* larg;

    std::string name = function->getName().str();
    if (Block::isCFunc(Block::getCFunc(name))) {
        name = Block::getCFunc(name);
    }

    if (program->includes) {
        if (isExtern && isStdLibFunc(name)) {
            program->hasStdLib = true;
        }

        if (isExtern && isStringFunc(name)) {
            program->hasString = true;
        }

        if (isExtern && isStdioFunc(name)) {
            program->hasStdio = true;
        }

        if (isExtern && isPthreadFunc(name)) {
            program->hasPthread = true;
        }
    }

    for (const llvm::Value& arg : function->args()) {
        std::string varName = "var";
        varName += std::to_string(varCount);
        exprMap[&arg] = std::make_unique<Value>(varName, getType(arg.getType()));
        varCount++;
        larg = &arg;
    }

    lastArg = exprMap[larg].get();
    if (lastArg) {
        isVarArg = function->isVarArg();
    }

    for (const auto& block : *function) {
        getBlockName(&block);
    }

    for (const auto& block : *function) {
        blockMap[&block]->parseLLVMBlock();
    }
}

void Func::print() {
    std::string name = function->getName().str();

    if (Block::isCFunc(Block::getCFunc(name))) {
        name = Block::getCFunc(name);
        if (name.compare("va_start") == 0
                || name.compare("va_end") == 0
                || name.compare("va_copy") == 0
                || Block::isCMath(name)) {
            return;
        }
    }

    if (program->includes) {
        if ((isStdLibFunc(name) || isStringFunc(name) || isStdioFunc(name) || isPthreadFunc(name)) && isExtern) {
            return;
        }
    } else {
        //sometimes LLVM uses these functions with more arguments than their C counterparts
        if ((name.compare("memcpy") == 0 || name.compare("memset") == 0 || name.compare("memmove") == 0) && function->arg_size() > 3) {
            return;
        }
    }

    if (name.substr(0, 4).compare("llvm") == 0) {
        std::replace(name.begin(), name.end(), '.', '_');
    }

    if (isExtern) {
        llvm::outs() << "extern ";
    }

    returnType->print();
    auto PT = dynamic_cast<PointerType*>(returnType.get());
    if (PT && PT->isArrayPointer) {
        llvm::outs() << " (";
        for (unsigned i = 0; i < PT->levels; i++) {
            llvm::outs() << "*";
        }
        llvm::outs() << name << "(";
    } else {
        llvm::outs() << " " << name << "(";
    }

    bool first = true;
    for (const llvm::Value& arg : function->args()) {
        if (!first) {
            llvm::outs() << ", ";
        }
        first = false;

        Value* val = static_cast<Value*>(exprMap.find(&arg)->second.get());
        val->getType()->print();
        llvm::outs() << " ";
        val->print();

        val->init = true;

    }

    if (isVarArg) {
        if (!first) {
            llvm::outs() << ", ";
        }
        llvm::outs() << "...";
    }

    llvm::outs() << ")";

    if (PT && PT->isArrayPointer) {
        llvm::outs() << ")" + PT->sizes;
    }

    if (isDeclaration) {
        llvm::outs() << ";\n";
        return;
    }

    llvm::outs() << " {\n";

    first = true;
    for (const auto& block : *function) {
        if (!first) {
            llvm::outs() << blockMap.find(&block)->second->blockName;
            llvm::outs() << ":\n    ;\n";
        }
        blockMap.find(&block)->second->print();
        first = false;
    }

    llvm::outs() << "}\n\n";
}

void Func::saveFile(std::ofstream& file) {
    std::string name = function->getName().str();

    if (Block::isCFunc(Block::getCFunc(name))) {
        name = Block::getCFunc(name);
        if (name.compare("va_start") == 0
                || name.compare("va_end") == 0
                || name.compare("va_copy") == 0
                || Block::isCMath(name)) {
            return;
        }

    }

    if (program->includes) {
        if ((isStdLibFunc(name) || isStringFunc(name) || isStdioFunc(name) || isPthreadFunc(name)) && isExtern) {
            return;
        }
    } else {
        //sometimes LLVM uses these functions with more arguments than their C counterparts
        if ((name.compare("memcpy") == 0 || name.compare("memset") == 0 || name.compare("memmove") == 0) && function->arg_size() > 3) {
            return;
        }
    }

    if (name.substr(0, 4).compare("llvm") == 0) {
        std::replace(name.begin(), name.end(), '.', '_');
    }

    if (isExtern) {
        file << "extern ";
    }

    file << returnType->toString();
    auto PT = dynamic_cast<PointerType*>(returnType.get());
    if (PT && PT->isArrayPointer) {
        file << " (";
        for (unsigned i = 0; i < PT->levels; i++) {
            file << "*";
        }
        file << name << "(";
    } else {
        file << " " << name << "(";
    }

    bool first = true;

    for (const llvm::Value& arg : function->args()) {
        if (!first) {
            file << ", ";
        }
        first = false;

        Value* val = static_cast<Value*>(exprMap.find(&arg)->second.get());
        file << val->getType()->toString();
        file << " ";
        file << val->toString();

        val->init = true;
    }

    if (isVarArg) {
        if (!first) {
            file << ", ";
        }
        file << "...";
    }

    file << ")";

    if (PT && PT->isArrayPointer) {
        file << ")" + PT->sizes;
    }

    if (isDeclaration) {
        file << ";\n";
        return;
    }

    file << " {\n";

    first = true;
    for (const auto& block : *function) {
        if (!first) {
            file << blockMap.find(&block)->second->blockName;
            file << ":\n    ;\n";
        }
        blockMap.find(&block)->second->saveFile(file);
        first = false;
    }

    file << "}\n\n";
}

Struct* Func::getStruct(const llvm::StructType* strct) const {
    return program->getStruct(strct);
}

Struct* Func::getStruct(const std::string& name) const {
    return program->getStruct(name);
}

RefExpr* Func::getGlobalVar(llvm::Value* val) const {
    return program->getGlobalVar(val);
}

void Func::addDeclaration(llvm::Function* func) {
    program->addDeclaration(func);
}

void Func::stackIgnored() {
    program->stackIgnored = true;
}

void Func::createNewUnnamedStruct(const llvm::StructType* strct) {
    program->createNewUnnamedStruct(strct);
}

std::unique_ptr<Type> Func::getType(const llvm::Type* type) {
    return program->getType(type);
}

bool Func::isStdLibFunc(const std::string& func) {
    return STDLIB_FUNCTIONS.find(func) != STDLIB_FUNCTIONS.end();
}

bool Func::isStringFunc(const std::string& func) {
    return STRING_FUNCTIONS.find(func) != STRING_FUNCTIONS.end();
}

bool Func::isStdioFunc(const std::string& func) {
    return STDIO_FUNCTIONS.find(func) != STDIO_FUNCTIONS.end();
}

bool Func::isPthreadFunc(const std::string& func) {
    return PTHREAD_FUNCTIONS.find(func) != PTHREAD_FUNCTIONS.end();
}


